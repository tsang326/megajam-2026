// Match-cut movement prototype (brief 01). Greybox only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MatchCutGameMode.generated.h"

class ACameraActor;
class ACharacter;
class AStaticMeshActor;
class ASceneCapture2D;
class UTextureRenderTarget2D;
class UStaticMesh;
class UMaterialInterface;

DECLARE_LOG_CATEGORY_EXTERN(LogMatchCut, Log, All);

/** One greybox box inside a shot. Coordinates are shot-local. */
struct FMatchCutBox
{
	TWeakObjectPtr<AStaticMeshActor> Actor;
	FVector Center = FVector::ZeroVector;
	FVector Size = FVector(100.0);
	bool bCuttable = false;
	bool bGoal = false;
	bool bMoves = false;
	FVector MoveTo = FVector::ZeroVector;
	float Period = 4.f;
	FVector MeshOffset = FVector::ZeroVector;
};

/** One shot: a fixed camera looking at a small greybox location. */
struct FMatchCutShot
{
	FString Name;
	FString Hint;
	FVector Origin = FVector::ZeroVector;
	FVector CamPos = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	FVector Start = FVector::ZeroVector; // feet position, world space
	TWeakObjectPtr<ACameraActor> Camera;
	TArray<FMatchCutBox> Boxes;
};

UENUM()
enum class EMatchCutState : uint8
{
	Playing,
	NG,
	Wrap
};

/**
 * Runs the whole prototype: loads Content/MatchCut/shots.json, spawns the shots,
 * owns the cut rule, NG/retake flow, the ghost capture and a small script runner
 * used for offscreen verification.
 */
UCLASS()
class AMatchCutGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMatchCutGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The cut. Returns true when the take succeeded. */
	bool TryCut();

	/** Back to the previous shot's start (or this shot's start in shot 0). */
	void Retake();

	/** Restart from shot 0. */
	void RestartAll();

	bool AcceptsPlayerInput() const { return State == EMatchCutState::Playing && !bScriptActive; }
	bool IsWrapped() const { return State == EMatchCutState::Wrap; }

	/** Extra movement input injected by the test script (x = right, y = forward). */
	FVector2D GetScriptMove() const { return ScriptMoveLeft > 0.f ? ScriptMove : FVector2D::ZeroVector; }

	// HUD accessors
	EMatchCutState GetState() const { return State; }
	int32 GetShotIndex() const { return Cur; }
	int32 GetNumShots() const { return Shots.Num(); }
	int32 GetTake() const { return Take; }
	const FString& GetShotName() const;
	const FString& GetShotHint() const;
	const FString& GetNGReason() const { return NGReason; }
	float GetFlash() const { return CutFlash; }
	UTextureRenderTarget2D* GetGhostTarget() const { return (Cur + 1 < Shots.Num()) ? GhostTarget : nullptr; }
	float GetGhostOpacity() const { return GhostOpacity; }

	/** Screen position in [-1,1]^2 of a world point seen by a shot camera (16:9, independent of the window). */
	bool ProjectToShot(int32 ShotIndex, const FVector& World, FVector2D& OutNdc) const;
	void DeprojectFromShot(int32 ShotIndex, const FVector2D& Ndc, FVector& OutStart, FVector& OutDir) const;

protected:
	UPROPERTY()
	TSubclassOf<ACharacter> CharacterClass;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> FlatMaterial;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> GhostTarget;

	UPROPERTY()
	TObjectPtr<ASceneCapture2D> GhostCapture;

	UPROPERTY()
	TObjectPtr<ACharacter> Hero;

private:
	bool LoadLayout();
	void SpawnShot(FMatchCutShot& Shot);
	void SpawnLights();
	void SetShot(int32 Index, bool bRespawn);
	void RespawnAtStart();
	void FailTake(const FString& Reason);
	void UpdateMovers();
	FVector GetFeet() const;
	void LogProjectionCheck(const TCHAR* Label) const;
	void ReportFrameTime();

	// script runner
	void LoadScript();
	void TickScript(float DeltaSeconds);
	bool RunScriptLine(const FString& Line);

	TArray<FMatchCutShot> Shots;
	int32 Cur = 0;
	int32 Take = 1;
	EMatchCutState State = EMatchCutState::Playing;
	float StateTime = 0.f;
	FString NGReason;
	float CutFlash = 0.f;
	float ShotClock = 0.f;
	bool bClockFrozen = false;
	float Fov = 60.f;
	float GhostOpacity = 0.35f;
	float ExposureEV = 2.5f;

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
	bool bScriptWalking = false;
	FVector ScriptWalkTarget = FVector::ZeroVector;
	float ScriptWalkTime = 0.f;
	float ScriptClockTarget = -1.f;
	float ScriptClockPeriod = 1.f;
};
