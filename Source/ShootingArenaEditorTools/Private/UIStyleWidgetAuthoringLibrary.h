#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UIStyleWidgetAuthoringLibrary.generated.h"

class UBlueprint;
class UUserWidget;
class UReusableStyleHost;
class UWidget;

UCLASS()
class UUIStyleWidgetAuthoringLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Editor|UI Styles")
    static UWidget* CreateWidgetInTree(UBlueprint* Blueprint, FName WidgetName,
        TSubclassOf<UWidget> WidgetClass, FName ParentName);

    UFUNCTION(BlueprintCallable, Category="Editor|UI Styles")
    static bool CreateStyleHostTemplate(UBlueprint* Blueprint);

    UFUNCTION(BlueprintCallable, Category="Editor|UI Styles")
    static UReusableStyleHost* WrapWithStyleHost(UBlueprint* Blueprint, FName WidgetName,
        TSubclassOf<UUserWidget> HostClass, FName HostName);

    UFUNCTION(BlueprintCallable, Category="Editor|UI Styles")
    static bool UpgradeStyleableCombo(UBlueprint* Blueprint, FName WidgetName);

    /** Removes only the retired theme variables after their graph references are cleared. */
    UFUNCTION(BlueprintCallable, Category="Editor|UI Styles")
    static bool RemoveUnusedStyleVariable(UBlueprint* Blueprint, FName VariableName);
};
