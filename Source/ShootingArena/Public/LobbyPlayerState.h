#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

/**
 * 멀티 로비에 접속한 사람 한 명의 상태. 서버가 권한을 갖습니다.
 * 클라이언트 UI 는 이 값을 직접 읽지 않고 ALobbyGameState 의 스냅샷(FLobbySnapshot)을 읽습니다 —
 * 이 프로젝트는 Push Model 때문에 PlayerState 의 일반 복제 값이 중간에 안 바뀌는 문제가 있어서입니다.
 */
UCLASS()
class SHOOTINGARENA_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// 로비에 들어온 순서 (1부터). 방장 승계와 카드 순서의 기준입니다. 서버 전용으로 기록합니다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 JoinOrder = 0;

	// 준비 완료 여부. 방장은 준비 개념이 없어서 값을 무시합니다. 서버 전용으로 기록합니다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bReady = false;

	// 플레이어 이름 = 캐릭터 이름 (명세 §1). 별도 값을 두지 않고 PlayerName 을 그대로 씁니다.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	FString GetCharacterName() const { return GetPlayerName(); }
};
