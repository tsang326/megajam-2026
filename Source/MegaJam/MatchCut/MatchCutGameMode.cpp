// Match-cut movement prototype (brief 01). Greybox only.

#include "MatchCutGameMode.h"
#include "MatchCutPlayerController.h"
#include "MatchCutHUD.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SceneView.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogMatchCut);

namespace
{
	const FName CuttableTag(TEXT("Cuttable"));
	const FName GoalTag(TEXT("Goal"));
	const double AspectRatio = 16.0 / 9.0;
	const float NGSeconds = 0.9f;
	const double KillDepth = 1500.0;
	const double TraceLength = 12000.0;

	FVector ReadVec(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, const FVector& Default = FVector::ZeroVector)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Obj.IsValid() && Obj->TryGetArrayField(Field, Arr) && Arr && Arr->Num() == 3)
		{
			return FVector((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
		}
		return Default;
	}

	void SetupLook(FPostProcessSettings& PP, float EV)
	{
		// fixed exposure so every shot and the ghost capture look the same; no Lumen (greybox, M1)
		PP.bOverride_AutoExposureMinBrightness = true;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMinBrightness = EV;
		PP.AutoExposureMaxBrightness = EV;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = 0.f;
		PP.bOverride_DynamicGlobalIlluminationMethod = true;
		PP.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
		PP.bOverride_ReflectionMethod = true;
		PP.ReflectionMethod = EReflectionMethod::None;
		PP.bOverride_MotionBlurAmount = true;
		PP.MotionBlurAmount = 0.f;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 0.f;
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = 0.f;
	}
}

AMatchCutGameMode::AMatchCutGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	// the hero is spawned by hand at the first shot's start mark
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AMatchCutPlayerController::StaticClass();
	HUDClass = AMatchCutHUD::StaticClass();

	static ConstructorHelpers::FClassFinder<ACharacter> HeroFinder(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	CharacterClass = HeroFinder.Class;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	CubeMesh = CubeFinder.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	FlatMaterial = MatFinder.Object;
}

const FString& AMatchCutGameMode::GetShotName() const
{
	static const FString Empty;
	return Shots.IsValidIndex(Cur) ? Shots[Cur].Name : Empty;
}

const FString& AMatchCutGameMode::GetShotHint() const
{
	static const FString Empty;
	return Shots.IsValidIndex(Cur) ? Shots[Cur].Hint : Empty;
}

bool AMatchCutGameMode::LoadLayout()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("MatchCut/shots.json");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogMatchCut, Error, TEXT("Cannot read layout %s"), *Path);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogMatchCut, Error, TEXT("Layout %s is not valid JSON"), *Path);
		return false;
	}

	Root->TryGetNumberField(TEXT("fov"), Fov);
	Root->TryGetNumberField(TEXT("ghost"), GhostOpacity);
	Root->TryGetNumberField(TEXT("exposure"), ExposureEV);
	double Spacing = 50000.0;
	Root->TryGetNumberField(TEXT("spacing"), Spacing);

	const TArray<TSharedPtr<FJsonValue>>* ShotArr = nullptr;
	if (!Root->TryGetArrayField(TEXT("shots"), ShotArr) || !ShotArr)
	{
		return false;
	}

	for (int32 i = 0; i < ShotArr->Num(); ++i)
	{
		const TSharedPtr<FJsonObject> S = (*ShotArr)[i]->AsObject();
		if (!S.IsValid())
		{
			continue;
		}
		FMatchCutShot& Shot = Shots.AddDefaulted_GetRef();
		Shot.Origin = FVector(0.0, Spacing * i, 0.0);
		S->TryGetStringField(TEXT("name"), Shot.Name);
		S->TryGetStringField(TEXT("hint"), Shot.Hint);
		Shot.Start = Shot.Origin + ReadVec(S, TEXT("start"));

		const TSharedPtr<FJsonObject>* Cam = nullptr;
		if (S->TryGetObjectField(TEXT("cam"), Cam) && Cam)
		{
			const FVector Pos = ReadVec(*Cam, TEXT("pos"));
			const FVector Look = ReadVec(*Cam, TEXT("look"));
			Shot.CamPos = Shot.Origin + Pos;
			Shot.CamRot = (Look - Pos).Rotation();
		}

		const TArray<TSharedPtr<FJsonValue>>* BoxArr = nullptr;
		if (S->TryGetArrayField(TEXT("boxes"), BoxArr) && BoxArr)
		{
			for (const TSharedPtr<FJsonValue>& BV : *BoxArr)
			{
				const TSharedPtr<FJsonObject> B = BV->AsObject();
				if (!B.IsValid())
				{
					continue;
				}
				FMatchCutBox& Box = Shot.Boxes.AddDefaulted_GetRef();
				Box.Center = Shot.Origin + ReadVec(B, TEXT("c"));
				Box.Size = ReadVec(B, TEXT("s"), FVector(100.0));
				B->TryGetBoolField(TEXT("cut"), Box.bCuttable);
				B->TryGetBoolField(TEXT("goal"), Box.bGoal);
				const TSharedPtr<FJsonObject>* Move = nullptr;
				if (B->TryGetObjectField(TEXT("move"), Move) && Move)
				{
					Box.bMoves = true;
					Box.MoveTo = ReadVec(*Move, TEXT("to"));
					(*Move)->TryGetNumberField(TEXT("period"), Box.Period);
				}
			}
		}
	}

	UE_LOG(LogMatchCut, Display, TEXT("Layout loaded: %d shots, fov %.1f"), Shots.Num(), Fov);
	return Shots.Num() > 0;
}

void AMatchCutGameMode::SpawnShot(FMatchCutShot& Shot)
{
	UWorld* World = GetWorld();

	ACameraActor* Cam = World->SpawnActor<ACameraActor>(Shot.CamPos, Shot.CamRot);
	UCameraComponent* CC = Cam->GetCameraComponent();
	CC->SetFieldOfView(Fov);
	CC->SetAspectRatio(AspectRatio);
	CC->SetConstraintAspectRatio(true); // 16:9 letterbox at any window size
	SetupLook(CC->PostProcessSettings, ExposureEV);
	CC->PostProcessBlendWeight = 1.f;
	Shot.Camera = Cam;

	const FBoxSphereBounds MeshBounds = CubeMesh->GetBounds();
	const FVector MeshSize = MeshBounds.BoxExtent * 2.0;

	for (FMatchCutBox& Box : Shot.Boxes)
	{
		const FVector Scale = Box.Size / MeshSize;
		Box.MeshOffset = -MeshBounds.Origin * Scale; // the prototype cube's pivot is not at its centre

		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Box.Center + Box.MeshOffset, FRotator::ZeroRotator);
		UStaticMeshComponent* SMC = Actor->GetStaticMeshComponent();
		SMC->SetMobility(EComponentMobility::Movable);
		SMC->SetStaticMesh(CubeMesh);
		Actor->SetActorScale3D(Scale);

		FLinearColor Color(0.10f, 0.11f, 0.13f);
		if (Box.bGoal)
		{
			Color = FLinearColor(0.10f, 0.55f, 0.15f);
			Actor->Tags.Add(GoalTag);
		}
		else if (Box.bCuttable)
		{
			Color = FLinearColor(0.75f, 0.30f, 0.04f);
		}
		if (Box.bCuttable)
		{
			Actor->Tags.Add(CuttableTag);
		}
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(FlatMaterial, Actor))
		{
			MID->SetVectorParameterValue(TEXT("Base Color"), Color);
			SMC->SetMaterial(0, MID);
		}
		Box.Actor = Actor;
	}
}

void AMatchCutGameMode::SpawnLights()
{
	UWorld* World = GetWorld();

	ADirectionalLight* Key = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-55.0, 35.0, 0.0));
	if (ULightComponent* LC = Key->GetLightComponent())
	{
		LC->SetMobility(EComponentMobility::Movable);
		LC->SetIntensity(6.f);
		if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(LC))
		{
			DLC->SetForwardShadingPriority(1); // two directional lights: say which one is the main one
		}
	}

	ADirectionalLight* Fill = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-25.0, 200.0, 0.0));
	if (ULightComponent* LC = Fill->GetLightComponent())
	{
		LC->SetMobility(EComponentMobility::Movable);
		LC->SetIntensity(2.5f);
		LC->SetCastShadows(false);
	}
}

void AMatchCutGameMode::StartPlay()
{
	Super::StartPlay();

	if (!CharacterClass || !CubeMesh || !FlatMaterial || !LoadLayout())
	{
		UE_LOG(LogMatchCut, Error, TEXT("Match-cut prototype could not start (missing template asset or layout)."));
		return;
	}

	// engine debug text ("Preparing shaders", light warnings) would sit on top of the test captures
	GAreScreenMessagesEnabled = false;

	SpawnLights();
	for (FMatchCutShot& Shot : Shots)
	{
		SpawnShot(Shot);
	}

	// ghost of the next shot
	GhostTarget = NewObject<UTextureRenderTarget2D>(this);
	GhostTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	GhostTarget->ClearColor = FLinearColor::Black;
	GhostTarget->InitAutoFormat(960, 540);
	GhostTarget->UpdateResourceImmediate(true);

	GhostCapture = GetWorld()->SpawnActor<ASceneCapture2D>(FVector::ZeroVector, FRotator::ZeroRotator);
	USceneCaptureComponent2D* Cap = GhostCapture->GetCaptureComponent2D();
	Cap->TextureTarget = GhostTarget;
	Cap->FOVAngle = Fov;
	Cap->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Cap->bCaptureEveryFrame = true;
	Cap->bCaptureOnMovement = false;
	Cap->bAlwaysPersistRenderingState = true;
	SetupLook(Cap->PostProcessSettings, ExposureEV);
	Cap->PostProcessBlendWeight = 1.f;

	// hero
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Hero = GetWorld()->SpawnActor<ACharacter>(CharacterClass, Shots[0].Start + FVector(0, 0, 100), FRotator::ZeroRotator, Params);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (Hero && PC)
	{
		PC->Possess(Hero);
	}

	UpdateMovers();
	SetShot(0, true);
	LoadScript();
}

FVector AMatchCutGameMode::GetFeet() const
{
	if (!Hero)
	{
		return FVector::ZeroVector;
	}
	return Hero->GetActorLocation() - FVector(0, 0, Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

bool AMatchCutGameMode::ProjectToShot(int32 ShotIndex, const FVector& World, FVector2D& OutNdc) const
{
	const FMatchCutShot& Shot = Shots[ShotIndex];
	const FRotationMatrix M(Shot.CamRot);
	const FVector D = World - Shot.CamPos;
	const double Depth = FVector::DotProduct(D, M.GetUnitAxis(EAxis::X));
	if (Depth <= 1.0)
	{
		return false;
	}
	const double TanH = FMath::Tan(FMath::DegreesToRadians(Fov) * 0.5);
	const double TanV = TanH / AspectRatio;
	OutNdc.X = FVector::DotProduct(D, M.GetUnitAxis(EAxis::Y)) / Depth / TanH;
	OutNdc.Y = FVector::DotProduct(D, M.GetUnitAxis(EAxis::Z)) / Depth / TanV;
	return FMath::Abs(OutNdc.X) <= 1.0 && FMath::Abs(OutNdc.Y) <= 1.0;
}

void AMatchCutGameMode::DeprojectFromShot(int32 ShotIndex, const FVector2D& Ndc, FVector& OutStart, FVector& OutDir) const
{
	const FMatchCutShot& Shot = Shots[ShotIndex];
	const FRotationMatrix M(Shot.CamRot);
	const double TanH = FMath::Tan(FMath::DegreesToRadians(Fov) * 0.5);
	const double TanV = TanH / AspectRatio;
	OutStart = Shot.CamPos;
	OutDir = (M.GetUnitAxis(EAxis::X) + M.GetUnitAxis(EAxis::Y) * (Ndc.X * TanH) + M.GetUnitAxis(EAxis::Z) * (Ndc.Y * TanV)).GetSafeNormal();
}

bool AMatchCutGameMode::TryCut()
{
	if (State != EMatchCutState::Playing || !Hero || Cur + 1 >= Shots.Num())
	{
		return false;
	}

	const int32 Next = Cur + 1;
	const FVector Feet = GetFeet();
	const FVector Local = Feet - Shots[Cur].Origin;
	LogProjectionCheck(TEXT("before"));

	FVector2D Ndc;
	if (!ProjectToShot(Cur, Feet, Ndc))
	{
		UE_LOG(LogMatchCut, Display, TEXT("CUT %d->%d NG reason=offscreen from=(%.0f,%.0f,%.0f)"), Cur, Next, Local.X, Local.Y, Local.Z);
		FailTake(TEXT("out of frame"));
		return false;
	}

	FVector Start, Dir;
	DeprojectFromShot(Next, Ndc, Start, Dir);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MatchCut), false, Hero);
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Dir * TraceLength, ECC_Visibility, Params);

	FString Reason;
	FVector NewLoc = FVector::ZeroVector;
	const UCapsuleComponent* Capsule = Hero->GetCapsuleComponent();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();

	if (!bHit)
	{
		Reason = TEXT("nothing there");
	}
	else if (!Hit.GetActor() || !Hit.GetActor()->ActorHasTag(CuttableTag))
	{
		Reason = TEXT("not a landing surface");
	}
	else if (Hit.ImpactNormal.Z < 0.7f)
	{
		Reason = TEXT("hit a wall");
	}
	else
	{
		NewLoc = Hit.ImpactPoint + FVector(0, 0, HalfHeight + 2.f);
		if (GetWorld()->OverlapBlockingTestByChannel(NewLoc, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params))
		{
			Reason = TEXT("no room to stand");
		}
	}

	if (!Reason.IsEmpty())
	{
		UE_LOG(LogMatchCut, Display, TEXT("CUT %d->%d NG reason=\"%s\" from=(%.0f,%.0f,%.0f) ndc=(%.4f,%.4f) hit=%s"),
			Cur, Next, *Reason, Local.X, Local.Y, Local.Z, Ndc.X, Ndc.Y, bHit ? *GetNameSafe(Hit.GetActor()) : TEXT("none"));
		FailTake(Reason);
		return false;
	}

	// keep the motion's on-screen direction: rotate by the yaw difference between the two cameras
	const double DeltaYaw = FRotator::NormalizeAxis(Shots[Next].CamRot.Yaw - Shots[Cur].CamRot.Yaw);
	UCharacterMovementComponent* CMC = Hero->GetCharacterMovement();
	FVector Vel = CMC->Velocity.RotateAngleAxis(DeltaYaw, FVector::UpVector);
	Vel.Z = 0.0;
	FRotator Rot = Hero->GetActorRotation();
	Rot.Yaw += DeltaYaw;

	Hero->SetActorLocationAndRotation(NewLoc, Rot, false, nullptr, ETeleportType::TeleportPhysics);
	// Stand on the surface right away. Leaving the hero falling for the last 2 units lets a moving
	// platform slide out from under them during one long frame (seen at ~5 fps in the offscreen runs).
	CMC->SetMovementMode(MOVE_Walking);
	Hero->SetBase(Hit.GetComponent());
	CMC->bForceNextFloorCheck = true;
	CMC->Velocity = Vel;

	FVector2D After;
	ProjectToShot(Next, GetFeet(), After);
	const FVector NewLocal = GetFeet() - Shots[Next].Origin;
	const double ErrPx = FVector2D((After.X - Ndc.X) * 960.0, (After.Y - Ndc.Y) * 540.0).Size();
	UE_LOG(LogMatchCut, Display, TEXT("CUT %d->%d OK from=(%.0f,%.0f,%.0f) ndc=(%.4f,%.4f) to=(%.0f,%.0f,%.0f) hit=%s ndcAfter=(%.4f,%.4f) err=%.2fpx@1080p"),
		Cur, Next, Local.X, Local.Y, Local.Z, Ndc.X, Ndc.Y, NewLocal.X, NewLocal.Y, NewLocal.Z, *GetNameSafe(Hit.GetActor()), After.X, After.Y, ErrPx);

	CutFlash = 0.5f;
	Take = 1;
	SetShot(Next, false);
	return true; // the view target changes next frame; scripts log "proj" after a short wait
}

void AMatchCutGameMode::LogProjectionCheck(const TCHAR* Label) const
{
	// compares our window-independent screen position with where the engine really draws the feet
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	if (!LP || !LP->ViewportClient || !LP->ViewportClient->Viewport)
	{
		return;
	}
	FVector2D Ndc;
	ProjectToShot(Cur, GetFeet(), Ndc);

	FSceneViewProjectionData PD;
	if (!LP->GetProjectionData(LP->ViewportClient->Viewport, PD))
	{
		return;
	}
	const FIntRect R = PD.GetConstrainedViewRect();
	const FIntPoint VS = LP->ViewportClient->Viewport->GetSizeXY();
	const FVector2D Predicted(R.Min.X + (Ndc.X + 1.0) * 0.5 * R.Width(), R.Min.Y + (1.0 - Ndc.Y) * 0.5 * R.Height());
	FVector2D Engine(0, 0);
	PC->ProjectWorldLocationToScreen(GetFeet(), Engine, false);
	UE_LOG(LogMatchCut, Display, TEXT("PROJ %s shot=%d viewport=%dx%d viewrect=(%d,%d)-(%d,%d) aspect=%.4f ndc=(%.4f,%.4f) predictedPx=(%.1f,%.1f) enginePx=(%.1f,%.1f) diff=%.2fpx"),
		Label, Cur, VS.X, VS.Y, R.Min.X, R.Min.Y, R.Max.X, R.Max.Y, R.Height() > 0 ? (double)R.Width() / R.Height() : 0.0,
		Ndc.X, Ndc.Y, Predicted.X, Predicted.Y, Engine.X, Engine.Y, (Predicted - Engine).Size());
}

void AMatchCutGameMode::ReportFrameTime()
{
	if (FrameCount > 0)
	{
		UE_LOG(LogMatchCut, Display, TEXT("FRAMETIME shot=%d frames=%d avg=%.2fms max=%.2fms"), Cur, FrameCount, FrameAccum / FrameCount * 1000.0, FrameMax * 1000.0);
	}
	FrameAccum = 0.0;
	FrameMax = 0.0;
	FrameCount = 0;
	ShotElapsed = 0.f;
}

void AMatchCutGameMode::SetShot(int32 Index, bool bRespawn)
{
	ReportFrameTime();
	Cur = FMath::Clamp(Index, 0, Shots.Num() - 1);
	const FMatchCutShot& Shot = Shots[Cur];

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetViewTarget(Shot.Camera.Get());
		PC->SetControlRotation(FRotator(0.0, Shot.CamRot.Yaw, 0.0));
	}

	if (GhostCapture)
	{
		USceneCaptureComponent2D* Cap = GhostCapture->GetCaptureComponent2D();
		const bool bHasNext = Cur + 1 < Shots.Num();
		Cap->bCaptureEveryFrame = bHasNext && bGhostEnabled;
		if (bHasNext)
		{
			GhostCapture->SetActorLocationAndRotation(Shots[Cur + 1].CamPos, Shots[Cur + 1].CamRot);
		}
	}

	if (bRespawn)
	{
		RespawnAtStart();
	}
}

void AMatchCutGameMode::RespawnAtStart()
{
	if (!Hero)
	{
		return;
	}
	const FMatchCutShot& Shot = Shots[Cur];
	const float HalfHeight = Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	UCharacterMovementComponent* CMC = Hero->GetCharacterMovement();
	CMC->StopMovementImmediately();
	Hero->SetActorLocationAndRotation(Shot.Start + FVector(0, 0, HalfHeight + 2.f), FRotator(0.0, Shot.CamRot.Yaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	CMC->SetMovementMode(MOVE_Falling);
}

void AMatchCutGameMode::FailTake(const FString& Reason)
{
	State = EMatchCutState::NG;
	StateTime = 0.f;
	NGReason = Reason;
	if (Hero)
	{
		Hero->GetCharacterMovement()->StopMovementImmediately();
		Hero->GetCharacterMovement()->DisableMovement();
	}
}

void AMatchCutGameMode::Retake()
{
	if (State != EMatchCutState::Playing)
	{
		return;
	}
	++Take;
	UE_LOG(LogMatchCut, Display, TEXT("RETAKE from shot %d"), Cur);
	SetShot(Cur > 0 ? Cur - 1 : 0, true);
}

void AMatchCutGameMode::RestartAll()
{
	State = EMatchCutState::Playing;
	Take = 1;
	SetShot(0, true);
}

void AMatchCutGameMode::UpdateMovers()
{
	for (FMatchCutShot& Shot : Shots)
	{
		for (FMatchCutBox& Box : Shot.Boxes)
		{
			if (Box.bMoves && Box.Actor.IsValid())
			{
				const double Alpha = 0.5 - 0.5 * FMath::Cos(2.0 * UE_DOUBLE_PI * ShotClock / FMath::Max(Box.Period, 0.1f));
				Box.Actor->SetActorLocation(Box.Center + Box.MeshOffset + Box.MoveTo * Alpha);
			}
		}
	}
}

void AMatchCutGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Shots.Num() == 0 || !Hero)
	{
		return;
	}

	TickScript(DeltaSeconds);

	if (!bClockFrozen)
	{
		ShotClock += DeltaSeconds;
	}
	UpdateMovers();

	// frame time, skipping the first second after each shot change (hitches from first-use shaders)
	ShotElapsed += DeltaSeconds;
	if (ShotElapsed > 1.f)
	{
		FrameAccum += DeltaSeconds;
		FrameMax = FMath::Max(FrameMax, (double)DeltaSeconds);
		++FrameCount;
	}

	CutFlash = FMath::Max(0.f, CutFlash - DeltaSeconds);
	if (ScriptMoveLeft > 0.f)
	{
		ScriptMoveLeft -= DeltaSeconds;
	}

	StateTime += DeltaSeconds;
	if (State == EMatchCutState::NG)
	{
		if (StateTime >= NGSeconds)
		{
			State = EMatchCutState::Playing;
			++Take;
			if (bFellThisTake && Cur > 0)
			{
				// Falling after a cut sends you back to redo the cut. Respawning on this shot's own
				// start mark would turn a fall into a shortcut past whatever the landing demanded.
				SetShot(Cur - 1, true);
			}
			else
			{
				RespawnAtStart();
			}
			bFellThisTake = false;
		}
		return;
	}
	if (State == EMatchCutState::Wrap)
	{
		return;
	}

	// fell out of the shot
	if (GetFeet().Z < Shots[Cur].Origin.Z - KillDepth)
	{
		UE_LOG(LogMatchCut, Display, TEXT("FELL in shot %d"), Cur);
		FailTake(TEXT("fell out of the shot"));
		bFellThisTake = true;
		return;
	}

	// standing on the goal mark
	if (const UPrimitiveComponent* Base = Cast<UPrimitiveComponent>(Hero->GetMovementBaseObject()))
	{
		if (Base->GetOwner() && Base->GetOwner()->ActorHasTag(GoalTag) && Hero->GetCharacterMovement()->IsMovingOnGround())
		{
			UE_LOG(LogMatchCut, Display, TEXT("WRAP reached in shot %d"), Cur);
			State = EMatchCutState::Wrap;
			StateTime = 0.f;
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Script runner for offscreen verification: -MCScript=<file> [-MCOut=<dir>]
// One command per line:
//   goto N | tp X Y Z (shot-local feet) | wait S | snap NAME | cut | retake | restart
//   move RIGHT FORWARD SECONDS | jump | mtime T (set the mover clock) | proj LABEL | quit
//   freeze 1|0 (stop/resume the mover clock) | ghost 1|0 (ghost overlay on/off) | walkto X Y (walk to a shot-local spot) | waitclock T PERIOD (wait until the mover clock, modulo PERIOD, reaches T)
// ---------------------------------------------------------------------------------------------

void AMatchCutGameMode::LoadScript()
{
	FString Path;
	if (!FParse::Value(FCommandLine::Get(), TEXT("MCScript="), Path))
	{
		return;
	}
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogMatchCut, Error, TEXT("Cannot read script %s"), *Path);
		return;
	}
	Text.ParseIntoArrayLines(ScriptLines, true);
	if (!FParse::Value(FCommandLine::Get(), TEXT("MCOut="), ScriptOutDir))
	{
		ScriptOutDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("MatchCut"));
	}
	bScriptActive = true;
	ScriptWait = 3.f; // let the first shaders settle
	UE_LOG(LogMatchCut, Display, TEXT("Script %s: %d lines, output %s"), *Path, ScriptLines.Num(), *ScriptOutDir);
}

void AMatchCutGameMode::TickScript(float DeltaSeconds)
{
	if (!bScriptActive)
	{
		return;
	}
	if (bScriptWalking)
	{
		FVector To = Shots[Cur].Origin + ScriptWalkTarget - GetFeet();
		To.Z = 0.0;
		ScriptWalkTime += DeltaSeconds;
		if (To.Size() < 12.0 || ScriptWalkTime > 8.f || State != EMatchCutState::Playing)
		{
			const FVector L = GetFeet() - Shots[Cur].Origin;
			UE_LOG(LogMatchCut, Display, TEXT("SCRIPT walkto ended at (%.0f,%.0f,%.0f) after %.2fs"), L.X, L.Y, L.Z, ScriptWalkTime);
			bScriptWalking = false;
		}
		else
		{
			Hero->AddMovementInput(To.GetSafeNormal(), FMath::Clamp(To.Size() / 60.0, 0.25, 1.0));
		}
		return;
	}
	if (ScriptClockTarget >= 0.f)
	{
		const float Phase = FMath::Fmod(ShotClock, ScriptClockPeriod);
		if (Phase < ScriptClockTarget || Phase > ScriptClockTarget + 0.1f)
		{
			return;
		}
		ScriptClockTarget = -1.f;
	}
	if (ScriptWait > 0.f)
	{
		ScriptWait -= DeltaSeconds;
		return;
	}
	if (State == EMatchCutState::NG && ScriptPc < ScriptLines.Num() && !ScriptLines[ScriptPc].StartsWith(TEXT("snap")) && !ScriptLines[ScriptPc].StartsWith(TEXT("wait")))
	{
		return; // wait for the NG to finish before acting again
	}
	while (ScriptPc < ScriptLines.Num() && ScriptWait <= 0.f && !bScriptWalking && ScriptClockTarget < 0.f)
	{
		const FString Line = ScriptLines[ScriptPc++].TrimStartAndEnd();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		if (!RunScriptLine(Line))
		{
			break;
		}
	}
	if (ScriptPc >= ScriptLines.Num() && ScriptWait <= 0.f)
	{
		bScriptActive = false;
	}
}

bool AMatchCutGameMode::RunScriptLine(const FString& Line)
{
	TArray<FString> T;
	Line.ParseIntoArrayWS(T);
	const FString& Cmd = T[0];
	auto Num = [&T](int32 i) { return T.IsValidIndex(i) ? FCString::Atof(*T[i]) : 0.f; };

	UE_LOG(LogMatchCut, Display, TEXT("SCRIPT %s"), *Line);

	if (Cmd == TEXT("goto"))
	{
		State = EMatchCutState::Playing;
		SetShot((int32)Num(1), true);
	}
	else if (Cmd == TEXT("tp"))
	{
		const float HalfHeight = Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Hero->GetCharacterMovement()->StopMovementImmediately();
		Hero->SetActorLocation(Shots[Cur].Origin + FVector(Num(1), Num(2), Num(3) + HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
	else if (Cmd == TEXT("wait"))
	{
		ScriptWait = Num(1);
	}
	else if (Cmd == TEXT("snap"))
	{
		const FString File = ScriptOutDir / (T.IsValidIndex(1) ? T[1] : TEXT("snap")) + TEXT(".png");
		FScreenshotRequest::RequestScreenshot(File, true, false);
		ScriptWait = 0.3f;
	}
	else if (Cmd == TEXT("cut"))
	{
		TryCut();
		return false; // let a frame pass
	}
	else if (Cmd == TEXT("retake"))
	{
		Retake();
	}
	else if (Cmd == TEXT("restart"))
	{
		RestartAll();
	}
	else if (Cmd == TEXT("move"))
	{
		ScriptMove = FVector2D(Num(1), Num(2));
		ScriptMoveLeft = Num(3);
		ScriptWait = Num(3);
	}
	else if (Cmd == TEXT("jump"))
	{
		Hero->Jump();
	}
	else if (Cmd == TEXT("mtime"))
	{
		ShotClock = Num(1);
		UpdateMovers();
	}
	else if (Cmd == TEXT("ghost"))
	{
		bGhostEnabled = Num(1) > 0.f; // measure what the ghost capture costs
		SetShot(Cur, false);
	}
	else if (Cmd == TEXT("freeze"))
	{
		bClockFrozen = Num(1) > 0.f; // hold the movers still so a capture and the cut after it see the same thing
	}
	else if (Cmd == TEXT("proj"))
	{
		LogProjectionCheck(T.IsValidIndex(1) ? *T[1] : TEXT("script"));
	}
	else if (Cmd == TEXT("walkto"))
	{
		ScriptWalkTarget = FVector(Num(1), Num(2), 0.0);
		ScriptWalkTime = 0.f;
		bScriptWalking = true;
	}
	else if (Cmd == TEXT("waitclock"))
	{
		ScriptClockTarget = Num(1);
		ScriptClockPeriod = FMath::Max(Num(2), 0.1f);
	}
	else if (Cmd == TEXT("quit"))
	{
		ReportFrameTime();
		FPlatformMisc::RequestExit(false);
		return false;
	}
	else
	{
		UE_LOG(LogMatchCut, Warning, TEXT("Unknown script command: %s"), *Line);
	}
	return true;
}
