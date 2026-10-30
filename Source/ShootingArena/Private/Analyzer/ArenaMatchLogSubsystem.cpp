#include "Analyzer/ArenaMatchLogSubsystem.h"
#include "Analyzer/ArenaLoggingSettings.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Weapon/InitialWeaponLoadoutComponent.h"
#include "GameplayTagContainer.h"
#include "Items/DeathDropCarrier.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "MatchRulesInfo.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "SQLiteDatabase.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogArenaMatchLog, Log, All);

#include "ArenaMatchLogSchema.inc"

namespace
{
	FString Q(const FString& Text)
	{
		return TEXT("'") + Text.Replace(TEXT("'"), TEXT("''")) + TEXT("'");
	}

	FString QOrNull(const FString& Text)
	{
		return Text.IsEmpty() ? FString(TEXT("NULL")) : Q(Text);
	}

	FString N(double Value)
	{
		return FMath::IsFinite(Value) ? FString::Printf(TEXT("%.17g"), Value) : FString(TEXT("NULL"));
	}

	FString NOrNull(bool bHas, double Value)
	{
		return bHas ? N(Value) : FString(TEXT("NULL"));
	}

	FString I(int32 Value)
	{
		return Value == INDEX_NONE ? FString(TEXT("NULL")) : FString::FromInt(Value);
	}

	FString B(bool bValue)
	{
		return bValue ? TEXT("1") : TEXT("0");
	}

	/** Blueprint member names carry a "_<n>_<guid>" suffix; spaces and underscores are cosmetic. */
	FString NormalizeName(const FString& In)
	{
		// Drop a trailing "_<digits>_<32 hex>" without a regex: this runs for every property of a class.
		int32 End = In.Len();
		constexpr int32 GuidLen = 32;
		if (End > GuidLen + 2 && In[End - GuidLen - 1] == TEXT('_'))
		{
			bool bGuid = true;
			for (int32 Index = End - GuidLen; Index < End && bGuid; ++Index)
			{
				bGuid = FChar::IsHexDigit(In[Index]);
			}
			int32 DigitsStart = End - GuidLen - 1;
			while (bGuid && DigitsStart > 0 && FChar::IsDigit(In[DigitsStart - 1]))
			{
				--DigitsStart;
			}
			if (bGuid && DigitsStart < End - GuidLen - 1 && DigitsStart > 1 && In[DigitsStart - 1] == TEXT('_'))
			{
				End = DigitsStart - 1;
			}
		}
		FString S;
		S.Reserve(End);
		for (int32 Index = 0; Index < End; ++Index)
		{
			const TCHAR C = In[Index];
			if (C != TEXT('_') && C != TEXT(' '))
			{
				S.AppendChar(FChar::ToLower(C));
			}
		}
		return S;
	}

	/** Property lookups are repeated for every damage / pickup; classes do not change during a match, so remember them (misses too). */
	struct FLooseCache
	{
		struct FEntry
		{
			TWeakObjectPtr<const UStruct> Owner;
			TMap<FName, FProperty*> Properties;
		};
		TMap<const UStruct*, FEntry> Structs;
	};
	FLooseCache GLooseCache;

	FProperty* FindLoose(const UStruct* Struct, const TCHAR* Name)
	{
		if (!Struct)
		{
			return nullptr;
		}
		FLooseCache::FEntry& Entry = GLooseCache.Structs.FindOrAdd(Struct);
		if (Entry.Owner.Get() != Struct)
		{
			// New class (or a class reallocated at an old address, e.g. after a Blueprint recompile).
			Entry.Owner = Struct;
			Entry.Properties.Reset();
		}
		const FName Key(Name);
		if (FProperty** Cached = Entry.Properties.Find(Key))
		{
			return *Cached;
		}
		FProperty* Found = nullptr;
		const FString Target = NormalizeName(Name);
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			if (NormalizeName(It->GetName()) == Target)
			{
				Found = *It;
				break;
			}
		}
		Entry.Properties.Add(Key, Found);
		return Found;
	}

	bool ReadNumber(const UStruct* Struct, const void* Container, const TCHAR* Name, double& Out)
	{
		const FNumericProperty* Property = CastField<FNumericProperty>(FindLoose(Struct, Name));
		if (!Property || !Container)
		{
			return false;
		}
		const void* Ptr = Property->ContainerPtrToValuePtr<void>(Container);
		Out = Property->IsFloatingPoint() ? Property->GetFloatingPointPropertyValue(Ptr) : double(Property->GetSignedIntPropertyValue(Ptr));
		return true;
	}

	bool ReadString(const UStruct* Struct, const void* Container, const TCHAR* Name, FString& Out)
	{
		const FProperty* Property = FindLoose(Struct, Name);
		if (!Property || !Container)
		{
			return false;
		}
		const void* Ptr = Property->ContainerPtrToValuePtr<void>(Container);
		if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			const FName Value = NameProperty->GetPropertyValue(Ptr);
			Out = Value.IsNone() ? FString() : Value.ToString();
			return true;
		}
		if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
		{
			Out = StrProperty->GetPropertyValue(Ptr);
			return true;
		}
		if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			Out = TextProperty->GetPropertyValue(Ptr).ToString();
			return true;
		}
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			const int64 Value = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Ptr);
			Out = EnumProperty->GetEnum()->GetNameStringByValue(Value);
			return true;
		}
		if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property); ByteProperty && ByteProperty->Enum)
		{
			Out = ByteProperty->Enum->GetNameStringByValue(ByteProperty->GetPropertyValue(Ptr));
			return true;
		}
		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			return ReadString(StructProperty->Struct, Ptr, TEXT("RowName"), Out);
		}
		return false;
	}

	bool ReadBool(const UStruct* Struct, const void* Container, const TCHAR* Name)
	{
		const FBoolProperty* Property = CastField<FBoolProperty>(FindLoose(Struct, Name));
		return Property && Container && Property->GetPropertyValue_InContainer(Container);
	}

	/** DT_Character row (C01...) whose Characteristic_ID list holds the given characteristic row, empty when none does. */
	FString FindCharacterRowByCharacteristic(const FString& CharacteristicRow)
	{
		const UDataTable* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_Character.DT_Character"));
		if (!Table || !Table->GetRowStruct() || CharacteristicRow.IsEmpty())
		{
			return FString();
		}
		const FArrayProperty* List = CastField<FArrayProperty>(FindLoose(Table->GetRowStruct(), TEXT("Characteristic_ID")));
		const FNameProperty* Item = List ? CastField<FNameProperty>(List->Inner) : nullptr;
		if (!Item)
		{
			return FString();
		}
		for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
		{
			FScriptArrayHelper Helper(List, List->ContainerPtrToValuePtr<void>(Row.Value));
			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				if (Item->GetPropertyValue(Helper.GetElementPtr(Index)).ToString() == CharacteristicRow)
				{
					return Row.Key.ToString();
				}
			}
		}
		return FString();
	}

	FString WeaponNameFromPath(const FString& Path)
	{
		// "/Game/.../DA_Pistol.DA_Pistol" -> "DA_Pistol", "/Game/.../BP_Weapon_Pistol.BP_Weapon_Pistol_C" -> "BP_Weapon_Pistol_C"
		FString Name = Path;
		int32 Dot = INDEX_NONE;
		if (Name.FindLastChar(TEXT('.'), Dot))
		{
			Name = Name.Mid(Dot + 1);
		}
		return Name;
	}

	/** Active buff of a BPC_Buff-like component: the current gameplay tag, empty when none. */
	FString ReadBuffTag(const UActorComponent* Component)
	{
		const FStructProperty* TagProperty = CastField<FStructProperty>(FindLoose(Component->GetClass(), TEXT("currentBuffTag")));
		if (!TagProperty || TagProperty->Struct != TBaseStructure<FGameplayTag>::Get())
		{
			return FString();
		}
		const FGameplayTag* Tag = TagProperty->ContainerPtrToValuePtr<FGameplayTag>(Component);
		return Tag && Tag->IsValid() ? Tag->ToString() : FString();
	}

	FString WeaponNameOfItem(const AActor* Item)
	{
		if (!IsValid(Item))
		{
			return FString();
		}
		if (const FObjectPropertyBase* Data = CastField<FObjectPropertyBase>(FindLoose(Item->GetClass(), TEXT("weaponData"))))
		{
			if (const UObject* Object = Data->GetObjectPropertyValue_InContainer(Item))
			{
				return Object->GetName();
			}
		}
		return Item->GetClass()->GetName();
	}

	FString ToJsonText(const TSharedRef<FJsonObject>& Object)
	{
		FString Text;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text);
		FJsonSerializer::Serialize(Object, Writer);
		return Text;
	}
}

bool UArenaMatchLogSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	if (!IsArenaLoggingEnabled() || !World || World->GetNetMode() == NM_Client)
	{
		return false;
	}
	return World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE;
}

void UArenaMatchLogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GLooseCache.Structs.Reset();
}

void UArenaMatchLogSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickHandle);
	}
	if (bMatchStarted && !bMatchEnded)
	{
		EndMatch(TEXT("Forced"));
	}
	Flush();
	if (!PendingSql.IsEmpty())
	{
		Spool(PendingSql, TEXT("database unavailable at shutdown"));
		FailedWrites += PendingSql.Num();
		PendingSql.Reset();
	}
	CloseDatabase();
	GLooseCache.Structs.Reset();
	Super::Deinitialize();
}

UArenaMatchLogSubsystem* UArenaMatchLogSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UArenaMatchLogSubsystem>() : nullptr;
}

FString UArenaMatchLogSubsystem::ResolveDatabasePath()
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("-ArenaLogDb="), Path) && !Path.IsEmpty())
	{
		return FPaths::ConvertRelativePathToFull(Path);
	}
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Analyze") / TEXT("ShootingArenaLog.db"));
}

bool UArenaMatchLogSubsystem::OpenDatabase()
{
	DbPath = ResolveDatabasePath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(DbPath), true);

	Db = new FSQLiteDatabase();
	if (!Db->Open(*DbPath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		UE_LOG(LogArenaMatchLog, Error, TEXT("Cannot open match log database %s: %s"), *DbPath, *Db->GetLastError());
		delete Db;
		Db = nullptr;
		return false;
	}
	Db->Execute(TEXT("PRAGMA busy_timeout=250"));
	// Test hook: pretend the next n commits failed after they actually succeeded, which is the case
	// the idempotent retry exists for (the batch is stored but the caller cannot know).
	FParse::Value(FCommandLine::Get(), TEXT("-ArenaLogSimulateCommitFailures="), SimulatedCommitFailures);
	Db->Execute(TEXT("PRAGMA synchronous=NORMAL"));

	int64 TableCount = 0;
	Db->Execute(TEXT("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='event'"),
		[&TableCount](const FSQLitePreparedStatement& Row)
		{
			Row.GetColumnValueByIndex(0, TableCount);
			return ESQLitePreparedStatementExecuteRowResult::Stop;
		});
	if (TableCount == 0)
	{
		Db->Execute(TEXT("BEGIN IMMEDIATE"));
		for (const TCHAR* Statement : GArenaMatchLogSchema)
		{
			if (!Db->Execute(Statement))
			{
				UE_LOG(LogArenaMatchLog, Error, TEXT("Schema statement failed: %s"), *Db->GetLastError());
				++FailedWrites;
			}
		}
		Db->Execute(TEXT("COMMIT"));
	}
	UE_LOG(LogArenaMatchLog, Log, TEXT("Match log database: %s"), *DbPath);
	return true;
}

void UArenaMatchLogSubsystem::CloseDatabase()
{
	if (Db)
	{
		Db->Close();
		delete Db;
		Db = nullptr;
	}
}

double UArenaMatchLogSubsystem::ElapsedSeconds() const
{
	const UWorld* World = GetWorld();
	return (World && bClockAnchored) ? FMath::Max(0.0, World->GetTimeSeconds() - MatchStartSeconds) : 0.0;
}

void UArenaMatchLogSubsystem::TryAnchorClock()
{
	if (bClockAnchored || !bMatchStarted)
	{
		return;
	}
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	AMatchRulesInfo* Info = RulesInfo.Get();
	if (!Info)
	{
		for (TActorIterator<AMatchRulesInfo> It(const_cast<UWorld*>(World)); It; ++It)
		{
			Info = *It;
			RulesInfo = Info;
			break;
		}
	}
	if (!Info)
	{
		// No rules actor in this world: the match clock starts with the first life.
		MatchStartSeconds = World->GetTimeSeconds();
		bClockAnchored = true;
		return;
	}
	const AGameStateBase* GameState = World->GetGameState();
	if (Info->MatchStartServerTime > 0.0 && GameState)
	{
		// Everyone is connected and loaded: t=0 is the moment the rules actor started the match.
		const double Ago = FMath::Max(0.0, GameState->GetServerWorldTimeSeconds() - Info->MatchStartServerTime);
		MatchStartSeconds = World->GetTimeSeconds() - Ago;
		bClockAnchored = true;
	}
}

void UArenaMatchLogSubsystem::Enqueue(const FString& Sql)
{
	PendingSql.Add(Sql);
}

int32 UArenaMatchLogSubsystem::AllocateEvent(const TCHAR* EventType, int32 CauseSeq)
{
	EnsureMatchStarted();
	if (!Db || bMatchEnded || !bMatchStarted)
	{
		return INDEX_NONE;
	}
	TryAnchorClock();
	const int32 Seq = NextEventSeq++;
	Enqueue(FString::Printf(TEXT("INSERT INTO event (MatchId,EventSequence,ElapsedSeconds,EventType,CauseEventSequence) VALUES (%s,%d,%s,%s,%s)"),
		*Q(MatchId), Seq, *N(ElapsedSeconds()), *Q(EventType), *I(CauseSeq)));
	return Seq;
}

void UArenaMatchLogSubsystem::EnsureMatchStarted()
{
	if (bMatchStarted || bDisabled)
	{
		return;
	}
	UWorld* World = GetWorld();
	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	// Only maps whose game mode runs a match clock are recorded (not lobby / main menu).
	if (!GameMode || !GameMode->GameStateClass || !GameMode->GameStateClass->FindPropertyByName(TEXT("ElapsedMatchTime")))
	{
		return;
	}
	if (!OpenDatabase())
	{
		bDisabled = true;
		return;
	}

	MatchId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	MatchStartSeconds = World->GetTimeSeconds();
	bMatchStarted = true;
	TryAnchorClock();

	int32 TimeLimit = 0;
	int32 KillTarget = 0;
	for (TActorIterator<AMatchRulesInfo> It(World); It; ++It)
	{
		TimeLimit = It->MatchDurationSeconds;
		KillTarget = It->KillTarget;
		break;
	}
	if (const AGameStateBase* GameState = World->GetGameState())
	{
		if (TimeLimit <= 0)
		{
			if (const FObjectPropertyBase* ConfigProperty = CastField<FObjectPropertyBase>(GameState->GetClass()->FindPropertyByName(TEXT("GameConfig"))))
			{
				if (const UObject* Config = ConfigProperty->GetObjectPropertyValue_InContainer(GameState))
				{
					double Value = 0.0;
					if (ReadNumber(Config->GetClass(), Config, TEXT("MatchDuration"), Value))
					{
						TimeLimit = int32(Value);
					}
					if (KillTarget <= 0 && ReadNumber(Config->GetClass(), Config, TEXT("MaxScore"), Value))
					{
						KillTarget = int32(Value);
					}
				}
			}
		}
		double EndKill = 0.0;
		if (KillTarget <= 0 && ReadNumber(GameState->GetClass(), GameState, TEXT("EndKillPoint"), EndKill))
		{
			KillTarget = int32(EndKill);
		}
	}

	FString SettingsVersion;
	ArchiveBalanceSettings(SettingsVersion);

	FString MapId = World->GetMapName();
	MapId.RemoveFromStart(World->StreamingLevelsPrefix);

	const int32 Seq = NextEventSeq++;
	Enqueue(FString::Printf(TEXT("INSERT INTO event (MatchId,EventSequence,ElapsedSeconds,EventType,CauseEventSequence) VALUES (%s,%d,0,'MatchStarted',NULL)"), *Q(MatchId), Seq));
	Enqueue(FString::Printf(TEXT("INSERT INTO match (MatchId,EventSequence,MapId,GameBuildVersion,SettingsVersion,TimeLimitSeconds,KillTarget) VALUES (%s,%d,%s,%s,%s,%d,%d)"),
		*Q(MatchId), Seq, *Q(MapId), *Q(FApp::GetBuildVersion()), *Q(SettingsVersion), TimeLimit, KillTarget));

	World->GetTimerManager().SetTimer(TickHandle, this, &UArenaMatchLogSubsystem::Tick, 0.25f, true);
	UE_LOG(LogArenaMatchLog, Log, TEXT("Match %s started on %s (time limit %d s, kill target %d)"), *MatchId, *MapId, TimeLimit, KillTarget);
}

void UArenaMatchLogSubsystem::ArchiveBalanceSettings(FString& OutSettingsVersion)
{
	struct FRow { FString Table; FString Row; FString Json; };
	TArray<FRow> Rows;

	FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	Registry.Get().ScanPathsSynchronous({TEXT("/Game/QuakeLike_1_0/Data/Item")}, true);
	Registry.Get().GetAssetsByPath(FName(TEXT("/Game/QuakeLike_1_0/Data/Item")), Assets, true);
	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetClassPath != UDataTable::StaticClass()->GetClassPathName())
		{
			continue;
		}
		const UDataTable* Table = Cast<UDataTable>(Asset.GetAsset());
		if (!Table || !Table->GetRowStruct())
		{
			continue;
		}
		for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
		{
			TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
			if (FJsonObjectConverter::UStructToJsonObject(Table->GetRowStruct(), Pair.Value, Object, 0, 0))
			{
				Rows.Add({Asset.AssetName.ToString(), Pair.Key.ToString(), ToJsonText(Object)});
			}
		}
	}
	// Rows reference data assets (weapon and item data); their values are part of what the match applied.
	{
		TSet<FString> Seen;
		TArray<const UObject*> Referenced;
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetClassPath != UDataTable::StaticClass()->GetClassPathName())
			{
				continue;
			}
			const UDataTable* Table = Cast<UDataTable>(Asset.GetAsset());
			if (!Table || !Table->GetRowStruct())
			{
				continue;
			}
			for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
			{
				for (TFieldIterator<FObjectPropertyBase> It(Table->GetRowStruct()); It; ++It)
				{
					const UObject* Object = It->GetObjectPropertyValue_InContainer(Pair.Value);
					if (Object && Object->IsA<UPrimaryDataAsset>() && !Seen.Contains(Object->GetPathName()))
					{
						Seen.Add(Object->GetPathName());
						Referenced.Add(Object);
					}
				}
			}
		}
		// A held item's data asset points at its weapon data asset, so follow one more level.
		for (int32 Index = 0; Index < Referenced.Num(); ++Index)
		{
			const UObject* Object = Referenced[Index];
			for (TFieldIterator<FObjectPropertyBase> It(Object->GetClass()); It; ++It)
			{
				const UObject* Child = It->GetObjectPropertyValue_InContainer(Object);
				if (Child && Child->IsA<UPrimaryDataAsset>() && !Seen.Contains(Child->GetPathName()))
				{
					Seen.Add(Child->GetPathName());
					Referenced.Add(Child);
				}
			}
		}
		for (const UObject* Object : Referenced)
		{
			TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
			if (FJsonObjectConverter::UStructToJsonObject(Object->GetClass(), Object, Json, 0, 0))
			{
				Rows.Add({FString(TEXT("DataAsset:")) + Object->GetClass()->GetName(), Object->GetName(), ToJsonText(Json)});
			}
		}
	}
	Rows.Sort([](const FRow& A, const FRow& B) { return A.Table == B.Table ? A.Row < B.Row : A.Table < B.Table; });

	FString Canonical;
	for (const FRow& Row : Rows)
	{
		Canonical += Row.Table + TEXT("|") + Row.Row + TEXT("|") + Row.Json + TEXT("\n");
	}
	OutSettingsVersion = TEXT("S") + FMD5::HashAnsiString(*Canonical).Left(12);

	for (const FRow& Row : Rows)
	{
		Enqueue(FString::Printf(TEXT("INSERT OR IGNORE INTO balance_settings (SettingsVersion,SourceTable,RowName,\"Values\") VALUES (%s,%s,%s,%s)"),
			*Q(OutSettingsVersion), *Q(Row.Table), *Q(Row.Row), *Q(Row.Json)));
	}
	if (Rows.IsEmpty())
	{
		UE_LOG(LogArenaMatchLog, Warning, TEXT("No balance data tables found under /Game/QuakeLike_1_0/Data/Item; settings version has no archive."));
	}
}

bool UArenaMatchLogSubsystem::TryWriteParticipant(int32 ParticipantId, FParticipantState& State, int32 SpawnNumber)
{
	if (State.bWritten)
	{
		return true;
	}
	AController* Controller = State.Controller.Get();
	if (!IsValid(Controller))
	{
		return false;
	}

	const bool bBot = !Controller->IsPlayerController();
	const APlayerState* PlayerState = Controller->PlayerState;
	FString CharacterId;
	if (PlayerState)
	{
		ReadString(PlayerState->GetClass(), PlayerState, TEXT("SelectedCharacterRow"), CharacterId);
	}

	FString DifficultyId, CharacteristicId, Hash;
	if (bBot)
	{
		// AI spawned by the game modes get their characteristic row on the controller (e.g. "PJH_Normal"); the suffix is the difficulty.
		ReadString(Controller->GetClass(), Controller, TEXT("CharacteristicRowNameOverride"), CharacteristicId);
		if (CharacteristicId.IsEmpty() && PlayerState)
		{
			ReadString(PlayerState->GetClass(), PlayerState, TEXT("Characteristic_ID"), CharacteristicId);
		}
		int32 Underscore = INDEX_NONE;
		if (CharacteristicId.FindLastChar(TEXT('_'), Underscore))
		{
			const FString Suffix = CharacteristicId.Mid(Underscore + 1);
			if (Suffix == TEXT("Easy") || Suffix == TEXT("Normal") || Suffix == TEXT("Hard"))
			{
				DifficultyId = Suffix;
			}
		}
		if (DifficultyId.IsEmpty())
		{
			if (const AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
			{
				ReadString(GameMode->GetClass(), GameMode, TEXT("Selected_Difficulty"), DifficultyId);
			}
		}
		if (CharacterId.IsEmpty())
		{
			CharacterId = FindCharacterRowByCharacteristic(CharacteristicId);
		}
		UFunction* Function = Controller->FindFunction(TEXT("GetCharacteristic"));
		if (!Function)
		{
			Function = Controller->FindFunction(TEXT("Get Characteristic"));
		}
		static const TCHAR* const Keys[9] = {TEXT("Attack_Skill"), TEXT("Aim_Skill"), TEXT("Aim_Accuracy"), TEXT("Fire_Throttle"),
			TEXT("View_Max_Change"), TEXT("ReactionTime"), TEXT("Aggression"), TEXT("Aggression_Threshold"), TEXT("Jumper")};
		double Values[9] = {};
		bool bAll = false;
		if (Function)
		{
			FStructOnScope Params(Function);
			Controller->ProcessEvent(Function, Params.GetStructMemory());
			for (TFieldIterator<FStructProperty> It(Function); It && !bAll; ++It)
			{
				if (!It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
				{
					continue;
				}
				const void* Struct = It->ContainerPtrToValuePtr<void>(Params.GetStructMemory());
				bAll = true;
				for (int32 Index = 0; Index < 9; ++Index)
				{
					bAll &= ReadNumber(It->Struct, Struct, Keys[Index], Values[Index]);
				}
			}
		}
		if (bAll)
		{
			FString Canonical;
			for (const double Value : Values)
			{
				Canonical += FString::Printf(TEXT("%.9g|"), Value);
			}
			Hash = TEXT("B") + FMD5::HashAnsiString(*Canonical).Left(16);
			Enqueue(FString::Printf(TEXT("INSERT OR IGNORE INTO bot_settings (BotSettingsHash,Attack_Skill,Aim_Skill,Aim_Accuracy,Fire_Throttle,View_Max_Change,ReactionTime,Aggression,Aggression_Threshold,Jumper) VALUES (%s,%s,%s,%s,%s,%d,%s,%s,%s,%s)"),
				*Q(Hash), *N(Values[0]), *N(Values[1]), *N(Values[2]), *N(Values[3]), int32(Values[4]), *N(Values[5]), *N(Values[6]), *N(Values[7]), *N(Values[8])));
		}
		else
		{
			UE_LOG(LogArenaMatchLog, Error, TEXT("Bot participant %d: the controller's Characteristic could not be read; bot_settings not archived"), ParticipantId);
			++FailedWrites;
		}
		if (DifficultyId.IsEmpty()) DifficultyId = TEXT("Unknown");
		if (CharacteristicId.IsEmpty()) CharacteristicId = TEXT("Unknown");
	}

	if (CharacterId.IsEmpty())
	{
		CharacterId = Controller->GetPawn() ? Controller->GetPawn()->GetClass()->GetName() : Controller->GetClass()->GetName();
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Participant %d has no character row; using class name %s"), ParticipantId, *CharacterId);
	}

	Enqueue(FString::Printf(TEXT("INSERT INTO participant (MatchId,ParticipantId,bIsBot,CharacterId,DifficultyId,CharacteristicId,BotSettingsHash) VALUES (%s,%d,%s,%s,%s,%s,%s)"),
		*Q(MatchId), ParticipantId, *B(bBot), *Q(CharacterId), *QOrNull(DifficultyId), *QOrNull(CharacteristicId), *QOrNull(Hash)));
	State.bWritten = true;
	return true;
}

void UArenaMatchLogSubsystem::NoteController(int32 ParticipantId, AController* Controller)
{
	if (bDisabled || !IsValid(Controller))
	{
		return;
	}
	FParticipantState& State = Participants.FindOrAdd(ParticipantId);
	State.Controller = Controller;
}

bool UArenaMatchLogSubsystem::FindLife(const APawn* Pawn, int32& OutParticipantId, int32& OutSpawnNumber) const
{
	if (const int64* Key = PawnLives.Find(const_cast<APawn*>(Pawn)))
	{
		if (const FLifeState* Life = Lives.Find(*Key))
		{
			OutParticipantId = Life->ParticipantId;
			OutSpawnNumber = Life->SpawnNumber;
			return true;
		}
	}
	return false;
}

void UArenaMatchLogSubsystem::NoteLifeBegin(APawn* Pawn, int32 ParticipantId, int32 SpawnNumber)
{
	if (bDisabled || !IsValid(Pawn))
	{
		return;
	}
	// A new possession ends any life of this participant that is still open.
	for (TPair<int64, FLifeState>& Pair : Lives)
	{
		FLifeState& Open = Pair.Value;
		if (Open.ParticipantId == ParticipantId && !Open.bEnded && Open.SpawnNumber != SpawnNumber && Open.Pawn.IsValid())
		{
			EndLife(Open.Pawn.Get(), TEXT("Leave"));
		}
	}
	const int64 Key = LifeKey(ParticipantId, SpawnNumber);
	FLifeState& Life = Lives.FindOrAdd(Key);
	Life.ParticipantId = ParticipantId;
	Life.SpawnNumber = SpawnNumber;
	Life.Pawn = Pawn;
	Life.BeganAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	PawnLives.Add(Pawn, Key);

	// The match (and its clock) starts with the first life, then Tick() completes lives whose loadout never reports.
	EnsureMatchStarted();
	if (const FPendingLoadout* Pending = PendingLoadouts.Find(Pawn))
	{
		const FPendingLoadout Copy = *Pending;
		PendingLoadouts.Remove(Pawn);
		CompleteSpawn(Pawn, Copy.Ids, Copy.Defaults, Copy.Equipped);
	}
}

void UArenaMatchLogSubsystem::CompleteSpawn(APawn* Pawn, const TArray<FString>& WeaponIds, const TArray<bool>& DefaultFlags, const FString& EquippedWeapon)
{
	const int64* Key = PawnLives.Find(Pawn);
	FLifeState* Life = Key ? Lives.Find(*Key) : nullptr;
	if ((!Life || Life->bEnded) && !bDisabled && IsValid(Pawn) && !bMatchEnded)
	{
		// The loadout can be granted before the controller possesses the pawn, or while the pawn still maps to the
		// life that just ended (pooled respawn); apply it once the next life begins.
		FPendingLoadout& Pending = PendingLoadouts.FindOrAdd(Pawn);
		Pending.Ids = WeaponIds;
		Pending.Defaults = DefaultFlags;
		Pending.Equipped = EquippedWeapon;
		return;
	}
	if (!Life || Life->bSpawnWritten || Life->bEnded || !bMatchStarted || bMatchEnded)
	{
		return;
	}
	FParticipantState* Participant = Participants.Find(Life->ParticipantId);
	if (!Participant || !TryWriteParticipant(Life->ParticipantId, *Participant, Life->SpawnNumber))
	{
		return;
	}

	// Respawns that never re-run the loadout grant start with the participant's previous initial loadout.
	TArray<FString> Ids = WeaponIds;
	TArray<bool> Defaults = DefaultFlags;
	FString Equipped = EquippedWeapon;
	if (Equipped.IsEmpty())
	{
		Equipped = Life->PendingInitialEquip;
	}
	if (Ids.IsEmpty() && IsValid(Pawn))
	{
		CollectHeldWeapons(Pawn, Ids, Defaults); // no grant reported: use what the pawn actually holds
	}
	if (Ids.IsEmpty())
	{
		if (const FPendingLoadout* Last = LastLoadoutByParticipant.Find(Life->ParticipantId))
		{
			Ids = Last->Ids;
			Defaults = Last->Defaults;
			Equipped = Last->Equipped;
		}
	}
	else
	{
		FPendingLoadout& Last = LastLoadoutByParticipant.FindOrAdd(Life->ParticipantId);
		Last.Ids = Ids;
		Last.Defaults = Defaults;
		Last.Equipped = Equipped;
	}

	FString Initial = Equipped;
	if (Initial.IsEmpty())
	{
		for (int32 Index = 0; Index < Ids.Num(); ++Index)
		{
			if (Defaults.IsValidIndex(Index) && Defaults[Index])
			{
				Initial = Ids[Index];
				break;
			}
		}
	}
	if (Initial.IsEmpty() && !Ids.IsEmpty())
	{
		Initial = Ids[0];
	}
	if (Initial.IsEmpty())
	{
		Initial = TEXT("None");
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Spawn %d/%d has no initial weapon"), Life->ParticipantId, Life->SpawnNumber);
	}

	const int32 Seq = AllocateEvent(TEXT("Spawned"), INDEX_NONE);
	if (Seq == INDEX_NONE)
	{
		return;
	}
	Life->bSpawnWritten = true;
	Life->SpawnSeq = Seq;
	Enqueue(FString::Printf(TEXT("INSERT INTO spawn (MatchId,EventSequence,ParticipantId,SpawnNumber,InitialEquippedWeaponId) VALUES (%s,%d,%d,%d,%s)"),
		*Q(MatchId), Seq, Life->ParticipantId, Life->SpawnNumber, *Q(Initial)));
	TSet<FString> Seen;
	for (int32 Index = 0; Index < Ids.Num(); ++Index)
	{
		if (Seen.Contains(Ids[Index]))
		{
			continue;
		}
		Seen.Add(Ids[Index]);
		Enqueue(FString::Printf(TEXT("INSERT INTO spawn_weapon (MatchId,ParticipantId,SpawnNumber,WeaponId,bIsDefaultWeapon) VALUES (%s,%d,%d,%s,%s)"),
			*Q(MatchId), Life->ParticipantId, Life->SpawnNumber, *Q(Ids[Index]), *B(Defaults.IsValidIndex(Index) && Defaults[Index])));
	}
	if (!Seen.Contains(Initial) && Initial != TEXT("None"))
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO spawn_weapon (MatchId,ParticipantId,SpawnNumber,WeaponId,bIsDefaultWeapon) VALUES (%s,%d,%d,%s,0)"),
			*Q(MatchId), Life->ParticipantId, Life->SpawnNumber, *Q(Initial)));
	}
}

void UArenaMatchLogSubsystem::EndLife(APawn* Pawn, const TCHAR* Reason, int32 CauseSeq)
{
	const int64* Key = PawnLives.Find(Pawn);
	FLifeState* Life = Key ? Lives.Find(*Key) : nullptr;
	if (!Life || Life->bEnded)
	{
		return;
	}
	Life->bEnded = true;
	if (!Life->bSpawnWritten)
	{
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Life %d/%d ended (%s) before its spawn was recorded"), Life->ParticipantId, Life->SpawnNumber, Reason);
		return;
	}
	const int32 Seq = AllocateEvent(TEXT("SpawnEnded"), CauseSeq);
	if (Seq != INDEX_NONE)
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO spawn_end (MatchId,EventSequence,ParticipantId,SpawnNumber,SpawnEndReason) VALUES (%s,%d,%d,%d,%s)"),
			*Q(MatchId), Seq, Life->ParticipantId, Life->SpawnNumber, *Q(Reason)));
	}
}

int32 UArenaMatchLogSubsystem::RecordFire(int32 AttackId, int32 ShooterParticipantId, int32 ShooterSpawnNumber, const FString& WeaponId, const UObject* WeaponData)
{
	if (bDisabled || AttackId == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	if (bMatchStarted && !bClockAnchored)
	{
		MatchStartSeconds = GetWorld()->GetTimeSeconds();
		bClockAnchored = true;
	}
	const int32 Seq = AllocateEvent(TEXT("WeaponFired"), INDEX_NONE);
	if (Seq == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	FPendingFire& Fire = PendingFires.Add(AttackId);
	Fire.EventSeq = Seq;
	Fire.ShooterParticipantId = ShooterParticipantId;
	Fire.ShooterSpawnNumber = ShooterSpawnNumber;
	Fire.WeaponId = WeaponNameFromPath(WeaponId);
	if (WeaponData)
	{
		// Pellets per shot come from the weapon data; projectile weapons list 0 and fire one projectile.
		double Pellets = 0.0;
		FFireTraits Traits;
		if (ReadNumber(WeaponData->GetClass(), WeaponData, TEXT("pellet"), Pellets))
		{
			Fire.ElementCount = FMath::Max(1, int32(Pellets));
			Fire.bCountFromData = true;
		}
		ReadNumber(WeaponData->GetClass(), WeaponData, TEXT("damage"), Traits.Damage);
		ReadNumber(WeaponData->GetClass(), WeaponData, TEXT("explosionRadius"), Traits.ExplosionRadius);
		FireTraits.Add(AttackId, Traits);
	}
	FireSeqs.Add(AttackId, Seq);
	return Seq;
}

void UArenaMatchLogSubsystem::SetFireElementCount(int32 AttackId, int32 Count)
{
	if (FPendingFire* Fire = PendingFires.Find(AttackId))
	{
		if (!Fire->bCountFromData)
		{
			Fire->ElementCount = FMath::Max(Fire->ElementCount, Count);
		}
	}
}

void UArenaMatchLogSubsystem::FlushPendingFires()
{
	for (const TPair<int32, FPendingFire>& Pair : PendingFires)
	{
		const FPendingFire& Fire = Pair.Value;
		Enqueue(FString::Printf(TEXT("INSERT INTO fire (MatchId,EventSequence,AttackId,ShooterParticipantId,ShooterSpawnNumber,WeaponId,ShotElementCount) VALUES (%s,%d,%d,%d,%d,%s,%d)"),
			*Q(MatchId), Fire.EventSeq, Pair.Key, Fire.ShooterParticipantId, Fire.ShooterSpawnNumber, *Q(Fire.WeaponId), Fire.ElementCount));
	}
	PendingFires.Reset();
}

FArenaLifeSnapshot UArenaMatchLogSubsystem::SnapshotLife(AActor* Pawn) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArenaMatchLog_SnapshotLife);
	FArenaLifeSnapshot Snapshot;
	if (!IsValid(Pawn))
	{
		return Snapshot;
	}
	TArray<UActorComponent*> Components;
	Pawn->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (!IsValid(Component))
		{
			continue;
		}
		if (!Snapshot.bValid)
		{
			if (const FStructProperty* LifeInfo = CastField<FStructProperty>(FindLoose(Component->GetClass(), TEXT("LifeInfo"))))
			{
				const void* Struct = LifeInfo->ContainerPtrToValuePtr<void>(Component);
				Snapshot.bValid = ReadNumber(LifeInfo->Struct, Struct, TEXT("NowHealth"), Snapshot.Health)
					&& ReadNumber(LifeInfo->Struct, Struct, TEXT("NowPlate"), Snapshot.Armor);
			}
		}
		if (Snapshot.BuffTag.IsEmpty())
		{
			Snapshot.BuffTag = ReadBuffTag(Component);
		}
	}
	return Snapshot;
}

int32 UArenaMatchLogSubsystem::BeginDamage(int32 AttackId)
{
	if (bDisabled)
	{
		return INDEX_NONE;
	}
	const int32* FireSeq = FireSeqs.Find(AttackId);
	DamageInputs.Reset(); // a value left from damage that was not recorded must not leak into this one
	const int32 Seq = AllocateEvent(TEXT("DamageApplied"), FireSeq ? *FireSeq : INDEX_NONE);
	ActiveDamageSeqs.Add(Seq);
	return Seq;
}

void UArenaMatchLogSubsystem::EndDamage(int32 DamageSeq, const FArenaDamageRecord& Record)
{
	if (!ActiveDamageSeqs.IsEmpty())
	{
		ActiveDamageSeqs.Pop(EAllowShrinking::No);
	}
	if (DamageSeq == INDEX_NONE)
	{
		return;
	}

	const bool bWeapon = Record.AttackId != INDEX_NONE;
	const bool bSelf = bWeapon && Record.AttackerParticipantId == Record.VictimParticipantId;
	const bool bHasState = Record.Before.bValid && Record.After.bValid;
	// The distributed input is whatever left health and armor; negative health is kept as the raw remainder.
	const double* Reported = Record.VictimActor.IsValid() ? DamageInputs.Find(Record.VictimActor) : nullptr;
	double Input = bHasState
		? (Record.Before.Health - Record.After.Health) + (Record.Before.Armor - Record.After.Armor)
		: Record.AppliedDamage;
	if (Reported)
	{
		Input = *Reported;
		DamageInputs.Remove(Record.VictimActor);
	}
	if (!bHasState)
	{
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Damage %d: victim health/armor could not be read; InputDamage falls back to the ApplyDamage result"), DamageSeq);
		++FailedWrites;
	}

	// Weapons with a blast radius deal their listed direct damage on impact; any other amount is the explosion.
	bool bExplosion = false;
	if (bWeapon)
	{
		const FFireTraits* Traits = FireTraits.Find(Record.AttackId);
		bExplosion = Traits && Traits->ExplosionRadius > 0.0 && FMath::Abs(Record.RawDamage - Traits->Damage) > 0.01;
	}
	const FString HitType = !bWeapon ? FString(TEXT("NULL")) : Q(bExplosion ? TEXT("Explosion") : TEXT("Direct"));
	Enqueue(FString::Printf(TEXT("INSERT INTO damage (MatchId,EventSequence,AttackId,DamageSource,HitType,AttackerParticipantId,AttackerSpawnNumber,VictimParticipantId,VictimSpawnNumber,bSelfDamage,AttackerBuffTag,VictimBuffTag,InputDamage,HealthBefore,HealthAfter,ArmorBefore,ArmorAfter,EngagementDistance) VALUES (%s,%d,%s,%s,%s,%s,%s,%d,%d,%s,%s,%s,%s,%s,%s,%s,%s,%s)"),
		*Q(MatchId), DamageSeq, *I(Record.AttackId), bWeapon ? TEXT("'Weapon'") : TEXT("'Environment'"), *HitType,
		*I(bWeapon ? Record.AttackerParticipantId : INDEX_NONE), *I(bWeapon ? Record.AttackerSpawnNumber : INDEX_NONE),
		Record.VictimParticipantId, Record.VictimSpawnNumber, *B(bSelf),
		*QOrNull(Record.AttackerBuffTag), *QOrNull(Record.VictimBuffTag), *N(Input),
		*NOrNull(bHasState, Record.Before.Health), *NOrNull(bHasState, Record.After.Health),
		*NOrNull(bHasState, Record.Before.Armor), *NOrNull(bHasState, Record.After.Armor),
		*NOrNull(bWeapon && Record.Distance >= 0.0, Record.Distance)));

	if (FLifeState* Victim = Lives.Find(LifeKey(Record.VictimParticipantId, Record.VictimSpawnNumber)))
	{
		Victim->LastDamageSeq = DamageSeq;
		if (bWeapon && !bSelf)
		{
			Victim->LastDamageFrom.Add(Record.AttackerParticipantId, DamageSeq);
		}
	}

	if (ActiveDamageSeqs.IsEmpty() && bFlushWanted)
	{
		Flush();
	}
}

void UArenaMatchLogSubsystem::RecordDeath(APawn* Victim, const FString& Cause, int32 KillerParticipantId, int32 KillerDamageSeq)
{
	const int64* Key = PawnLives.Find(Victim);
	FLifeState* Life = Key ? Lives.Find(*Key) : nullptr;
	if (!Life || Life->DeathSeq != INDEX_NONE || !Life->bSpawnWritten)
	{
		return;
	}
	const int32 CauseSeq = GetActiveDamageSeq();
	const int32 Seq = AllocateEvent(TEXT("Died"), CauseSeq);
	if (Seq == INDEX_NONE)
	{
		return;
	}
	Life->DeathSeq = Seq;
	Life->DeathDamageSeq = KillerDamageSeq;
	LastDeathSeq = Seq;

	FString DeathCause = Cause;
	FString Source = KillerParticipantId != INDEX_NONE ? TEXT("DirectPlayerKill") : TEXT("None");
	int32 Killer = KillerParticipantId;
	int32 KillerSeq = KillerParticipantId != INDEX_NONE ? KillerDamageSeq : INDEX_NONE;
	if (Life->bHasPendingCredit)
	{
		DeathCause = Life->PendingDeathCause;
		Source = Life->PendingCreditSource;
		Killer = Source == TEXT("None") ? INDEX_NONE : Life->PendingCreditedParticipant;
		if (Source == TEXT("DirectPlayerKill"))
		{
			KillerSeq = KillerDamageSeq;
		}
		else if (Source == TEXT("RecentAttacker") && Killer != INDEX_NONE)
		{
			const int32* Recent = Life->LastDamageFrom.Find(Killer);
			KillerSeq = Recent ? *Recent : INDEX_NONE;
		}
		else
		{
			KillerSeq = INDEX_NONE;
		}
	}
	Enqueue(FString::Printf(TEXT("INSERT INTO death (MatchId,EventSequence,VictimParticipantId,VictimSpawnNumber,DeathCause,KillCreditSource,KillerParticipantId,KillerDamageEventSequence) VALUES (%s,%d,%d,%d,%s,%s,%s,%s)"),
		*Q(MatchId), Seq, Life->ParticipantId, Life->SpawnNumber, *Q(DeathCause), *Q(Source), *I(Killer), *I(KillerSeq)));
	// The active buff is dropped with the body, so it ends at the moment of death.
	if (const FString* Active = BuffByPawn.Find(Victim); Active && !Active->IsEmpty())
	{
		BuffByPawn.Add(Victim, FString());
		RecordBuff(Victim, FString(), TEXT("Expire"), 0.0, false, Seq);
	}
	EndLife(Victim, TEXT("Death"), Seq);
}

void UArenaMatchLogSubsystem::RecordCredit(APawn* Victim, const FString& DeathCause, const FString& CreditSource, int32 CreditedParticipantId)
{
	const int64* Key = PawnLives.Find(Victim);
	FLifeState* Life = Key ? Lives.Find(*Key) : nullptr;
	if (!Life)
	{
		return;
	}
	Life->bHasPendingCredit = true;
	Life->PendingDeathCause = DeathCause;
	Life->PendingCreditSource = CreditSource;
	Life->PendingCreditedParticipant = CreditedParticipantId;
	if (Life->DeathSeq == INDEX_NONE)
	{
		return; // applied when the death row is written
	}

	int32 Killer = CreditSource == TEXT("None") ? INDEX_NONE : CreditedParticipantId;
	int32 KillerSeq = INDEX_NONE;
	if (CreditSource == TEXT("DirectPlayerKill"))
	{
		KillerSeq = Life->DeathDamageSeq;
	}
	else if (CreditSource == TEXT("RecentAttacker") && Killer != INDEX_NONE)
	{
		const int32* Recent = Life->LastDamageFrom.Find(Killer);
		KillerSeq = Recent ? *Recent : INDEX_NONE;
	}
	Enqueue(FString::Printf(TEXT("UPDATE death SET DeathCause=%s,KillCreditSource=%s,KillerParticipantId=%s,KillerDamageEventSequence=%s WHERE MatchId=%s AND EventSequence=%d"),
		*Q(DeathCause), *Q(CreditSource), *I(Killer), *I(KillerSeq), *Q(MatchId), Life->DeathSeq));
}

void UArenaMatchLogSubsystem::RecordWeaponEquip(APawn* Pawn, const FString& Before, const FString& After, const FString& Reason)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (bDisabled || !FindLife(Pawn, Participant, SpawnNumber))
	{
		return;
	}
	if (FLifeState* Life = Lives.Find(LifeKey(Participant, SpawnNumber)))
	{
		if (Life->bEnded)
		{
			return;
		}
		if (!Life->bSpawnWritten)
		{
			Life->PendingInitialEquip = After; // covered by spawn.InitialEquippedWeaponId
			return;
		}
	}
	const int32 Seq = AllocateEvent(TEXT("WeaponEquipped"), INDEX_NONE);
	if (Seq != INDEX_NONE)
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO weapon_equip (MatchId,EventSequence,ParticipantId,SpawnNumber,EquippedWeaponBefore,EquippedWeaponAfter,EquipChangeReason) VALUES (%s,%d,%d,%d,%s,%s,%s)"),
			*Q(MatchId), Seq, Participant, SpawnNumber, *QOrNull(Before), *QOrNull(After), *Q(Reason)));
	}
}

int32 UArenaMatchLogSubsystem::RecordItemSpawn(AActor* Item, const FString& Category, const FString& RowName, const FString& Reason)
{
	if (bDisabled || !IsValid(Item))
	{
		return INDEX_NONE;
	}
	if (const int32* Existing = ItemIds.Find(Item))
	{
		return *Existing; // the same actor is never spawned twice
	}
	const int32 Seq = AllocateEvent(TEXT("ItemSpawned"), Reason == TEXT("DeathDrop") ? LastDeathSeq : INDEX_NONE);
	if (Seq == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	const int32 Id = NextItemId++;
	ItemIds.Add(Item, Id);
	OpenItems.Add(Id);
	Enqueue(FString::Printf(TEXT("INSERT INTO item_spawn (MatchId,EventSequence,SpawnedItemId,ItemCategory,ItemRowName,ItemSpawnReason) VALUES (%s,%d,%d,%s,%s,%s)"),
		*Q(MatchId), Seq, Id, *Q(Category), *Q(RowName), *Q(Reason)));
	return Id;
}

void UArenaMatchLogSubsystem::RecordItemPickup(APawn* Pawn, AActor* Item, double ItemAmount, double AppliedAmount, bool bHasAmounts)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	const int32* ItemId = ItemIds.Find(Item);
	if (bDisabled || !ItemId || !FindLife(Pawn, Participant, SpawnNumber) || PickupSeqs.Contains(*ItemId))
	{
		return;
	}
	FArenaLifeSnapshot State = SnapshotLife(Pawn);
	if (const FArenaLifeSnapshot* Captured = PickupBefore.Find(Pawn))
	{
		State = *Captured;
	}
	const int32 Seq = AllocateEvent(TEXT("ItemPickedUp"), INDEX_NONE);
	if (Seq == INDEX_NONE)
	{
		return;
	}
	PickupSeqs.Add(*ItemId, Seq);
	Enqueue(FString::Printf(TEXT("INSERT INTO pickup (MatchId,EventSequence,ParticipantId,SpawnNumber,SpawnedItemId,HealthBefore,ArmorBefore,ItemAmount,AppliedAmount) VALUES (%s,%d,%d,%d,%d,%s,%s,%s,%s)"),
		*Q(MatchId), Seq, Participant, SpawnNumber, *ItemId, *NOrNull(State.bValid, State.Health), *NOrNull(State.bValid, State.Armor),
		*NOrNull(bHasAmounts, ItemAmount), *NOrNull(bHasAmounts, AppliedAmount)));
}

void UArenaMatchLogSubsystem::RecordItemRemove(AActor* Item, const FString& Reason)
{
	const int32* ItemId = ItemIds.Find(Item);
	if (bDisabled || !ItemId || !OpenItems.Contains(*ItemId))
	{
		return;
	}
	const int32* PickupSeq = PickupSeqs.Find(*ItemId);
	const int32 Seq = AllocateEvent(TEXT("ItemRemoved"), (Reason == TEXT("PickedUp") && PickupSeq) ? *PickupSeq : INDEX_NONE);
	if (Seq == INDEX_NONE)
	{
		return;
	}
	OpenItems.Remove(*ItemId);
	Enqueue(FString::Printf(TEXT("INSERT INTO item_remove (MatchId,EventSequence,SpawnedItemId,ItemRemoveReason) VALUES (%s,%d,%d,%s)"),
		*Q(MatchId), Seq, *ItemId, *Q(Reason)));
}

void UArenaMatchLogSubsystem::RecordBuff(APawn* Pawn, const FString& BuffAfter, const FString& ChangeType, double ExpiresAtSeconds, bool bHasExpiry, int32 CauseSeq)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (bDisabled || !FindLife(Pawn, Participant, SpawnNumber))
	{
		return;
	}
	const int32 Seq = AllocateEvent(TEXT("BuffChanged"), CauseSeq);
	if (Seq != INDEX_NONE)
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO buff (MatchId,EventSequence,ParticipantId,SpawnNumber,BuffAfter,BuffChangeType,BuffExpiresAtSeconds) VALUES (%s,%d,%d,%d,%s,%s,%s)"),
			*Q(MatchId), Seq, Participant, SpawnNumber, *QOrNull(BuffAfter), *Q(ChangeType), *NOrNull(bHasExpiry, ExpiresAtSeconds)));
	}
}

bool UArenaMatchLogSubsystem::ReadMatchEnded() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	if (const AGameStateBase* GameState = World->GetGameState())
	{
		if (ReadBool(GameState->GetClass(), GameState, TEXT("bIsMatchEnded")))
		{
			return true;
		}
	}
	const AGameModeBase* GameMode = World->GetAuthGameMode();
	return GameMode && ReadBool(GameMode->GetClass(), GameMode, TEXT("bIsMatchEnded"));
}

void UArenaMatchLogSubsystem::Tick()
{
	if (!bMatchStarted || bMatchEnded)
	{
		return;
	}
	TryAnchorClock();
	const double Now = GetWorld()->GetTimeSeconds();
	for (TPair<int32, FParticipantState>& Pair : Participants)
	{
		if (const AController* Controller = Pair.Value.Controller.Get(); Controller && Controller->PlayerState)
		{
			double Value = 0.0;
			if (ReadNumber(Controller->PlayerState->GetClass(), Controller->PlayerState, TEXT("KillCount"), Value)) Pair.Value.CachedKills = int32(Value);
			if (ReadNumber(Controller->PlayerState->GetClass(), Controller->PlayerState, TEXT("DeathCount"), Value)) Pair.Value.CachedDeaths = int32(Value);
		}
	}
	// A life whose loadout never reported still needs a spawn row for the other tables to reference.
	for (TPair<int64, FLifeState>& Pair : Lives)
	{
		FLifeState& Life = Pair.Value;
		if (!Life.bSpawnWritten && !Life.bEnded && Life.Pawn.IsValid() && Now - Life.BeganAt >= 2.0)
		{
			CompleteSpawn(Life.Pawn.Get(), {}, {}, FString());
		}
	}
	if (ReadMatchEnded())
	{
		FString Reason = TEXT("TimeLimit");
		for (TActorIterator<AMatchRulesInfo> It(GetWorld()); It; ++It)
		{
			if (It->KillTarget > 0)
			{
				for (const TPair<int32, FParticipantState>& Pair : Participants)
				{
					if (Pair.Value.CachedKills >= It->KillTarget)
					{
						Reason = TEXT("KillTarget");
					}
				}
			}
			break;
		}
		EndMatch(Reason);
		return;
	}
	Flush();
}

void UArenaMatchLogSubsystem::EndMatch(const FString& Reason)
{
	if (!bMatchStarted || bMatchEnded)
	{
		return;
	}
	// Open lives end with the match (death already closed its own).
	for (TPair<int64, FLifeState>& Pair : Lives)
	{
		if (!Pair.Value.bEnded && Pair.Value.Pawn.IsValid())
		{
			EndLife(Pair.Value.Pawn.Get(), TEXT("MatchEnd"));
		}
	}
	for (const int32 ItemId : OpenItems.Array())
	{
		for (const TPair<TWeakObjectPtr<AActor>, int32>& Pair : ItemIds)
		{
			if (Pair.Value == ItemId)
			{
				if (AActor* Item = Pair.Key.Get())
				{
					RecordItemRemove(Item, TEXT("MatchEnd"));
				}
				break;
			}
		}
	}

	const int32 Seq = AllocateEvent(TEXT("MatchEnded"), INDEX_NONE);
	if (Seq != INDEX_NONE)
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO match_end (MatchId,EventSequence,MatchEndReason) VALUES (%s,%d,%s)"), *Q(MatchId), Seq, *Q(Reason)));
	}
	bMatchEnded = true;

	for (TPair<int32, FParticipantState>& Pair : Participants)
	{
		FParticipantState& State = Pair.Value;
		if (const AController* Controller = State.Controller.Get(); Controller && Controller->PlayerState)
		{
			double Value = 0.0;
			if (ReadNumber(Controller->PlayerState->GetClass(), Controller->PlayerState, TEXT("KillCount"), Value)) State.CachedKills = int32(Value);
			if (ReadNumber(Controller->PlayerState->GetClass(), Controller->PlayerState, TEXT("DeathCount"), Value)) State.CachedDeaths = int32(Value);
		}
		if (!State.bWritten)
		{
			continue; // the participant never spawned
		}
		if (State.CachedKills == INDEX_NONE || State.CachedDeaths == INDEX_NONE)
		{
			UE_LOG(LogArenaMatchLog, Error, TEXT("Participant %d: final kill/death count could not be read; participant_result not written"), Pair.Key);
			++FailedWrites;
			continue;
		}
		Enqueue(FString::Printf(TEXT("INSERT INTO participant_result (MatchId,ParticipantId,FinalKills,FinalDeaths) VALUES (%s,%d,%d,%d)"),
			*Q(MatchId), Pair.Key, State.CachedKills, State.CachedDeaths));
		State.bResultWritten = true;
	}
	UE_LOG(LogArenaMatchLog, Log, TEXT("Match %s ended (%s)"), *MatchId, *Reason);
	bFlushWanted = true;
	if (ActiveDamageSeqs.IsEmpty())
	{
		Flush();
	}
}

namespace
{
	/** Errors that can go away on their own; anything else is a data problem a retry cannot fix. */
	bool IsTransientError(const FString& Error)
	{
		for (const TCHAR* Needle : {TEXT("locked"), TEXT("busy"), TEXT("I/O"), TEXT("full"), TEXT("readonly"), TEXT("unable to open"), TEXT("cannot start a transaction")})
		{
			if (Error.Contains(Needle, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	/** A retried batch may already be partly stored, so its inserts must not create a second copy. */
	FString AsIdempotent(const FString& Sql)
	{
		static const FString Plain = TEXT("INSERT INTO ");
		return Sql.StartsWith(Plain) ? TEXT("INSERT OR IGNORE INTO ") + Sql.RightChop(Plain.Len()) : Sql;
	}

	constexpr int32 MaxFlushAttempts = 8;
}

void UArenaMatchLogSubsystem::Spool(const TArray<FString>& Statements, const FString& Reason)
{
	if (Statements.IsEmpty())
	{
		return;
	}
	FString Text = FString::Printf(TEXT("-- %s match %s: %s\n"), *FDateTime::UtcNow().ToIso8601(), *MatchId, *Reason);
	for (const FString& Sql : Statements)
	{
		Text += Sql + TEXT(";\n");
	}
	const FString Path = (DbPath.IsEmpty() ? ResolveDatabasePath() : DbPath) + TEXT(".failed.sql");
	if (!FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append))
	{
		UE_LOG(LogArenaMatchLog, Error, TEXT("Could not write %d unsaved statements to %s"), Statements.Num(), *Path);
		return;
	}
	UE_LOG(LogArenaMatchLog, Error, TEXT("%d statements were not stored and were kept in %s"), Statements.Num(), *Path);
}

void UArenaMatchLogSubsystem::Flush()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArenaMatchLog_Flush);
	if (!Db)
	{
		return;
	}
	if (!ActiveDamageSeqs.IsEmpty())
	{
		bFlushWanted = true; // a damage event is mid-flight; its detail row is not queued yet
		return;
	}
	bFlushWanted = false;
	// Fire rows wait for pellet counts; a later trace for the same attack is no longer expected.
	FlushPendingFires();
	if (PendingSql.IsEmpty())
	{
		MaybeValidate();
		return;
	}

	const bool bRetry = FlushAttempts > 0;
	FString TransientError;
	TArray<FString> Rejected;
	if (!Db->Execute(TEXT("BEGIN IMMEDIATE")))
	{
		TransientError = Db->GetLastError();
	}
	else
	{
		for (const FString& Sql : PendingSql)
		{
			if (Db->Execute(bRetry ? *AsIdempotent(Sql) : *Sql))
			{
				continue;
			}
			const FString Error = Db->GetLastError();
			if (IsTransientError(Error))
			{
				TransientError = Error;
				break;
			}
			++FailedWrites;
			Rejected.Add(Sql + TEXT(" -- ") + Error.Replace(TEXT("\n"), TEXT(" ")));
			UE_LOG(LogArenaMatchLog, Error, TEXT("Write rejected: %s\n  %s"), *Error, *Sql.Left(300));
		}
		if (TransientError.IsEmpty() && !Db->Execute(TEXT("COMMIT")))
		{
			TransientError = Db->GetLastError();
			Db->Execute(TEXT("ROLLBACK"));
		}
		else if (TransientError.IsEmpty() && SimulatedCommitFailures > 0)
		{
			--SimulatedCommitFailures;
			TransientError = TEXT("simulated commit failure (batch is already stored)");
		}
		else if (!TransientError.IsEmpty())
		{
			Db->Execute(TEXT("ROLLBACK"));
		}
	}

	if (!TransientError.IsEmpty())
	{
		// Nothing was committed: keep the whole batch and try again on the next flush.
		++FlushAttempts;
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Flush failed (attempt %d/%d): %s"), FlushAttempts, MaxFlushAttempts, *TransientError);
		if (FlushAttempts >= MaxFlushAttempts)
		{
			Spool(PendingSql, TransientError);
			FailedWrites += PendingSql.Num();
			PendingSql.Reset();
			FlushAttempts = 0;
		}
		return;
	}

	FlushAttempts = 0;
	PendingSql.Reset();
	Spool(Rejected, TEXT("rejected by the database"));
	MaybeValidate();
}

void UArenaMatchLogSubsystem::MaybeValidate()
{
	if (!Db || !bMatchEnded || bValidated || !PendingSql.IsEmpty() || !ActiveDamageSeqs.IsEmpty())
	{
		return;
	}
	bValidated = true;

	// Everything is stored: check references and counts of this match against the stored rows.
	TArray<FString> Findings;
	Db->Execute(*FString::Printf(TEXT("SELECT Code, COUNT(*) FROM v_record_issues WHERE MatchId=%s AND Severity='ERROR' GROUP BY Code"), *Q(MatchId)),
		[&Findings](const FSQLitePreparedStatement& Row)
		{
			FString Code;
			int64 Count = 0;
			Row.GetColumnValueByIndex(0, Code);
			Row.GetColumnValueByIndex(1, Count);
			Findings.Add(FString::Printf(TEXT("%s x%lld"), *Code, Count));
			return ESQLitePreparedStatementExecuteRowResult::Continue;
		});
	int64 DanglingReferences = 0;
	Db->Execute(TEXT("PRAGMA foreign_key_check"), [&DanglingReferences](const FSQLitePreparedStatement&)
	{
		++DanglingReferences;
		return ESQLitePreparedStatementExecuteRowResult::Continue;
	});
	if (DanglingReferences > 0)
	{
		Findings.Add(FString::Printf(TEXT("dangling references x%lld"), DanglingReferences));
	}
	if (FailedWrites > 0)
	{
		Findings.Add(FString::Printf(TEXT("failed writes x%d"), FailedWrites));
	}
	LastValidationIssues = Findings.Num();
	if (Findings.IsEmpty())
	{
		UE_LOG(LogArenaMatchLog, Log, TEXT("Match %s validated: no missing events or references"), *MatchId);
	}
	else
	{
		UE_LOG(LogArenaMatchLog, Error, TEXT("Match %s validation found: %s"), *MatchId, *FString::Join(Findings, TEXT(", ")));
	}
}

bool UArenaMatchLogSubsystem::DescribeDropItem(AActor* Item, FString& OutCategory, FString& OutRowName, double& OutAmount, bool& bOutHasAmount) const
{
	OutCategory.Reset();
	bOutHasAmount = false;
	OutAmount = 0.0;
	for (const UClass* Class = Item->GetClass(); Class && OutCategory.IsEmpty(); Class = Class->GetSuperClass())
	{
		const FString Name = Class->GetName();
		if (Name == TEXT("BP_LifeItem_C")) OutCategory = TEXT("Life");
		else if (Name == TEXT("BP_AmmoItem_C")) OutCategory = TEXT("Ammo");
		else if (Name == TEXT("BP_BuffItem_C")) OutCategory = TEXT("Buff");
		else if (Name == TEXT("BP_HoldableItem_C")) OutCategory = TEXT("HeldItem");
	}
	if (OutCategory.IsEmpty())
	{
		return false;
	}
	const FStructProperty* HandleProperty = CastField<FStructProperty>(FindLoose(Item->GetClass(), TEXT("DataTableHandle")));
	if (!HandleProperty || HandleProperty->Struct != FDataTableRowHandle::StaticStruct())
	{
		return false;
	}
	const FDataTableRowHandle* Handle = HandleProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(Item);
	if (!Handle || Handle->RowName.IsNone())
	{
		return false;
	}
	OutRowName = Handle->RowName.ToString();
	const uint8* Row = IsValid(Handle->DataTable) ? Handle->DataTable->FindRowUnchecked(Handle->RowName) : nullptr;
	if (!Row)
	{
		return true;
	}
	const UScriptStruct* RowStruct = Handle->DataTable->GetRowStruct();
	if (OutCategory == TEXT("HeldItem"))
	{
		// Weapons use the data-asset name so item rows line up with fire.WeaponId.
		for (TFieldIterator<FObjectPropertyBase> It(RowStruct); It; ++It)
		{
			if (const UObject* Object = It->GetObjectPropertyValue_InContainer(Row); Object && Object->IsA<UPrimaryDataAsset>())
			{
				OutRowName = Object->GetName();
				break;
			}
		}
	}
	else if (OutCategory == TEXT("Life"))
	{
		bOutHasAmount = ReadNumber(RowStruct, Row, TEXT("amount"), OutAmount);
	}
	else if (OutCategory == TEXT("Ammo"))
	{
		bOutHasAmount = ReadNumber(RowStruct, Row, TEXT("ammo"), OutAmount);
	}
	return true;
}

void UArenaMatchLogSubsystem::RecordDropItemSpawned(AActor* Item)
{
	if (bDisabled || !IsValid(Item) || ItemIds.Contains(Item))
	{
		return;
	}
	FString Category, RowName;
	double Amount = 0.0;
	bool bHasAmount = false;
	if (!DescribeDropItem(Item, Category, RowName, Amount, bHasAmount))
	{
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Item %s: category or data row could not be resolved; item_spawn not written"), *GetNameSafe(Item));
		++FailedWrites;
		return;
	}
	const bool bDeathDrop = Cast<ADeathDropCarrier>(Item->GetOwner()) != nullptr;
	const int32 Id = RecordItemSpawn(Item, Category, RowName, bDeathDrop ? TEXT("DeathDrop") : TEXT("Spawner"));
	if (Id != INDEX_NONE)
	{
		ItemCategoryById.Add(Id, Category);
		ItemAmountById.Add(Id, Amount);
		ItemHasAmountById.Add(Id, bHasAmount);
		Item->OnDestroyed.AddUniqueDynamic(this, &UArenaMatchLogSubsystem::HandleItemDestroyed);
	}
}

void UArenaMatchLogSubsystem::HandleItemDestroyed(AActor* DestroyedActor)
{
	if (bDisabled || !bMatchStarted || bMatchEnded)
	{
		return;
	}
	const int32* Id = ItemIds.Find(DestroyedActor);
	RecordItemRemove(DestroyedActor, (Id && PickupSeqs.Contains(*Id)) ? TEXT("PickedUp") : TEXT("Despawned"));
}

void UArenaMatchLogSubsystem::BeginItemPickup(AActor* Item, AActor* Picker)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArenaMatchLog_BeginItemPickup);
	if (bDisabled || !IsValid(Picker) || !ItemIds.Contains(Item))
	{
		return;
	}
	PickupBefore.Add(Picker, SnapshotLife(Picker));
	PickupFrame.Add(Picker, GFrameCounter);
	if (ItemCategoryById.FindRef(ItemIds.FindRef(Item)) == TEXT("Ammo"))
	{
		CollectAmmo(Picker, AmmoBefore.FindOrAdd(Picker));
	}
}

void UArenaMatchLogSubsystem::CommitItemPickup(AActor* Item, AActor* Picker)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArenaMatchLog_CommitItemPickup);
	APawn* Pawn = Cast<APawn>(Picker);
	const int32* Id = ItemIds.Find(Item);
	if (bDisabled || !Pawn || !Id)
	{
		return;
	}
	const double Amount = ItemAmountById.FindRef(*Id);
	const bool bHasAmount = ItemHasAmountById.FindRef(*Id);
	double Applied = 0.0;
	bool bHasApplied = false;
	if (ItemCategoryById.FindRef(*Id) == TEXT("Ammo"))
	{
		// Applied ammo = how much the holder's weapons actually gained (full magazines absorb less than offered).
		if (const TMap<TWeakObjectPtr<AActor>, int32>* Before = AmmoBefore.Find(Picker); Before && !Before->IsEmpty())
		{
			TMap<TWeakObjectPtr<AActor>, int32> After;
			CollectAmmo(Picker, After);
			for (const TPair<TWeakObjectPtr<AActor>, int32>& Pair : *Before)
			{
				if (const int32* Now = After.Find(Pair.Key))
				{
					Applied += FMath::Max(0, *Now - Pair.Value);
				}
			}
			bHasApplied = true;
		}
		AmmoBefore.Remove(Picker);
	}
	else if (ItemCategoryById.FindRef(*Id) == TEXT("Life"))
	{
		const FArenaLifeSnapshot* Before = PickupBefore.Find(Picker);
		const FArenaLifeSnapshot After = SnapshotLife(Picker);
		if (Before && Before->bValid && After.bValid)
		{
			Applied = (After.Health - Before->Health) + (After.Armor - Before->Armor);
			bHasApplied = true;
		}
	}
	const FString Category = ItemCategoryById.FindRef(*Id);
	const bool bTracksApplied = Category == TEXT("Life") || Category == TEXT("Ammo");
	RecordItemPickup(Pawn, Item, bHasAmount ? Amount : 0.0, Applied, bHasAmount && (!bTracksApplied || bHasApplied));
	PickupBefore.Remove(Picker);
}

void UArenaMatchLogSubsystem::RecordWeaponSwap(AActor* Owner, AActor* BeforeItem, AActor* AfterItem)
{
	APawn* Pawn = Cast<APawn>(Owner);
	if (bDisabled || !Pawn)
	{
		return;
	}
	const FString Before = WeaponNameOfItem(BeforeItem);
	const FString After = WeaponNameOfItem(AfterItem);
	if (Before == After)
	{
		return;
	}
	const uint64* PickupAt = PickupFrame.Find(Pawn);
	const bool bAuto = PickupAt && *PickupAt == GFrameCounter;
	RecordWeaponEquip(Pawn, Before, After, bAuto ? TEXT("AutoOnPickup") : TEXT("Manual"));
}

void UArenaMatchLogSubsystem::RecordBuffApplied(UActorComponent* BuffComponent)
{
	APawn* Pawn = IsValid(BuffComponent) ? Cast<APawn>(BuffComponent->GetOwner()) : nullptr;
	if (bDisabled || !Pawn)
	{
		return;
	}
	const FString Tag = ReadBuffTag(BuffComponent);
	if (Tag.IsEmpty())
	{
		return;
	}
	double End = 0.0;
	ReadNumber(BuffComponent->GetClass(), BuffComponent, TEXT("buffEndServerTime"), End);
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const bool bHasExpiry = End > 0.0 && GameState;
	const double Expires = bHasExpiry ? ElapsedSeconds() + FMath::Max(0.0, End - GameState->GetServerWorldTimeSeconds()) : 0.0;
	const FString* Previous = BuffByPawn.Find(Pawn);
	const TCHAR* Type = (!Previous || Previous->IsEmpty()) ? TEXT("Gain") : (*Previous == Tag ? TEXT("Refresh") : TEXT("Replace"));
	BuffByPawn.Add(Pawn, Tag);
	RecordBuff(Pawn, Tag, Type, Expires, bHasExpiry);
}

void UArenaMatchLogSubsystem::RecordBuffExpired(UActorComponent* BuffComponent)
{
	APawn* Pawn = IsValid(BuffComponent) ? Cast<APawn>(BuffComponent->GetOwner()) : nullptr;
	const FString* Previous = Pawn ? BuffByPawn.Find(Pawn) : nullptr;
	if (bDisabled || !Pawn || !Previous || Previous->IsEmpty())
	{
		return;
	}
	BuffByPawn.Add(Pawn, FString());
	RecordBuff(Pawn, FString(), TEXT("Expire"), 0.0, false);
}

namespace
{
	bool HeldBy(const AActor* Actor, const AActor* Holder)
	{
		return Actor && (Actor->GetOwner() == Holder || Actor->GetAttachParentActor() == Holder || Actor->GetInstigator() == Holder);
	}
}

void UArenaMatchLogSubsystem::CollectAmmo(const AActor* Holder, TMap<TWeakObjectPtr<AActor>, int32>& Out) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArenaMatchLog_CollectAmmo);
	Out.Reset();
	const UWorld* World = GetWorld();
	if (!World || !IsValid(Holder))
	{
		return;
	}
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (*It == Holder || !HeldBy(*It, Holder))
		{
			continue;
		}
		const FStructProperty* Count = CastField<FStructProperty>(FindLoose(It->GetClass(), TEXT("currentUsageCount")));
		double Value = 0.0;
		if (Count && ReadNumber(Count->Struct, Count->ContainerPtrToValuePtr<void>(*It), TEXT("currentUsageCount"), Value))
		{
			Out.Add(*It, int32(Value));
		}
	}
}

void UArenaMatchLogSubsystem::CollectHeldWeapons(APawn* Pawn, TArray<FString>& OutIds, TArray<bool>& OutDefaults) const
{
	const UWorld* World = GetWorld();
	if (!World || !IsValid(Pawn))
	{
		return;
	}
	const UInitialWeaponLoadoutComponent* Loadout = Pawn->FindComponentByClass<UInitialWeaponLoadoutComponent>();
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (*It == Pawn || !HeldBy(*It, Pawn) || !FindLoose(It->GetClass(), TEXT("weaponData")))
		{
			continue;
		}
		OutIds.Add(WeaponNameOfItem(*It));
		OutDefaults.Add(Loadout && Loadout->IsDefaultWeapon(*It));
	}
}

void UArenaMatchLogSubsystem::RecordDamageInput(AActor* Victim, double InputDamage)
{
	if (!bDisabled && IsValid(Victim) && !ActiveDamageSeqs.IsEmpty())
	{
		DamageInputs.Add(Victim, InputDamage);
	}
}
