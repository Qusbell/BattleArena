#include "Analyzer/ArenaMatchLogSubsystem.h"
#include "Async/Async.h"
#include "Analyzer/ArenaLoggingSettings.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Weapon/InitialWeaponLoadoutComponent.h"
#include "GameplayTagContainer.h"
#include "Items/DeathDropCarrier.h"
#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Analyzer/ArenaLoggingDataAsset.h"
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
	FTSTicker::GetCoreTicker().RemoveTicker(MatchTicker);
	FTSTicker::GetCoreTicker().RemoveTicker(BehaviorTicker);
	if (bMatchStarted && !bMatchEnded)
	{
		EndMatch(TEXT("Forced"));
	}
	// The database connection stays alive until the sole writer has finished.
	while (Db && (InFlightSql || !PendingSql.IsEmpty() || !PendingFires.IsEmpty())
		&& ActiveDamageSeqs.IsEmpty())
	{
		CollectWrite(true);
		Flush();
	}
	CollectWrite(true);
	if (InFlightSql)
	{
		Spool(*InFlightSql, TEXT("database unavailable at shutdown"));
		FailedWrites += InFlightSql->Num();
		InFlightSql.Reset();
	}
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
	else
	{
		// Databases created before match.StartedAtKst existed get the column so the match insert keeps working.
		int64 HasStartedAtKst = 0;
		Db->Execute(TEXT("SELECT COUNT(*) FROM pragma_table_info('match') WHERE name='StartedAtKst'"),
			[&HasStartedAtKst](const FSQLitePreparedStatement& Row)
			{
				Row.GetColumnValueByIndex(0, HasStartedAtKst);
				return ESQLitePreparedStatementExecuteRowResult::Stop;
			});
		if (HasStartedAtKst == 0 && !Db->Execute(TEXT("ALTER TABLE match ADD COLUMN StartedAtKst TEXT")))
		{
			UE_LOG(LogArenaMatchLog, Error, TEXT("Could not add match.StartedAtKst: %s"), *Db->GetLastError());
			++FailedWrites;
		}

		// Databases from before collection version 1 get the extension columns, tables, indexes and views.
		// Existing rows stay as they are: values that were never collected remain NULL.
		static const TCHAR* const AddedColumns[][3] = {
			{TEXT("match"), TEXT("CollectionVersion"), TEXT("INTEGER")},
			{TEXT("participant"), TEXT("PlayerKey"), TEXT("TEXT")},
			{TEXT("spawn_weapon"), TEXT("InitialAmmo"), TEXT("INTEGER")},
			{TEXT("spawn_weapon"), TEXT("bUnlimitedAmmo"), TEXT("INTEGER")},
			{TEXT("damage"), TEXT("bBuffsRecorded"), TEXT("INTEGER")},
			{TEXT("buff"), TEXT("BuffId"), TEXT("INTEGER")},
			{TEXT("buff"), TEXT("PreviousBuffId"), TEXT("INTEGER")},
		};
		for (const TCHAR* const (&Column)[3] : AddedColumns)
		{
			int64 Present = 0;
			Db->Execute(*FString::Printf(TEXT("SELECT COUNT(*) FROM pragma_table_info('%s') WHERE name='%s'"), Column[0], Column[1]),
				[&Present](const FSQLitePreparedStatement& Row)
				{
					Row.GetColumnValueByIndex(0, Present);
					return ESQLitePreparedStatementExecuteRowResult::Stop;
				});
			if (Present == 0 && !Db->Execute(*FString::Printf(TEXT("ALTER TABLE %s ADD COLUMN %s %s"), Column[0], Column[1], Column[2])))
			{
				UE_LOG(LogArenaMatchLog, Error, TEXT("Could not add %s.%s: %s"), Column[0], Column[1], *Db->GetLastError());
				++FailedWrites;
			}
		}
		Db->Execute(TEXT("BEGIN IMMEDIATE"));
		for (const TCHAR* Statement : GArenaMatchLogSchema)
		{
			FString Sql(Statement);
			bool bOk = true;
			for (const TCHAR* Kind : {TEXT("CREATE TABLE "), TEXT("CREATE INDEX "), TEXT("CREATE TRIGGER ")})
			{
				if (Sql.StartsWith(Kind))
				{
					Sql = FString(Kind) + TEXT("IF NOT EXISTS ") + Sql.RightChop(FCString::Strlen(Kind));
					break;
				}
			}
			if (Sql.StartsWith(TEXT("CREATE VIEW ")))
			{
				// Views change with the schema, so they are always rebuilt.
				const FString Rest = Sql.RightChop(12);
				int32 NameEnd = Rest.Len();
				for (int32 Index = 0; Index < Rest.Len(); ++Index)
				{
					if (FChar::IsWhitespace(Rest[Index]))
					{
						NameEnd = Index;
						break;
					}
				}
				bOk = Db->Execute(*FString::Printf(TEXT("DROP VIEW IF EXISTS %s"), *Rest.Left(NameEnd)));
			}
			if (bOk && !Db->Execute(*Sql))
			{
				UE_LOG(LogArenaMatchLog, Error, TEXT("Schema upgrade statement failed: %s"), *Db->GetLastError());
				++FailedWrites;
			}
		}
		Db->Execute(TEXT("COMMIT"));
	}
	// Analysis parameters are data rather than schema, so they are seeded here (existing values are kept).
	Db->Execute(TEXT("INSERT OR IGNORE INTO analysis_param (Name,Value) VALUES ('DistanceBandCm',500)"));
	Db->Execute(TEXT("INSERT OR IGNORE INTO analysis_param (Name,Value) VALUES ('GapToleranceFactor',2.0)"));
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
	const FString StartedAtKst = (FDateTime::UtcNow() + FTimespan::FromHours(9)).ToString(TEXT("%y-%m-%d_%H:%M:%S"));
	SettingsVersionId = SettingsVersion;
	TimeLimitSeconds = TimeLimit;
	Enqueue(FString::Printf(TEXT("INSERT INTO match (MatchId,EventSequence,MapId,GameBuildVersion,SettingsVersion,TimeLimitSeconds,KillTarget,StartedAtKst,CollectionVersion) VALUES (%s,%d,%s,%s,%s,%d,%d,%s,1)"),
		*Q(MatchId), Seq, *Q(MapId), *Q(FApp::GetBuildVersion()), *Q(SettingsVersion), TimeLimit, KillTarget, *Q(StartedAtKst)));
	BeginBehaviorObservation();

	MatchTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		Tick();
		return true;
	}), 0.25f);
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
	ArchiveParticipantSettings(ParticipantId, Controller, CharacteristicId);
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
	if (const FString* EquippedBeforeLife = PendingEquipByPawn.Find(Pawn))
	{
		if (Equipped.IsEmpty())
		{
			Equipped = *EquippedBeforeLife;
		}
		PendingEquipByPawn.Remove(Pawn);
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
	NoteEquipped(Life->ParticipantId, Life->SpawnNumber, Initial);

	// The ammo the loadout really handed out (after any adjustment); not collected -> NULL, never a guessed 0.
	TArray<FAmmoReading> Held;
	if (IsValid(Pawn))
	{
		CollectWeaponAmmo(Pawn, Held);
	}
	const auto AmmoColumns = [&Held](const FString& WeaponId) -> FString
	{
		for (const FAmmoReading& Reading : Held)
		{
			if (Reading.WeaponId == WeaponId)
			{
				return FString::Printf(TEXT("%s,%s"), Reading.bUnlimited ? TEXT("NULL") : *FString::FromInt(Reading.Count), Reading.bUnlimited ? TEXT("1") : TEXT("0"));
			}
		}
		return FString(TEXT("NULL,NULL"));
	};
	for (const FAmmoReading& Reading : Held)
	{
		LastAmmo.Add(Reading.Weapon, Reading);
	}

	TSet<FString> Seen;
	for (int32 Index = 0; Index < Ids.Num(); ++Index)
	{
		if (Seen.Contains(Ids[Index]))
		{
			continue;
		}
		Seen.Add(Ids[Index]);
		Enqueue(FString::Printf(TEXT("INSERT INTO spawn_weapon (MatchId,ParticipantId,SpawnNumber,WeaponId,bIsDefaultWeapon,InitialAmmo,bUnlimitedAmmo) VALUES (%s,%d,%d,%s,%s,%s)"),
			*Q(MatchId), Life->ParticipantId, Life->SpawnNumber, *Q(Ids[Index]), *B(Defaults.IsValidIndex(Index) && Defaults[Index]), *AmmoColumns(Ids[Index])));
	}
	if (!Seen.Contains(Initial) && Initial != TEXT("None"))
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO spawn_weapon (MatchId,ParticipantId,SpawnNumber,WeaponId,bIsDefaultWeapon,InitialAmmo,bUnlimitedAmmo) VALUES (%s,%d,%d,%s,0,%s)"),
			*Q(MatchId), Life->ParticipantId, Life->SpawnNumber, *Q(Initial), *AmmoColumns(Initial)));
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
	PendingEquipByPawn.Remove(Pawn); // equips seen before this point belong to the life that just ended
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

int32 UArenaMatchLogSubsystem::RecordFire(int32 AttackId, int32 ShooterParticipantId, int32 ShooterSpawnNumber, const FString& WeaponId, const UObject* WeaponData, AActor* WeaponActor)
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

	// Fire context: read now, before the shot's follow-up effects (auto swap, target change) can alter it.
	Fire.EquippedWeaponId = ReadEquippedWeapon(ShooterParticipantId, ShooterSpawnNumber);
	const FLifeState* ShooterLife = Lives.Find(LifeKey(ShooterParticipantId, ShooterSpawnNumber));
	APawn* ShooterPawn = ShooterLife ? ShooterLife->Pawn.Get() : nullptr;
	if (ShooterPawn && !ShooterLife->bEnded)
	{
		double Distance = -1.0;
		int32 TargetSpawn = INDEX_NONE;
		const int32 TargetId = FindTargetPawnLife(ShooterPawn->GetController(), TargetSpawn, Distance);
		if (TargetId != INDEX_NONE)
		{
			Fire.TargetParticipantId = TargetId;
			Fire.TargetSpawnNumber = TargetSpawn;
			Fire.TargetDistance = Distance;
		}
	}

	// Ammo: the weapon blueprint spends the shot before it reports it, so the value now is the "after".
	if (IsValid(WeaponActor) && ShooterPawn)
	{
		FAmmoReading Now;
		if (ReadAmmo(WeaponActor, Now))
		{
			const FAmmoReading* Previous = LastAmmo.Find(WeaponActor);
			const int32 Before = Previous ? Previous->Count : 0;
			WriteAmmoChange(ShooterPawn, Now, (Previous && !Now.bUnlimited) ? &Before : nullptr, TEXT("Fire"), Seq);
		}
	}
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
		Enqueue(FString::Printf(TEXT("INSERT INTO fire_context (MatchId,EventSequence,TargetParticipantId,TargetSpawnNumber,TargetDistance,EquippedWeaponId) VALUES (%s,%d,%s,%s,%s,%s)"),
			*Q(MatchId), Fire.EventSeq, *I(Fire.TargetParticipantId), *I(Fire.TargetSpawnNumber),
			*NOrNull(Fire.TargetParticipantId != INDEX_NONE && Fire.TargetDistance >= 0.0, Fire.TargetDistance), *QOrNull(Fire.EquippedWeaponId)));
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
	Enqueue(FString::Printf(TEXT("INSERT INTO damage (MatchId,EventSequence,AttackId,DamageSource,HitType,AttackerParticipantId,AttackerSpawnNumber,VictimParticipantId,VictimSpawnNumber,bSelfDamage,AttackerBuffTag,VictimBuffTag,InputDamage,HealthBefore,HealthAfter,ArmorBefore,ArmorAfter,EngagementDistance,bBuffsRecorded) VALUES (%s,%d,%s,%s,%s,%s,%s,%d,%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)"),
		*Q(MatchId), DamageSeq, *I(Record.AttackId), bWeapon ? TEXT("'Weapon'") : TEXT("'Environment'"), *HitType,
		*I(bWeapon ? Record.AttackerParticipantId : INDEX_NONE), *I(bWeapon ? Record.AttackerSpawnNumber : INDEX_NONE),
		Record.VictimParticipantId, Record.VictimSpawnNumber, *B(bSelf),
		*QOrNull(Record.AttackerBuffTag), *QOrNull(Record.VictimBuffTag), *N(Input),
		*NOrNull(bHasState, Record.Before.Health), *NOrNull(bHasState, Record.After.Health),
		*NOrNull(bHasState, Record.Before.Armor), *NOrNull(bHasState, Record.After.Armor),
		*NOrNull(bWeapon && Record.Distance >= 0.0, Record.Distance), *B(Record.bBuffsRecorded)));
	// Only the effects the calculation used get a row; a damage with no buff on either side has none.
	for (const FArenaBuffLink& Link : Record.AttackerBuffs)
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO damage_buff (MatchId,DamageEventSequence,BuffId,BuffEventSequence,Role) VALUES (%s,%d,%d,%d,'Attacker')"),
			*Q(MatchId), DamageSeq, Link.BuffId, Link.EventSequence));
	}
	for (const FArenaBuffLink& Link : Record.VictimBuffs)
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO damage_buff (MatchId,DamageEventSequence,BuffId,BuffEventSequence,Role) VALUES (%s,%d,%d,%d,'Victim')"),
			*Q(MatchId), DamageSeq, Link.BuffId, Link.EventSequence));
	}

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
		const FActiveBuff* ActiveEffect = ActiveBuffs.Find(Victim);
		RecordBuff(Victim, FString(), TEXT("Expire"), 0.0, false, Seq, ActiveEffect ? ActiveEffect->BuffId : INDEX_NONE);
		ActiveBuffs.Remove(Victim);
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
	if (bDisabled || !IsValid(Pawn))
	{
		return;
	}
	if (!FindLife(Pawn, Participant, SpawnNumber))
	{
		// The loadout is granted before the controller possesses the pawn; keep the last equip for the spawn row.
		PendingEquipByPawn.Add(Pawn, After);
		return;
	}
	if (FLifeState* Life = Lives.Find(LifeKey(Participant, SpawnNumber)))
	{
		if (Life->bEnded)
		{
			PendingEquipByPawn.Add(Pawn, After); // pooled respawn: the grant arrives while the pawn still maps to the ended life
			return;
		}
		if (!Life->bSpawnWritten)
		{
			Life->PendingInitialEquip = After; // covered by spawn.InitialEquippedWeaponId
			return;
		}
	}
	NoteEquipped(Participant, SpawnNumber, After);
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
	if (ItemCategoryById.FindRef(*ItemId) == TEXT("Buff"))
	{
		// The buff component may report the new effect before or after this event; link whichever came second.
		BuffPickupByPawn.Add(Pawn, TPair<int32, uint64>(Seq, GFrameCounter));
		if (const TPair<int32, uint64>* Waiting = UnlinkedBuffByPawn.Find(Pawn); Waiting && Waiting->Value == GFrameCounter)
		{
			Enqueue(FString::Printf(TEXT("UPDATE buff_instance SET SourceEventSequence=%d WHERE MatchId=%s AND BuffId=%d AND SourceEventSequence IS NULL"),
				Seq, *Q(MatchId), Waiting->Key));
			UnlinkedBuffByPawn.Remove(Pawn);
		}
	}
	Enqueue(FString::Printf(TEXT("INSERT INTO pickup(MatchId,EventSequence,ParticipantId,SpawnNumber,SpawnedItemId,HealthBefore,ArmorBefore,ItemAmount,AppliedAmount) VALUES (%s,%d,%d,%d,%d,%s,%s,%s,%s)"),
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

int32 UArenaMatchLogSubsystem::RecordBuff(APawn* Pawn, const FString& BuffAfter, const FString& ChangeType, double ExpiresAtSeconds, bool bHasExpiry, int32 CauseSeq, int32 BuffId, int32 PreviousBuffId)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (bDisabled || !FindLife(Pawn, Participant, SpawnNumber))
	{
		return INDEX_NONE;
	}
	const int32 Seq = AllocateEvent(TEXT("BuffChanged"), CauseSeq);
	if (Seq != INDEX_NONE)
	{
		Enqueue(FString::Printf(TEXT("INSERT INTO buff (MatchId,EventSequence,ParticipantId,SpawnNumber,BuffAfter,BuffChangeType,BuffExpiresAtSeconds,BuffId,PreviousBuffId) VALUES (%s,%d,%d,%d,%s,%s,%s,%s,%s)"),
			*Q(MatchId), Seq, Participant, SpawnNumber, *QOrNull(BuffAfter), *Q(ChangeType), *NOrNull(bHasExpiry, ExpiresAtSeconds), *I(BuffId), *I(PreviousBuffId)));
	}
	return Seq;
}

bool UArenaMatchLogSubsystem::ReadMatchEnded()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const AGameStateBase* GameState = World->GetGameState();
	const AGameModeBase* GameMode = World->GetAuthGameMode();
	if (GameState && ReadBool(GameState->GetClass(), GameState, TEXT("bIsMatchEnded")))
	{
		return true;
	}
	if (GameMode && ReadBool(GameMode->GetClass(), GameMode, TEXT("bIsMatchEnded")))
	{
		return true;
	}
	// The game's own end flow also clears the game mode's bMatchStarted (see AMatchRulesInfo::EndMatchOnServer);
	// a match that was running and is no longer started has ended even if the state flag is not visible here.
	if (GameMode && FindLoose(GameMode->GetClass(), TEXT("bMatchStarted")))
	{
		const bool bStarted = ReadBool(GameMode->GetClass(), GameMode, TEXT("bMatchStarted"));
		bSeenModeStarted |= bStarted;
		if (bSeenModeStarted && !bStarted)
		{
			return true;
		}
	}
	// The match clock is past the configured limit and the game reported no end (seen on a separate dedicated server):
	// the time limit is itself the end condition, so close the match as a time-limit end and say so in the log.
	if (TimeLimitSeconds > 0 && bClockAnchored && !(RulesInfo.IsValid() && RulesInfo->bUnlimitedTime)
		&& ElapsedSeconds() >= double(TimeLimitSeconds) + 1.0)
	{
		double Elapsed = -1.0;
		if (GameState)
		{
			ReadNumber(GameState->GetClass(), GameState, TEXT("ElapsedMatchTime"), Elapsed);
		}
		UE_LOG(LogArenaMatchLog, Warning, TEXT("No end signal %.1f s after the %d s limit; ending the match as TimeLimit (GameState %s bIsMatchEnded=%d, ElapsedMatchTime=%.1f; GameMode %s bMatchStarted=%d)"),
			ElapsedSeconds() - double(TimeLimitSeconds), TimeLimitSeconds,
			GameState ? *GameState->GetClass()->GetName() : TEXT("none"),
			GameState && ReadBool(GameState->GetClass(), GameState, TEXT("bIsMatchEnded")) ? 1 : 0, Elapsed,
			GameMode ? *GameMode->GetClass()->GetName() : TEXT("none"),
			GameMode && ReadBool(GameMode->GetClass(), GameMode, TEXT("bMatchStarted")) ? 1 : 0);
		return true;
	}
	return false;
}

void UArenaMatchLogSubsystem::Tick()
{
	if (bMatchEnded)
	{
		Flush();
		return;
	}
	if (!bMatchStarted)
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
	if (!CollectWrite(false)) return;
	if (!InFlightSql)
	{
		if (PendingSql.IsEmpty())
		{
			MaybeValidate();
			return;
		}
		InFlightSql = MakeShared<TArray<FString>, ESPMode::ThreadSafe>(MoveTemp(PendingSql));
	}
	const bool bRetry = FlushAttempts > 0;
	WriteFuture = Async(EAsyncExecution::ThreadPool, [Database = Db, Statements = InFlightSql, bRetry]
	{
		return WriteBatch(*Database, *Statements, bRetry);
	});
}

UArenaMatchLogSubsystem::FWriteResult UArenaMatchLogSubsystem::WriteBatch(
	FSQLiteDatabase& Database, const TArray<FString>& Statements, bool bRetry)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArenaMatchLog_WriteBatch);
	FWriteResult Result;
	if (!Database.Execute(TEXT("BEGIN IMMEDIATE")))
	{
		Result.TransientError = Database.GetLastError();
		return Result;
	}

	for (const FString& Sql : Statements)
	{
		if (Database.Execute(bRetry ? *AsIdempotent(Sql) : *Sql)) continue;
		const FString Error = Database.GetLastError();
		if (IsTransientError(Error))
		{
			Result.TransientError = Error;
			break;
		}
		Result.Rejected.Add(Sql + TEXT(" -- ") + Error.Replace(TEXT("\n"), TEXT(" ")));
		UE_LOG(LogArenaMatchLog, Error, TEXT("Write rejected: %s\n  %s"), *Error, *Sql.Left(300));
	}
	if (Result.TransientError.IsEmpty() && !Database.Execute(TEXT("COMMIT")))
	{
		Result.TransientError = Database.GetLastError();
	}
	if (!Result.TransientError.IsEmpty()) Database.Execute(TEXT("ROLLBACK"));
	return Result;
}

bool UArenaMatchLogSubsystem::CollectWrite(bool bWait)
{
	if (!WriteFuture.IsValid()) return true;
	if (!bWait && !WriteFuture.IsReady()) return false;

	FWriteResult Result = WriteFuture.Consume();
	if (Result.TransientError.IsEmpty() && SimulatedCommitFailures > 0)
	{
		--SimulatedCommitFailures;
		Result.TransientError = TEXT("simulated commit failure (batch is already stored)");
	}
	if (!Result.TransientError.IsEmpty())
	{
		++FlushAttempts;
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Flush failed (attempt %d/%d): %s"), FlushAttempts, MaxFlushAttempts, *Result.TransientError);
		if (FlushAttempts < MaxFlushAttempts) return true;

		Spool(*InFlightSql, Result.TransientError);
		FailedWrites += InFlightSql->Num();
	}
	else
	{
		FailedWrites += Result.Rejected.Num();
		Spool(Result.Rejected, TEXT("rejected by the database"));
	}
	InFlightSql.Reset();
	FlushAttempts = 0;
	return true;
}

void UArenaMatchLogSubsystem::MaybeValidate()
{
	if (!Db || !bMatchEnded || bValidated || InFlightSql || !PendingSql.IsEmpty() || !ActiveDamageSeqs.IsEmpty())
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
		if (Category == TEXT("Ammo"))
		{
			// Which weapon this ammo refills (the row's target item tag), to attribute the pickup to the right weapon.
			const FStructProperty* HandleProperty = CastField<FStructProperty>(FindLoose(Item->GetClass(), TEXT("DataTableHandle")));
			const FDataTableRowHandle* Handle = HandleProperty ? HandleProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(Item) : nullptr;
			const uint8* Row = (Handle && IsValid(Handle->DataTable)) ? Handle->DataTable->FindRowUnchecked(Handle->RowName) : nullptr;
			const FStructProperty* TagProperty = Row ? CastField<FStructProperty>(FindLoose(Handle->DataTable->GetRowStruct(), TEXT("targetItemId"))) : nullptr;
			if (TagProperty && TagProperty->Struct == TBaseStructure<FGameplayTag>::Get())
			{
				ItemTargetTagById.Add(Id, TagProperty->ContainerPtrToValuePtr<FGameplayTag>(Row)->ToString());
			}
		}
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
	TMap<TWeakObjectPtr<AActor>, int32> AmmoBeforeSnapshot;
	if (ItemCategoryById.FindRef(*Id) == TEXT("Ammo"))
	{
		// Applied ammo = how much the holder's weapons actually gained (full magazines absorb less than offered).
		if (const TMap<TWeakObjectPtr<AActor>, int32>* Before = AmmoBefore.Find(Picker); Before && !Before->IsEmpty())
		{
			AmmoBeforeSnapshot = *Before;
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
	if (const int32* PickupSeq = PickupSeqs.Find(*Id); PickupSeq && (Category == TEXT("Ammo") || Category == TEXT("HeldItem")))
	{
		// Ammo handed out by this pickup, one ammo_change per weapon it reached (also when a full weapon absorbed nothing).
		TArray<FAmmoReading> Readings;
		CollectWeaponAmmo(Pawn, Readings);
		const FString* TargetTag = ItemTargetTagById.Find(*Id);
		bool bTagMatched = false;
		if (Category == TEXT("Ammo") && TargetTag)
		{
			for (const FAmmoReading& Reading : Readings)
			{
				bTagMatched |= (Reading.ItemTag == *TargetTag);
			}
		}
		for (const FAmmoReading& Reading : Readings)
		{
			const FAmmoReading* Known = LastAmmo.Find(Reading.Weapon);
			const int32* Snapshot = AmmoBeforeSnapshot.Find(Reading.Weapon);
			// A weapon this log has not seen yet had no ammo for this participant until this pickup.
			const int32 Before = Snapshot ? *Snapshot : (Known ? Known->Count : 0);
			const bool bChanged = !Reading.bUnlimited && Before != Reading.Count;
			const bool bSelected = (Category == TEXT("Ammo") && bTagMatched) ? (Reading.ItemTag == *TargetTag) : (bChanged || !Known);
			if (bSelected)
			{
				WriteAmmoChange(Pawn, Reading, Reading.bUnlimited ? nullptr : &Before, TEXT("Pickup"), *PickupSeq);
			}
			else
			{
				LastAmmo.Add(Reading.Weapon, Reading);
			}
		}
	}
	PickupBefore.Remove(Picker);
	// A picked-up item is gone from play even when its actor is kept alive (buff items are not destroyed on pickup),
	// so close it here; the destroy hook and the match-end sweep skip items that are no longer open.
	if (PickupSeqs.Contains(*Id))
	{
		RecordItemRemove(Item, TEXT("PickedUp"));
	}
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
	// The exact effect is the data row (Buff.Add.Atk vs Buff.Multi.Atk share one tag); fall back to the tag only when the row is unreadable.
	FString Row;
	ReadString(BuffComponent->GetClass(), BuffComponent, TEXT("currentBuffRowName"), Row);
	if (Row.IsEmpty())
	{
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Buff on %s: currentBuffRowName is empty; using the tag %s as the effect"), *GetNameSafe(Pawn), *Tag);
		++FailedWrites;
		Row = Tag;
	}
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (!FindLife(Pawn, Participant, SpawnNumber))
	{
		return;
	}
	const FActiveBuff* Previous = ActiveBuffs.Find(Pawn);
	const bool bHadBuff = Previous && Previous->BuffId != INDEX_NONE;
	const TCHAR* Type = !bHadBuff ? TEXT("Gain") : (Previous->Row == Row ? TEXT("Refresh") : TEXT("Replace"));
	const int32 PreviousId = (bHadBuff && FCString::Strcmp(Type, TEXT("Replace")) == 0) ? Previous->BuffId : INDEX_NONE;
	const bool bNewInstance = FCString::Strcmp(Type, TEXT("Refresh")) != 0;
	const int32 BuffId = bNewInstance ? NextBuffId++ : Previous->BuffId;

	if (bNewInstance)
	{
		// Source event: the pickup of this buff item when it was recorded in the same frame, otherwise linked when it arrives.
		int32 SourceSeq = INDEX_NONE;
		if (const TPair<int32, uint64>* Pickup = BuffPickupByPawn.Find(Pawn); Pickup && Pickup->Value == GFrameCounter)
		{
			SourceSeq = Pickup->Key;
		}
		else
		{
			UnlinkedBuffByPawn.Add(Pawn, TPair<int32, uint64>(BuffId, GFrameCounter));
		}
		FString SourceTable = TEXT("DT_BuffTable");
		if (const FObjectPropertyBase* ItemProperty = CastField<FObjectPropertyBase>(FindLoose(BuffComponent->GetClass(), TEXT("CurrentBuffItem"))))
		{
			if (const UObject* BuffItem = ItemProperty->GetObjectPropertyValue_InContainer(BuffComponent))
			{
				if (const FStructProperty* HandleProperty = CastField<FStructProperty>(FindLoose(BuffItem->GetClass(), TEXT("DataTableHandle"))))
				{
					const FDataTableRowHandle* Handle = HandleProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(BuffItem);
					if (Handle && IsValid(Handle->DataTable))
					{
						SourceTable = Handle->DataTable->GetName();
					}
				}
			}
		}
		Enqueue(FString::Printf(TEXT("INSERT INTO buff_instance (MatchId,BuffId,ParticipantId,SpawnNumber,SettingsVersion,SourceTable,RowName,SourceEventSequence) VALUES (%s,%d,%d,%d,%s,%s,%s,%s)"),
			*Q(MatchId), BuffId, Participant, SpawnNumber, *Q(SettingsVersionId), *Q(SourceTable), *Q(Row), *I(SourceSeq)));
	}
	BuffByPawn.Add(Pawn, Tag);
	const int32 Seq = RecordBuff(Pawn, Tag, Type, Expires, bHasExpiry, INDEX_NONE, BuffId, PreviousId);
	FActiveBuff& Active = ActiveBuffs.FindOrAdd(Pawn);
	Active.BuffId = BuffId;
	Active.EventSequence = Seq;
	Active.Row = Row;
	Active.Tag = Tag;
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
	const FActiveBuff* Active = ActiveBuffs.Find(Pawn);
	RecordBuff(Pawn, FString(), TEXT("Expire"), 0.0, false, INDEX_NONE, Active ? Active->BuffId : INDEX_NONE);
	ActiveBuffs.Remove(Pawn);
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

// ===== Collection version 1: ammo, fire context, applied settings, buff effects, player behaviour =====

bool UArenaMatchLogSubsystem::ReadAmmo(const AActor* Weapon, FAmmoReading& Out) const
{
	const FStructProperty* CountProperty = IsValid(Weapon) ? CastField<FStructProperty>(FindLoose(Weapon->GetClass(), TEXT("currentUsageCount"))) : nullptr;
	if (!CountProperty)
	{
		return false;
	}
	const void* Struct = CountProperty->ContainerPtrToValuePtr<void>(Weapon);
	double Value = 0.0;
	if (!ReadNumber(CountProperty->Struct, Struct, TEXT("currentUsageCount"), Value))
	{
		return false;
	}
	Out.Weapon = const_cast<AActor*>(Weapon);
	Out.WeaponId = WeaponNameOfItem(Weapon);
	Out.Count = int32(Value);
	// Unlimited ammo: the weapon data has no usable cap (<= 0) or carries the project's "endless" marker count (DA_Pistol: 99999).
	// bIsConsum is not used because it is only set once a weapon is in use, so it reads false for any weapon just granted.
	constexpr double EndlessAmmoMarker = 99999.0;
	double MaxAmmo = 0.0;
	bool bHasMax = false;
	if (const FObjectPropertyBase* DataProperty = CastField<FObjectPropertyBase>(FindLoose(Weapon->GetClass(), TEXT("weaponData"))))
	{
		if (const UObject* Data = DataProperty->GetObjectPropertyValue_InContainer(Weapon))
		{
			bHasMax = ReadNumber(Data->GetClass(), Data, TEXT("maxUsageCount"), MaxAmmo);
		}
	}
	Out.bUnlimited = bHasMax ? (MaxAmmo <= 0.0 || MaxAmmo >= EndlessAmmoMarker)
		: (FindLoose(CountProperty->Struct, TEXT("bIsConsum")) != nullptr && !ReadBool(CountProperty->Struct, Struct, TEXT("bIsConsum")));
	Out.ItemTag.Reset();
	if (const FStructProperty* TagProperty = CastField<FStructProperty>(FindLoose(Weapon->GetClass(), TEXT("gameplayTag"))); TagProperty && TagProperty->Struct == TBaseStructure<FGameplayTag>::Get())
	{
		Out.ItemTag = TagProperty->ContainerPtrToValuePtr<FGameplayTag>(Weapon)->ToString();
	}
	return true;
}

void UArenaMatchLogSubsystem::CollectWeaponAmmo(const AActor* Holder, TArray<FAmmoReading>& Out) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArenaMatchLog_CollectWeaponAmmo);
	Out.Reset();
	const UWorld* World = GetWorld();
	if (!World || !IsValid(Holder))
	{
		return;
	}
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (*It == Holder || !HeldBy(*It, Holder) || !FindLoose(It->GetClass(), TEXT("weaponData")))
		{
			continue;
		}
		FAmmoReading Reading;
		if (ReadAmmo(*It, Reading))
		{
			Out.Add(Reading);
		}
	}
}

void UArenaMatchLogSubsystem::WriteAmmoChange(APawn* Pawn, const FAmmoReading& Now, const int32* Before, const TCHAR* Reason, int32 CauseSeq)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (bDisabled || !FindLife(Pawn, Participant, SpawnNumber))
	{
		return;
	}
	// The cause event is allocated before this one, so the cause always has the smaller sequence.
	const int32 Seq = AllocateEvent(TEXT("AmmoChanged"), CauseSeq);
	if (Seq == INDEX_NONE)
	{
		return;
	}
	const bool bHasValues = !Now.bUnlimited;
	Enqueue(FString::Printf(TEXT("INSERT INTO ammo_change (MatchId,EventSequence,ParticipantId,SpawnNumber,WeaponId,bUnlimitedAmmo,AmmoBefore,AmmoAfter,ChangeReason) VALUES (%s,%d,%d,%d,%s,%s,%s,%s,%s)"),
		*Q(MatchId), Seq, Participant, SpawnNumber, *Q(Now.WeaponId), *B(Now.bUnlimited),
		*NOrNull(bHasValues && Before != nullptr, Before ? double(*Before) : 0.0), *NOrNull(bHasValues, double(Now.Count)), *Q(Reason)));
	if (bHasValues && !Before)
	{
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Ammo change %d (%s): the ammo before is unknown; AmmoBefore stays NULL"), Seq, Reason);
		++FailedWrites;
	}
	LastAmmo.Add(Now.Weapon, Now);
}

void UArenaMatchLogSubsystem::NoteEquipped(int32 ParticipantId, int32 SpawnNumber, const FString& WeaponId)
{
	EquippedByLife.Add(LifeKey(ParticipantId, SpawnNumber), WeaponId);
}

FString UArenaMatchLogSubsystem::ReadEquippedWeapon(int32 ParticipantId, int32 SpawnNumber) const
{
	const FString* Weapon = EquippedByLife.Find(LifeKey(ParticipantId, SpawnNumber));
	return Weapon ? *Weapon : FString();
}

int32 UArenaMatchLogSubsystem::FindTargetPawnLife(AController* Controller, int32& OutSpawnNumber, double& OutDistance) const
{
	OutSpawnNumber = INDEX_NONE;
	OutDistance = -1.0;
	// Only a target the game itself selected counts: AI controllers keep it in CurrentTarget; players have none.
	if (!IsValid(Controller) || Controller->IsPlayerController() || !IsValid(Controller->GetPawn()))
	{
		return INDEX_NONE;
	}
	const FStructProperty* TargetProperty = CastField<FStructProperty>(FindLoose(Controller->GetClass(), TEXT("CurrentTarget")));
	if (!TargetProperty)
	{
		return INDEX_NONE;
	}
	const void* Struct = TargetProperty->ContainerPtrToValuePtr<void>(Controller);
	APawn* TargetPawn = nullptr;
	for (TFieldIterator<FObjectPropertyBase> It(TargetProperty->Struct); It && !TargetPawn; ++It)
	{
		UObject* Object = It->GetObjectPropertyValue_InContainer(Struct);
		TargetPawn = Cast<APawn>(Object);
		if (!TargetPawn)
		{
			if (const AController* TargetController = Cast<AController>(Object))
			{
				TargetPawn = TargetController->GetPawn();
			}
		}
	}
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (!IsValid(TargetPawn) || !FindLife(TargetPawn, Participant, SpawnNumber))
	{
		return INDEX_NONE;
	}
	const FLifeState* TargetLife = Lives.Find(LifeKey(Participant, SpawnNumber));
	if (!TargetLife || TargetLife->bEnded || !TargetLife->bSpawnWritten)
	{
		return INDEX_NONE;
	}
	OutSpawnNumber = SpawnNumber;
	OutDistance = FVector::Dist(Controller->GetPawn()->GetActorLocation(), TargetPawn->GetActorLocation());
	return Participant;
}

void UArenaMatchLogSubsystem::CaptureBuffLinks(FArenaDamageRecord& Record, const APawn* Victim)
{
	Record.AttackerBuffs.Reset();
	Record.VictimBuffs.Reset();
	Record.bBuffsRecorded = false;

	bool bVictimKnown = false;
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (IsValid(Victim) && FindLife(Victim, Participant, SpawnNumber) && Participant == Record.VictimParticipantId && SpawnNumber == Record.VictimSpawnNumber)
	{
		bVictimKnown = true;
		if (const FActiveBuff* Active = ActiveBuffs.Find(Victim); Active && Active->BuffId != INDEX_NONE)
		{
			Record.VictimBuffs.Add({Active->BuffId, Active->EventSequence});
		}
	}
	bool bAttackerKnown = true;
	if (Record.AttackId != INDEX_NONE)
	{
		// The attacker's current pawn may belong to a later life than the shot; then its buffs are not the ones used.
		bAttackerKnown = false;
		const APawn* Attacker = Record.AttackerPawn.Get();
		if (IsValid(Attacker) && FindLife(Attacker, Participant, SpawnNumber) && Participant == Record.AttackerParticipantId && SpawnNumber == Record.AttackerSpawnNumber)
		{
			bAttackerKnown = true;
			if (const FActiveBuff* Active = ActiveBuffs.Find(Attacker); Active && Active->BuffId != INDEX_NONE)
			{
				Record.AttackerBuffs.Add({Active->BuffId, Active->EventSequence});
			}
		}
	}
	Record.bBuffsRecorded = bVictimKnown && bAttackerKnown;
}

void UArenaMatchLogSubsystem::EnqueueSetting(int32 ParticipantId, const FString& SourceTable, const FString& RowName, const FString& Json)
{
	Enqueue(FString::Printf(TEXT("INSERT INTO participant_settings (MatchId,ParticipantId,SourceTable,RowName,\"Values\") VALUES (%s,%d,%s,%s,%s)"),
		*Q(MatchId), ParticipantId, *Q(SourceTable), *Q(RowName), *Q(Json)));
}

void UArenaMatchLogSubsystem::ArchiveParticipantSettings(int32 ParticipantId, AController* Controller, const FString& CharacteristicId)
{
	// What was really applied to this participant: its pawn's life values and movement, every data asset its pawn,
	// components and controller point at, and for bots the data rows their decisions read. Nothing is invented.
	TSet<FString> Seen;
	const auto AddObject = [&](const FString& Table, const FString& Row, const UStruct* Struct, const void* Data)
	{
		if (Seen.Contains(Table + TEXT("|") + Row))
		{
			return;
		}
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (FJsonObjectConverter::UStructToJsonObject(Struct, Data, Json, 0, 0))
		{
			Seen.Add(Table + TEXT("|") + Row);
			EnqueueSetting(ParticipantId, Table, Row, ToJsonText(Json));
		}
	};
	const auto ScanDataAssets = [&](const UObject* Owner)
	{
		for (TFieldIterator<FObjectPropertyBase> It(Owner->GetClass()); It; ++It)
		{
			const UObject* Asset = It->GetObjectPropertyValue_InContainer(Owner);
			if (Asset && Asset->IsA<UDataAsset>())
			{
				AddObject(FString(TEXT("DataAsset:")) + Asset->GetClass()->GetName(), Asset->GetName(), Asset->GetClass(), Asset);
			}
		}
	};

	APawn* Pawn = IsValid(Controller) ? Controller->GetPawn() : nullptr;
	if (IsValid(Pawn))
	{
		TArray<UActorComponent*> Components;
		Pawn->GetComponents(Components);
		bool bLifeInfo = false;
		for (UActorComponent* Component : Components)
		{
			if (!IsValid(Component))
			{
				continue;
			}
			if (!bLifeInfo)
			{
				if (const FStructProperty* LifeInfo = CastField<FStructProperty>(FindLoose(Component->GetClass(), TEXT("LifeInfo"))))
				{
					AddObject(TEXT("Pawn:LifeInfo"), Pawn->GetClass()->GetName(), LifeInfo->Struct, LifeInfo->ContainerPtrToValuePtr<void>(Component));
					bLifeInfo = true;
				}
			}
			ScanDataAssets(Component);
		}
		ScanDataAssets(Pawn);
		if (const UCharacterMovementComponent* Move = Pawn->FindComponentByClass<UCharacterMovementComponent>())
		{
			TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetNumberField(TEXT("maxWalkSpeed"), Move->MaxWalkSpeed);
			Json->SetNumberField(TEXT("maxWalkSpeedCrouched"), Move->MaxWalkSpeedCrouched);
			Json->SetNumberField(TEXT("maxAcceleration"), Move->MaxAcceleration);
			Json->SetNumberField(TEXT("jumpZVelocity"), Move->JumpZVelocity);
			Json->SetNumberField(TEXT("gravityScale"), Move->GravityScale);
			Json->SetNumberField(TEXT("airControl"), Move->AirControl);
			Json->SetNumberField(TEXT("groundFriction"), Move->GroundFriction);
			Json->SetNumberField(TEXT("brakingDecelerationWalking"), Move->BrakingDecelerationWalking);
			Seen.Add(TEXT("Pawn:CharacterMovement|") + Pawn->GetClass()->GetName());
			EnqueueSetting(ParticipantId, TEXT("Pawn:CharacterMovement"), Pawn->GetClass()->GetName(), ToJsonText(Json));
		}
	}
	else
	{
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Participant %d: no pawn when archiving applied settings"), ParticipantId);
		++FailedWrites;
	}
	if (IsValid(Controller))
	{
		ScanDataAssets(Controller);
		if (!Controller->IsPlayerController())
		{
			// Rows the bot's weapon / item / retreat decisions read; whole tables because the lookup key is chosen at play time.
			const auto AddTable = [&](const TCHAR* Path, const TCHAR* Name, const FString& OnlyRow)
			{
				const UDataTable* Table = LoadObject<UDataTable>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
				if (!Table || !Table->GetRowStruct())
				{
					return;
				}
				TSharedRef<FJsonObject> Rows = MakeShared<FJsonObject>();
				for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
				{
					if (!OnlyRow.IsEmpty() && Pair.Key.ToString() != OnlyRow)
					{
						continue;
					}
					TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
					if (FJsonObjectConverter::UStructToJsonObject(Table->GetRowStruct(), Pair.Value, Row, 0, 0))
					{
						Rows->SetObjectField(Pair.Key.ToString(), Row);
					}
				}
				const FString Key = FString(TEXT("DataTable:")) + Name;
				if (!Seen.Contains(Key + TEXT("|") + (OnlyRow.IsEmpty() ? FString(Name) : OnlyRow)))
				{
					Seen.Add(Key + TEXT("|") + (OnlyRow.IsEmpty() ? FString(Name) : OnlyRow));
					EnqueueSetting(ParticipantId, Key, OnlyRow.IsEmpty() ? FString(Name) : OnlyRow, ToJsonText(Rows));
				}
			};
			AddTable(TEXT("/Game/QuakeLike_1_0/Data/AI/DT_Characteristic.DT_Characteristic"), TEXT("DT_Characteristic"), CharacteristicId);
			AddTable(TEXT("/Game/QuakeLike_1_0/Data/AI/DT_WeaponChange_Weight.DT_WeaponChange_Weight"), TEXT("DT_WeaponChange_Weight"), FString());
			AddTable(TEXT("/Game/QuakeLike_1_0/Data/AI/DT_Item_Weight.DT_Item_Weight"), TEXT("DT_Item_Weight"), FString());
		}
	}
	if (Seen.IsEmpty())
	{
		UE_LOG(LogArenaMatchLog, Warning, TEXT("Participant %d: nothing could be read for participant_settings"), ParticipantId);
		++FailedWrites;
	}
}

// ---- player behaviour observation ---------------------------------------------------------------

void UArenaMatchLogSubsystem::BeginBehaviorObservation()
{
	float Interval = 0.0f;
	if (!FParse::Value(FCommandLine::Get(), TEXT("-ArenaBehaviorSampleInterval="), Interval) || Interval <= 0.0f)
	{
		const UArenaLoggingDataAsset* Asset = LoadObject<UArenaLoggingDataAsset>(nullptr, UArenaLoggingDataAsset::GetAssetPath(), nullptr, LOAD_NoWarn | LOAD_Quiet);
		Interval = Asset ? Asset->BehaviorSampleIntervalSeconds : 0.0f;
	}
	if (Interval <= 0.0f)
	{
		return; // observation is opt-in; without a chosen interval nothing is observed and no config row exists
	}
	BehaviorSampleInterval = Interval;
	bBehaviorEnabled = true;
	Enqueue(FString::Printf(TEXT("INSERT INTO observation_config (MatchId,ObservationVersion,SampleIntervalSeconds,ExposureRule) VALUES (%s,1,%s,%s)"),
		*Q(MatchId), *N(Interval),
		*Q(TEXT("exposed=target location projects inside the local viewport; occluded=visibility line trace from the eye to the target is blocked"))));
	BehaviorTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UArenaMatchLogSubsystem::TickBehavior), Interval);
}

bool UArenaMatchLogSubsystem::TickBehavior(float DeltaTime)
{
	if (!bMatchStarted || bMatchEnded || bDisabled)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	if (!World || !bClockAnchored)
	{
		return true; // the match clock has not started; nothing to observe against yet
	}
	const double Now = ElapsedSeconds();
	for (TPair<int64, FLifeState>& Pair : Lives)
	{
		FLifeState& Life = Pair.Value;
		APawn* Pawn = Life.Pawn.Get();
		const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
		if (!Life.bSpawnWritten || Life.bEnded || !IsValid(Pawn) || !Controller || !Controller->IsPlayerController())
		{
			continue;
		}
		ObserveHumanLife(Life, Pawn, Now);
	}
	return true;
}

void UArenaMatchLogSubsystem::ObserveHumanLife(FLifeState& Life, APawn* Pawn, double Now)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArenaMatchLog_ObserveHumanLife);
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	FBehaviorPawnState& State = BehaviorStates.FindOrAdd(LifeKey(Life.ParticipantId, Life.SpawnNumber));
	const FString Equipped = ReadEquippedWeapon(Life.ParticipantId, Life.SpawnNumber);
	const FVector Velocity = Pawn->GetVelocity();
	const bool bGrounded = Move ? Move->IsMovingOnGround() : false;

	// Fire availability: the weapon's own usable rule is "count above zero" (BP_HeldItem CanUse); unlimited weapons are always usable.
	{
		TArray<FAmmoReading> Held;
		CollectWeaponAmmo(Pawn, Held);
		int32 CanFire = 0;
		FString Reason = TEXT("NoWeapon");
		for (const FAmmoReading& Reading : Held)
		{
			if (Reading.WeaponId == Equipped)
			{
				CanFire = (Reading.bUnlimited || Reading.Count > 0) ? 1 : 0;
				Reason = CanFire ? FString() : FString(TEXT("NoAmmo"));
				break;
			}
		}
		if (State.CanFire != CanFire || State.BlockReason != Reason)
		{
			const int32 Seq = AllocateEvent(TEXT("FireAvailabilityChanged"), INDEX_NONE);
			if (Seq != INDEX_NONE)
			{
				Enqueue(FString::Printf(TEXT("INSERT INTO fire_availability (MatchId,EventSequence,ParticipantId,SpawnNumber,EquippedWeaponId,bCanFire,BlockReason) VALUES (%s,%d,%d,%d,%s,%s,%s)"),
					*Q(MatchId), Seq, Life.ParticipantId, Life.SpawnNumber, *QOrNull(Equipped), *B(CanFire != 0), *QOrNull(Reason)));
				State.CanFire = CanFire;
				State.BlockReason = Reason;
			}
		}
	}

	// A ground-to-air change the player did not cause by a jump is reported through NoteForcedMovement, which also silences this check.
	if (State.bHasPrevious && State.bGrounded && !bGrounded && Velocity.Z > 0.0 && Now > State.ForcedSuppressUntil)
	{
		const int32 Seq = AllocateEvent(TEXT("JumpObserved"), INDEX_NONE);
		if (Seq != INDEX_NONE)
		{
			Enqueue(FString::Printf(TEXT("INSERT INTO jump_observation (MatchId,EventSequence,ParticipantId,SpawnNumber,JumpKind,bGrounded,bCanJump,ForcedCause) VALUES (%s,%d,%d,%d,'Executed',0,NULL,NULL)"),
				*Q(MatchId), Seq, Life.ParticipantId, Life.SpawnNumber));
		}
	}
	State.bGrounded = bGrounded;
	State.bHasPrevious = true;

	// The nearest target the player's machine reported as on screen and not hidden is the sample's observed target.
	// (Exposure itself is reported by UArenaBehaviorReporterComponent; the server only has the verdicts.)
	const APawn* Observed = nullptr;
	int32 ObservedParticipant = INDEX_NONE, ObservedSpawn = INDEX_NONE;
	double ObservedDistance = 0.0;
	for (const TPair<int64, int32>& Entry : State.ExposureState)
	{
		if (Entry.Value != 1)
		{
			continue; // bit0 exposed, bit1 occluded: only "exposed and unoccluded"
		}
		const FLifeState* OtherLife = Lives.Find(Entry.Key);
		const APawn* OtherPawn = OtherLife ? OtherLife->Pawn.Get() : nullptr;
		if (!OtherLife || OtherLife->bEnded || !OtherLife->bSpawnWritten || !IsValid(OtherPawn))
		{
			continue;
		}
		const double Distance = FVector::Dist(Pawn->GetActorLocation(), OtherPawn->GetActorLocation());
		if (!Observed || Distance < ObservedDistance)
		{
			Observed = OtherPawn;
			ObservedParticipant = OtherLife->ParticipantId;
			ObservedSpawn = OtherLife->SpawnNumber;
			ObservedDistance = Distance;
		}
	}

	// Continuous sample. Aim is the controller's aim before any weapon spread; relative motion is measured against the observed target.
	const double Spacing = State.LastSampleAt < 0.0 ? BehaviorSampleInterval : (Now - State.LastSampleAt);
	State.LastSampleAt = Now;
	const FRotator Aim = PC->GetControlRotation();
	const FVector AimVector = Aim.Vector();
	FString Relative = TEXT("NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL");
	if (Observed)
	{
		const FVector Rel = Observed->GetActorLocation() - Pawn->GetActorLocation();
		const FVector Target = Observed->GetVelocity();
		const FVector2D Direction = FVector2D(Rel.X, Rel.Y).GetSafeNormal();
		const double Forward = Velocity.X * Direction.X + Velocity.Y * Direction.Y;
		const double Lateral = Velocity.X * -Direction.Y + Velocity.Y * Direction.X;
		Relative = FString::Printf(TEXT("%d,%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"), ObservedParticipant, ObservedSpawn, *N(ObservedDistance), *N(Rel.Z),
			*N(Rel.X), *N(Rel.Y), *N(Rel.Z), *N(Target.X), *N(Target.Y), *N(Target.Z), *N(Forward), *N(Lateral));
	}
	Enqueue(FString::Printf(TEXT("INSERT OR IGNORE INTO observation_sample (MatchId,ParticipantId,SpawnNumber,ElapsedSeconds,SampleIntervalSeconds,ViewYaw,ViewPitch,AimX,AimY,AimZ,SpeedXY,VelocityZ,bGrounded,bCrouched,EquippedWeaponId,ObservedParticipantId,ObservedSpawnNumber,ObservedDistance,ObservedHeightDelta,ObservedRelX,ObservedRelY,ObservedRelZ,ObservedVelX,ObservedVelY,ObservedVelZ,RelForwardSpeed,RelLateralSpeed) VALUES (%s,%d,%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)"),
		*Q(MatchId), Life.ParticipantId, Life.SpawnNumber, *N(Now), *N(Spacing), *N(Aim.Yaw), *N(Aim.Pitch),
		*N(AimVector.X), *N(AimVector.Y), *N(AimVector.Z), *N(Velocity.Size2D()), *N(Velocity.Z), *B(bGrounded),
		*B(Character && Character->bIsCrouched), *QOrNull(Equipped), *Relative));
}

void UArenaMatchLogSubsystem::RecordAttackInput(APawn* Pawn, bool bPressed)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (bDisabled || !bBehaviorEnabled || !IsValid(Pawn) || !FindLife(Pawn, Participant, SpawnNumber))
	{
		return;
	}
	const FLifeState* Life = Lives.Find(LifeKey(Participant, SpawnNumber));
	if (!Life || Life->bEnded || !Life->bSpawnWritten)
	{
		return;
	}
	FBehaviorPawnState& State = BehaviorStates.FindOrAdd(LifeKey(Participant, SpawnNumber));
	if (State.bAttackHeld == bPressed)
	{
		return; // key repeat is not a new press
	}
	const int32 Seq = AllocateEvent(TEXT("AttackInput"), INDEX_NONE);
	if (Seq == INDEX_NONE)
	{
		return;
	}
	State.bAttackHeld = bPressed;
	Enqueue(FString::Printf(TEXT("INSERT INTO attack_input (MatchId,EventSequence,ParticipantId,SpawnNumber,InputState,EquippedWeaponId) VALUES (%s,%d,%d,%d,%s,%s)"),
		*Q(MatchId), Seq, Participant, SpawnNumber, bPressed ? TEXT("'Start'") : TEXT("'End'"), *QOrNull(ReadEquippedWeapon(Participant, SpawnNumber))));
}

void UArenaMatchLogSubsystem::RecordJumpInput(APawn* Pawn)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (bDisabled || !bBehaviorEnabled || !IsValid(Pawn) || !FindLife(Pawn, Participant, SpawnNumber))
	{
		return;
	}
	const FLifeState* Life = Lives.Find(LifeKey(Participant, SpawnNumber));
	if (!Life || Life->bEnded || !Life->bSpawnWritten)
	{
		return;
	}
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	const int32 Seq = AllocateEvent(TEXT("JumpObserved"), INDEX_NONE);
	if (Seq == INDEX_NONE)
	{
		return;
	}
	Enqueue(FString::Printf(TEXT("INSERT INTO jump_observation (MatchId,EventSequence,ParticipantId,SpawnNumber,JumpKind,bGrounded,bCanJump,ForcedCause) VALUES (%s,%d,%d,%d,'InputPressed',%s,%s,NULL)"),
		*Q(MatchId), Seq, Participant, SpawnNumber, Move ? *B(Move->IsMovingOnGround()) : TEXT("NULL"), Character ? *B(Character->CanJump()) : TEXT("NULL")));
}

void UArenaMatchLogSubsystem::NoteForcedMovement(APawn* Pawn, const FString& Cause)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE;
	if (bDisabled || !bBehaviorEnabled || !IsValid(Pawn) || Cause.IsEmpty() || !FindLife(Pawn, Participant, SpawnNumber))
	{
		return;
	}
	const FLifeState* Life = Lives.Find(LifeKey(Participant, SpawnNumber));
	if (!Life || Life->bEnded || !Life->bSpawnWritten)
	{
		return;
	}
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	const int32 Seq = AllocateEvent(TEXT("JumpObserved"), INDEX_NONE);
	if (Seq == INDEX_NONE)
	{
		return;
	}
	// The take-off that follows belongs to this cause, so the poll must not also report it as a jump the player made.
	BehaviorStates.FindOrAdd(LifeKey(Participant, SpawnNumber)).ForcedSuppressUntil = ElapsedSeconds() + 2.0 * BehaviorSampleInterval;
	Enqueue(FString::Printf(TEXT("INSERT INTO jump_observation (MatchId,EventSequence,ParticipantId,SpawnNumber,JumpKind,bGrounded,bCanJump,ForcedCause) VALUES (%s,%d,%d,%d,'Forced',%s,NULL,%s)"),
		*Q(MatchId), Seq, Participant, SpawnNumber, Move ? *B(Move->IsMovingOnGround()) : TEXT("NULL"), *Q(Cause)));
}

void UArenaMatchLogSubsystem::RecordExposureReport(APawn* Observer, APawn* Target, bool bExposed, bool bOccluded)
{
	int32 Participant = INDEX_NONE, SpawnNumber = INDEX_NONE, OtherParticipant = INDEX_NONE, OtherSpawn = INDEX_NONE;
	if (bDisabled || !bBehaviorEnabled || !IsValid(Observer) || !IsValid(Target) || Observer == Target
		|| !FindLife(Observer, Participant, SpawnNumber) || !FindLife(Target, OtherParticipant, OtherSpawn))
	{
		return;
	}
	const FLifeState* Life = Lives.Find(LifeKey(Participant, SpawnNumber));
	const FLifeState* OtherLife = Lives.Find(LifeKey(OtherParticipant, OtherSpawn));
	if (!Life || Life->bEnded || !Life->bSpawnWritten || !OtherLife || OtherLife->bEnded || !OtherLife->bSpawnWritten)
	{
		return;
	}
	// Distance and height are measured here from the real positions; the client only supplies screen / wall verdicts.
	const int32 Code = bExposed ? (bOccluded ? 3 : 1) : 0;
	FBehaviorPawnState& State = BehaviorStates.FindOrAdd(LifeKey(Participant, SpawnNumber));
	const int64 OtherKey = LifeKey(OtherParticipant, OtherSpawn);
	const int32* Previous = State.ExposureState.Find(OtherKey);
	if ((Previous && *Previous != Code) || (!Previous && bExposed))
	{
		const int32 Seq = AllocateEvent(TEXT("ExposureChanged"), INDEX_NONE);
		if (Seq != INDEX_NONE)
		{
			Enqueue(FString::Printf(TEXT("INSERT INTO exposure_change (MatchId,EventSequence,ParticipantId,SpawnNumber,ObservedParticipantId,ObservedSpawnNumber,bExposed,bOccluded,Distance,HeightDelta) VALUES (%s,%d,%d,%d,%d,%d,%s,%s,%s,%s)"),
				*Q(MatchId), Seq, Participant, SpawnNumber, OtherParticipant, OtherSpawn, *B(bExposed),
				bExposed ? *B(bOccluded) : TEXT("NULL"), *N(FVector::Dist(Observer->GetActorLocation(), Target->GetActorLocation())),
				*N(Target->GetActorLocation().Z - Observer->GetActorLocation().Z)));
		}
	}
	State.ExposureState.Add(OtherKey, Code);
}
