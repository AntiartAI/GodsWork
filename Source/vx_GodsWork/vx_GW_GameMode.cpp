// Fill out your copyright notice in the Description page of Project Settings.


#include "vx_GW_GameMode.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogvxGWConnection, Log, All);

Avx_GW_GameMode::Avx_GW_GameMode()
{
}

TArray<APlayerController*> Avx_GW_GameMode::GetConnectedPlayers() const
{
	return ConnectedControllers;
}

bool Avx_GW_GameMode::ApprovePlayerLogin_Implementation(const FString& Options, const FString& Address, const FString& UniqueId, FString& OutRejectReason)
{
	// Default policy: enforce the configured connection cap. Everything else is open.
	if (MaxConnectedPlayers > 0 && ConnectedControllers.Num() >= MaxConnectedPlayers)
	{
		OutRejectReason = TEXT("Server is full.");
		return false;
	}

	return true;
}

void Avx_GW_GameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	// Let the engine run its own validation first (session state, ban list, etc).
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		UE_LOG(LogvxGWConnection, Log, TEXT("PreLogin rejected by engine for %s: %s"), *Address, *ErrorMessage);
		return;
	}

	const FString UniqueIdStr = UniqueId.IsValid() ? UniqueId.ToString() : FString();

	// Custom approval policy (overridable in C++/Blueprint).
	FString RejectReason;
	if (!ApprovePlayerLogin(Options, Address, UniqueIdStr, RejectReason))
	{
		// Guarantee a non-empty error so the engine refuses the connection.
		ErrorMessage = RejectReason.IsEmpty() ? TEXT("Connection refused.") : RejectReason;
		UE_LOG(LogvxGWConnection, Log, TEXT("PreLogin rejected for %s (%s): %s"), *Address, *UniqueIdStr, *ErrorMessage);
		return;
	}

	UE_LOG(LogvxGWConnection, Verbose, TEXT("PreLogin approved for %s (%s)"), *Address, *UniqueIdStr);
}

void Avx_GW_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer)
	{
		ConnectedControllers.AddUnique(NewPlayer);
		UE_LOG(LogvxGWConnection, Log, TEXT("Player connected: %s (%d connected)"), *GetNameSafe(NewPlayer), ConnectedControllers.Num());

		OnPlayerJoined(NewPlayer);
		OnPlayerConnected.Broadcast(NewPlayer);
	}
}

void Avx_GW_GameMode::Logout(AController* Exiting)
{
	if (APlayerController* ExitingPC = Cast<APlayerController>(Exiting))
	{
		ConnectedControllers.Remove(ExitingPC);
	}

	UE_LOG(LogvxGWConnection, Log, TEXT("Player disconnected: %s (%d connected)"), *GetNameSafe(Exiting), ConnectedControllers.Num());

	OnPlayerLeft(Exiting);
	OnPlayerDisconnected.Broadcast(Exiting);

	Super::Logout(Exiting);
}
