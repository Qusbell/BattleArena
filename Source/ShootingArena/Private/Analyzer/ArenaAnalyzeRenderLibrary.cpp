#include "Analyzer/ArenaAnalyzeRenderLibrary.h"
#include "Analyzer/ArenaAnalyzeLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
UInstancedStaticMeshComponent* Find(AActor* Actor, FName Name)
{
    TInlineComponentArray<UInstancedStaticMeshComponent*> Components(Actor);
    for (auto* C : Components) if (C->GetFName() == Name) return C;
    return nullptr;
}
struct FLayer
{
    UInstancedStaticMeshComponent* Component = nullptr;
    TArray<FTransform> Transforms;
};
FTransform Point(FVector Location, float Scale)
{ return FTransform(FQuat::Identity, Location, FVector(Scale)); }
void Line(FLayer& Layer, FVector Start, FVector End)
{
    if (Start.ContainsNaN() || End.ContainsNaN() || Start.Equals(End)) return;
    // Same cube transform as BP_ArenaAnalyzeVisualizer.Set Link ISM: midpoint,
    // X-axis look rotation, length / 100, Y/Z thickness 0.02.
    const FVector Delta = End - Start;
    Layer.Transforms.Emplace(Delta.Rotation(), (Start + End) * 0.5, FVector(Delta.Size() * 0.01, 0.02, 0.02));
}
}

void UArenaAnalyzeRenderLibrary::ClearVisualizer(AActor* Actor)
{
    if (!IsValid(Actor)) return;
    TInlineComponentArray<UInstancedStaticMeshComponent*> Components(Actor);
    for (auto* C : Components) C->ClearInstances();
}

int32 UArenaAnalyzeRenderLibrary::RenderSession(AActor* Actor, const FArenaAnalyzeSession& S, int32 Event)
{
    if (!IsValid(Actor)) return 0;
    ClearVisualizer(Actor); // Replace previous draw, including the original BP layers.
    TMap<FName, FLayer> Layers;
    auto Layer = [&](FName Name, FName TemplateName) -> FLayer&
    {
        if (auto* Existing = Layers.Find(Name)) return *Existing;
        FLayer& Result = Layers.Add(Name);
        Result.Component = Find(Actor, Name);
        if (!Result.Component)
        {
            auto* Template = Find(Actor, TemplateName);
            if (Template)
            {
                auto* C = NewObject<UInstancedStaticMeshComponent>(Actor, Name, RF_Transient);
                C->SetStaticMesh(Template->GetStaticMesh());
                for (int32 I = 0; I < Template->GetNumMaterials(); ++I) C->SetMaterial(I, Template->GetMaterial(I));
                C->SetMobility(EComponentMobility::Movable);
                C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                C->SetCanEverAffectNavigation(false); C->SetCastShadow(false);
#if WITH_EDITOR
                C->SetIsVisualizationComponent(true);
#endif
                C->SetupAttachment(Actor->GetRootComponent()); Actor->AddInstanceComponent(C); C->RegisterComponent();
                Result.Component = C;
            }
        }
        return Result;
    };
    if (Event == 0 || Event == 4)
    {
        for (const auto& M : S.MovementSamples)
            Layer(TEXT("MovementISM"), TEXT("MovementISM")).Transforms.Add(Point(M.Location, 0.15f));
        for (const auto& Segment : UArenaAnalyzeLibrary::BuildMovementLineSegments(S.MovementSamples))
            Line(Layer(TEXT("MovementLinkISM"), TEXT("MovementLinkISM")), Segment.StartLocation, Segment.EndLocation);
    }
    TMap<int32, const FArenaShotSample*> Shots;
    for (const auto& Shot : S.ShotSamples) Shots.Add(Shot.ShotId, &Shot);
    if (Event == 0 || Event == 1) for (const auto& Shot : S.ShotSamples)
    {
        Layer(TEXT("ShotISM"), TEXT("InstigatorISM")).Transforms.Add(Point(Shot.Location, 0.1f));
        Line(Layer(TEXT("ShotLinkISM"), TEXT("DamageLinkISM")), Shot.Location, Shot.Location + Shot.Direction * 150);
    }
    if (Event == 0 || Event == 2) for (const auto& H : S.HitSamples)
    {
        Layer(TEXT("HitISM"), TEXT("DamagedISM")).Transforms.Add(Point(H.Location, H.bDamageContact ? 0.07f : 0.12f));
        if (const auto* Shot = Shots.Find(H.ShotId)) Line(Layer(TEXT("HitLinkISM"), TEXT("DamageLinkISM")), (*Shot)->Location, H.Location);
    }
    if (Event == 0 || Event == 3) for (const auto& D : S.DeathSamples)
    {
        Layer(TEXT("DeathISM"), TEXT("DamagedISM")).Transforms.Add(Point(D.VictimLocation, 0.3f));
        if (D.bHasKillerLocation)
        {
            Layer(TEXT("KillerISM"), TEXT("InstigatorISM")).Transforms.Add(Point(D.KillerLocation, 0.3f));
            Line(Layer(TEXT("KillLinkISM"), TEXT("DamageLinkISM")), D.KillerLocation, D.VictimLocation);
        }
    }
    if (Event == 0 || Event == 5) for (const auto& D : S.DamageSamples)
    {
        Layer(TEXT("DamagedISM"), TEXT("DamagedISM")).Transforms.Add(Point(D.DamagedLocation, 0.2f));
        if (D.bHasInstigatorLocation || (S.SchemaVersion == 1 && D.InstigatorControllerId != INDEX_NONE))
        {
            Layer(TEXT("InstigatorISM"), TEXT("InstigatorISM")).Transforms.Add(Point(D.InstigatorLocation, 0.2f));
            Line(Layer(TEXT("DamageLinkISM"), TEXT("DamageLinkISM")), D.InstigatorLocation, D.DamagedLocation);
        }
    }
    int32 Count = 0;
    for (auto& Pair : Layers) if (auto* C = Pair.Value.Component)
    {
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCanEverAffectNavigation(false);
        C->AddInstances(Pair.Value.Transforms, false, true, false);
        Count += Pair.Value.Transforms.Num();
    }
    return Count;
}
