#include "LobbyConnectLibrary.h"
#include "ShootingArenaGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

FString ULobbyConnectLibrary::SanitizeIpInput(const FString& Text)
{
	FString Result;
	for (const TCHAR Char : Text)
	{
		if (FChar::IsDigit(Char) || (Char >= 'a' && Char <= 'z') || (Char >= 'A' && Char <= 'Z') || Char == '.')
		{
			Result.AppendChar(Char);
		}
	}
	return Result.Left(MaxIpLength);
}

FString ULobbyConnectLibrary::SanitizePortInput(const FString& Text)
{
	FString Result;
	for (const TCHAR Char : Text)
	{
		if (FChar::IsDigit(Char))
		{
			Result.AppendChar(Char);
		}
	}
	return Result.Left(MaxPortLength);
}

bool ULobbyConnectLibrary::IsValidServerAddress(const FString& IP)
{
	const FString Trimmed = IP.TrimStartAndEnd();
	if (Trimmed.Equals(TEXT("localhost"), ESearchCase::IgnoreCase))
	{
		return true;
	}

	TArray<FString> Octets;
	Trimmed.ParseIntoArray(Octets, TEXT("."), /*bCullEmpty=*/false);
	if (Octets.Num() != 4)
	{
		return false;
	}

	for (const FString& Octet : Octets)
	{
		if (Octet.IsEmpty() || Octet.Len() > 3 || !Octet.IsNumeric())
		{
			return false;
		}

		for (const TCHAR Char : Octet)
		{
			if (!FChar::IsDigit(Char))
			{
				return false;
			}
		}

		if (FCString::Atoi(*Octet) > 255)
		{
			return false;
		}
	}

	return true;
}

bool ULobbyConnectLibrary::IsValidPort(const FString& PortText, int32& OutPort)
{
	OutPort = 0;

	const FString Trimmed = PortText.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || Trimmed.Len() > MaxPortLength)
	{
		return false;
	}

	for (const TCHAR Char : Trimmed)
	{
		if (!FChar::IsDigit(Char))
		{
			return false;
		}
	}

	const int32 Port = FCString::Atoi(*Trimmed);
	if (Port < 1 || Port > 65535)
	{
		return false;
	}

	OutPort = Port;
	return true;
}

void ULobbyConnectLibrary::ConnectToServer(UObject* WorldContextObject, const FString& IP, int32 Port, const FString& Nickname)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	if (UShootingArenaGameInstance* GameInstance = World->GetGameInstance<UShootingArenaGameInstance>())
	{
		GameInstance->BeginConnectAttempt(IP.TrimStartAndEnd(), Port);
	}

	FString URL = FString::Printf(TEXT("%s:%d"), *IP.TrimStartAndEnd(), Port);
	if (!Nickname.IsEmpty())
	{
		URL += TEXT("?Nickname=") + Nickname;
	}

	PC->ClientTravel(URL, ETravelType::TRAVEL_Absolute);
}
