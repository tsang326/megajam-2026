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
class USceneComponent;
class UProceduralMeshComponent;
class UPoseableMeshComponent;
class UMaterialInterface;
struct FDCDoc;

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
	// ---- the menus and the settings (DCMenu.cpp)
	float BaseFov = 72.f;   // the field of view the player chose
	bool bHeadBobOn = true; // the walking and swimming sway of the eyes
	bool bFrozen = false;   // a menu is open: nothing moves, the air does not run down
	bool bTitleCam = false; // the start menu: the camera circles the base outside
	// ---- walking (first person): one step every StepLen cm (the footfalls). The eyes stay level: no sway to the side,
	// no roll, no lean into a strafe; with "Head bob" on they dip a few millimetres at each footfall, and that is all.
	float StepPhase = 0.f;
	float StepAmt = 0.f;
	float LandDip = 0.f;
	float WalkDip = 0.f; // cm, down is negative
	float StrokeSoundPhase = 0.f;
	// test scripts (feel3): how far the eyes left their steady place since the last reading (cm right, cm up, degrees roll, degrees pitch)
	FVector4 FeelMin = FVector4(0.0, 0.0, 0.0, 0.0), FeelMax = FVector4(0.0, 0.0, 0.0, 0.0);

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
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> HeroSwim; // Unit 0's swimming materials
	float KickAmt = 0.25f;

	// ---- first person (DCArms.cpp): the camera is the diver's eyes; two arms swim, hold the camera, reach and shield
	bool bFirstPerson = true;
	TWeakObjectPtr<UPoseableMeshComponent> Arms;
	TWeakObjectPtr<UStaticMeshComponent> HandCam;   // the camera in the right hand
	TWeakObjectPtr<UStaticMeshComponent> HandCamRec; // its red light
	TWeakObjectPtr<USceneComponent> HandCamHolder;   // places the camera; the model sits turned inside it
	struct FArmRef { FName Upper, Lower, Hand; FTransform U, L, H; };
	FArmRef ArmL, ArmR;
	FQuat HandFrameL = FQuat::Identity, HandFrameR = FQuat::Identity; // each hand's own frame in the reference pose (DCArms.cpp)
	FVector CamHalf = FVector(13.0, 5.0, 6.0); // half the hand camera's size in its holder (x along the lens)
	float StrokePhase = 0.f;
	float StrokeAmt = 0.f;   // 0 still .. 1 swimming hard
	float ReachAmt = 0.f;    // the left hand reaches for what is in front (the base's controls)
	float ShieldAmt = 0.f;   // both arms up: a bite
	float HeadBob = 0.f;
	void SpawnArms();
	// the base (DCCareer.cpp): a flooded habitat on legs over the station, its controls, the hatch in its floor
	FVector BaseC = FVector(1150.0, -1150.0, 1150.0);
	FVector BaseHalf = FVector(1150.0, 650.0, 190.0);
	static constexpr double Stand = 145.0; // body centre over the deck when standing: the eyes at about 1.67 m
	TArray<FVector> BaseSpots; // desk, locker, bunk, board
	FVector BaseWake = FVector::ZeroVector;
	// the base is dry: air held in by its own pressure, the sea a flat surface in the hatch (a moon pool)
	FVector HatchAt = FVector::ZeroVector; // the middle of the hatch, at the water's surface
	double HatchHalf = 150.0;
	bool bDry = false;   // the diver stands in the air of the base
	bool bHatchAssist = false; // under the moon pool, close enough that SPACE lifts the diver in
	float DryAmt = 0.f;  // 0 water .. 1 air, eased: the look
	double BaseFloor() const { return BaseC.Z - BaseHalf.Z; }
	bool OverHatch(const FVector& P, double Margin = 0.0) const { return FMath::Abs(P.X - HatchAt.X) < HatchHalf - Margin && FMath::Abs(P.Y - HatchAt.Y) < HatchHalf - Margin; }
	void BuildBase();
	bool InBase(const FVector& P) const;
	void TickArms(float Dt, const FVector& Eye, const FRotator& View);
	FVector SolveArm(const FArmRef& A, const FVector& Target, const FVector& Pole, const FQuat& HandTurn); // gives the way the forearm points

	// ---- camera
	float Yaw = 0.f;
	float Pitch = -8.f;
	float BoomFrac = 1.f;
	float Fov = 72.f;
	float ViewFov = 72.f; // what the lens shows: Fov, or the hand camera's zoom
	float Dark = 0.f;     // 0..1: how far into the trench the lens is
	FDCDoc* Doc = nullptr; // the documentary: creatures, takes, the film
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

	// ---- the sea floor's low mounds (seen, not solid) and what rises from the vents
	struct FMound { FVector2D At; float Yaw; FVector Radii; float CentreZ; };
	TArray<FMound> Mounds;
	TWeakObjectPtr<UProceduralMeshComponent> Ground;
	float GroundZ(double X, double Y) const; // height of the sand at a place: the terrain plus the mounds
	float Terrain(double X, double Y) const; // the sea floor's shape: the station's flat, basins, slopes, the abyss trench
	float DepthAt(double Z) const { return FMath::Max(0.0, (2950.0 - Z) / 100.0 * 4.0); } // metres the readout shows
	void BuildTerrain(UMaterialInterface* Mat);
	struct FPlumeBit { TWeakObjectPtr<UStaticMeshComponent> Part; TWeakObjectPtr<UMaterialInstanceDynamic> MID; FVector Base; float Phase; float Speed; float Size; };
	TArray<FPlumeBit> PlumeBits;
	TArray<FVector> PlumeAt; // the mouth of each vent and chimney

	// ---- the wider sea: zones round the first site, each with its own water and depth (brief 08)
	struct FZone
	{
		const TCHAR* Title;
		FVector2D At;
		float Radius;      // full look inside this, blending out to 1.6x
		float Depth;       // metres added to the readout
		FLinearColor Fog;
		float FogDensity;
		float Key;
		FLinearColor KeyColor;
		float Fill;
		FLinearColor Snow;
	};
	TArray<FZone> Zones;
	float ZoneDepth = 0.f;      // what the depth readout adds here
	int32 ZoneNow = INDEX_NONE; // the zone Unit 0 is in, or none (the first site)
	FString ZoneTitle;
	float ZoneTitleTime = -1.f; // seconds the title card has been up; < 0 = none
	float LookTime = 0.f;
	void SetupZones();
	void TickZones(float Dt);

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
	TArray<TWeakObjectPtr<UStaticMeshComponent>> LateSolid; // big hard set dressing: made solid once everything has found its place
	UStaticMeshComponent* Bare(UStaticMesh* Mesh, UMaterialInterface* Mat, const FVector& At, const FRotator& Rot, const FVector& Scale, const FLinearColor& Color, UMaterialInstanceDynamic** OutMID = nullptr);
	AActor* Block(const FVector& Centre, const FRotator& Rot, const FVector& Size, const FLinearColor& Color);
	void AddRing(const FVector& At, float YawDeg, float PitchDeg);
	UPointLightComponent* Glow(const FVector& At, const FLinearColor& Color, float Power, float Reach);
	void MoveHero(const FVector& Delta, bool bStanding = false);
	void TickCamera(float Dt);
	void TickRings(const FVector& From);
};
