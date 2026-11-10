#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MapResultImageLibrary.generated.h"

class UTexture2D;

/**
 * DT_MapData(ST_MapData) 의 Result_Image 컬럼에서 "지금 열려 있는 레벨"의 결과창 배경 이미지를 읽는 헬퍼입니다.
 * 행 선택: 현재 월드 패키지와 같은 Map_Asset 을 가진 행. 같은 맵을 쓰는 행이 여러 개면 GameInstance 의 Selected_Map_ID 행을 우선합니다.
 * 일치하는 행이 없으면 Selected_Map_ID 행을 사용합니다. 행/이미지가 없으면 nullptr 이므로 호출 쪽에서 기존 배경을 유지하면 됩니다.
 * 구조체가 블루프린트 구조체라서 리플렉션으로 필드를 읽습니다(속성 이름 뒤에 GUID 가 붙어 접두어로 찾습니다).
 */
UCLASS()
class SHOOTINGARENA_API UMapResultImageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "MapData", meta = (WorldContext = "WorldContextObject"))
	static UTexture2D* GetCurrentMapResultImage(const UObject* WorldContextObject);
};
