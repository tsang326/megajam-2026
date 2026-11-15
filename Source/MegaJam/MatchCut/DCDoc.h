// Deep Cut (brief 07): the documentary. Creatures that live in the 3D world, the hand camera, takes and their score,
// the shot list, the cutting room and the finished film.
// Every creature is a function of the clock (plus two recorded numbers), so a take can be shown again from what the
// camera did: nothing is saved but the camera's path.

#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/TextureRenderTarget2D.h"
class ASceneCapture2D;

class AMatchCutGameMode;
class AActor;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USpotLightComponent;
class UStaticMeshComponent;
struct FDC3D;

/** Gives a part the swimming material (M_DC_Creature) with the numbers of its kind; Q turns the mesh so its front is along x. */
void DCApplySwim(UStaticMeshComponent* C, const FString& Id, const FQuat& Q, float Pace, float Phase);

namespace DCSub
{
	enum Type { Manta = 0, Shoal, Angler, Jelly, Giant, Crew, Num };
}

enum class EDCDocState : uint8
{
	Shooting,
	Edit,   // the cutting room: the dive's clips, each in or out
	Film,   // title card, the kept clips in order, credits
	Rating, // the last card: the channel after the last day
	Upload, // the video goes up: views per clip, comments, money
	Shop    // between dives: gear for money, then the next dive
};

/** Something worth filming: one of the six moments, a species of the ambient life, or the legend of the abyss. */
struct FDCSubject
{
	FString Id;      // creature id, or the moment's name
	FString Name;    // what the upload calls it
	int32 Value = 0; // views for a three-star clip the first time it is seen
	float Radius = 50.f;
	bool bDanger = false;   // it bites
	bool bEvidence = false; // one of the proofs the Director's Cut needs
	bool bLensOnly = false; // seen only through the camera
	int32 Zone = -1;        // where it lives (FDC3D::Zones), -1 the station's site
};

/** One clip on the uploaded video. */
struct FDCUploadLine
{
	int32 Subject = 0;
	int32 Stars = 0;
	int32 Views = 0;
	bool bNew = false;   // never uploaded before
	bool bDup = false;   // already in this video
};

/** Gear: what the shop sells, by level. */
namespace DCGear
{
	enum Type { Tank = 0, Hull, Lamp, Lens, Scooter, Tape, Num };
}

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
	FVector Hunt = FVector::ZeroVector; // the legend of the abyss: it hunts, so its path is kept
	float HuntYaw = 0.f;
};

struct FDCTake
{
	int32 Shot = 0;
	int32 Stars = 0;
	float T0 = 0.f;   // the clock when the take began
	float Len = 3.f;
	bool bKeep = true;  // in the cut
	bool bDanger = false; // something that bites was close while it rolled: viewers love it
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
	bool bUsePressed = false; // E / ENTER: the base's desk, locker, bunk, board
	int32 TrackSub = INDEX_NONE; // test script: keep the lens on this subject
	TWeakObjectPtr<AActor> TrackActor; // test script: or on this one creature
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
	TArray<FTally> Tally; // per subject
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
	TArray<FDCSeen> Seen; // per subject
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
	TArray<FTransform> FishPrevXf; // the fish's places last frame (motion vectors)
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
		bool bAway = false; // far off in another zone: hidden and not moved
		int32 Subject = INDEX_NONE;
		float Radius = 50.f;
		// how it lives (DCLife in DCDoc.cpp)
		int32 Beh = 0, Flags = 0;
		float Speed = 30.f, Length = 100.f, Half = 0.f; // its usual pace (cm/s), its size, half its height
		float Seed = 0.f, Rot = 0.f, Warp = 0.f, Rise = 0.f, Dive = 0.f; // its loop: turned, uneven in speed, rising and falling, dipping to its prey
		float JetW = 0.f;          // what swims by a jet: its pulses per second (radians)
		TArray<FVector> Way;       // what rests: its resting places (floor life: the top of what lies there)
		float Cycle = 10.f, Rest = 0.5f, Lift = 0.f, Flee = 0.f; // seconds from one rest to the next, the part of it spent resting, how high it lifts off, how near the diver may come
		int32 Lead = INDEX_NONE;   // one of a school: the one whose path it follows
		FVector Slot = FVector::ZeroVector; // its place in the school: seconds behind, cm to the side, cm above
		bool bFixed = false;       // its loop is laid by hand (the trench)
		float Yaw = 0.f;           // the last heading it had while moving
		float Beat = 0.f, Beats = 0.f; // the phase of its body's swimming motion, and its rate
		TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Skins;
	};
	TArray<FAmbient> Ambient;
	TWeakObjectPtr<UPointLightComponent> ShoalLight, JellyLight;
	FVector LampAt = FVector::ZeroVector, LensAt = FVector::ZeroVector;

	// ---- the expedition: days, air, gear, the channel (DCCareer.cpp)
	TArray<FDCSubject> Subjects; // DCSub first, then the species, then the legend
	int32 Legend = INDEX_NONE;
	int32 Day = 1;
	int32 Days = 6;
	int32 Money = 0;
	int64 TotalViews = 0;
	int32 Subscribers = 0;
	int32 Gear[DCGear::Num] = { 0, 0, 0, 0, 0, 0 };
	TMap<int32, int32> TimesUploaded; // subject -> videos it was in
	float O2 = 180.f, O2Max = 180.f;
	bool bOverDepth = false;   // deeper than the hull is rated for
	bool bDocked = false;      // at the station: ENTER surfaces
	bool bBlackout = false;    // the air ran out: the tape is lost
	float BlackT = -1.f;       // seconds into the blackout fade; < 0 = none
	float HurtTime = 0.f;      // the red flash of a bite
	float HitCooldown = 0.f;
	FString Alert;             // one warning line (air, depth, tape)
	float AlertTime = 0.f;
	bool bLostTape = false;    // this day's upload: nothing came back
	TArray<FDCUploadLine> UploadLines;
	int64 VideoViews = 0;
	int32 Earned = 0;
	int32 NewSubs = 0;
	TArray<FString> Comments;
	FString VideoTitle;
	int32 ShopSel = 0;
	FString ShopNote;
	float ShopNoteTime = 0.f;
	int32 Trending = INDEX_NONE; // the subject viewers ask for today: double views
	// the base: a flooded habitat over the station. The day starts and ends here.
	bool bInBase = true;
	bool bLeftBase = false;     // went out on today's dive
	int32 Spot = INDEX_NONE;    // the base's control the diver looks at: 0 desk, 1 locker, 2 bunk, 3 board, 4 news
	int32 Headline = INDEX_NONE; // index in Takes: the clip on the thumbnail
	bool bUploadedToday = false;
	TArray<FDCTake> Archive;    // the best clip of every proof found, kept for the Director's Cut
	int32 Ending = 0;           // 0 none yet, 1 the truth, 2 doubt, 3 the official story
	int32 EvidenceCount() const;
	int32 EvidenceTotal() const;
	bool bFinalCut = false;     // the film now playing is the Director's Cut
	float DangerNear = 0.f;     // seconds a biter or the legend was close during this take
	bool bBoardOpen = false;    // the board on the wall: the proofs found and missing
	bool bNewsOpen = false;     // the newspaper on the wall: the missing divers, the shadow in the photo
	bool bBagOpen = false;      // the bag (TAB): the targets, ticked once filmed, with a still of each
	bool bEverBag = false;
	bool bBagPressed = false;
	bool PageOpen() const { return bBoardOpen || bNewsOpen || bBagOpen; }
	// the targets: first an easy animal near the base (the tutorial's), then the proofs, nearest first
	TArray<int32> Missions;
	int32 FirstTarget = INDEX_NONE;
	TSet<int32> Uploaded;       // every subject ever uploaded
	void BuildMissions();
	bool IsUploaded(int32 Sub) const;
	bool IsFound(int32 Sub) const;   // on today's tape, or uploaded
	int32 Mission() const;           // the target to go for now; INDEX_NONE when all are found
	bool MissionWhere(int32 Sub, FVector& Out) const; // the nearest one of it (its home when it is far off)
	float SavedTime = 0.f;           // a clip was just saved: its still flies into the bag
	int32 SavedSub = INDEX_NONE;
	TMap<int32, TStrongObjectPtr<UTextureRenderTarget2D>> Photos; // a still of each subject's latest clip
	TWeakObjectPtr<ASceneCapture2D> PhotoCam;
	void TakePhoto(int32 Sub);
	float SleepAsk = 0.f;       // the bunk asks once when there is footage not uploaded
	void UseSpot(int32 S);
	void Sleep();
	// the legend of the abyss: it hunts what makes light or noise in its water
	TWeakObjectPtr<AActor> Hunter;
	FVector HuntPos = FVector::ZeroVector;
	FVector HuntVel = FVector::ZeroVector;
	float HuntYaw = 0.f;
	float HuntAlert = 0.f;     // 0 calm .. 1 chasing
	bool bHunting = false;

	// what is big enough to be in the way: the diver cannot swim through it (small life is swum through as before).
	// Each is a rounded body (an ellipsoid) fitted inside the model's own box, moving with the creature.
	struct FSolid { TWeakObjectPtr<AActor> Actor; FVector Centre = FVector::ZeroVector; FVector Semi = FVector(50.0); };
	TArray<FSolid> Solids;
	void AddSolid(AActor* A, float MaxSemi = 0.f); // MaxSemi > 0: never bigger than this (the legend must still reach the diver to bite)
	/** P inside one of the solids (a ball of radius R): the way out to its skin and the direction, true when there is one. */
	bool Blocked(const FVector& P, float R, FVector& OutPush, FVector& OutNormal) const;
	bool bSolidsOff = false; // test scripts (solid3 0): the sea as it was, everything swum through
	mutable float SolidNear = 99.f; // test scripts: how near the nearest big animal's skin is, 1 = touching, less = inside it

	void SetupSubjects();
	void StartDay();
	void TickDive(float Dt);
	void Surface(bool bLost);
	void MakeUpload();
	void TickHunter(float Dt, bool bLive);
	int32 TapeCap() const;
	float HullLimit() const;   // metres below the first site the hull can go
	int32 GearCost(int32 G) const; // INDEX_NONE when fully upgraded
	int32 GearLevels(int32 G) const;
	const TCHAR* GearName(int32 G) const;
	FString GearLine(int32 G) const;
	void Buy(int32 G);
	int32 ClipsKept() const;
	FString SubjectName(int32 S) const { return Subjects.IsValidIndex(S) ? Subjects[S].Name : FString(TEXT("?")); }
	float SpeedMul() const;

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
