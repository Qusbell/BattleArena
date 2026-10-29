#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LobbyConnectLibrary.generated.h"

/**
 * 서버 접속 화면(WBP_MultiConnect)이 쓰는 입력 검사/정리와 접속 시작 함수 모음. (명세 §2)
 */
UCLASS()
class SHOOTINGARENA_API ULobbyConnectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxIpLength = 39;
	static constexpr int32 MaxPortLength = 5;

	// IP 칸 입력 제한: 숫자·영문·점만 남기고 최대 39자로 자릅니다.
	UFUNCTION(BlueprintPure, Category = "Multi|Connect")
	static FString SanitizeIpInput(const FString& Text);

	// 포트 칸 입력 제한: 숫자만 남기고 최대 5자로 자릅니다.
	UFUNCTION(BlueprintPure, Category = "Multi|Connect")
	static FString SanitizePortInput(const FString& Text);

	// IPv4(0~255 네 칸) 또는 localhost 인지.
	UFUNCTION(BlueprintPure, Category = "Multi|Connect")
	static bool IsValidServerAddress(const FString& IP);

	// 1~65535 사이의 숫자인지. 맞으면 OutPort 에 값을 담습니다.
	UFUNCTION(BlueprintPure, Category = "Multi|Connect")
	static bool IsValidPort(const FString& PortText, int32& OutPort);

	// 접속을 시작합니다: GameInstance 에 주소를 기억시키고 접속 시도 상태로 바꾼 뒤 ClientTravel 합니다.
	// IP/포트 검사는 호출하기 전에 끝내세요. Nickname 이 비어 있지 않으면 ?Nickname= 옵션으로 실어 보냅니다.
	UFUNCTION(BlueprintCallable, Category = "Multi|Connect", meta = (WorldContext = "WorldContextObject"))
	static void ConnectToServer(UObject* WorldContextObject, const FString& IP, int32 Port, const FString& Nickname);
};
