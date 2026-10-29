#include "UI/UIPalette.h"
#include "UI/ReusableStyleHost.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Async/Async.h"
#include "Components/NamedSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "UObject/ObjectSaveContext.h"
#endif
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "UObject/UObjectIterator.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectThreadContext.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIThemePalette, Log, All);

namespace
{
constexpr const TCHAR* PaletteClassPath = TEXT("/Game/QuakeLike_1_0/GameFlow/UI/02_Styles/03_Palette/PDA_UIPalette.PDA_UIPalette_C");
TWeakObjectPtr<UUIThemePalette> CachedPalette;
TWeakObjectPtr<UUIThemePalette> PreparedPalette;
bool bLoadingPalette = false;
bool bApplyingPalette = false;
bool bApplyingObject = false;
FTSTicker::FDelegateHandle RefreshHandle;
// Keep inverse identity across Blueprint CDO reinstancing, without retaining
// old material instances. Live brushes retain their own referenced MID.
TMap<TWeakObjectPtr<UMaterialInterface>, TWeakObjectPtr<UMaterialInterface>> OriginalMaterials;
#if WITH_EDITOR
TSet<TWeakObjectPtr<UPackage>> PackagesPreparedForSave;
#endif

bool CanApplyNow()
{
    return !IsRunningDedicatedServer() && !IsRunningCommandlet() && IsInGameThread() &&
        !IsGarbageCollecting() && !GIsSavingPackage && !FUObjectThreadContext::Get().IsRoutingPostLoad;
}

/** Resolve only inline reflected structs/arrays; never follow UObject references. */
bool ResolvePath(UStruct* Type, void* Container, const FString& Path, FProperty*& OutProperty, void*& OutValue)
{
    TArray<FString> Segments;
    Path.ParseIntoArray(Segments, TEXT("."), true);
    for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
    {
        FString Name = Segments[SegmentIndex];
        int32 Index = 0;
        int32 Bracket = INDEX_NONE;
        const bool bIndexed = Name.FindChar(TEXT('['), Bracket);
        if (bIndexed)
        {
            if (!Name.EndsWith(TEXT("]"))) return false;
            const FString IndexText = Name.Mid(Bracket + 1, Name.Len() - Bracket - 2);
            if (!IndexText.IsNumeric()) return false;
            Index = FCString::Atoi(*IndexText);
            Name.LeftInline(Bracket);
        }
        FProperty* Property = FindFProperty<FProperty>(Type, *Name);
        if (!Property) return false;
        void* Value = nullptr;
        if (FArrayProperty* Array = CastField<FArrayProperty>(Property))
        {
            if (!bIndexed) return false;
            FScriptArrayHelper Helper(Array, Array->ContainerPtrToValuePtr<void>(Container));
            if (!Helper.IsValidIndex(Index)) return false;
            Value = Helper.GetRawPtr(Index);
            Property = Array->Inner;
        }
        else
        {
            if (Index < 0 || Index >= Property->ArrayDim) return false;
            Value = Property->ContainerPtrToValuePtr<void>(Container, Index);
        }
        if (SegmentIndex == Segments.Num() - 1)
        {
            OutProperty = Property;
            OutValue = Value;
            return true;
        }
        FStructProperty* Struct = CastField<FStructProperty>(Property);
        if (!Struct) return false;
        Type = Struct->Struct;
        Container = Value;
    }
    return false;
}

bool WriteColor(UObject* Object, const FString& Path, const FLinearColor& Color)
{
    FProperty* Property = nullptr;
    void* Value = nullptr;
    if (!ResolvePath(Object->GetClass(), Object, Path, Property, Value)) return false;
    FStructProperty* Struct = CastField<FStructProperty>(Property);
    if (!Struct) return false;
    if (Struct->Struct == TBaseStructure<FLinearColor>::Get())
    {
        *static_cast<FLinearColor*>(Value) = Color;
        return true;
    }
    if (Struct->Struct == FSlateColor::StaticStruct())
    {
        FSlateColor& SlateColor = *static_cast<FSlateColor*>(Value);
        if (SlateColor.IsColorSpecified()) SlateColor = FSlateColor(Color);
        return true;
    }
    return false;
}

template<typename CallbackType>
void VisitInlineProperties(UStruct* Type, void* Container, const FString& Prefix, CallbackType& Callback, int32 Depth = 0)
{
    if (!Type || !Container || Depth > 32) return;
    for (TFieldIterator<FProperty> It(Type); It; ++It)
    {
        FProperty* Property = *It;
        if (Property->HasAnyPropertyFlags(CPF_Deprecated)) continue;
        const FString BasePath = Prefix.IsEmpty() ? Property->GetName() : Prefix + TEXT(".") + Property->GetName();
        if (FArrayProperty* Array = CastField<FArrayProperty>(Property))
        {
            FScriptArrayHelper Helper(Array, Array->ContainerPtrToValuePtr<void>(Container));
            FStructProperty* Inner = CastField<FStructProperty>(Array->Inner);
            if (!Inner) continue;
            for (int32 Index = 0; Index < Helper.Num(); ++Index)
            {
                const FString Path = FString::Printf(TEXT("%s[%d]"), *BasePath, Index);
                void* Value = Helper.GetRawPtr(Index);
                if (!Callback(Inner->Struct, Value, Path))
                    VisitInlineProperties(Inner->Struct, Value, Path, Callback, Depth + 1);
            }
            continue;
        }
        FStructProperty* Struct = CastField<FStructProperty>(Property);
        if (!Struct) continue;
        for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
        {
            const FString Path = Property->ArrayDim > 1 ? FString::Printf(TEXT("%s[%d]"), *BasePath, Index) : BasePath;
            void* Value = Property->ContainerPtrToValuePtr<void>(Container, Index);
            if (!Callback(Struct->Struct, Value, Path))
                VisitInlineProperties(Struct->Struct, Value, Path, Callback, Depth + 1);
        }
    }
}

void ReplaceMaterials(UObject* Object, const UUIThemePalette* Palette)
{
    auto Visit = [Palette](UScriptStruct* Type, void* Value, const FString&)
    {
        if (Type != FSlateBrush::StaticStruct()) return false;
        FSlateBrush& Brush = *static_cast<FSlateBrush*>(Value);
        if (UMaterialInterface* Material = Cast<UMaterialInterface>(Brush.GetResourceObject()))
            Brush.SetResourceObject(Palette->ResolveMaterial(Material));
        return true;
    };
    VisitInlineProperties(Object->GetClass(), Object, FString(), Visit);
}

void PatchWidgetTemplates(UWidgetBlueprintGeneratedClass* Class, FName WidgetName, UUIThemePalette* Palette);

UClass* WidgetOwnerClass(const UWidget* Widget)
{
    if (const UUserWidget* Screen = Widget->GetTypedOuter<UUserWidget>()) return Screen->GetClass();
    if (UClass* Class = Widget->GetTypedOuter<UClass>()) return Class;
#if WITH_EDITOR
    if (const UBlueprint* Blueprint = Widget->GetTypedOuter<UBlueprint>()) return Blueprint->GeneratedClass;
#endif
    return nullptr;
}

bool Matches(const FUIPaletteObjectBinding& Binding, UObject* Object)
{
    if (!Binding.TargetAsset.IsNull() && Binding.TargetAsset.Get() == Object) return true;
    UClass* Target = Binding.TargetClass.Get();
    if (!Target) return false;
    if (Binding.WidgetName.IsNone()) return Object->GetClass() == Target;
    const UWidget* Widget = Cast<UWidget>(Object);
    UClass* Owner = Widget ? WidgetOwnerClass(Widget) : nullptr;
    return Widget && Widget->GetFName() == Binding.WidgetName && Owner == Target;
}

bool PatchObject(UObject* Object, UUIThemePalette* Palette)
{
    if (!IsValid(Object) || Object == Palette || Object->HasAnyFlags(RF_NeedPostLoad | RF_NeedPostLoadSubobjects)) return false;
    bool bBound = false;
    for (const FUIPaletteObjectBinding& Binding : Palette->ObjectBindings)
    {
        if (!Matches(Binding, Object)) continue;
        bBound = true;
        for (const TPair<FString, FName>& Field : Binding.Fields)
        {
            FLinearColor Color;
            if (Palette->TryGetColor(Field.Value, Color)) WriteColor(Object, Field.Key, Color);
        }
    }
    if (bBound) ReplaceMaterials(Object, Palette);
    return bBound;
}

void PatchWidgetTemplates(UWidgetBlueprintGeneratedClass* Class, FName WidgetName, UUIThemePalette* Palette)
{
    auto PatchTree = [WidgetName, Palette](UWidgetTree* Tree)
    {
        if (!Tree) return;
        UWidget* Template = FindObjectFast<UWidget>(Tree, WidgetName);
        if (!Template) Template = Tree->FindWidget(WidgetName);
        if (Template) PatchObject(Template, Palette);
    };
    PatchTree(Class->GetWidgetTreeArchetype());
#if WITH_EDITOR
    // Designer duplication may use the source Blueprint tree instead of the
    // generated archetype. These are in-memory updates, never authoring edits.
    if (Class->ClassGeneratedBy)
        PatchTree(FindObjectFast<UWidgetTree>(Class->ClassGeneratedBy, TEXT("WidgetTree")));
#endif
    Class->GetNamedSlotArchetypeContent([WidgetName, Palette](FName, UWidget* Content)
    {
        if (!Content) return;
        if (Content->GetFName() == WidgetName) PatchObject(Content, Palette);
        if (UWidgetTree* Tree = Content->GetTypedOuter<UWidgetTree>())
            if (UWidget* Template = FindObjectFast<UWidget>(Tree, WidgetName)) PatchObject(Template, Palette);
    });
}

void ApplyLiveObject(UObject* Object, UUIThemePalette* Palette)
{
    if (!IsValid(Object) || Object->HasAnyFlags(RF_NeedPostLoad | RF_NeedPostLoadSubobjects)) return;
    UWidget* Widget = Cast<UWidget>(Object);
    const bool bHasSlateWidget = Widget && Widget->GetCachedWidget().IsValid();
    UCommonTextBlock* CommonText = Cast<UCommonTextBlock>(Object);
    FSlateColor InheritedForeground;
    bool bPreserveForeground = false;
    if (CommonText)
    {
        InheritedForeground = CommonText->GetColorAndOpacity();
        bPreserveForeground = !InheritedForeground.IsColorSpecified();
        if (const FClassProperty* StyleProperty = FindFProperty<FClassProperty>(CommonText->GetClass(), TEXT("Style")))
            if (UClass* TextStyle = Cast<UClass>(StyleProperty->GetObjectPropertyValue_InContainer(CommonText)))
            {
                PatchObject(TextStyle->GetDefaultObject(), Palette);
                if (bHasSlateWidget) CommonText->SetStyle(TextStyle);
            }
        if (bPreserveForeground) CommonText->SetColorAndOpacity(InheritedForeground);
    }
    if (UCommonButtonBase* CommonButton = Cast<UCommonButtonBase>(Object))
        if (UCommonButtonStyle* Style = CommonButton->GetStyle()) PatchObject(Style, Palette);

    if (bHasSlateWidget)
        if (UReusableStyleHost* Host = Cast<UReusableStyleHost>(Object)) Host->ApplyStyle();

    const bool bBound = PatchObject(Object, Palette);
    if (!bHasSlateWidget) return;
    if (CommonText)
    {
        // CommonTextBlock::SynchronizeProperties reapplies its style color. Push
        // the final authored overrides directly, retaining button foregrounds.
        if (bPreserveForeground) CommonText->SetColorAndOpacity(InheritedForeground);
        else CommonText->SetColorAndOpacity(CommonText->GetColorAndOpacity());
        CommonText->SetShadowColorAndOpacity(CommonText->GetShadowColorAndOpacity());
        CommonText->SetFont(CommonText->GetFont());
    }
    else if (bBound || Object->IsA<UCommonButtonBase>())
    {
        // Dispatch through the public base API, including protected CommonUI
        // overrides that rebuild cached Slate button styles for the same class.
        Widget->SynchronizeProperties();
    }
}

void VisitWidgets(UWidget* Widget, UUIThemePalette* Palette, TSet<UWidget*>& Seen)
{
    if (!Widget || Seen.Contains(Widget)) return;
    Seen.Add(Widget);
    ApplyLiveObject(Widget, Palette);
    if (UUserWidget* Screen = Cast<UUserWidget>(Widget))
    {
        TArray<FName> Slots;
        Screen->GetSlotNames(Slots);
        for (FName Slot : Slots) VisitWidgets(Screen->GetContentForSlot(Slot), Palette, Seen);
        if (Screen->WidgetTree) VisitWidgets(Screen->WidgetTree->RootWidget, Palette, Seen);
    }
    if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        for (UWidget* Child : Panel->GetAllChildren()) VisitWidgets(Child, Palette, Seen);
}
}

bool UUIThemePalette::TryGetColor(FName Role, FLinearColor& OutColor) const
{
    if (const FLinearColor* Additional = AdditionalColors.Find(Role))
    {
        OutColor = *Additional;
        return true;
    }
    const FStructProperty* Property = FindFProperty<FStructProperty>(GetClass(), Role);
    if (Property && Property->Struct == TBaseStructure<FLinearColor>::Get())
    {
        OutColor = *Property->ContainerPtrToValuePtr<FLinearColor>(this);
        return true;
    }
    return false;
}

void UUIThemePalette::RefreshMaterials()
{
    for (auto It = OriginalMaterials.CreateIterator(); It; ++It)
        if (!It.Key().IsValid() || !It.Value().IsValid()) It.RemoveCurrent();

    TSet<UMaterialInterface*> BoundParents;
    for (const FUIPaletteMaterialBinding& Binding : MaterialBindings)
        if (UMaterialInterface* Parent = Binding.Parent.LoadSynchronous()) BoundParents.Add(Parent);
    // Removed bindings must release their strong cache references. The weak
    // inverse map still resolves any older MID retained by a live brush.
    for (auto It = ResolvedMaterials.CreateIterator(); It; ++It)
        if (!BoundParents.Contains(It.Key().Get())) It.RemoveCurrent();

    for (const FUIPaletteMaterialBinding& Binding : MaterialBindings)
    {
        UMaterialInterface* Parent = Binding.Parent.LoadSynchronous();
        if (!Parent) continue;
        TObjectPtr<UMaterialInstanceDynamic>& Instance = ResolvedMaterials.FindOrAdd(Parent);
        if (!Instance) Instance = UMaterialInstanceDynamic::Create(Parent, GetTransientPackage());
        OriginalMaterials.Add(Instance.Get(), Parent);
        for (const TPair<FName, FName>& Parameter : Binding.Parameters)
        {
            FLinearColor Color;
            if (TryGetColor(Parameter.Value, Color)) Instance->SetVectorParameterValue(Parameter.Key, Color);
        }
    }
}

UMaterialInterface* UUIThemePalette::ResolveMaterial(UMaterialInterface* Material) const
{
    if (const TObjectPtr<UMaterialInstanceDynamic>* Instance = ResolvedMaterials.Find(ResolveOriginalMaterial(Material))) return Instance->Get();
    return Material;
}

UMaterialInterface* UUIThemePalette::ResolveOriginalMaterial(UMaterialInterface* Material) const
{
    if (const TWeakObjectPtr<UMaterialInterface>* Original = OriginalMaterials.Find(Material))
        if (Original->IsValid()) return Original->Get();
    for (const TPair<TObjectPtr<UMaterialInterface>, TObjectPtr<UMaterialInstanceDynamic>>& Pair : ResolvedMaterials)
        if (Pair.Value.Get() == Material) return Pair.Key.Get();
    return Material;
}

#if WITH_EDITOR
void UUIThemePalette::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);
    UUIThemePaletteLibrary::RequestRefresh();
}

void UUIThemePalette::PostEditChangeChainProperty(FPropertyChangedChainEvent& Event)
{
    Super::PostEditChangeChainProperty(Event);
    UUIThemePaletteLibrary::RequestRefresh();
}
#endif

UUIThemePalette* UUIThemePaletteLibrary::GetPalette()
{
    if (CachedPalette.IsValid() && !CachedPalette->GetClass()->HasAnyClassFlags(CLASS_NewerVersionExists)) return CachedPalette.Get();
    if (!CanApplyNow() || bLoadingPalette) return nullptr;
    TGuardValue<bool> Loading(bLoadingPalette, true);
    if (UClass* PaletteClass = LoadClass<UUIThemePalette>(nullptr, PaletteClassPath))
        CachedPalette = Cast<UUIThemePalette>(PaletteClass->GetDefaultObject());
    if (GEngine)
        if (UUIThemePaletteEngineSubsystem* Subsystem = GEngine->GetEngineSubsystem<UUIThemePaletteEngineSubsystem>())
            Subsystem->ActivePalette = CachedPalette.Get();
    return CachedPalette.Get();
}

FLinearColor UUIThemePaletteLibrary::GetColor(FName Role)
{
    FLinearColor Result(ForceInit);
    UUIThemePalette* Palette = GetPalette();
    if (Palette && Palette->TryGetColor(Role, Result)) return Result;
    UE_LOG(LogUIThemePalette, Warning, TEXT("UI palette role '%s' was not found."), *Role.ToString());
    return Result;
}

void UUIThemePaletteLibrary::PreparePalette()
{
    if (bApplyingPalette || bLoadingPalette || !CanApplyNow()) return;
    UUIThemePalette* Palette = GetPalette();
    if (Palette && PreparedPalette.Get() != Palette) ApplyPalette();
}

void UUIThemePaletteLibrary::ApplyObject(UObject* Object)
{
    if (!Object || bApplyingObject || !CanApplyNow()) return;
    PreparePalette();
    if (UUIThemePalette* Palette = GetPalette())
    {
        TGuardValue<bool> Applying(bApplyingObject, true);
        ApplyLiveObject(Object, Palette);
    }
}

void UUIThemePaletteLibrary::ApplyPaletteToObject(UObject* Object)
{
    ApplyObject(Object);
}

void UUIThemePaletteLibrary::ApplyPalette()
{
    if (bApplyingPalette || !CanApplyNow()) return;
    UUIThemePalette* Palette = GetPalette();
    if (!Palette) return;
    TGuardValue<bool> Applying(bApplyingPalette, true);
    Palette->RefreshMaterials();
    for (const FUIPaletteObjectBinding& Binding : Palette->ObjectBindings)
    {
        if (UObject* Asset = Binding.TargetAsset.LoadSynchronous()) PatchObject(Asset, Palette);
        UClass* Class = Binding.TargetClass.LoadSynchronous();
        if (!Class) continue;
        if (Binding.WidgetName.IsNone()) PatchObject(Class->GetDefaultObject(), Palette);
        else if (UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(Class))
            PatchWidgetTemplates(WidgetClass, Binding.WidgetName, Palette);
    }
    PreparedPalette = Palette;
    TGuardValue<bool> ApplyingObjects(bApplyingObject, true);
    for (TObjectIterator<UWidget> It; It; ++It)
    {
        UWidget* Widget = *It;
        if (Widget->HasAnyFlags(RF_ArchetypeObject | RF_NeedPostLoad | RF_NeedPostLoadSubobjects)) continue;
        ApplyLiveObject(Widget, Palette);
    }
}

void UUIThemePaletteLibrary::ApplyWidgetTree(UUserWidget* Screen)
{
    if (!Screen || bApplyingObject || !CanApplyNow()) return;
    PreparePalette();
    if (UUIThemePalette* Palette = GetPalette())
    {
        TGuardValue<bool> Applying(bApplyingObject, true);
        TSet<UWidget*> Seen;
        VisitWidgets(Screen, Palette, Seen);
    }
}

FString UUIThemePaletteLibrary::DescribeColorProperties(UObject* Object)
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Fields;
    if (IsValid(Object))
    {
        auto Visit = [&Fields](UScriptStruct* Type, void* Value, const FString& Path)
        {
            FLinearColor Color;
            FString ColorType;
            if (Type == TBaseStructure<FLinearColor>::Get())
            {
                Color = *static_cast<FLinearColor*>(Value);
                ColorType = TEXT("LinearColor");
            }
            else if (Type == FSlateColor::StaticStruct())
            {
                const FSlateColor& SlateColor = *static_cast<FSlateColor*>(Value);
                if (!SlateColor.IsColorSpecified()) return true;
                Color = SlateColor.GetSpecifiedColor();
                ColorType = TEXT("SlateColor");
            }
            else return false;
            TSharedRef<FJsonObject> Field = MakeShared<FJsonObject>();
            Field->SetStringField(TEXT("path"), Path);
            Field->SetStringField(TEXT("type"), ColorType);
            TArray<TSharedPtr<FJsonValue>> Components;
            for (double Component : {double(Color.R), double(Color.G), double(Color.B), double(Color.A)})
                Components.Add(MakeShared<FJsonValueNumber>(Component));
            Field->SetArrayField(TEXT("color"), Components);
            Fields.Add(MakeShared<FJsonValueObject>(Field));
            return true;
        };
        VisitInlineProperties(Object->GetClass(), Object, FString(), Visit);
    }
    Root->SetArrayField(TEXT("fields"), Fields);
    FString Result;
    FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Result));
    return Result;
}

void UUIThemePaletteLibrary::RequestRefresh()
{
    if (IsRunningDedicatedServer() || IsRunningCommandlet()) return;
    if (!IsInGameThread())
    {
        AsyncTask(ENamedThreads::GameThread, [] { UUIThemePaletteLibrary::RequestRefresh(); });
        return;
    }
    if (CachedPalette.IsValid() && CachedPalette->GetClass()->HasAnyClassFlags(CLASS_NewerVersionExists))
        CachedPalette.Reset();
    PreparedPalette.Reset();
    if (bApplyingPalette || bLoadingPalette || RefreshHandle.IsValid()) return;
    RefreshHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
    {
        if (!CanApplyNow()) return true;
        RefreshHandle.Reset();
#if WITH_EDITOR
        // A failed save has no PackageSaved callback. At the next safe frame
        // serialization has ended, so both successful and failed saves recover.
        PackagesPreparedForSave.Reset();
#endif
        UUIThemePaletteLibrary::ApplyPalette();
        return false;
    }), 0.05f);
}

void UUIThemePaletteSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (IsRunningDedicatedServer() || IsRunningCommandlet()) return;
    RefreshPalette();
}

void UUIThemePaletteSubsystem::RefreshPalette()
{
    UUIThemePaletteLibrary::ApplyPalette();
}

void UUIThemePaletteEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (IsRunningDedicatedServer() || IsRunningCommandlet()) return;
#if WITH_EDITOR
    AssetLoadedHandle = FCoreUObjectDelegates::OnAssetLoaded.AddUObject(this, &UUIThemePaletteEngineSubsystem::OnAssetLoaded);
    RegisterBlueprintCompileHooks();
    ObjectPreSaveHandle = FCoreUObjectDelegates::OnObjectPreSave.AddUObject(this, &UUIThemePaletteEngineSubsystem::OnObjectPreSave);
    PackagePreSaveHandle = UPackage::PreSavePackageWithContextEvent.AddUObject(this, &UUIThemePaletteEngineSubsystem::OnPackagePreSave);
    PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddUObject(this, &UUIThemePaletteEngineSubsystem::OnPackageSaved);
#endif
    // Runtime initialization is independent of the editor-only asset callback.
    // ApplyPalette loads bound assets synchronously before patching their defaults.
    UUIThemePaletteLibrary::RequestRefresh();
}

void UUIThemePaletteEngineSubsystem::Deinitialize()
{
#if WITH_EDITOR
    FCoreUObjectDelegates::OnAssetLoaded.Remove(AssetLoadedHandle);
    AssetLoadedHandle.Reset();
    if (GEditor)
    {
        GEditor->OnBlueprintPreCompile().Remove(BlueprintPreCompileHandle);
        GEditor->OnBlueprintCompiled().Remove(BlueprintCompiledHandle);
        GEditor->OnBlueprintReinstanced().Remove(BlueprintReinstancedHandle);
    }
    BlueprintPreCompileHandle.Reset();
    BlueprintCompiledHandle.Reset();
    BlueprintReinstancedHandle.Reset();
    FCoreUObjectDelegates::OnObjectPreSave.Remove(ObjectPreSaveHandle);
    UPackage::PreSavePackageWithContextEvent.Remove(PackagePreSaveHandle);
    UPackage::PackageSavedWithContextEvent.Remove(PackageSavedHandle);
    PackagesPreparedForSave.Reset();
#endif
    if (RefreshHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(RefreshHandle);
    RefreshHandle.Reset();
    PreparedPalette.Reset();
    CachedPalette.Reset();
    ActivePalette = nullptr;
    Super::Deinitialize();
}

#if WITH_EDITOR
void UUIThemePaletteEngineSubsystem::OnAssetLoaded(UObject* Object)
{
    // Engine subsystems can initialize before the editor instance is available.
    RegisterBlueprintCompileHooks();
    if (bApplyingPalette || bLoadingPalette || !Object) return;
    if (Object->GetPathName().StartsWith(TEXT("/Game/QuakeLike_1_0/GameFlow/UI/")))
        UUIThemePaletteLibrary::RequestRefresh();
}

void UUIThemePaletteEngineSubsystem::RegisterBlueprintCompileHooks()
{
    if (!GEditor || BlueprintPreCompileHandle.IsValid()) return;
    BlueprintPreCompileHandle = GEditor->OnBlueprintPreCompile().AddUObject(this, &UUIThemePaletteEngineSubsystem::OnBlueprintPreCompile);
    BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddUObject(this, &UUIThemePaletteEngineSubsystem::OnBlueprintCompiled);
    BlueprintReinstancedHandle = GEditor->OnBlueprintReinstanced().AddUObject(this, &UUIThemePaletteEngineSubsystem::OnBlueprintCompiled);
}

void UUIThemePaletteEngineSubsystem::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
    // CDO reinstancing can discard transient MID references before presave runs.
    // Restore authored resources without entering the package-save guard.
    if (Blueprint) RestorePackageMaterials(Blueprint->GetOutermost(), false);
}

void UUIThemePaletteEngineSubsystem::OnBlueprintCompiled()
{
    // Reapply only after the compiler has replaced classes and instances.
    UUIThemePaletteLibrary::RequestRefresh();
}

void UUIThemePaletteEngineSubsystem::RestorePackageMaterials(UPackage* Package, bool bPreparingSave)
{
    // Saving callbacks must never load or find the palette: package saving is
    // already collecting object references, and our active palette is retained.
    UUIThemePalette* Palette = ActivePalette.Get();
    if (!Palette || !Package || (bPreparingSave && PackagesPreparedForSave.Contains(Package))) return;
    const FString PackageName = Package->GetName();
    bool bIsBoundPackage = false;
    for (const FUIPaletteObjectBinding& Binding : Palette->ObjectBindings)
    {
        if (Binding.TargetClass.ToSoftObjectPath().GetLongPackageName() == PackageName ||
            Binding.TargetAsset.ToSoftObjectPath().GetLongPackageName() == PackageName)
        {
            bIsBoundPackage = true;
            break;
        }
    }
    if (!bIsBoundPackage) return;
    if (bPreparingSave) PackagesPreparedForSave.Add(Package);
    TArray<UObject*> Objects;
    GetObjectsWithPackage(Package, Objects, true);
    auto Restore = [Palette](UScriptStruct* Type, void* Value, const FString&)
    {
        if (Type != FSlateBrush::StaticStruct()) return false;
        FSlateBrush& Brush = *static_cast<FSlateBrush*>(Value);
        if (UMaterialInterface* Material = Cast<UMaterialInterface>(Brush.GetResourceObject()))
            Brush.SetResourceObject(Palette->ResolveOriginalMaterial(Material));
        return true;
    };
    // Include generated class CDOs, source widget trees, nested templates, and
    // inline style structs. UObject references themselves are never traversed.
    for (UObject* Object : Objects)
        if (IsValid(Object)) VisitInlineProperties(Object->GetClass(), Object, FString(), Restore);
    if (bPreparingSave) UUIThemePaletteLibrary::RequestRefresh();
}

void UUIThemePaletteEngineSubsystem::OnObjectPreSave(UObject* Object, FObjectPreSaveContext)
{
    if (Object) RestorePackageMaterials(Object->GetOutermost());
}

void UUIThemePaletteEngineSubsystem::OnPackagePreSave(UPackage* Package, FObjectPreSaveContext)
{
    RestorePackageMaterials(Package);
}

void UUIThemePaletteEngineSubsystem::OnPackageSaved(const FString&, UPackage* Package, FObjectPostSaveContext)
{
    if (PackagesPreparedForSave.Remove(Package) > 0) UUIThemePaletteLibrary::RequestRefresh();
}
#endif
