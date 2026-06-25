// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "vx_GW_GameMode.generated.h"

/**
 * Fired on the server after a player has fully joined (PostLogin).
 * @param NewPlayer  The controller that just connected.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FvxOnPlayerConnected, APlayerController*, NewPlayer);

/**
 * Fired on the server when a player leaves (Logout).
 * @param ExitingController  The controller that is disconnecting.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FvxOnPlayerDisconnected, AController*, ExitingController);

/**
 * Base game mode for the dedicated server.
 *
 * Owns the player connection lifecycle (approval -> login -> logout) and surfaces
 * generic, Blueprint-friendly hooks so connection policy and reactions can be
 * authored without touching C++. Intended to be subclassed / extended.
 */
UCLASS()
class VX_GODSWORK_API Avx_GW_GameMode : public AGameMode
{
	GENERATED_BODY()

public:
	Avx_GW_GameMode();

	// --- Blueprint events --------------------------------------------------

	/** Broadcast after a player has successfully joined. */
	UPROPERTY(BlueprintAssignable, Category = "GodsWork|Connection")
	FvxOnPlayerConnected OnPlayerConnected;

	/** Broadcast when a player disconnects. */
	UPROPERTY(BlueprintAssignable, Category = "GodsWork|Connection")
	FvxOnPlayerDisconnected OnPlayerDisconnected;

	// --- Configuration -----------------------------------------------------

	/**
	 * Maximum number of simultaneously connected players the server will accept.
	 * A value <= 0 means "unlimited" (the engine's own session limits still apply).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GodsWork|Connection")
	int32 MaxConnectedPlayers = 0;

	// --- Queries -----------------------------------------------------------

	/** Number of player controllers currently connected and tracked. */
	UFUNCTION(BlueprintPure, Category = "GodsWork|Connection")
	int32 GetConnectedPlayerCount() const { return ConnectedControllers.Num(); }

	/** Snapshot of all currently connected player controllers. */
	UFUNCTION(BlueprintPure, Category = "GodsWork|Connection")
	TArray<APlayerController*> GetConnectedPlayers() const;

	// --- Extension points --------------------------------------------------

	/**
	 * Decide whether an incoming connection should be accepted, before the player
	 * is spawned. Override in Blueprint or C++ to add custom policy (bans, allow
	 * lists, version checks, queue logic, etc). The default implementation enforces
	 * MaxConnectedPlayers.
	 *
	 * @param Options        URL options string supplied by the connecting client.
	 * @param Address        Remote network address of the connecting client.
	 * @param UniqueId       Stringified unique net id of the client (may be empty).
	 * @param OutRejectReason  Set this to a non-empty reason to reject the connection.
	 * @return true to accept, false to reject (OutRejectReason should explain why).
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "GodsWork|Connection")
	bool ApprovePlayerLogin(const FString& Options, const FString& Address, const FString& UniqueId, FString& OutRejectReason);
	virtual bool ApprovePlayerLogin_Implementation(const FString& Options, const FString& Address, const FString& UniqueId, FString& OutRejectReason);

	/** Called after a player has fully joined, before the connected delegate fires. Generic hook for subclasses/Blueprints. */
	UFUNCTION(BlueprintNativeEvent, Category = "GodsWork|Connection")
	void OnPlayerJoined(APlayerController* NewPlayer);
	virtual void OnPlayerJoined_Implementation(APlayerController* NewPlayer) {}

	/** Called when a player leaves, before the disconnected delegate fires. Generic hook for subclasses/Blueprints. */
	UFUNCTION(BlueprintNativeEvent, Category = "GodsWork|Connection")
	void OnPlayerLeft(AController* ExitingController);
	virtual void OnPlayerLeft_Implementation(AController* ExitingController) {}

protected:
	// --- AGameMode connection lifecycle ------------------------------------

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Controllers currently connected to this server. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "GodsWork|Connection")
	TArray<APlayerController*> ConnectedControllers;
};
