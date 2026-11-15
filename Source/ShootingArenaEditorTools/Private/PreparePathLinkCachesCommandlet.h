#pragma once

#include "Commandlets/Commandlet.h"
#include "PreparePathLinkCachesCommandlet.generated.h"

UCLASS()
class UPreparePathLinkCachesCommandlet final : public UCommandlet
{
    GENERATED_BODY()

public:
    virtual int32 Main(const FString& Params) override;
};
