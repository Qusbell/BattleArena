#include "Buff/BuffPresentationComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "DataTableUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UObject/UnrealType.h"

namespace
{
	bool IsQuad(FGameplayTag Tag)
	{
		return Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Atk"), false));
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
		Presentation = FBuffPresentationState();
		StopEffects();
	}
}

void UBuffPresentationComponent::SetBuffPresentation(FGameplayTag BuffTag, double EndServerTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Buff_SetBuffPresentation);
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	Presentation.Tag = BuffTag;
	Presentation.EndServerTime = FMath::IsFinite(EndServerTime) ? EndServerTime : 0.0;
	Presentation.NiagaraVFX = nullptr;
	Presentation.ParticleVFX = nullptr;
	if (BuffTag.IsValid())
	{
		// Both Blueprint grant functions set this row before calling us. Keep their damage/drop logic intact.
		UDataTable* Table = nullptr;
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Buff_SetBuffPresentation_LoadDataTable);
			Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/QuakeLike_1_0/Data/Item/DropTable/DT_BuffTable.DT_BuffTable"));
		}
		if (Table)
		{
			const FNameProperty* RowName = nullptr;
			const uint8* Row = nullptr;
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(Buff_SetBuffPresentation_FindRow);
				RowName = FindFProperty<FNameProperty>(GetClass(), TEXT("currentBuffRowName"));
				Row = RowName ? Table->FindRowUnchecked(RowName->GetPropertyValue_InContainer(this)) : nullptr;
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
			}
			else UE_LOG(LogTemp, Warning, TEXT("Buff VFX row is missing on %s"), *GetNameSafe(this));
		}
		else UE_LOG(LogTemp, Warning, TEXT("Buff DT failed to load on %s"), *GetNameSafe(this));
	}
	OnRep_Presentation();
	GetOwner()->ForceNetUpdate();
}

void UBuffPresentationComponent::ClearBuffPresentation()
{
	SetBuffPresentation(FGameplayTag(), 0.0);
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
