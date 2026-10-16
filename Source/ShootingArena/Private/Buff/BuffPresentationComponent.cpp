#include "Buff/BuffPresentationComponent.h"

#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "DataTableUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/UnrealType.h"

namespace
{
	TAutoConsoleVariable<int32> CVarBuffPickupSoundMode(
		TEXT("Buff.PickupSound.Mode"),
		0,
		TEXT("Buff pickup sound A/B test: 0 = owner-only 2D, 1 = world 3D at pickup location."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarBuffWarningSoundMode(
		TEXT("Buff.WarningSound.Mode"), -1,
		TEXT("Buff warning audio: -1 = DT, 0 = None, 1 = Local, 2 = World, 3 = Both. Set on each test client."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarBuffWarningSoundDebug(
		TEXT("Buff.WarningSound.Debug"), 0,
		TEXT("Log actual warning audio components per interval for multiplayer verification."), ECVF_Default);

	bool IsQuad(FGameplayTag Tag)
	{
		return Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Atk"), false));
	}

	const FProperty* FindBuffRowProperty(const UScriptStruct* RowStruct, const TCHAR* ExportName)
	{
		if (!RowStruct) return nullptr;
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (DataTableUtils::GetPropertyExportName(*It) == ExportName) return *It;
		}
		return nullptr;
	}
}

UBuffPresentationComponent::UBuffPresentationComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UBuffPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	// BPC_Life is a Blueprint component: bind its existing death dispatcher without changing damage/drop code.
	TArray<UActorComponent*> Components;
	GetOwner()->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		const FMulticastDelegateProperty* Death = FindFProperty<FMulticastDelegateProperty>(Component->GetClass(), TEXT("OnDeath"));
		if (!Death || !Death->SignatureFunction->IsSignatureCompatibleWith(FindFunction(GET_FUNCTION_NAME_CHECKED(UBuffPresentationComponent, OnOwnerDeath)))) continue;
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuffPresentationComponent, OnOwnerDeath));
		Death->AddDelegate(Delegate, Component, Death->ContainerPtrToValuePtr<void>(Component));
		LifeComponent = Component;
		break;
	}
	OnRep_Presentation();
}

void UBuffPresentationComponent::OnOwnerDeath(AController* InstigatedBy, AActor* DamageCauser)
{
	if (GetOwner()->HasAuthority()) ClearBuffPresentation();
	else
	{
		// Preserve the revision so an old update cannot re-arm this same grant after death.
		Presentation.Tag = FGameplayTag();
		Presentation.EndServerTime = 0.0;
		NextWarningSecond = 0;
		StopWarningSounds();
		StopEffects();
	}
}

void UBuffPresentationComponent::SetBuffPresentation(FGameplayTag BuffTag, double EndServerTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Buff_SetBuffPresentation);
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	const FNameProperty* RowNameProperty = FindFProperty<FNameProperty>(GetClass(), TEXT("currentBuffRowName"));
	const FName RowName = BuffTag.IsValid() && RowNameProperty ? RowNameProperty->GetPropertyValue_InContainer(this) : NAME_None;
	const double Deadline = FMath::IsFinite(EndServerTime) ? EndServerTime : 0.0;
	const bool bNewGrant = Presentation.Tag != BuffTag || Presentation.RowName != RowName || Presentation.EndServerTime != Deadline;
	const uint32 Revision = Presentation.Revision + (bNewGrant ? 1u : 0u);
	const int32 PreviousWarningStart = Presentation.WarningStartSecond;
	Presentation = FBuffPresentationState();
	Presentation.Tag = BuffTag;
	Presentation.EndServerTime = Deadline;
	Presentation.RowName = RowName;
	Presentation.Revision = Revision;
	const double RemainingAtGrant = GetPresentationRemainingTime();
	// Transfers/shortened durations skip thresholds already passed before this grant, on every client.
	Presentation.WarningStartSecond = bNewGrant
		? (RemainingAtGrant >= 5.0 ? 6 : FMath::Clamp(FMath::FloorToInt(RemainingAtGrant + 0.0001) + 1, 1, 6))
		: PreviousWarningStart;
	if (BuffTag.IsValid())
	{
		// Both Blueprint grant functions set this row before calling us. Keep their damage/drop logic intact.
		UDataTable* Table = BuffDataTable;
		if (Table)
		{
			const uint8* Row = nullptr;
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(Buff_SetBuffPresentation_FindRow);
				Row = RowName.IsNone() ? nullptr : Table->FindRowUnchecked(RowName);
			}
			if (Row && Table->GetRowStruct())
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(Buff_SetBuffPresentation_ExtractVFXAssets);
				// User-defined struct fields have generated GUID names: match their exported designer-facing names.
				for (TFieldIterator<FObjectPropertyBase> It(Table->GetRowStruct()); It; ++It)
				{
					const FString Name = DataTableUtils::GetPropertyExportName(*It);
					if (Name == TEXT("NiagaraVFX"))
						Presentation.NiagaraVFX = Cast<UNiagaraSystem>(It->GetObjectPropertyValue_InContainer(Row));
					else if (Name == TEXT("ParticleVFX"))
						Presentation.ParticleVFX = Cast<UParticleSystem>(It->GetObjectPropertyValue_InContainer(Row));
				}
				const UScriptStruct* RowStruct = Table->GetRowStruct();
				if (const FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(FindBuffRowProperty(RowStruct, TEXT("WarningSound"))))
					Presentation.WarningSound = Cast<USoundBase>(Property->GetObjectPropertyValue_InContainer(Row));
				if (const FBoolProperty* Property = CastField<FBoolProperty>(FindBuffRowProperty(RowStruct, TEXT("WarningLocalEnabled"))))
					Presentation.bWarningLocalEnabled = Property->GetPropertyValue_InContainer(Row);
				if (const FBoolProperty* Property = CastField<FBoolProperty>(FindBuffRowProperty(RowStruct, TEXT("WarningWorldEnabled"))))
					Presentation.bWarningWorldEnabled = Property->GetPropertyValue_InContainer(Row);
				if (const FNumericProperty* Property = CastField<FNumericProperty>(FindBuffRowProperty(RowStruct, TEXT("WarningLocalVolume"))))
					Presentation.WarningLocalVolume = static_cast<float>(Property->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Row)));
				if (const FNumericProperty* Property = CastField<FNumericProperty>(FindBuffRowProperty(RowStruct, TEXT("WarningWorldVolume"))))
					Presentation.WarningWorldVolume = static_cast<float>(Property->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Row)));
				if (const FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(FindBuffRowProperty(RowStruct, TEXT("WarningAttenuation"))))
					Presentation.WarningAttenuation = Cast<USoundAttenuation>(Property->GetObjectPropertyValue_InContainer(Row));
			}
			else UE_LOG(LogTemp, Warning, TEXT("Buff VFX row is missing on %s"), *GetNameSafe(this));
		}
		else UE_LOG(LogTemp, Warning, TEXT("BuffDataTable is not assigned in BPC_Buff defaults on %s"), *GetNameSafe(this));
	}
	OnRep_Presentation();
	GetOwner()->ForceNetUpdate();
}

void UBuffPresentationComponent::ClearBuffPresentation()
{
	SetBuffPresentation(FGameplayTag(), 0.0);
}

void UBuffPresentationComponent::PlayBuffWarningSoundFromDataTable()
{
	if (GetOwner() && GetOwner()->HasAuthority()) UpdateWarningPlayback();
}

bool UBuffPresentationComponent::TryPlayBuffPickupSound(AActor* PickupItem, AActor* BuffOwner, USoundBase* Sound, float Volume)
{
	if (!IsValid(PickupItem)) return false;

	const FStructProperty* HandleProperty = FindFProperty<FStructProperty>(PickupItem->GetClass(), TEXT("DataTable Handle"));
	if (!HandleProperty || HandleProperty->Struct != FDataTableRowHandle::StaticStruct()) return false;
	const FDataTableRowHandle* Handle = HandleProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(PickupItem);
	if (!Handle || !IsValid(Handle->DataTable)
		|| Handle->DataTable->GetPathName() != TEXT("/Game/QuakeLike_1_0/Data/Item/DropTable/DT_BuffTable.DT_BuffTable"))
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(BuffOwner);
	UBuffPresentationComponent* BuffPresentation = IsValid(BuffOwner)
		? BuffOwner->FindComponentByClass<UBuffPresentationComponent>() : nullptr;
	if (!Pawn || !Pawn->IsPlayerControlled() || !BuffOwner->HasAuthority() || !IsValid(BuffPresentation)) return false;

	// The helper handled this buff item even when its sound is intentionally absent or muted.
	if (!IsValid(Sound) || !FMath::IsFinite(Volume) || Volume <= 0.0f) return true;

	if (CVarBuffPickupSoundMode.GetValueOnGameThread() == 1)
	{
		BuffPresentation->MulticastPlayBuffPickupSound(Sound, Volume, PickupItem->GetActorLocation());
	}
	else if (const UWorld* World = BuffPresentation->GetWorld(); World && World->GetNetMode() == NM_Standalone)
	{
		BuffPresentation->PlayBuffPickupSound2D(Sound, Volume);
	}
	else
	{
		BuffPresentation->ClientPlayBuffPickupSound(Sound, Volume);
	}
	return true;
}

void UBuffPresentationComponent::MulticastPlayBuffPickupSound_Implementation(USoundBase* Sound, float Volume, FVector Location)
{
	PlayBuffPickupSound3D(Sound, Volume, Location);
}

void UBuffPresentationComponent::ClientPlayBuffPickupSound_Implementation(USoundBase* Sound, float Volume)
{
	PlayBuffPickupSound2D(Sound, Volume);
}

void UBuffPresentationComponent::PlayBuffPickupSound2D(USoundBase* Sound, float Volume)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer
		|| !IsValid(Sound) || !FMath::IsFinite(Volume) || Volume <= 0.0f)
	{
		return;
	}
	UGameplayStatics::PlaySound2D(World, Sound, Volume);
}

void UBuffPresentationComponent::PlayBuffPickupSound3D(USoundBase* Sound, float Volume, const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer
		|| !IsValid(Sound) || !FMath::IsFinite(Volume) || Volume <= 0.0f
		|| !FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y) || !FMath::IsFinite(Location.Z))
	{
		return;
	}

	if (!IsValid(DefaultPickup3DAttenuation))
	{
		DefaultPickup3DAttenuation = NewObject<USoundAttenuation>(this, NAME_None, RF_Transient);
	}
	FSoundAttenuationSettings& Settings = DefaultPickup3DAttenuation->Attenuation;
	Settings.bAttenuate = true;
	Settings.bSpatialize = true;
	Settings.AttenuationShape = EAttenuationShape::Sphere;
	Settings.AttenuationShapeExtents = FVector(300.0f, 0.0f, 0.0f);
	Settings.FalloffDistance = 1200.0f;
	Settings.DistanceAlgorithm = EAttenuationDistanceModel::Linear;

	UGameplayStatics::PlaySoundAtLocation(World, Sound, Location, FRotator::ZeroRotator,
		Volume, 1.0f, 0.0f, DefaultPickup3DAttenuation);
}

void UBuffPresentationComponent::PlayBuffWarningSound2D(USoundBase* Sound, float Volume)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer
		|| !IsValid(Sound) || !FMath::IsFinite(Volume) || Volume <= 0.0f)
	{
		return;
	}
	StopWarningAudio(WarningLocalAudio);
	WarningLocalAudio = UGameplayStatics::SpawnSound2D(World, Sound, Volume);
}

void UBuffPresentationComponent::PlayBuffWarningSound3D(USoundBase* Sound, float Volume, USoundAttenuation* Attenuation)
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!IsValid(World) || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer
		|| !IsValid(Owner) || !IsValid(Sound) || !FMath::IsFinite(Volume) || Volume <= 0.0f)
	{
		return;
	}

	USceneComponent* AttachTo = nullptr;
	if (const ACharacter* Character = Cast<ACharacter>(Owner)) AttachTo = Character->GetMesh();
	if (!AttachTo) AttachTo = Owner->GetRootComponent();
	if (!AttachTo) return;

	if (!IsValid(Attenuation))
	{
		if (!IsValid(DefaultWarning3DAttenuation))
		{
			DefaultWarning3DAttenuation = NewObject<USoundAttenuation>(this, NAME_None, RF_Transient);
		}
		FSoundAttenuationSettings& Settings = DefaultWarning3DAttenuation->Attenuation;
		Settings.bAttenuate = true;
		Settings.bSpatialize = true;
		Settings.AttenuationShape = EAttenuationShape::Sphere;
		Settings.AttenuationShapeExtents = FVector(300.0f, 0.0f, 0.0f);
		Settings.FalloffDistance = 1200.0f;
		Settings.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
		Attenuation = DefaultWarning3DAttenuation;
	}

	StopWarningAudio(WarningWorldAudio);
	WarningWorldAudio = UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, NAME_None, FVector::ZeroVector,
		FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, Volume, 1.0f, 0.0f, Attenuation);
}

int32 UBuffPresentationComponent::ConsumeWarningSecond(double Remaining)
{
	if (!FMath::IsFinite(Remaining) || Remaining <= 0.0)
	{
		NextWarningSecond = 0;
		return 0;
	}
	const int32 Second = Remaining > 5.0 ? 6 : FMath::CeilToInt(Remaining);
	if (Second >= NextWarningSecond) return 0;
	// Consume skipped intervals too, so a hitch or clock correction cannot replay them later.
	NextWarningSecond = Second;
	return Second <= 5 ? Second : 0;
}

int32 UBuffPresentationComponent::GetWarningOutputMask() const
{
	const int32 Mode = CVarBuffWarningSoundMode.GetValueOnGameThread();
	if (Mode >= 0 && Mode <= 3) return Mode;
	return (Presentation.bWarningLocalEnabled ? 1 : 0) | (Presentation.bWarningWorldEnabled ? 2 : 0);
}

void UBuffPresentationComponent::UpdateWarningPlayback()
{
	UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();
	if (!World || World->bIsTearingDown || !IsValid(Owner) || Owner->IsActorBeingDestroyed()
		|| !Presentation.Tag.IsValid())
	{
		NextWarningSecond = 0;
		StopWarningSounds();
		return;
	}
	if (World->GetNetMode() == NM_DedicatedServer) return;
	if (World->GetNetMode() == NM_Client && !World->GetGameState()) return;
	const double Remaining = GetPresentationRemainingTime();
	if (Remaining <= 0.0)
	{
		NextWarningSecond = 0;
		StopWarningSounds();
		return;
	}

	const APawn* Pawn = Cast<APawn>(Owner);
	const int32 Mask = GetWarningOutputMask();
	const bool bLocal = (Mask & 1) && Pawn && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled()
		&& FMath::IsFinite(Presentation.WarningLocalVolume) && Presentation.WarningLocalVolume > 0.0f;
	const bool bWorld = (Mask & 2) && FMath::IsFinite(Presentation.WarningWorldVolume) && Presentation.WarningWorldVolume > 0.0f;
	if (!bLocal || !IsValid(Presentation.WarningSound)) StopWarningAudio(WarningLocalAudio);
	if (!bWorld || !IsValid(Presentation.WarningSound)) StopWarningAudio(WarningWorldAudio);

	const int32 Second = ConsumeWarningSecond(Remaining);
	if (Second == 0 || !IsValid(Presentation.WarningSound) || (!bLocal && !bWorld)) return;
	const double Now = Presentation.EndServerTime - Remaining;
	// A refresh inside the same interval must not emit the same second twice in quick succession.
	if (Second == LastWarningPlayedSecond && Now - LastWarningPlayedTime < 1.0) return;
	LastWarningPlayedSecond = Second;
	LastWarningPlayedTime = Now;
	if (bLocal) PlayBuffWarningSound2D(Presentation.WarningSound, Presentation.WarningLocalVolume);
	if (bWorld) PlayBuffWarningSound3D(Presentation.WarningSound, Presentation.WarningWorldVolume, Presentation.WarningAttenuation);
	if (CVarBuffWarningSoundDebug.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogTemp, Display, TEXT("[BuffWarning] Owner=%s Remaining=%.3f Second=%d Local=%d World3D=%d Revision=%u"),
			*Owner->GetPathName(), Remaining, Second, IsValid(WarningLocalAudio) ? 1 : 0,
			IsValid(WarningWorldAudio) ? 1 : 0, Presentation.Revision);
	}
}

void UBuffPresentationComponent::StopWarningAudio(TObjectPtr<UAudioComponent>& Audio)
{
	if (IsValid(Audio)) Audio->Stop();
	if (IsValid(Audio)) Audio->DestroyComponent();
	Audio = nullptr;
}

void UBuffPresentationComponent::StopWarningSounds()
{
	StopWarningAudio(WarningLocalAudio);
	StopWarningAudio(WarningWorldAudio);
}

double UBuffPresentationComponent::GetPresentationRemainingTime() const
{
	const UWorld* World = GetWorld();
	if (!World || !Presentation.Tag.IsValid()) return 0.0;
	const AGameStateBase* GameState = World->GetGameState();
	const double Now = GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	return FMath::Max(0.0, Presentation.EndServerTime - Now);
}

const FBuffVisualEffects* UBuffPresentationComponent::GetEffects() const
{
	if (IsQuad(Presentation.Tag)) return &QuadEffects;
	if (Presentation.Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Def"), false))) return &ProtectionEffects;
	return nullptr;
}

void UBuffPresentationComponent::OnRep_Presentation()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Buff_OnRepPresentation);
	if (ObservedWarningRevision != Presentation.Revision)
	{
		ObservedWarningRevision = Presentation.Revision;
		NextWarningSecond = Presentation.WarningStartSecond;
		StopWarningSounds();
	}
	UpdateWarningPlayback();
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer) return;
	const FBuffVisualEffects* Effects = GetEffects();
	if (!Effects || GetPresentationRemainingTime() <= 0.0)
	{
		StopEffects();
		return;
	}
	// Keep the aura on a time-only refresh; replace it if another DT row uses different assets for the same tag.
	if (PresentedTag == Presentation.Tag
		&& (IsValid(NiagaraComponent) ? NiagaraComponent->GetAsset() : nullptr) == Presentation.NiagaraVFX
		&& (IsValid(CascadeComponent) ? CascadeComponent->Template.Get() : nullptr) == Presentation.ParticleVFX) return;
	StopEffects();
	AActor* Owner = GetOwner();
	const ACharacter* Character = Cast<ACharacter>(Owner);
	USceneComponent* AttachTo = Character ? Character->GetMesh() : Owner ? Owner->GetRootComponent() : nullptr;
	if (!AttachTo)
	{
		UE_LOG(LogTemp, Warning, TEXT("Buff VFX has no attach component: Owner=%s Tag=%s"),
			*GetNameSafe(Owner), *Presentation.Tag.ToString());
		return;
	}
	PresentedTag = Presentation.Tag;
	if (Presentation.NiagaraVFX)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(Buff_SpawnNiagara);
		NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(Presentation.NiagaraVFX,
			AttachTo, Effects->AttachSocket, Effects->RelativeOffset, Effects->RelativeRotation,
			EAttachLocation::KeepRelativeOffset, false, true, ENCPoolMethod::None, false);
		if (NiagaraComponent) NiagaraComponent->SetRelativeScale3D(Effects->Scale);
		else UE_LOG(LogTemp, Warning, TEXT("Buff Niagara spawn failed: Owner=%s Tag=%s Asset=%s"),
			*GetNameSafe(Owner), *Presentation.Tag.ToString(), *GetNameSafe(Presentation.NiagaraVFX));
	}
	if (Presentation.ParticleVFX)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(Buff_SpawnCascade);
		CascadeComponent = UGameplayStatics::SpawnEmitterAttached(Presentation.ParticleVFX,
			AttachTo, Effects->AttachSocket, Effects->RelativeOffset, Effects->RelativeRotation,
			Effects->Scale, EAttachLocation::KeepRelativeOffset, false, EPSCPoolMethod::None, true);
		if (!CascadeComponent) UE_LOG(LogTemp, Warning, TEXT("Buff Cascade spawn failed: Owner=%s Tag=%s Asset=%s"),
			*GetNameSafe(Owner), *Presentation.Tag.ToString(), *GetNameSafe(Presentation.ParticleVFX));
	}
}

void UBuffPresentationComponent::StopEffects()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Buff_StopEffects);
	if (IsValid(NiagaraComponent)) NiagaraComponent->DestroyComponent();
	if (IsValid(CascadeComponent)) CascadeComponent->DestroyComponent();
	NiagaraComponent = nullptr;
	CascadeComponent = nullptr;
	PresentedTag = FGameplayTag();
}

void UBuffPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateWarningPlayback();
	// Clients also stop at the deadline if the server's removal update arrives later.
	if (PresentedTag.IsValid() && GetPresentationRemainingTime() <= 0.0) StopEffects();
}

void UBuffPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UActorComponent* Life = LifeComponent.Get())
	{
		if (const FMulticastDelegateProperty* Death = FindFProperty<FMulticastDelegateProperty>(Life->GetClass(), TEXT("OnDeath")))
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuffPresentationComponent, OnOwnerDeath));
			Death->RemoveDelegate(Delegate, Life, Death->ContainerPtrToValuePtr<void>(Life));
		}
	}
	StopEffects();
	StopWarningSounds();
	Super::EndPlay(EndPlayReason);
}

void UBuffPresentationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBuffPresentationComponent, Presentation);
}

void UBuffPresentationComponent::UpdateBuffRemainingTimeText(AActor* BuffOwner, UTextBlock* TimeText)
{
	if (!IsValid(TimeText)) return;
	const UBuffPresentationComponent* Buff = IsValid(BuffOwner) ? BuffOwner->FindComponentByClass<UBuffPresentationComponent>() : nullptr;
	const double Remaining = Buff ? Buff->GetPresentationRemainingTime() : 0.0;
	if (Remaining <= 0.0)
	{
		TimeText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const bool bQuad = IsQuad(Buff->Presentation.Tag);
	const FText Name = bQuad ? NSLOCTEXT("Buff", "Quad", "Quad") : NSLOCTEXT("Buff", "Protection", "Protection");
	const FText Text = FText::Format(NSLOCTEXT("Buff", "RemainingTime", "{0} {1}s"),
		Name, FText::AsNumber(FMath::CeilToInt(Remaining)));
	if (!TimeText->GetText().EqualTo(Text)) TimeText->SetText(Text);
	TimeText->SetColorAndOpacity(FSlateColor(bQuad ? FLinearColor(0.2f, 0.65f, 1.0f) : FLinearColor(0.4f, 1.0f, 0.4f)));
	TimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
}
