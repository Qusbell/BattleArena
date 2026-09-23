#pragma once

#include "CoreMinimal.h"

class FDressSwapTool
{
public:
	void Register();
	void Unregister();

private:
	TSharedRef<class SDockTab> CreateTab(const class FSpawnTabArgs& Args);

	bool bRegistered = false;
	FDelegateHandle ToolMenusStartupCallbackHandle;
};
