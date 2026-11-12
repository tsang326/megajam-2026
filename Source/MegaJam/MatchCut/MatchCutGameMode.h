// Deep Cut - underwater vertical slice (brief 02). Built on the brief-01 match-cut prototype.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MatchCutGameMode.generated.h"

class ACameraActor;
class AStaticMeshActor;
class ASceneCapture2D;
class ADirectionalLight;
class AExponentialHeightFog;
class UTextureRenderTarget2D;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USpotLightComponent;
class UPointLightComponent;
class USceneComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogMatchCut, Log, All);

/** A rock: a box facing the shot camera that spans the whole swim slab. The rectangle is in screen units at the shot's reference depth. */
struct FDCRock
{
	FVector4 Rect = FVector4(0, 0, 0, 0); // x0, y0, x1, y1 in [-1,1] screen units
	bool bMoves = false;
	bool bPlate = false;
	FVector2D MoveTo = FVector2D::ZeroVector; // screen units
	float Period = 6.f;
	TWeakObjectPtr<AActor> Actor;
	FVector BaseLoc = FVector::ZeroVector;
	FVector MoveWorld = FVector::ZeroVector;
};

/** One shot: a fixed survey camera and a thin slab of water facing it. */
struct FDCShot
{
	FString Name;
	FString Hint;
	FVector Origin = FVector::ZeroVector;
	FVector CamPos = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	double DepthMin = 800.0;
	double DepthMax = 1000.0;
	double Ref = 900.0;
	FVector4 Swim = FVector4(-0.95, -0.68, 0.95, 0.68); // where the robot may be, screen units
	FVector2D Start = FVector2D::ZeroVector;
	bool bDark = false;
	bool bHasPart = false;
	FVector2D PartNdc = FVector2D::ZeroVector;
	bool bHasCrew = false;
	FVector2D CrewNdc = FVector2D::ZeroVector;
	TWeakObjectPtr<ACameraActor> Camera;
	TArray<FDCRock> Rocks;
};

/** One of the three look-dev looks. All numbers come from the JSON so they can be tuned without a rebuild. */
struct FDCLook
{
	float FogDensity = 0.3f;
	FLinearColor FogColor = FLinearColor(0.02f, 0.15f, 0.14f);
	bool bVolumetric = false;
	float VolScatter = 1.f;
	float VolExtinction = 1.f;
	float VolEmissive = 1.f;
	float KeyIntensity = 6.f;
	FLinearColor KeyColor = FLinearColor(0.7f, 1.f, 0.95f);
	float FillIntensity = 2.f;
	FLinearColor FillColor = FLinearColor(0.4f, 0.7f, 0.8f);
	float LampIntensity = 30000.f;
	float GlowIntensity = 4000.f;
	bool bToon = false;
	bool bShafts = false;
	float Vignette = 0.4f;
	float Grain = 0.f;
	float DarkFactor = 0.04f;
	FLinearColor Backdrop = FLinearColor(0.01f, 0.05f, 0.06f);
};

struct FDCPaint
{
	TWeakObjectPtr<UStaticMeshComponent> Comp;
	FLinearColor Color = FLinearColor::White;
	FLinearColor Emissive = FLinearColor::Black;
	TWeakObjectPtr<UMaterialInstanceDynamic> MID;
};

/** A robot made of engine shapes. */
struct FDCRobot
{
	TWeakObjectPtr<AActor> Actor;
	int32 EyePaint = INDEX_NONE;
};

struct FDCCutEval
{
	bool bOk = false;
	const TCHAR* Reason = TEXT("");
	FVector Target = FVector::ZeroVector;
	double Depth = 0.0;
};

/** A stretch of the film timeline: a shot of the official film or a removed scene. */
struct FDCReelSeg
{
	float T0 = 0.f;
	float T1 = 0.f;
	int32 Gap = INDEX_NONE; // gap index, or INDEX_NONE for a film shot
	int32 Set = 0;          // which film set the camera looks at
	const TCHAR* Caption = TEXT("");
};

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

/**
 * Runs the slice: opening film, six survey-camera shots loaded from Content/Data/deepcut.json,
 * the underwater cut rule with its preview, the lamp part, restore-by-cut, one rescue, the end card,
 * three looks, and the script runner used for offscreen verification.
 */
UCLASS()
class AMatchCutGameMode : public AGameModeBase
{
	GENERATED_BODY()

	friend class AMatchCutHUD;

public:
	AMatchCutGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// input from the player controller
	void SetMoveInput(const FVector2D& In) { MoveInput = In; }
	void PressCut();
	void PressLamp();
	void PressRetake();
	void PressAnyKey();
	bool IsScriptActive() const { return bScriptActive; }

	/** The cut. Returns true when the take succeeded. */
	bool TryCut();

	static const int32 MaskW = 76;
	static const int32 MaskH = 28;
	static const int32 NumGaps = 3;

protected:
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
	TObjectPtr<UTextureRenderTarget2D> ThumbTarget;
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

private:
	// setup
	bool LoadLayout();
	void SpawnWorld();
	void SpawnShot(FDCShot& Shot, int32 Index);
	void SpawnFilmSets();
	ACameraActor* SpawnCamera(const FVector& Pos, const FRotator& Rot);
	AActor* SpawnBox(const FVector& Center, const FRotator& Rot, const FVector& Size, const FLinearColor& Color, bool bCollide, bool bHideFromGhost);
	FDCRobot SpawnRobot(const FVector& Loc, const FLinearColor& Body, const FLinearColor& Eye, bool bBattered);
	UStaticMeshComponent* AddPart(AActor* Owner, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor& Color, const FLinearColor& Emissive);
	int32 AddPaint(UStaticMeshComponent* Comp, const FLinearColor& Color, const FLinearColor& Emissive);
	void SetPaintEmissive(int32 Paint, const FLinearColor& Emissive);
	void ApplyLook(const FString& Name);
	void ApplyLighting();

	// shot maths (screen units are [-1,1]^2 of a fixed 16:9 frame; the window never enters)
	FVector CamPoint(const FVector& Pos, const FRotator& Rot, const FVector2D& Ndc, double Depth) const;
	FVector ShotPoint(int32 Shot, const FVector2D& Ndc, double Depth) const { return CamPoint(Shots[Shot].CamPos, Shots[Shot].CamRot, Ndc, Depth); }
	void ProjectToShot(int32 Shot, const FVector& World, FVector2D& OutNdc, double& OutDepth) const;
	FBox2D RockRect(const FDCShot& Shot, const FDCRock& Rock) const;
	bool Blocked(int32 ShotIndex, const FVector2D& Ndc, double Depth) const;
	FDCCutEval EvalCut(const FVector2D& Ndc, double Depth, bool bApplyLamp) const;
	bool InRestore() const { return bHasPart && !bGapFilled[0] && Cur == PartShot; }
	bool NextIsDark() const { return Shots.IsValidIndex(Cur + 1) && Shots[Cur + 1].bDark; }

	// flow
	void SetShot(int32 Index, bool bRespawn);
	void PointGhost();
	void RespawnAtStart();
	void FailTake(const TCHAR* Reason);
	void StartFilm();
	void EndFilm();
	void StartFlashback();
	void EndFlashback();
	void RestartAll();
	void ShowBanner(const FString& Line1, const FString& Line2, float Seconds);
	void SetView(ACameraActor* Cam);

	void TickFilm(float Dt);
	void TickDrop(float Dt);
	void TickPlaying(float Dt);
	void TickFlashback(float Dt);
	void MoveHero(const FVector2D& Delta);
	void UpdateHeroActor(float Dt);
	void UpdateMovers();
	void UpdateMask();
	void PoseFilm(int32 Set, float T);
	void PoseFlashback(float T);
	void PlaceRobot(const FDCRobot& Robot, const FVector& Loc, const FVector2D& Heading, const FRotator& CamRot, float Roll = 0.f);
	void ReportFrameTime();

	// script runner
	void LoadScript();
	void TickScript(float DeltaSeconds);
	bool RunScriptLine(const FString& Line);
	const TCHAR* StateName() const;

	// ---- data
	TArray<FDCShot> Shots;
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
	double StartDepth = 900.0;
	int32 CrewTotal = 7;
	int32 PartShot = INDEX_NONE;
	int32 CrewShot = INDEX_NONE;
	FVector2D RestoreNdc = FVector2D(-0.15, -0.1);
	float RestoreTol = 0.11f;
	float LampRadius = 0.42f; // reveal circle, in frame-height units

	// ---- state
	EDCState State = EDCState::Film;
	float StateTime = 0.f;
	int32 Cur = 0;
	int32 Take = 1;
	const TCHAR* TakeReason = TEXT("");
	float CutFlash = 0.f;
	float ShotClock = 0.f;
	float PlayClock = 0.f; // running timecode
	bool bClockFrozen = false;
	bool bFirstCutDone = false;
	float SoftFailTime = 0.f;
	bool bHasPart = false;
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

	// hero
	FDCRobot Hero;
	FDCRobot Crew;
	FDCRobot PartActor;
	FVector HeroPos = FVector::ZeroVector;
	double HeroDepth = 900.0;
	FVector2D Vel2D = FVector2D::ZeroVector;
	FVector2D Heading = FVector2D(1.0, 0.0);
	FVector2D MoveInput = FVector2D::ZeroVector;
	FVector CrewPos = FVector::ZeroVector;
	FDCCutEval CurEval;
	FVector2D HeroNdc = FVector2D::ZeroVector;
	TWeakObjectPtr<USpotLightComponent> LampSpot;
	TWeakObjectPtr<UPointLightComponent> LampGlow;

	// preview mask of the next shot
	TArray<uint8> Mask;
	float MaskTimer = 0.f;
	bool bMaskValid = false;

	// ghost
	int32 GhostMode = 1; // 0 off, 1 flat base-colour capture, 2 brief-01 style lit capture (with fog)
	float GhostTimer = 0.f;
	TArray<TObjectPtr<AActor>> GhostHidden;

	// paints, looks
	TArray<FDCPaint> Paints;
	TArray<TWeakObjectPtr<AActor>> ShaftSlats;
	TArray<TWeakObjectPtr<ACameraActor>> AllCameras;

	// film and flashback
	TArray<FDCReelSeg> Reel;
	float FilmLength = 0.f;
	float FilmTime = 0.f;
	FVector FilmOrigin[4];
	TWeakObjectPtr<ACameraActor> FilmCam[4];
	TArray<FDCRobot> FilmRobots;
	TWeakObjectPtr<AActor> FilmCable;
	FVector FlashOrigin = FVector::ZeroVector;
	TWeakObjectPtr<ACameraActor> FlashCam;
	FDCRobot FlashUnit7;
	TArray<FDCRobot> FlashCrew;
	TWeakObjectPtr<USpotLightComponent> FlashLamp;
	float FlashLength = 12.f;
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
	FVector2D ScriptMove = FVector2D::ZeroVector;
	float ScriptMoveLeft = 0.f;
	FString ScriptOutDir;
	bool bScriptSwimming = false;
	bool bScriptHold = false; // after swimto: keep station on the target until the next cut or move
	FVector2D ScriptSwimTarget = FVector2D::ZeroVector;
	float ScriptSwimTime = 0.f;
	float ScriptClockTarget = -1.f;
	float ScriptClockPeriod = 1.f;
	FString ScriptWaitState;
};
