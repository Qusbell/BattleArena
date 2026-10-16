#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Buff/BuffPresentationComponent.h"
#include "Components/TextBlock.h"
#include "DataTableUtils.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuffPresentationTest, "ShootingArena.Buff.Presentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)

bool FBuffPresentationTest::RunTest(const FString& Parameters)
{
	// Isolated runtime world: no map/default asset edits, even if unrelated loaded BPs block PIE.
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	// The project's area subsystem expects saved level data, intentionally absent in this transient world.
	AddExpectedError(TEXT("저장된 Area Graph가 없습니다"), EAutomationExpectedErrorFlags::Contains, 1);
	if (!Context.BeginPlayInTestWorld()) return false;
	ACharacter* Owner = World->SpawnActor<ACharacter>();
	UClass* BuffClass = LoadClass<UBuffPresentationComponent>(nullptr, TEXT("/Game/QuakeLike_1_0/Buff/BPC_Buff.BPC_Buff_C"));
	if (!TestNotNull(TEXT("BPC_Buff inherits the presentation component"), BuffClass)) return false;
	UBuffPresentationComponent* Buff = NewObject<UBuffPresentationComponent>(Owner, BuffClass);
	Owner->AddInstanceComponent(Buff);
	Buff->RegisterComponent();
	UTextBlock* Text = NewObject<UTextBlock>(Owner);
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(nullptr, Text);
	TestTrue(TEXT("Missing pawn hides the text"), Text->GetVisibility() == ESlateVisibility::Collapsed);
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(Owner, nullptr);
	TestTrue(TEXT("Buff component replicates"), Buff->GetIsReplicated());

	auto Grant = [this, Buff](const TCHAR* FunctionName, const TCHAR* RowParameter, FName Row, double End = 0.0)
	{
		UFunction* Function = Buff->FindFunction(FName(FunctionName));
		if (!TestNotNull(TEXT("Existing grant function"), Function)) return;
		FStructOnScope Args(Function);
		FindFProperty<FNameProperty>(Function, FName(RowParameter))->SetPropertyValue_InContainer(Args.GetStructMemory(), Row);
		if (FDoubleProperty* Time = FindFProperty<FDoubleProperty>(Function, TEXT("EndTime")))
			Time->SetPropertyValue_InContainer(Args.GetStructMemory(), End);
		Buff->ProcessEvent(Function, Args.GetStructMemory());
		TestTrue(TEXT("Existing buff grant succeeds"), FindFProperty<FBoolProperty>(Function, TEXT("Success"))->GetPropertyValue_InContainer(Args.GetStructMemory()));
	};
	Grant(TEXT("ApplyBuff"), TEXT("rowName"), TEXT("Buff.Multi.Atk"));
	TestTrue(TEXT("Normal grant publishes its actual 30-second deadline"), FMath::IsNearlyEqual(Buff->GetPresentationRemainingTime(), 30.0, 0.01));
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(Owner, Text);
	TestEqual(TEXT("Quad label and numeric seconds"), Text->GetText().ToString(), FString(TEXT("Quad 30s")));
	Context.TickTestWorld(2.0f);
	TestTrue(TEXT("Remaining time decreases with server game time"), Buff->GetPresentationRemainingTime() < 30.0);
	Grant(TEXT("ApplyBuff"), TEXT("rowName"), TEXT("Buff.Multi.Atk"));
	TestTrue(TEXT("Refresh uses the newly assigned full deadline"), FMath::IsNearlyEqual(Buff->GetPresentationRemainingTime(), 30.0, 0.01));
	const double DroppedEnd = World->GetGameState()->GetServerWorldTimeSeconds() + 12.0;
	Grant(TEXT("ApplyDroppedBuff"), TEXT("Row Name"), TEXT("Buff.Multi.Def"), DroppedEnd);
	TestTrue(TEXT("Dropped grant displays the inherited remaining time"), FMath::IsNearlyEqual(Buff->GetPresentationRemainingTime(), 12.0, 0.01));
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(Owner, Text);
	TestEqual(TEXT("Protection label"), Text->GetText().ToString(), FString(TEXT("Protection 12s")));

	UNiagaraSystem* Niagara = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/QuakeLike_1_0/HeldItem/NiagaraSystem/MuzzleFlashMG/NS_MuzzleFlash_MG_Stylized.NS_MuzzleFlash_MG_Stylized"));
	UParticleSystem* Cascade = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/ParagonSteel/FX/Particles/Steel/Abilities/AbilityArmor/FX/P_Steel_AbilityArmor_Looping.P_Steel_AbilityArmor_Looping"));
	if (!TestNotNull(TEXT("Diagnostic Niagara asset"), Niagara) || !TestNotNull(TEXT("Diagnostic Cascade asset"), Cascade)) return false;
	const FGameplayTag Quad = FGameplayTag::RequestGameplayTag(TEXT("Buff.Atk"));
	UDataTable* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/QuakeLike_1_0/Data/Item/DropTable/DT_BuffTable.DT_BuffTable"));
	if (!TestNotNull(TEXT("Existing Buff DT"), Table)) return false;
	FObjectPropertyBase* NiagaraField = nullptr;
	FObjectPropertyBase* ParticleField = nullptr;
	for (TFieldIterator<FObjectPropertyBase> It(Table->GetRowStruct()); It; ++It)
	{
		const FString Name = DataTableUtils::GetPropertyExportName(*It);
		if (Name == TEXT("NiagaraVFX")) NiagaraField = *It;
		if (Name == TEXT("ParticleVFX")) ParticleField = *It;
	}
	if (!TestNotNull(TEXT("DT NiagaraVFX field"), NiagaraField) || !TestNotNull(TEXT("DT ParticleVFX field"), ParticleField)) return false;
	TMap<FName, TPair<UObject*, UObject*>> OriginalEffects;
	for (const FName RowName : {FName(TEXT("Buff.Multi.Atk")), FName(TEXT("Buff.Multi.Def"))})
	{
		uint8* Row = Table->FindRowUnchecked(RowName);
		if (!TestNotNull(TEXT("DT test row"), Row)) return false;
		OriginalEffects.Add(RowName, {NiagaraField->GetObjectPropertyValue_InContainer(Row), ParticleField->GetObjectPropertyValue_InContainer(Row)});
	}
	// Only change the test process's loaded rows; restore on every exit and never save the DT or map.
	ON_SCOPE_EXIT
	{
		for (const auto& Original : OriginalEffects)
		{
			uint8* Row = Table->FindRowUnchecked(Original.Key);
			NiagaraField->SetObjectPropertyValue_InContainer(Row, Original.Value.Key);
			ParticleField->SetObjectPropertyValue_InContainer(Row, Original.Value.Value);
		}
	};
	for (const auto& Original : OriginalEffects)
	{
		uint8* Row = Table->FindRowUnchecked(Original.Key);
		for (int32 Mask = 0; Mask < 4; ++Mask)
		{
			NiagaraField->SetObjectPropertyValue_InContainer(Row, (Mask & 1) ? Niagara : nullptr);
			ParticleField->SetObjectPropertyValue_InContainer(Row, (Mask & 2) ? Cascade : nullptr);
			// No manual clear: re-granting the same tag with different DT VFX must also replace its old aura.
			Grant(TEXT("ApplyBuff"), TEXT("rowName"), Original.Key);
			TArray<UNiagaraComponent*> NiagaraComponents;
			TArray<UParticleSystemComponent*> CascadeComponents;
			Owner->GetComponents(NiagaraComponents);
			Owner->GetComponents(CascadeComponents);
			TestEqual(TEXT("Normal pickup reads Niagara from its DT row"), NiagaraComponents.Num(), (Mask & 1) ? 1 : 0);
			TestEqual(TEXT("Normal pickup reads Particle from its DT row"), CascadeComponents.Num(), (Mask & 2) ? 1 : 0);
			if (!NiagaraComponents.IsEmpty()) TestTrue(TEXT("DT Niagara asset matches"), NiagaraComponents[0]->GetAsset() == Niagara);
			if (!CascadeComponents.IsEmpty()) TestTrue(TEXT("DT Particle asset matches"), CascadeComponents[0]->Template == Cascade);
			const double End = World->GetGameState()->GetServerWorldTimeSeconds() + 7.0;
			Grant(TEXT("ApplyDroppedBuff"), TEXT("Row Name"), Original.Key, End);
			TArray<UNiagaraComponent*> RefreshedNiagara;
			TArray<UParticleSystemComponent*> RefreshedCascade;
			Owner->GetComponents(RefreshedNiagara);
			Owner->GetComponents(RefreshedCascade);
			TestTrue(TEXT("Dropped pickup uses DT VFX without duplicating the same aura"), RefreshedNiagara == NiagaraComponents && RefreshedCascade == CascadeComponents);
			TestTrue(TEXT("DT VFX does not reset the dropped deadline"), FMath::IsNearlyEqual(Buff->GetPresentationRemainingTime(), 7.0, 0.01));
		}
	}
	Grant(TEXT("ApplyBuff"), TEXT("rowName"), TEXT("Buff.Multi.Atk"));
	Buff->SetBuffPresentation(Quad, World->GetGameState()->GetServerWorldTimeSeconds() + 0.5);
	// Test component lifetime, not the diagnostic art's burst simulation under manually advanced frames.
	TArray<UNiagaraComponent*> ActiveNiagara;
	TArray<UParticleSystemComponent*> ActiveCascade;
	Owner->GetComponents(ActiveNiagara);
	Owner->GetComponents(ActiveCascade);
	for (UNiagaraComponent* Effect : ActiveNiagara) Effect->DeactivateImmediate();
	for (UParticleSystemComponent* Effect : ActiveCascade) Effect->DeactivateSystem();
	for (int32 Tick = 0; Tick < 10; ++Tick) Context.TickTestWorld(0.1f);
	Buff->TickComponent(0.1f, LEVELTICK_All, nullptr);
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(Owner, Text);
	TestEqual(TEXT("Expired presentation clamps to zero"), Buff->GetPresentationRemainingTime(), 0.0);
	TestTrue(TEXT("Expiry hides the number"), Text->GetVisibility() == ESlateVisibility::Collapsed);
	TArray<UNiagaraComponent*> NiagaraAfterExpiry;
	TArray<UParticleSystemComponent*> CascadeAfterExpiry;
	Owner->GetComponents(NiagaraAfterExpiry);
	Owner->GetComponents(CascadeAfterExpiry);
	TestTrue(TEXT("Expiry removes both effects"), NiagaraAfterExpiry.IsEmpty() && CascadeAfterExpiry.IsEmpty());
	Grant(TEXT("ApplyBuff"), TEXT("rowName"), TEXT("Buff.Multi.Atk"));
	UFunction* Death = Buff->FindFunction(TEXT("OnOwnerDeath"));
	FStructOnScope DeathArgs(Death);
	Buff->ProcessEvent(Death, DeathArgs.GetStructMemory());
	TestEqual(TEXT("Death clears presentation without changing the gameplay/drop deadline"), Buff->GetPresentationRemainingTime(), 0.0);
	Context.DestroyTestWorld(false);
	return !HasAnyErrors();
}

#endif
