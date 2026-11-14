// Deep Cut (brief 07): the documentary. Creatures that live in the 3D world, the hand camera, takes and their score,
// the shot list, the cutting room and the finished film.
// Every creature is a function of the clock (plus two recorded numbers), so a take can be shown again from what the
// camera did: nothing is saved but the camera's path.

#pragma once

#include "CoreMinimal.h"

class AMatchCutGameMode;
class AActor;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USpotLightComponent;
class UStaticMeshComponent;
struct FDC3D;

namespace DCSub
{
	enum Type { Manta = 0, Shoal, Angler, Jelly, Giant, Crew, Num };
}

enum class EDCDocState : uint8
{
	Shooting,
	Edit,   // the cutting room: one take per shot
	Film,   // title card, the takes in order, credits
	Rating  // the last card
};

struct FDCDocFrame
{
	FVector Cam = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;
	float Fov = 60.f;
	float Focus = 1000.f;
	FVector Hero = FVector::ZeroVector;
	float Split = 0.f;
	float Wave = 0.f;
	bool bLamp = false;
};

struct FDCTake
{
	int32 Shot = 0;
	int32 Stars = 0;
	float T0 = 0.f;   // the clock when the take began
	float Len = 3.f;
	TArray<FDCDocFrame> Frames; // evenly spread over Len
};

/** What one subject looks like through the lens right now. */
struct FDCSeen
{
	bool bIn = false;      // in the picture and not behind rock
	bool bWhole = false;   // not cut by the edge
	bool bAction = false;  // the moment the shot asks for is happening
	FVector2D At = FVector2D::ZeroVector; // -1..1, x right, y up
	float Size = 0.f;      // its radius as a part of half the picture's height
	float Dist = 0.f;
};

struct FDCDoc
{
	FDCDoc(AMatchCutGameMode* InGM, FDC3D* InW) : GM(InGM), W(InW) {}

	void Start();
	void Tick(float Dt);
	int32 RunLine(const TArray<FString>& T);

	AMatchCutGameMode* GM = nullptr;
	FDC3D* W = nullptr;

	// ---- input, written by the player controller
	bool bAimHeld = false;
	bool bAimToggle = false;
	bool bRecordPressed = false;
	bool bLampPressed = false;
	bool bEnterPressed = false;
	bool bBackPressed = false;
	int32 TrackSub = INDEX_NONE; // test script: keep the lens on this subject
	float TrackLeft = 0.f;
	float ZoomIn = 0.f;      // wheel clicks or held keys, used up each frame
	int32 NavX = 0, NavY = 0; // the cutting room: presses, used up each frame

	// ---- state
	EDCDocState State = EDCDocState::Shooting;
	float StateTime = 0.f;
	float Clock = 0.f;       // the world's clock: creatures are where this says
	float Now = 0.f;         // what the creatures show: Clock, or a take's own time while it plays
	bool bAim = false;
	float AimBlend = 0.f;
	float Zoom = 60.f;       // field of view of the hand camera
	float MinFov = 30.f;
	float Focus = 1000.f;
	float LampPower = 30.f;
	bool bLampOwned = false, bLampOn = false, bLens = false;
	bool bEverAimed = false, bEverRecorded = false, bEverZoomed = false;

	// recording
	bool bRecording = false;
	float RecTime = 0.f;
	FDCTake Rec;
	struct FTally { int32 In = 0, Whole = 0, Action = 0; float Size = 0.f, Off = 0.f; };
	FTally Tally[DCSub::Num];
	int32 RecFrames = 0;
	float RecShake = 0.f;
	FRotator LastRot = FRotator::ZeroRotator;

	// the last take's verdict, shown for a few seconds
	int32 ResultShot = INDEX_NONE;
	int32 ResultStars = 0;
	FString ResultReason;
	float ResultTime = 0.f;
	float ListTime = 8.f;    // the shot list is shown in full while this runs
	FString Note;            // one short line from the director
	float NoteTime = 0.f;

	// ---- guidance: one shot is the current one; a marker leads to it; the director talks between shots
	int32 Goal = INDEX_NONE;      // the current shot, or INDEX_NONE when every shot is done
	FVector GoalAt = FVector::ZeroVector; // where the marker points (the subject, or the headlamp first)
	bool bGoalIsItem = false;     // the marker is on the headlamp, not on the subject
	float SinceProgress = 0.f;    // seconds since the last new shot: the marker pulses after a minute
	float NoteAge = 0.f;          // the director's line types itself out
	FString Tip;                  // one tip after a weak take
	bool bDirStarted = false;
	bool Bearing(const FVector& P, FVector2D& Out) const; // true when P is in front of the lens; Out as Project, or the way to turn

	TArray<FDCTake> Takes;
	int32 Best[DCSub::Num];   // most stars so far per shot
	int32 Pick[DCSub::Num];   // index into Takes of the take that goes into the film, or INDEX_NONE
	int32 EditShot = 0;
	int32 FilmPart = 0;       // 0 title, 1..n takes, n+1 credits
	float FilmTime = 0.f;
	int32 PlayTake = INDEX_NONE;
	float PlayT = 0.f;

	// what the lens sees now (for the viewfinder)
	FDCSeen Seen[DCSub::Num];
	int32 Lock = INDEX_NONE;  // the subject nearest the middle of the picture

	// ---- creatures
	FVector SubPos[DCSub::Num];
	float SubRadius[DCSub::Num];
	TWeakObjectPtr<AActor> Manta, Angler, Giant, Crew;
	TArray<TWeakObjectPtr<AActor>> Jellies;
	TArray<FVector> JellyPos;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Fish;
	TArray<FVector> FishPos;
	TArray<FVector4> FishSeed;
	float FishScale = 1.f;
	FQuat FishFix = FQuat::Identity;
	float Split = 0.f, Wave = 0.f;
	FVector SplitFrom = FVector::ZeroVector;
	TWeakObjectPtr<UPointLightComponent> Lure, CrewLight, LampGlow, LensGlow;
	TWeakObjectPtr<AActor> LampItem, LensItem;
	bool bGiantCalled = false;
	// life that is not on the shot list: it wanders, crawls or passes far off. Each one is a loop in time.
	struct FAmbient
	{
		TWeakObjectPtr<AActor> Actor;
		FVector Centre = FVector::ZeroVector;
		float Ra = 0.f, Rb = 0.f, Phase = 0.f, Omega = 0.f, Bob = 0.f, Wiggle = 0.f;
		float Surge = 0.f;
	};
	TArray<FAmbient> Ambient;
	TWeakObjectPtr<UPointLightComponent> ShoalLight, JellyLight;
	FVector LampAt = FVector::ZeroVector, LensAt = FVector::ZeroVector;

	// ---- helpers the HUD and the 3D game use
	int32 ShotsDone() const;
	int32 StarsTotal() const;
	bool CanEnd() const { return ShotsDone() >= 4; }
	bool ViewOverride(FVector& OutPos, FRotator& OutRot, float& OutFov) const; // while a take plays
	bool Project(const FVector& P, FVector2D& Out, float& Depth) const;
	const FDCDocFrame* PlayFrame() const { return bHasPlay ? &Play : nullptr; }
	int32 TakesOf(int32 Shot, TArray<int32>* Out = nullptr) const;
	void Say(const FString& Line, float Seconds = 6.f);

private:
	FDCDocFrame Play;
	bool bHasPlay = false;
	FVector CamP = FVector::ZeroVector;
	FRotator CamR = FRotator::ZeroRotator;
	float CamFov = 72.f;

	AActor* SpawnCreature(const TCHAR* Id, float Length, float FrontYaw, bool bLampOnly = false, float* OutHalfHeight = nullptr, float Pace = 1.f);
	void SpawnAmbient();
	UPointLightComponent* SoftLight(const FLinearColor& Color, float Power, float Reach);
	void SpawnShoal();
	AActor* SpawnPickup(const FVector& At, const FLinearColor& Color, TWeakObjectPtr<UPointLightComponent>& OutLight);
	void TickCreatures(float Dt, const FVector& HeroPos, bool bLive);
	void Look(const FVector& P, const FRotator& R, float Fov);
	void TickShooting(float Dt);
	void EndTake();
	void SetState(EDCDocState S);
	void StartPlayTake(int32 TakeIndex);
	void TickPlay(float Dt, bool bLoop);
};
