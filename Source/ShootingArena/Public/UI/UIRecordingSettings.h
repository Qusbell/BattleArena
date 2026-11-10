#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/ContentWidget.h"
#include "UIRecordingSettings.generated.h"

UENUM(BlueprintType)
enum class ERecordingUISection : uint8
{
	Combat, Timer, Ranking, Scoreboard, Crosshair, KillFeed, Medals, Buffs,
	DamageEffects, DeathScreen, Menus, Results, Nicknames, Cursor, Loading
};

/** Local recording presentation settings; never changes gameplay state. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UUIRecordingSettings : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="전투 UI (체력·방어구·무기)")) bool Combat = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="타이머")) bool Timer = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="상단 순위·점수")) bool Ranking = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="Tab 점수판")) bool Scoreboard = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="조준점·히트마커·줌")) bool Crosshair = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="킬 로그·킬 알림")) bool KillFeed = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="메달·업적")) bool Medals = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="버프 아이콘·시간·화면 효과")) bool Buffs = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="피격 화면 효과·방향")) bool DamageEffects = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="사망·리스폰 화면")) bool DeathScreen = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="메뉴·설정·로비")) bool Menus = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="경기 결과")) bool Results = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="대상 닉네임")) bool Nicknames = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="커스텀 UI 커서")) bool Cursor = true;
	UPROPERTY(EditAnywhere, Category="촬영 UI", meta=(DisplayName="로딩·맵 전환 화면")) bool Loading = true;

	bool IsShown(ERecordingUISection Section) const;
	static UUIRecordingSettings* Get();
};

/** A paint-only mask: child visibility, animation, input and timers retain their original behavior. */
UCLASS(meta=(DisplayName="Recording UI Mask"))
class SHOOTINGARENA_API URecordingUIMask : public UContentWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="촬영 UI") ERecordingUISection Section = ERecordingUISection::Combat;
	bool IsContentShown() const;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
private:
	UPROPERTY(Transient) TObjectPtr<UUIRecordingSettings> ActiveSettings;
	TSharedPtr<class SRecordingUIMask> Mask;
};
