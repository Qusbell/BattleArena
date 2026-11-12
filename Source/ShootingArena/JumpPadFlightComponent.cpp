// Fill out your copyright notice in the Description page of Project Settings.

#include "JumpPadFlightComponent.h"

#include "Analyzer/ArenaMatchLogLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UJumpPadFlightComponent::UJumpPadFlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UJumpPadFlightComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		MoveComp = OwnerCharacter->GetCharacterMovement();
		OwnerCharacter->LandedDelegate.AddDynamic(this, &UJumpPadFlightComponent::HandleLanded);
	}
}

void UJumpPadFlightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerCharacter)
	{
		OwnerCharacter->LandedDelegate.RemoveDynamic(this, &UJumpPadFlightComponent::HandleLanded);
	}

	Super::EndPlay(EndPlayReason);
}

void UJumpPadFlightComponent::BeginFlight(float RiseGravityScale, float FallGravityScale)
{
	if (!MoveComp)
	{
		return;
	}

	// 전투 기록: 플레이어의 점프가 아니라 점프패드로 인한 이동임을 알립니다(기록 중이 아니면 아무 일도 하지 않음).
	UArenaMatchLogLibrary::RecordForcedMovement(Cast<APawn>(GetOwner()), TEXT("JumpPad"));

	// 이미 비행 중이면(연속 점프패드) 원래 값은 덮어쓰지 않는다.
	if (!bInFlight)
	{
		OrigGravityScale = MoveComp->GravityScale;
	}

	RiseScale = FMath::Abs(RiseGravityScale);
	FallScale = FMath::Abs(FallGravityScale);
	bInFlight = true;

	// 발사 직후는 상승 구간
	MoveComp->GravityScale = RiseScale;

	SetComponentTickEnabled(true);
}

void UJumpPadFlightComponent::EndFlight()
{
	if (bInFlight && MoveComp)
	{
		MoveComp->GravityScale = OrigGravityScale;
	}

	bInFlight = false;
	SetComponentTickEnabled(false);
}

void UJumpPadFlightComponent::HandleLanded(const FHitResult& Hit)
{
	EndFlight();
}

void UJumpPadFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInFlight || !MoveComp || !MoveComp->IsFalling())
	{
		return;
	}

	MoveComp->GravityScale = (MoveComp->Velocity.Z > 0.0f) ? RiseScale : FallScale;
}
