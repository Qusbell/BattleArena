#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PortalViewSubsystem.generated.h"

class AOneWayTeleportActor;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * 각 클라이언트의 로컬 카메라 기준으로만 포탈 화면을 계산하고 렌더링합니다.
 * 서버와 다른 클라이언트에는 Render Target, Material 상태를 복제하지 않습니다.
 */
UCLASS()
class SHOOTINGARENA_API UPortalViewSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float deltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	void EnsureRenderTarget(int32 size);
	void ClearActivePortal();

	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> sceneCapture;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> renderTarget;

	TWeakObjectPtr<AOneWayTeleportActor> activePortal;
	float captureAccumulator = 0.0f;
};
