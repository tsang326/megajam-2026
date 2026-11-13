// Deep Cut - stage 1 (brief 03). One class, split over several files so each concern can change alone:
//   MatchCutGameMode.cpp  state machine, tick, input, shot changes, retake history
//   DCLayout.cpp          reads Content/Data/deepcut.json and film.json
//   DCWorld.cpp           spawns shots, robots, props, lights; looks
//   DCCutRule.cpp         shot maths, collision, movement, the cut and what comes through it
//   DCPuzzles.cpp         gates, valves, props, sockets, currents, part, rescue
//   DCFilm.cpp            opening film and flashback, played from key poses
//   MatchCutScript.cpp    test-script runner
// Sound is its own class (DCAudio), drawing is the HUD (MatchCutHUD).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DeepCutTypes.h"
#include "MatchCutGameMode.generated.h"

class AStaticMeshActor;
class ASceneCapture2D;
class ADirectionalLight;
class AExponentialHeightFog;
class UTextureRenderTarget2D;
class UStaticMesh;
class UMaterialInterface;
class USpotLightComponent;
class UPointLightComponent;
class UDCAudio;
class FJsonObject;

DECLARE_LOG_CATEGORY_EXTERN(LogMatchCut, Log, All);

UENUM()
enum class EDCState : uint8
{
	Film,
	Drop,
	Playing,
	TakeCard,
	Flashback,
	EndCard
};

UCLASS()
class AMatchCutGameMode : public AGameModeBase
{
	GENERATED_BODY()

	friend class AMatchCutHUD;

public:
	AMatchCutGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// input from the player controller. In.X = right, In.Y = up, In.Z = away from the camera
	void SetMoveInput(const FVector& In) { MoveInput = In; }
	void PressCut();
	void PressLamp();
	void PressUse();
	void PressRetake();
	void PressAnyKey();
	/** Mouse wheel: one step farther (+) or closer (-). */
	void AddDepthStep(float Steps);
	bool IsScriptActive() const { return bScriptActive; }

	/** The cut. Returns true when the take succeeded. */
	bool TryCut();

	static const int32 NumGaps = 3;

protected:
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> ArtMeshes;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> LitMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> ToonMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> UnlitMaterial;
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> GhostTarget;
	UPROPERTY()
	TArray<TObjectPtr<UTextureRenderTarget2D>> ThumbTargets; // one per hole in the reel
	UPROPERTY()
	TObjectPtr<ASceneCapture2D> GhostCapture;
	UPROPERTY()
	TObjectPtr<ASceneCapture2D> ThumbCapture;
	UPROPERTY()
	TObjectPtr<ADirectionalLight> KeyLight;
	UPROPERTY()
	TObjectPtr<ADirectionalLight> FillLight;
	UPROPERTY()
	TObjectPtr<AExponentialHeightFog> Fog;
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BackdropMIDs;
	UPROPERTY()
	TObjectPtr<UDCAudio> Audio;
	UPROPERTY()
	TArray<TObjectPtr<AActor>> GhostHidden;

private:
	// ---- DCLayout.cpp
	bool LoadLayout();
	bool LoadFilm();
	void ReadSolids(const TSharedPtr<FJsonObject>& Obj, TArray<FDCSolid>& Out) const;
	int32 FindShot(const FString& Id) const;

	// ---- DCWorld.cpp
	void SpawnWorld();
	void SpawnShot(FDCShot& Shot, int32 Index);
	void SpawnSolid(FDCSolid& Solid, const FVector& CamPos, const FRotator& CamRot, bool bInGhost);
	void SpawnFilmSets();
	ACameraActor* SpawnCamera(const FVector& Pos, const FRotator& Rot);
	AActor* SpawnBox(const FVector& Center, const FRotator& Rot, const FVector& Size, const FLinearColor& Color, bool bHideFromGhost);
	AActor* SpawnGroup(const FVector& Loc);
	AActor* SpawnGentle(const FVector& Loc);
	FDCRobot SpawnRobot(const FVector& Loc, const FLinearColor& Body, const FLinearColor& Eye, bool bBattered, const TCHAR* ArtId = TEXT(""));
	// ---- DCArt.cpp: imported models, with the code-built shapes as the fallback
	void LoadArt();
	/** Adds the model for Id fitted into a box given in the owner's axes. Null when there is no model for Id. */
	UStaticMeshComponent* AddArt(AActor* Owner, const FString& Id, const FVector& Centre, const FVector& Size, uint32 Hash = 0, bool bFill = false);
	UStaticMeshComponent* AddPart(AActor* Owner, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor& Color, const FLinearColor& Emissive);
	int32 AddPaint(UStaticMeshComponent* Comp, const FLinearColor& Color, const FLinearColor& Emissive);
	void SetPaintEmissive(int32 Paint, const FLinearColor& Emissive);
	void ApplyLook(const FString& Name);
	void ApplyLighting();
	void PlaceRobot(const FDCRobot& Robot, const FVector& Loc, const FVector2D& Heading, const FRotator& CamRot, float Roll = 0.f);

	// ---- DCCutRule.cpp
	FVector ShotToWorld(int32 Shot, const FVector& S) const;
	FVector WorldToShot(int32 Shot, const FVector& W) const;
	/** Screen position of a shot-space point. bEntry: with the camera where it stands when the shot starts. */
	FVector2D ViewNdc(int32 Shot, const FVector& S, bool bEntry) const;
	FVector NdcToShot(int32 Shot, const FVector2D& Ndc, double Depth, bool bEntry) const;
	bool Blocked(int32 Shot, const FVector& S, double Radius) const;
	bool PushOut(int32 Shot, FVector& S, double Radius, FVector* VelS) const;
	FDCCutEval EvalCut() const;
	bool MarkVisible(const FDCExit& Exit, const FVector2D& MarkNdc) const;
	int32 ActiveExit() const { return CurEval.Exit; }
	/** Where a prop of the current shot would be after a cut through Exit: 0 stays, 1 comes along, 2 comes along and lands in a socket. */
	int32 PropAfterCut(const FDCProp& Prop, const FDCExit& Exit, FVector* OutS = nullptr, int32* OutSocket = nullptr) const;
	void MoveHero(const FVector& Delta);
	void TickMovement(float Dt);
	void UpdateRail(float Dt, bool bSnap);
	bool InRestore() const { const FDCShot& S = Shots[Cur]; return S.bHasPart && bFeature[S.PartFeature] && !bGapFilled[S.PartGap]; }
	bool ShotIsLit(int32 Shot) const { return !Shots[Shot].bDark; }

	// ---- DCPuzzles.cpp
	void TickPuzzles(float Dt);
	void UpdateGate(int32 Shot, bool bQuiet);
	void RevealMarks(int32 Shot);
	void FillSocket(int32 Socket, int32 Prop, bool bQuiet);
	void CheckSockets(int32 Shot);
	void UpdateProps(float Dt);
	void UpdateCurrents(float Dt);
	int32 PropInReach() const;
	/** True when the prop, seen from the current shot, sits on the socket's place in the frame (a socket of this or the next shot). */
	bool PropFitsSocket(const FDCProp& Prop, int32 Socket) const;
	void NudgeProps();
	void UpdateGentle(float Dt);
	bool SwitchInReach() const;
	FString UsePrompt(FVector& OutS) const;
	FVector PropHalf(const FDCProp& Prop) const;
	/** Height of the ground under a shot-space point (the highest solid below it). */
	double FloorUnder(int32 Shot, const FVector& S) const;
	// ---- DCTeach.cpp: controls are taught when they are first needed; a stronger hint after 20 s without progress
	void Teach(int32 Control);
	void TeachUpTo(int32 ShotIndex);
	void TickTeach(float Dt);
	void Progress() { StuckTime = 0.f; }
	FString ControlsLine() const;
	/** The thing a stuck player should use: its place on screen, and where it has to go (if it has to go somewhere). */
	bool StuckTarget(FVector2D& OutNdc, FVector2D& OutToNdc, bool& bHasTo) const;
	const TCHAR* FailHow(const TCHAR* Reason) const;
	void TakeSnap(FDCSnap& Out) const;
	void RestoreSnap(const FDCSnap& Snap);

	// ---- MatchCutGameMode.cpp
	void EnterShot(int32 Index, const FVector& HeroAtS, bool bPushHistory);
	void PointGhost();
	void FailTake(const TCHAR* Reason);
	void RestartAll();
	void ShowBanner(const FString& Line1, const FString& Line2, float Seconds);
	void SetView(ACameraActor* Cam);
	void TickDrop(float Dt);
	void TickPlaying(float Dt);
	void UpdateHeroActor(float Dt);
	void UpdateMusic();
	void ReportFrameTime();
	const TCHAR* StateName() const;

	// ---- DCFilm.cpp
	void StartFilm();
	void EndFilm();
	void StartFlashback();
	void EndFlashback();
	void TickFilm(float Dt);
	void TickFlashback(float Dt);
	void PoseClip(const FString& ClipName, float T);
	FDCRobot* FilmActor(const FString& Name);

	// ---- DCStage2.cpp
	AActor* SpawnPredator(const FVector& Loc, bool bOnFilm = false);
	void UpdatePredator(float Dt);
	void ResetPredator();
	void PredatorCatch();
	int32 CrewCount() const;
	int32 ScenesCount() const;
	bool HasNextStage() const;
	void ContinueStage();
	void GrantEarlierStages(int32 ShotIndex);
	void StartClip(const FString& ClipName, bool bOfficial, int32 Gap);
	ACameraActor* ClipCamera(const FString& ClipName) const;
	const FDCFilmSet* ClipSet(const FString& ClipName) const;

	// ---- MatchCutScript.cpp
	void LoadScript();
	void TickScript(float DeltaSeconds);
	bool RunScriptLine(const FString& Line);

	// ---- data
	TArray<FDCShot> Shots;
	TArray<FDCProp> Props;
	TArray<FDCSocket> Sockets;
	TMap<FString, FDCLook> Looks;
	FDCLook Look;
	FString LookName = TEXT("A");
	float Fov = 60.f;
	double TanH = 0.577;
	double TanV = 0.325;
	float GhostOpacity = 0.45f;
	FLinearColor GhostTint = FLinearColor(1.f, 0.75f, 0.45f);
	float ExposureEV = 2.5f;
	float HeroRadius = 36.f;
	int32 CrewTotal = 7;
	int32 PartShot = INDEX_NONE;
	int32 CrewShot = INDEX_NONE;
	FVector2D RestoreNdc = FVector2D(-0.2, -0.15);
	float RestoreTol = 0.13f;
	float LampRadius = 0.40f;   // reveal circle, in frame-height units
	float MarkRadius = 0.085f;  // capture radius of a tape mark, in frame-height units
	float MarkDepthTol = 160.f; // cm
	float CarryRadius = 340.f;  // cm
	float SocketRadius = 0.075f; // frame-height units: how close a prop must land to a socket
	float ReachRadius = 120.f;  // cm: how close to press E

	// ---- state
	EDCState State = EDCState::Film;
	float StateTime = 0.f;
	int32 Cur = 0;
	int32 Take = 1;
	const TCHAR* TakeReason = TEXT("");
	float CutFlash = 0.f;
	float PlayClock = 0.f;
	bool bFirstCutDone = false;
	float SoftFailTime = 0.f;
	bool bFeature[3] = { false, false, false }; // HEADLAMP, GRIPPER, MULTI-CAM: built again when its part is found
	bool bLampOn = false;
	bool bCrewRescued = false;
	bool bGapFilled[3] = { false, false, false };
	bool bIntroSeen = false;
	bool bFilmSkippable = false;
	float EndTimer = -1.f;
	FString Banner1;
	FString Banner2;
	float BannerTime = 0.f;
	float PartPulse = 0.f;
	float RevealTime = 0.f; // counts down after marks appear (drives the focus brackets)
	float ThrustTimer = 0.f;
	bool bWasMoving = false;
	bool bDepthUsed = false;
	TArray<FDCSnap> History;
	TArray<int32> Visited;

	TMap<FString, TArray<FDCArtRef>> Art;
	TSet<FString> bArtParamsLogged;
	TWeakObjectPtr<UPointLightComponent> HeroRim;
	float HeroFace = 0.f;  // 0 = faces right, 180 = faces left; turns through "faces the camera"
	float HeroLean = 0.f;
	bool bPredStayed = false;
	float HeroRimPower = 2500.f;
	float HeroBank = 0.f;

	// teaching (brief 05)
	bool bTaught[DCTeach::Num] = { false, false, false, false, false, false, false };
	int32 PromptId = INDEX_NONE;
	float PromptTime = 0.f;
	float StuckTime = 0.f;     // seconds in this shot without progress
	bool bStuckShown = false;
	float MovedFor = 0.f;
	float DepthQueue = 0.f;    // cm still to travel from mouse-wheel steps
	int32 PushProp = INDEX_NONE;
	float PushCue = 0.f;
	float PushDir = 1.f;
	double HeroFloorY = -215.0;
	const TCHAR* TakeHow = TEXT("");

	// hero
	FDCRobot Hero;
	FDCRobot Crew;
	FDCRobot PartActor;
	FVector HeroS = FVector(0.0, 0.0, 900.0); // shot space of the current shot
	FVector VelS = FVector::ZeroVector;
	FVector HeroPos = FVector::ZeroVector;    // world
	FVector2D Heading = FVector2D(1.0, 0.0);
	FVector MoveInput = FVector::ZeroVector;
	FVector CrewPos = FVector::ZeroVector;
	FDCCutEval CurEval;
	FVector2D HeroNdc = FVector2D::ZeroVector;
	int32 GhostExit = 0;
	TWeakObjectPtr<USpotLightComponent> LampSpot;
	TWeakObjectPtr<UPointLightComponent> LampGlow;

	// the gentle creature: big, slow, warm. In stage 1 only its glow is seen, far off in the dark.
	TWeakObjectPtr<AActor> GentleActor;
	TWeakObjectPtr<UPointLightComponent> GentleLight;
	int32 GentleShot = INDEX_NONE;
	FVector GentleS = FVector::ZeroVector;
	float GentleLightPower = 9000.f;
	float GentleLightRadius = 1300.f;
	float GentleBack = 650.f;

	// the predator (stage 2): thin, dark, fast. Only in a shot whose marks are showing; the only way out is to cut.
	TWeakObjectPtr<AActor> PredActor;
	int32 PredPhase = 0; // 0 not here, 1 warning at the edge of the frame, 2 coming
	float PredClock = 0.f;
	float PredChase = 0.f;
	FVector PredS = FVector::ZeroVector;
	bool bCaught = false;
	float ScriptPredWait = 0.f;
	TMap<int32, FDCRobot> PropRobots; // crew robots that cannot swim: props that look like a robot

	// ghost
	int32 GhostMode = 1; // 0 off, 1 flat base-colour capture
	float GhostTimer = 0.f;

	// paints, looks
	TArray<FDCPaint> Paints;
	TArray<TWeakObjectPtr<AActor>> ShaftSlats;
	// the water (DCVisual.cpp)
	void SpawnWater();
	void TickWater(float Dt);
	TArray<FVector> SlatBase;
	TWeakObjectPtr<AActor> SnowActor;
	TWeakObjectPtr<AActor> BubbleActor;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SnowMID;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BubbleMID;
	float WaterTime = 0.f;
	TArray<TWeakObjectPtr<ACameraActor>> AllCameras;
	float AmbientNow = 0.f;

	// film and flashback
	TMap<FString, FDCClip> Clips;
	TMap<FString, FDCFilmSet> FilmSets;
	TMap<FString, FDCRobot> FilmActors;
	TMap<FString, TArray<FDCSolid>> FilmSetSolids;
	TArray<FDCReelSeg> Reel;
	FString FlashClip = TEXT("removed1");
	float FilmLength = 0.f;
	float FilmTime = 0.f;
	TWeakObjectPtr<ACameraActor> FlashCam;
	TWeakObjectPtr<USpotLightComponent> FlashLamp;
	float FlashLength = 12.f;
	bool bFlashOfficial = false; // the clip plays as Fathom released it: without its cut seconds
	int32 FlashGap = 0;
	float FlashClipT = 0.f;      // time inside the clip (the official cut jumps)
	bool bThumbTaken = false;
	FVector2D Weave = FVector2D::ZeroVector;
	float WeaveTimer = 0.f;

	// frame time
	double FrameAccum = 0.0;
	double FrameMax = 0.0;
	int32 FrameCount = 0;
	float ShotElapsed = 0.f;

	// script
	bool bScriptActive = false;
	TArray<FString> ScriptLines;
	int32 ScriptPc = 0;
	float ScriptWait = 0.f;
	FVector ScriptMove = FVector::ZeroVector;
	float ScriptMoveLeft = 0.f;
	FString ScriptOutDir;
	bool bScriptSwimming = false;
	bool bScriptHold = false;
	FVector ScriptSwimTarget = FVector::ZeroVector; // shot space
	float ScriptSwimTime = 0.f;
	float ScriptOkWait = 0.f;
	FString ScriptWaitState;
};
