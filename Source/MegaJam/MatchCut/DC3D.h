// Deep Cut (brief 06): the 3D game. Free swimming with momentum, a camera behind Unit 0, one test area.
// Everything 3D lives in this struct so the game mode header does not change when the 3D game does.

#pragma once

#include "CoreMinimal.h"

class AMatchCutGameMode;
class AActor;
class ACameraActor;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USpotLightComponent;
class UStaticMeshComponent;

struct FDC3DRing
{
	FVector Pos = FVector::ZeroVector;
	FVector Axis = FVector::ForwardVector;
	float Radius = 150.f;
	bool bDone = false;
	TArray<int32> Paints;
};

struct FDC3D
{
	explicit FDC3D(AMatchCutGameMode* In) : GM(In) {}

	void Start();
	void Tick(float Dt);
	/** A script line. 0 = not a 3D command, 1 = done, 2 = done and the script waits. */
	int32 RunLine(const TArray<FString>& T);

	AMatchCutGameMode* GM = nullptr;

	// ---- input, written by the player controller (or by a test script)
	FVector Move = FVector::ZeroVector;        // x = where the camera looks, y = right, z = up
	FVector2D LookDelta = FVector2D::ZeroVector; // degrees, from the mouse, used up each frame
	FVector2D LookRate = FVector2D::ZeroVector;  // degrees per second, from keys or a stick
	bool bBoost = false;
	float Sens = 0.11f; // degrees per mouse count
	bool bInvertY = false;

	// ---- Unit 0
	FVector Pos = FVector(0.0, 0.0, 260.0);
	FVector Vel = FVector::ZeroVector;
	float BodyYaw = 0.f;
	float BodyPitch = 0.f;
	float BodyRoll = 0.f;
	float Radius = 46.f;
	float BumpTime = 0.f;
	float ThrustTime = 0.f;
	float IdleTime = 0.f; // seconds without input: the controls line comes back

	// ---- camera
	float Yaw = 0.f;
	float Pitch = -8.f;
	float BoomFrac = 1.f;
	float Fov = 72.f;
	FVector Pivot = FVector::ZeroVector;
	FVector CamPos = FVector::ZeroVector;
	TWeakObjectPtr<ACameraActor> Cam;

	// ---- the test area
	TArray<FDC3DRing> Rings;
	int32 RingsDone = 0;
	float RingFlash = 0.f;
	TWeakObjectPtr<AActor> Snow;
	TWeakObjectPtr<AActor> Bubbles;
	TWeakObjectPtr<UMaterialInstanceDynamic> SnowMID;
	TWeakObjectPtr<UMaterialInstanceDynamic> BubbleMID;
	TWeakObjectPtr<USpotLightComponent> Lamp;
	TArray<TWeakObjectPtr<UPointLightComponent>> Glows;
	TArray<TWeakObjectPtr<AActor>> Slats; // unseen bars high in the water: their shadows are the shafts of light
	TArray<FVector> SlatBase;
	bool bLumen = false;
	float Time = 0.f;

	// ---- test script
	FVector ScriptMove = FVector::ZeroVector;
	float ScriptMoveLeft = 0.f;
	bool bScriptBoost = false;
	FVector2D ScriptTurn = FVector2D::ZeroVector;
	float ScriptTurnLeft = 0.f;
	bool bSwimTo = false;
	FVector SwimTarget = FVector::ZeroVector;
	float SwimTime = 0.f;

private:
	void BuildArea();
	void SpawnHero();
	void SpawnFlecks();
	AActor* Place(const TCHAR* Id, int32 Variant, const FVector& Base, const FRotator& Rot, float Scale, bool bCollide = true);
	AActor* Block(const FVector& Centre, const FRotator& Rot, const FVector& Size, const FLinearColor& Color);
	void AddRing(const FVector& At, float YawDeg, float PitchDeg);
	UPointLightComponent* Glow(const FVector& At, const FLinearColor& Color, float Power, float Reach);
	void MoveHero(const FVector& Delta);
	void TickCamera(float Dt);
	void TickRings(const FVector& From);
};
