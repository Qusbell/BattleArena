#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DeathDropSettings.generated.h"

/** 캐릭터가 한 번 사망했을 때 사용할 드롭 설정. 맵에 배치된 아이템 스포너에는 적용하지 않는다. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FDeathDropSettings : public FTableRowBase
{
	GENERATED_BODY()

	/** 장착 중인 무기를 드롭 후보에 포함한다. 기본 무기이면 캐릭터 로드아웃의 GrantedWeapons[0]으로 대체한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool WeaponDropEnabled = true;

	/** 이전 에셋 데이터 보존용으로만 남겨 둔 값. 현재 드롭 로직에서는 사용하지 않으며, 대체 무기는 GrantedWeapons[0]을 사용한다. */
	UPROPERTY() FDataTableRowHandle SubstituteWeapon;

	/** 사망 시 남은 시간이 있는 활성 버프를 드롭 후보에 포함한다. 기존 종료 시각을 유지하며 지속 시간을 새로 시작하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool BuffDropEnabled = true;

	/** SmallHealthItem에 지정한 소형 힐팩을 드롭 후보에 포함한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool SmallHealthDropEnabled = true;

	/** SmallArmorItem에 지정한 소형 아머를 드롭 후보에 포함한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool SmallArmorDropEnabled = true;

	/** 한 번의 사망에서 생성할 최대 아이템 수. 후보가 더 많으면 중복 없이 균등 무작위로 선택한다. 0이면 아무것도 드롭하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 MaxDropCount = 4;

	/** 방사 방향에 더하는 수평 각도 오차의 최대 크기(도). 각 아이템에 -값~+값을 무작위로 적용하며, 드롭이 1개이면 적용하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="180")) int32 DirectionError = 10;

	/** 발사 방향의 최소 위쪽 각도(도). 수평이 0도, 수직 위쪽이 90도이며 MinUpAngle~MaxUpAngle에서 무작위로 선택한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="90")) int32 MinUpAngle = 35;

	/** 발사 방향의 최대 위쪽 각도(도). 최소/최대 값이 뒤바뀌어도 작은 값~큰 값의 범위로 처리한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="90")) int32 MaxUpAngle = 65;

	/** 한 번 적용하는 발사 임펄스의 최소 크기(kg·cm/s). 현재 carrier 질량은 1kg이므로 이 값은 초기 속도 크기(cm/s)와 같다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float MinLaunchForce = 350.0f;

	/** 발사 임펄스의 최대 크기(kg·cm/s). 최소~최대 범위에서 무작위로 선택하며, 값이 클수록 더 빠르게 발사한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float MaxLaunchForce = 550.0f;

	/** 발사 후 수평 속도가 0이 되기까지의 시간(초). 수직 속도와 중력에는 적용하지 않으며, 바닥에 닿으면 시간과 관계없이 정지한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01")) float HorizontalDecelTime = 1.2f;

	/** 수평 감속 곡선의 지수. 남은 속도 비율은 1-(경과 시간/감속 시간)^지수이며, 값이 클수록 초반 속도를 오래 유지하고 후반에 빠르게 감속한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.01")) float HorizontalDecelExponent = 2.0f;

	/** 미획득 드롭의 최대 생존 시간(초). 발사 시점부터 계산하므로 공중에 있는 시간도 포함한다. 획득 후 버프 지속 시간과는 별개다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01")) float DespawnTime = 30.0f;

	/** 소형 힐팩의 데이터 테이블과 행 이름. 실제 회복량과 아이템 정보는 이 행에 지정된 기존 설정을 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup Content") FDataTableRowHandle SmallHealthItem;

	/** 소형 아머의 데이터 테이블과 행 이름. 실제 아머 획득량과 아이템 정보는 이 행에 지정된 기존 설정을 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup Content") FDataTableRowHandle SmallArmorItem;

	/** 장착 무기 또는 기본 지급 무기에 대응하는 픽업 행을 찾을 테이블. 무기 데이터 에셋을 우선 비교하고, 무기 클래스 일치를 보조로 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup Content") TSoftObjectPtr<UDataTable> WeaponTable;

	/** 활성 버프의 currentBuffRowName으로 드롭할 버프 정보를 조회하는 테이블. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup Content") TSoftObjectPtr<UDataTable> BuffTable;

	/** 총기 드롭에 사용할 기존 픽업 액터 클래스(현재 BP_HoldableItem). 실제 무기 종류는 WeaponTable에서 찾은 행으로 설정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup Content") TSoftClassPtr<AActor> WeaponPickupClass;

	/** 버프 드롭에 사용할 기존 픽업 액터 클래스(현재 BP_BuffItem). 버프 행과 원래 종료 시각을 전달한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup Content") TSoftClassPtr<AActor> BuffPickupClass;

	/** 힐팩과 아머가 함께 사용하는 기존 픽업 액터 클래스(현재 BP_LifeItem). 각 아이템의 종류와 효과는 SmallHealthItem/SmallArmorItem 행으로 구분한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup Content") TSoftClassPtr<AActor> LifePickupClass;
};
