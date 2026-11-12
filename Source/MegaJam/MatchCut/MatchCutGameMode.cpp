// Deep Cut - underwater vertical slice (brief 02). Greybox: everything is spawned from engine shapes.

#include "MatchCutGameMode.h"
#include "MatchCutPlayerController.h"
#include "MatchCutHUD.h"
#include "DeepCutText.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogMatchCut);

namespace
{
	const double AspectRatio = 16.0 / 9.0;
	const double PictureHalf = 0.75;  // the picture is the middle 75 % of the 16:9 frame; the HUD owns the bars
	const float TakeSeconds = 0.85f;
	const float DropSeconds = 2.2f;
	const float FilmEndHold = 2.2f;
	const float Accel = 1500.f;
	const float MaxSpeed = 430.f;
	const float Drag = 2.4f;

	const FLinearColor RockColor(0.30f, 0.28f, 0.25f);
	const FLinearColor PlateColor(0.50f, 0.26f, 0.10f);
	const FLinearColor DecorColor(0.10f, 0.12f, 0.13f);
	const FLinearColor HeroColor(0.36f, 0.45f, 0.42f);
	const FLinearColor CrewColor(0.85f, 0.60f, 0.08f);
	const FLinearColor Unit7Color(0.80f, 0.78f, 0.72f);
	const FLinearColor DarkMetal(0.06f, 0.06f, 0.07f);
	const FLinearColor EyeCyan(0.2f, 3.0f, 3.5f);
	const FLinearColor EyeWarm(4.0f, 2.6f, 0.6f);

	FVector ReadVec(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, const FVector& Default = FVector::ZeroVector)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Obj.IsValid() && Obj->TryGetArrayField(Field, Arr) && Arr && Arr->Num() >= 3)
		{
			return FVector((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
		}
		return Default;
	}

	bool ReadVec2(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, FVector2D& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Obj.IsValid() && Obj->TryGetArrayField(Field, Arr) && Arr && Arr->Num() >= 2)
		{
			Out = FVector2D((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber());
			return true;
		}
		return false;
	}

	bool ReadVec4(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, FVector4& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Obj.IsValid() && Obj->TryGetArrayField(Field, Arr) && Arr && Arr->Num() >= 4)
		{
			Out = FVector4((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber(), (*Arr)[3]->AsNumber());
			return true;
		}
		return false;
	}

	FLinearColor ReadColor(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, const FLinearColor& Default)
	{
		const FVector V = ReadVec(Obj, Field, FVector(Default.R, Default.G, Default.B));
		return FLinearColor(V.X, V.Y, V.Z);
	}

	void ReadFloat(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, float& Out)
	{
		double D = 0.0;
		if (Obj->TryGetNumberField(Field, D))
		{
			Out = (float)D;
		}
	}

	void SetupLook(FPostProcessSettings& PP, float EV, float Vignette, float Grain)
	{
		// fixed exposure so every shot looks the same; no Lumen (must stay cheap on integrated GPUs)
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
		PP.VignetteIntensity = Vignette;
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = 0.35f;
		PP.bOverride_FilmGrainIntensity = true;
		PP.FilmGrainIntensity = Grain;
	}

	FString IntroFlagPath()
	{
		return FPaths::ProjectSavedDir() / TEXT("DeepCut/intro_seen.txt");
	}
}

AMatchCutGameMode::AMatchCutGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = nullptr; // the robot is a plain actor moved by this class
	PlayerControllerClass = AMatchCutPlayerController::StaticClass();
	HUDClass = AMatchCutHUD::StaticClass();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = CubeFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	SphereMesh = SphereFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	CylinderMesh = CylinderFinder.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LitFinder(TEXT("/Game/DeepCut/M_DC_Lit.M_DC_Lit"));
	LitMaterial = LitFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ToonFinder(TEXT("/Game/DeepCut/M_DC_Toon.M_DC_Toon"));
	ToonMaterial = ToonFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> UnlitFinder(TEXT("/Game/DeepCut/M_DC_Unlit.M_DC_Unlit"));
	UnlitMaterial = UnlitFinder.Object;
}

// ---------------------------------------------------------------------------------------------
// layout
// ---------------------------------------------------------------------------------------------

bool AMatchCutGameMode::LoadLayout()
{
	// Content/Data is staged as UFS (DefaultGame.ini), so this path also works in a packaged build
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data/deepcut.json");
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

	ReadFloat(Root, TEXT("fov"), Fov);
	ReadFloat(Root, TEXT("ghost"), GhostOpacity);
	GhostTint = ReadColor(Root, TEXT("ghostTint"), GhostTint);
	ReadFloat(Root, TEXT("exposure"), ExposureEV);
	ReadFloat(Root, TEXT("heroRadius"), HeroRadius);
	ReadFloat(Root, TEXT("restoreTol"), RestoreTol);
	ReadFloat(Root, TEXT("lampRadius"), LampRadius);
	Root->TryGetNumberField(TEXT("startDepth"), StartDepth);
	Root->TryGetNumberField(TEXT("crewTotal"), CrewTotal);
	ReadVec2(Root, TEXT("restore"), RestoreNdc);
	Root->TryGetStringField(TEXT("look"), LookName);
	double Spacing = 50000.0;
	Root->TryGetNumberField(TEXT("spacing"), Spacing);

	TanH = FMath::Tan(FMath::DegreesToRadians(Fov) * 0.5);
	TanV = TanH / AspectRatio;

	const TSharedPtr<FJsonObject>* LooksObj = nullptr;
	if (Root->TryGetObjectField(TEXT("looks"), LooksObj) && LooksObj)
	{
		for (const auto& Pair : (*LooksObj)->Values)
		{
			const TSharedPtr<FJsonObject> L = Pair.Value->AsObject();
			if (!L.IsValid())
			{
				continue;
			}
			FDCLook& Out = Looks.Add(FString(Pair.Key));
			ReadFloat(L, TEXT("fogDensity"), Out.FogDensity);
			Out.FogColor = ReadColor(L, TEXT("fogColor"), Out.FogColor);
			L->TryGetBoolField(TEXT("volumetric"), Out.bVolumetric);
			ReadFloat(L, TEXT("volScatter"), Out.VolScatter);
			ReadFloat(L, TEXT("volExtinction"), Out.VolExtinction);
			ReadFloat(L, TEXT("volEmissive"), Out.VolEmissive);
			ReadFloat(L, TEXT("keyIntensity"), Out.KeyIntensity);
			Out.KeyColor = ReadColor(L, TEXT("keyColor"), Out.KeyColor);
			ReadFloat(L, TEXT("fillIntensity"), Out.FillIntensity);
			Out.FillColor = ReadColor(L, TEXT("fillColor"), Out.FillColor);
			ReadFloat(L, TEXT("lampIntensity"), Out.LampIntensity);
			ReadFloat(L, TEXT("glowIntensity"), Out.GlowIntensity);
			L->TryGetBoolField(TEXT("toon"), Out.bToon);
			L->TryGetBoolField(TEXT("shafts"), Out.bShafts);
			ReadFloat(L, TEXT("vignette"), Out.Vignette);
			ReadFloat(L, TEXT("grain"), Out.Grain);
			ReadFloat(L, TEXT("darkFactor"), Out.DarkFactor);
			ReadFloat(L, TEXT("ambient"), Out.Ambient);
			Out.Backdrop = ReadColor(L, TEXT("backdrop"), Out.Backdrop);
		}
	}

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
		const int32 Index = Shots.Num();
		FDCShot& Shot = Shots.AddDefaulted_GetRef();
		Shot.Origin = FVector(0.0, Spacing * Index, 0.0);
		S->TryGetStringField(TEXT("name"), Shot.Name);
		S->TryGetStringField(TEXT("hint"), Shot.Hint);
		S->TryGetBoolField(TEXT("dark"), Shot.bDark);
		ReadVec2(S, TEXT("start"), Shot.Start);
		ReadVec4(S, TEXT("swim"), Shot.Swim);
		FVector2D Depth(800.0, 1000.0);
		ReadVec2(S, TEXT("depth"), Depth);
		Shot.DepthMin = Depth.X;
		Shot.DepthMax = Depth.Y;
		Shot.Ref = 0.5 * (Depth.X + Depth.Y);
		S->TryGetNumberField(TEXT("ref"), Shot.Ref);
		if (ReadVec2(S, TEXT("part"), Shot.PartNdc))
		{
			Shot.bHasPart = true;
			PartShot = Index;
		}
		if (ReadVec2(S, TEXT("crew"), Shot.CrewNdc))
		{
			Shot.bHasCrew = true;
			CrewShot = Index;
		}

		const TSharedPtr<FJsonObject>* Cam = nullptr;
		if (S->TryGetObjectField(TEXT("cam"), Cam) && Cam)
		{
			const FVector Pos = ReadVec(*Cam, TEXT("pos"));
			const FVector LookAt = ReadVec(*Cam, TEXT("look"));
			Shot.CamPos = Shot.Origin + Pos;
			Shot.CamRot = (LookAt - Pos).Rotation();
		}

		const TArray<TSharedPtr<FJsonValue>>* RockArr = nullptr;
		if (S->TryGetArrayField(TEXT("rocks"), RockArr) && RockArr)
		{
			for (const TSharedPtr<FJsonValue>& RV : *RockArr)
			{
				const TSharedPtr<FJsonObject> R = RV->AsObject();
				if (!R.IsValid())
				{
					continue;
				}
				FDCRock& Rock = Shot.Rocks.AddDefaulted_GetRef();
				ReadVec4(R, TEXT("r"), Rock.Rect);
				R->TryGetBoolField(TEXT("plate"), Rock.bPlate);
				if (ReadVec2(R, TEXT("move"), Rock.MoveTo))
				{
					Rock.bMoves = true;
					ReadFloat(R, TEXT("period"), Rock.Period);
				}
			}
		}
	}

	UE_LOG(LogMatchCut, Display, TEXT("Layout loaded from %s: %d shots, %d looks, fov %.1f"), *Path, Shots.Num(), Looks.Num(), Fov);
	return Shots.Num() > 1;
}

// ---------------------------------------------------------------------------------------------
// spawning
// ---------------------------------------------------------------------------------------------

ACameraActor* AMatchCutGameMode::SpawnCamera(const FVector& Pos, const FRotator& Rot)
{
	ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(Pos, Rot);
	UCameraComponent* CC = Cam->GetCameraComponent();
	CC->SetFieldOfView(Fov);
	CC->SetAspectRatio(AspectRatio);
	CC->SetConstraintAspectRatio(true); // 16:9 at any window size; the cut maths never reads the window
	SetupLook(CC->PostProcessSettings, ExposureEV, 0.4f, 0.f);
	CC->PostProcessBlendWeight = 1.f;
	AllCameras.Add(Cam);
	return Cam;
}

int32 AMatchCutGameMode::AddPaint(UStaticMeshComponent* Comp, const FLinearColor& Color, const FLinearColor& Emissive)
{
	FDCPaint& P = Paints.AddDefaulted_GetRef();
	P.Comp = Comp;
	P.Color = Color;
	P.Emissive = Emissive;
	return Paints.Num() - 1;
}

void AMatchCutGameMode::SetPaintEmissive(int32 Paint, const FLinearColor& Emissive)
{
	if (Paints.IsValidIndex(Paint))
	{
		Paints[Paint].Emissive = Emissive;
		if (UMaterialInstanceDynamic* MID = Paints[Paint].MID.Get())
		{
			MID->SetVectorParameterValue(TEXT("Emissive"), Emissive + Paints[Paint].Color * AmbientNow);
		}
	}
}

AActor* AMatchCutGameMode::SpawnBox(const FVector& Center, const FRotator& Rot, const FVector& Size, const FLinearColor& Color, bool bCollide, bool bHideFromGhost)
{
	AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Center, Rot);
	UStaticMeshComponent* SMC = Actor->GetStaticMeshComponent();
	SMC->SetMobility(EComponentMobility::Movable);
	SMC->SetStaticMesh(CubeMesh);
	Actor->SetActorScale3D(Size / 100.0);
	if (!bCollide)
	{
		SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	AddPaint(SMC, Color, FLinearColor::Black);
	if (bHideFromGhost)
	{
		GhostHidden.Add(Actor);
	}
	return Actor;
}

UStaticMeshComponent* AMatchCutGameMode::AddPart(AActor* Owner, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor& Color, const FLinearColor& Emissive)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetupAttachment(Owner->GetRootComponent());
	C->RegisterComponent();
	C->SetRelativeLocationAndRotation(Loc, Rot);
	C->SetRelativeScale3D(Scale);
	AddPaint(C, Color, Emissive);
	return C;
}

FDCRobot AMatchCutGameMode::SpawnRobot(const FVector& Loc, const FLinearColor& Body, const FLinearColor& Eye, bool bBattered)
{
	FDCRobot Robot;
	AActor* A = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Loc));
	USceneComponent* RootComp = NewObject<USceneComponent>(A);
	RootComp->SetMobility(EComponentMobility::Movable);
	A->SetRootComponent(RootComp);
	RootComp->RegisterComponent();
	A->SetActorLocation(Loc);

	// local frame: +X is forward. About 72 units across.
	AddPart(A, SphereMesh, FVector(0, 0, 0), FRotator::ZeroRotator, FVector(0.80, 0.68, 0.68), Body, FLinearColor::Black);
	AddPart(A, CylinderMesh, FVector(-6, 0, 0), FRotator(90, 0, 0), FVector(0.74, 0.74, 0.10), DarkMetal, FLinearColor::Black);
	AddPart(A, SphereMesh, FVector(30, 0, 8), FRotator::ZeroRotator, FVector(0.26, 0.40, 0.22), Eye.GetClamped(0.f, 1.f), Eye);
	Robot.EyePaint = Paints.Num() - 1;
	AddPart(A, CylinderMesh, FVector(-36, 22, -8), FRotator(90, 0, 0), FVector(0.20, 0.20, 0.30), DarkMetal, FLinearColor::Black);
	if (!bBattered)
	{
		AddPart(A, CylinderMesh, FVector(-36, -22, -8), FRotator(90, 0, 0), FVector(0.20, 0.20, 0.30), DarkMetal, FLinearColor::Black);
	}
	AddPart(A, CylinderMesh, FVector(-4, 0, 46), FRotator(bBattered ? 38.0 : 8.0, 0, 0), FVector(0.04, 0.04, 0.36), DarkMetal, FLinearColor::Black);
	if (bBattered)
	{
		// a dented patch where a number plate used to be
		AddPart(A, CubeMesh, FVector(2, 0, -24), FRotator(0, 0, 0), FVector(0.30, 0.66, 0.10), FLinearColor(0.16f, 0.18f, 0.17f), FLinearColor::Black);
	}
	Robot.Actor = A;
	return Robot;
}

void AMatchCutGameMode::PlaceRobot(const FDCRobot& Robot, const FVector& Loc, const FVector2D& Head, const FRotator& CamRot, float Roll)
{
	AActor* A = Robot.Actor.Get();
	if (!A)
	{
		return;
	}
	const FRotationMatrix M(CamRot);
	const FVector R = M.GetUnitAxis(EAxis::Y);
	const FVector U = M.GetUnitAxis(EAxis::Z);
	// face left or right, tilt up or down at most ~50 degrees so "up" stays up
	const double Side = Head.X >= 0.0 ? 1.0 : -1.0;
	const double Tilt = FMath::Clamp(Head.Y, -0.75, 0.75);
	const FVector X = (R * Side * FMath::Sqrt(1.0 - Tilt * Tilt) + U * Tilt).GetSafeNormal();
	FRotator Rot = FRotationMatrix::MakeFromXZ(X, U).Rotator();
	Rot.Roll += Roll;
	A->SetActorLocationAndRotation(Loc, Rot);
}

void AMatchCutGameMode::SpawnShot(FDCShot& Shot, int32 Index)
{
	Shot.Camera = SpawnCamera(Shot.CamPos, Shot.CamRot);

	const FRotationMatrix M(Shot.CamRot);
	const FVector F = M.GetUnitAxis(EAxis::X);
	const FVector R = M.GetUnitAxis(EAxis::Y);
	const FVector U = M.GetUnitAxis(EAxis::Z);

	// rocks: boxes that face the camera and run through the whole slab, so the collision is 2D on screen
	// The front face sits just in front of the robot's plane, so what you see is what you collide with.
	// Rocks are authored at the depth the robot has in that shot (ref).
	const double Near = Shot.Ref - HeroRadius - 6.0;
	const double Far = Shot.DepthMax + 260.0;
	for (FDCRock& Rock : Shot.Rocks)
	{
		const double Cx = 0.5 * (Rock.Rect.X + Rock.Rect.Z) * TanH * Shot.Ref;
		const double Cy = 0.5 * (Rock.Rect.Y + Rock.Rect.W) * TanV * Shot.Ref;
		const double Sx = (Rock.Rect.Z - Rock.Rect.X) * TanH * Shot.Ref;
		const double Sy = (Rock.Rect.W - Rock.Rect.Y) * TanV * Shot.Ref;
		const FVector Center = Shot.CamPos + F * (0.5 * (Near + Far)) + R * Cx + U * Cy;
		Rock.BaseLoc = Center;
		Rock.MoveWorld = R * (Rock.MoveTo.X * TanH * Shot.Ref) + U * (Rock.MoveTo.Y * TanV * Shot.Ref);
		Rock.Actor = SpawnBox(Center, Shot.CamRot, FVector(Far - Near, Sx, Sy), Rock.bPlate ? PlateColor : RockColor, false, false);
	}

	// far backdrop and loose background rocks (no collision, never in the ghost)
	FRandomStream Rand(Index * 7919 + 13);
	AStaticMeshActor* Back = GetWorld()->SpawnActor<AStaticMeshActor>(Shot.CamPos + F * (Shot.DepthMax + 5200.0), Shot.CamRot);
	{
		UStaticMeshComponent* SMC = Back->GetStaticMeshComponent();
		SMC->SetMobility(EComponentMobility::Movable);
		SMC->SetStaticMesh(CubeMesh);
		SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SMC->SetCastShadow(false);
		Back->SetActorScale3D(FVector(0.2, 200.0, 120.0));
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(UnlitMaterial, Back);
		SMC->SetMaterial(0, MID);
		BackdropMIDs.Add(MID);
		GhostHidden.Add(Back);
	}
	for (int32 i = 0; i < 16; ++i)
	{
		const double D = Shot.DepthMax + 650.0 + Rand.FRandRange(0.f, 2600.f);
		const FVector2D N(Rand.FRandRange(-1.3f, 1.3f), Rand.FRandRange(-1.1f, 0.5f));
		const FVector Size(Rand.FRandRange(150.f, 520.f), Rand.FRandRange(150.f, 520.f), Rand.FRandRange(200.f, 900.f));
		const FRotator Rot(Rand.FRandRange(-25.f, 25.f), Rand.FRandRange(0.f, 360.f), Rand.FRandRange(-25.f, 25.f));
		AActor* Decor = SpawnBox(CamPoint(Shot.CamPos, Shot.CamRot, N, D), Rot, Size, DecorColor, false, true);
		Cast<AStaticMeshActor>(Decor)->GetStaticMeshComponent()->SetCastShadow(false);
	}

	// slats high above the slab: they only exist to cut the key light into shafts (look A)
	const FVector Mid = Shot.CamPos + F * (Shot.Ref + 300.0);
	for (int32 i = 0; i < 9; ++i)
	{
		const FVector C = Mid + R * ((i - 4) * 230.0 + Rand.FRandRange(-40.f, 40.f)) + FVector(0, 0, 1000.0);
		AActor* Slat = SpawnBox(C, FRotator(0.0, Shot.CamRot.Yaw, 0.0), FVector(2600.0, Rand.FRandRange(70.f, 130.f), 20.0), DecorColor, false, true);
		ShaftSlats.Add(Slat);
	}
}

void AMatchCutGameMode::SpawnFilmSets()
{
	// The official film and the removed scene are small sets far away from the stage, shot by their own cameras.
	// All film cameras look down +X: screen right is +Y, up is +Z.
	for (int32 k = 0; k < 4; ++k)
	{
		FilmOrigin[k] = FVector(0.0, -200000.0 - 30000.0 * k, 0.0);
		FilmCam[k] = SpawnCamera(FilmOrigin[k] + FVector(-900.0, 0.0, 0.0), FRotator::ZeroRotator);
		AActor* Back = SpawnBox(FilmOrigin[k] + FVector(5000.0, 0, 0), FRotator::ZeroRotator, FVector(20.0, 20000.0, 12000.0), DecorColor, false, true);
		Cast<AStaticMeshActor>(Back)->GetStaticMeshComponent()->SetCastShadow(false);
	}
	// set 0: launch platform
	SpawnBox(FilmOrigin[0] + FVector(0, 0, 215.0), FRotator::ZeroRotator, FVector(260.0, 1150.0, 50.0), FLinearColor(0.25f, 0.27f, 0.30f), false, true);
	SpawnBox(FilmOrigin[0] + FVector(0, -480.0, 420.0), FRotator::ZeroRotator, FVector(40.0, 40.0, 400.0), DarkMetal, false, true);
	SpawnBox(FilmOrigin[0] + FVector(0, 480.0, 420.0), FRotator::ZeroRotator, FVector(40.0, 40.0, 400.0), DarkMetal, false, true);
	// set 2: sea floor
	SpawnBox(FilmOrigin[2] + FVector(600.0, 0, -200.0), FRotator::ZeroRotator, FVector(4000.0, 4000.0, 60.0), RockColor, false, true);
	SpawnBox(FilmOrigin[2] + FVector(350.0, -380.0, -110.0), FRotator(0, 30, 12), FVector(260.0, 200.0, 190.0), RockColor, false, true);
	SpawnBox(FilmOrigin[2] + FVector(500.0, 420.0, -90.0), FRotator(8, 60, 0), FVector(300.0, 240.0, 260.0), RockColor, false, true);
	// set 3: the cable that lifts Unit 7
	FilmCable = SpawnBox(FilmOrigin[3], FRotator::ZeroRotator, FVector(6.0, 6.0, 700.0), DarkMetal, false, true);

	for (int32 i = 0; i < 7; ++i)
	{
		FilmRobots.Add(SpawnRobot(FilmOrigin[0], i == 6 ? Unit7Color : CrewColor, EyeWarm, false));
	}

	// removed scene 1: a dark gap between two rock walls, Unit 7 holding its lamp on while the crew passes
	FlashOrigin = FVector(0.0, -400000.0, 0.0);
	FlashCam = SpawnCamera(FlashOrigin + FVector(-1000.0, 0.0, 0.0), FRotator::ZeroRotator);
	{
		FPostProcessSettings& PP = FlashCam->GetCameraComponent()->PostProcessSettings;
		PP.bOverride_ColorSaturation = true;
		PP.ColorSaturation = FVector4(0.18, 0.18, 0.18, 1.0);
		PP.bOverride_ColorGain = true;
		PP.ColorGain = FVector4(1.15, 1.02, 0.80, 1.0);
		PP.bOverride_ColorContrast = true;
		PP.ColorContrast = FVector4(1.15, 1.15, 1.15, 1.0);
		PP.VignetteIntensity = 1.0f;
		PP.FilmGrainIntensity = 1.0f;
	}
	const double HW = TanH * 1000.0;
	const double HH = TanV * 1000.0;
	SpawnBox(FlashOrigin + FVector(150.0, 0.0, HH * 0.80), FRotator::ZeroRotator, FVector(900.0, HW * 2.6, HH * 0.50), RockColor, false, false);
	SpawnBox(FlashOrigin + FVector(150.0, 0.0, -HH * 0.82), FRotator::ZeroRotator, FVector(900.0, HW * 2.6, HH * 0.46), RockColor, false, false);
	SpawnBox(FlashOrigin + FVector(500.0, HW * 0.75, 0.0), FRotator(0, 12, 0), FVector(300.0, HW * 0.5, HH * 1.4), RockColor, false, false);
	AActor* FlashBack = SpawnBox(FlashOrigin + FVector(5000.0, 0, 0), FRotator::ZeroRotator, FVector(20.0, 20000.0, 12000.0), DecorColor, false, true);
	Cast<AStaticMeshActor>(FlashBack)->GetStaticMeshComponent()->SetCastShadow(false);

	FlashUnit7 = SpawnRobot(FlashOrigin, Unit7Color, EyeWarm, false);
	if (AActor* A = FlashUnit7.Actor.Get())
	{
		USpotLightComponent* L = NewObject<USpotLightComponent>(A);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(A->GetRootComponent());
		L->RegisterComponent();
		L->SetRelativeLocation(FVector(34.0, 0.0, 8.0));
		L->SetIntensityUnits(ELightUnits::Unitless);
		L->SetIntensity(60000.f);
		L->SetLightColor(FLinearColor(1.f, 0.9f, 0.7f));
		L->SetInnerConeAngle(16.f);
		L->SetOuterConeAngle(34.f);
		L->SetAttenuationRadius(2200.f);
		L->SetVolumetricScatteringIntensity(4.f);
		FlashLamp = L;
		UPointLightComponent* G = NewObject<UPointLightComponent>(A);
		G->SetMobility(EComponentMobility::Movable);
		G->SetupAttachment(A->GetRootComponent());
		G->RegisterComponent();
		G->SetRelativeLocation(FVector(0.0, 0.0, 0.0));
		G->SetAbsolute(false, true, false);
		G->SetIntensityUnits(ELightUnits::Unitless);
		G->SetIntensity(9000.f);
		G->SetLightColor(FLinearColor(1.f, 0.9f, 0.7f));
		G->SetAttenuationRadius(900.f);
		G->SetCastShadows(false);
		G->AddWorldOffset(FVector(-260.0, 0.0, 60.0));
	}
	for (int32 i = 0; i < 3; ++i)
	{
		FlashCrew.Add(SpawnRobot(FlashOrigin, CrewColor, EyeWarm, false));
	}

	// timeline of the official film
	auto Seg = [this](float Len, int32 Gap, int32 Set, const TCHAR* Caption)
	{
		FDCReelSeg& S = Reel.AddDefaulted_GetRef();
		S.T0 = FilmLength;
		S.T1 = FilmLength + Len;
		S.Gap = Gap;
		S.Set = Set;
		S.Caption = Caption;
		FilmLength = S.T1;
	};
	Seg(5.0f, INDEX_NONE, 0, DCText::FilmCap1);
	Seg(4.0f, INDEX_NONE, 1, DCText::FilmCap2);
	Seg(2.5f, 0, 0, TEXT(""));
	Seg(4.5f, INDEX_NONE, 2, DCText::FilmCap3);
	Seg(2.5f, 1, 0, TEXT(""));
	Seg(4.5f, INDEX_NONE, 3, DCText::FilmCap4);
	Seg(2.5f, 2, 0, TEXT(""));
}

void AMatchCutGameMode::SpawnWorld()
{
	UWorld* World = GetWorld();

	KeyLight = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-68.0, 25.0, 0.0));
	if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(KeyLight->GetLightComponent()))
	{
		DLC->SetMobility(EComponentMobility::Movable);
		DLC->SetForwardShadingPriority(1);
		DLC->SetDynamicShadowDistanceMovableLight(9000.f);
	}
	FillLight = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-12.0, 10.0, 0.0));
	if (ULightComponent* LC = FillLight->GetLightComponent())
	{
		LC->SetMobility(EComponentMobility::Movable);
		LC->SetCastShadows(false);
	}
	Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0, 0, 0), FRotator::ZeroRotator);

	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		SpawnShot(Shots[i], i);
	}
	SpawnFilmSets();

	// the player's robot: battered, one thruster missing, number plate scraped off
	Hero = SpawnRobot(Shots[0].CamPos, HeroColor, EyeCyan, true);
	if (AActor* A = Hero.Actor.Get())
	{
		USpotLightComponent* L = NewObject<USpotLightComponent>(A);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(A->GetRootComponent());
		L->RegisterComponent();
		L->SetRelativeLocation(FVector(36.0, 0.0, 8.0));
		L->SetIntensityUnits(ELightUnits::Unitless);
		L->SetLightColor(FLinearColor(1.f, 0.93f, 0.75f));
		L->SetInnerConeAngle(14.f);
		L->SetOuterConeAngle(30.f);
		L->SetAttenuationRadius(2000.f);
		L->SetVolumetricScatteringIntensity(3.f);
		L->SetVisibility(false);
		LampSpot = L;

		// soft glow on the camera side of the robot so the faces we actually see get some of the lamp
		UPointLightComponent* G = NewObject<UPointLightComponent>(A);
		G->SetMobility(EComponentMobility::Movable);
		G->SetupAttachment(A->GetRootComponent());
		G->RegisterComponent();
		G->SetAbsolute(false, true, false);
		G->SetIntensityUnits(ELightUnits::Unitless);
		G->SetLightColor(FLinearColor(1.f, 0.93f, 0.75f));
		G->SetAttenuationRadius(800.f);
		G->SetCastShadows(false);
		G->SetVisibility(false);
		LampGlow = G;
	}

	if (PartShot != INDEX_NONE)
	{
		// the part: Unit 7's headlamp
		const FDCShot& S = Shots[PartShot];
		FDCRobot& P = PartActor;
		AActor* A = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(S.CamPos));
		USceneComponent* RootComp = NewObject<USceneComponent>(A);
		RootComp->SetMobility(EComponentMobility::Movable);
		A->SetRootComponent(RootComp);
		RootComp->RegisterComponent();
		AddPart(A, CylinderMesh, FVector(0, 0, 0), FRotator(90, 0, 0), FVector(0.30, 0.30, 0.34), Unit7Color, FLinearColor::Black);
		AddPart(A, SphereMesh, FVector(18, 0, 0), FRotator::ZeroRotator, FVector(0.26, 0.26, 0.26), FLinearColor(1.f, 0.85f, 0.4f), FLinearColor(6.f, 4.5f, 1.2f));
		P.EyePaint = Paints.Num() - 1;
		P.Actor = A;
		A->SetActorLocation(ShotPoint(PartShot, S.PartNdc, S.Ref));
	}
	if (CrewShot != INDEX_NONE)
	{
		const FDCShot& S = Shots[CrewShot];
		Crew = SpawnRobot(ShotPoint(CrewShot, S.CrewNdc, S.Ref), CrewColor, EyeWarm, false);
		CrewPos = ShotPoint(CrewShot, S.CrewNdc, S.Ref);
		PlaceRobot(Crew, CrewPos, FVector2D(1.0, -0.3), S.CamRot, 35.f);
	}

	// ghost of the next shot (or of the removed scene)
	GhostTarget = NewObject<UTextureRenderTarget2D>(this);
	GhostTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	GhostTarget->ClearColor = FLinearColor::Black;
	GhostTarget->InitAutoFormat(960, 540);
	GhostTarget->UpdateResourceImmediate(true);

	GhostCapture = World->SpawnActor<ASceneCapture2D>(FVector::ZeroVector, FRotator::ZeroRotator);
	{
		USceneCaptureComponent2D* Cap = GhostCapture->GetCaptureComponent2D();
		Cap->TextureTarget = GhostTarget;
		Cap->FOVAngle = Fov;
		Cap->bCaptureEveryFrame = false; // refreshed about 12 times a second from Tick
		Cap->bCaptureOnMovement = false;
		Cap->bAlwaysPersistRenderingState = true;
		SetupLook(Cap->PostProcessSettings, ExposureEV, 0.f, 0.f);
		Cap->PostProcessBlendWeight = 1.f;
	}

	ThumbTarget = NewObject<UTextureRenderTarget2D>(this);
	ThumbTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	ThumbTarget->ClearColor = FLinearColor::Black;
	ThumbTarget->InitAutoFormat(256, 144);
	ThumbTarget->UpdateResourceImmediate(true);
	ThumbCapture = World->SpawnActor<ASceneCapture2D>(FlashCam->GetActorLocation(), FRotator::ZeroRotator);
	{
		USceneCaptureComponent2D* Cap = ThumbCapture->GetCaptureComponent2D();
		Cap->TextureTarget = ThumbTarget;
		Cap->FOVAngle = Fov;
		Cap->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Cap->bCaptureEveryFrame = false;
		Cap->bCaptureOnMovement = false;
		Cap->PostProcessSettings = FlashCam->GetCameraComponent()->PostProcessSettings;
		Cap->PostProcessBlendWeight = 1.f;
	}
}

// ---------------------------------------------------------------------------------------------
// looks
// ---------------------------------------------------------------------------------------------

void AMatchCutGameMode::ApplyLook(const FString& Name)
{
	if (const FDCLook* Found = Looks.Find(Name))
	{
		Look = *Found;
		LookName = Name;
	}
	else
	{
		UE_LOG(LogMatchCut, Warning, TEXT("Look %s not found in the layout, keeping %s"), *Name, *LookName);
	}

	UMaterialInterface* Parent = Look.bToon ? ToonMaterial : LitMaterial;
	for (FDCPaint& P : Paints)
	{
		if (UStaticMeshComponent* C = P.Comp.Get())
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, C);
			MID->SetVectorParameterValue(TEXT("Color"), P.Color);
			MID->SetVectorParameterValue(TEXT("Emissive"), P.Emissive + P.Color * AmbientNow);
			C->SetMaterial(0, MID);
			P.MID = MID;
		}
	}
	for (UMaterialInstanceDynamic* MID : BackdropMIDs)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Look.Backdrop);
	}
	for (const TWeakObjectPtr<AActor>& Slat : ShaftSlats)
	{
		if (AStaticMeshActor* A = Cast<AStaticMeshActor>(Slat.Get()))
		{
			A->GetStaticMeshComponent()->SetCastShadow(Look.bShafts);
			A->SetActorHiddenInGame(!Look.bShafts);
		}
	}
	for (const TWeakObjectPtr<ACameraActor>& Cam : AllCameras)
	{
		if (Cam.IsValid() && Cam.Get() != FlashCam.Get())
		{
			FPostProcessSettings& PP = Cam->GetCameraComponent()->PostProcessSettings;
			PP.VignetteIntensity = Look.Vignette;
			PP.FilmGrainIntensity = Look.Grain;
		}
	}
	if (USpotLightComponent* L = LampSpot.Get())
	{
		L->SetIntensity(Look.LampIntensity);
	}
	if (UPointLightComponent* G = LampGlow.Get())
	{
		G->SetIntensity(Look.GlowIntensity);
	}
	if (Fog)
	{
		UExponentialHeightFogComponent* FC = Fog->GetComponent();
		FC->SetFogDensity(Look.FogDensity);
		FC->SetFogHeightFalloff(0.001f);
		FC->SetFogMaxOpacity(1.f);
		FC->SetStartDistance(0.f);
		FC->SetVolumetricFog(Look.bVolumetric);
		FC->SetVolumetricFogDistance(6000.f);
		FC->SetVolumetricFogExtinctionScale(Look.VolExtinction);
		FC->SetVolumetricFogScatteringDistribution(0.5f);
	}
	ApplyLighting();
	UE_LOG(LogMatchCut, Display, TEXT("LOOK %s fog=%.2f volumetric=%d toon=%d shafts=%d"), *LookName, Look.FogDensity, Look.bVolumetric ? 1 : 0, Look.bToon ? 1 : 0, Look.bShafts ? 1 : 0);
}

void AMatchCutGameMode::ApplyLighting()
{
	// "dark" is global: only one place is on screen at a time, and the ghost capture does not use lights
	bool bDark = State == EDCState::Flashback;
	if ((State == EDCState::Playing || State == EDCState::TakeCard || State == EDCState::Drop || State == EDCState::EndCard) && Shots.IsValidIndex(Cur))
	{
		bDark = Shots[Cur].bDark;
	}
	float K = bDark ? Look.DarkFactor : 1.f;
	if (State == EDCState::Flashback)
	{
		K = FMath::Max(K, 0.30f); // old footage: dim, but the robots must read
	}
	AmbientNow = Look.bToon ? 0.f : Look.Ambient * K;
	for (FDCPaint& P : Paints)
	{
		if (UMaterialInstanceDynamic* MID = P.MID.Get())
		{
			MID->SetVectorParameterValue(TEXT("Emissive"), P.Emissive + P.Color * AmbientNow);
		}
	}
	if (KeyLight)
	{
		ULightComponent* LC = KeyLight->GetLightComponent();
		LC->SetIntensity((Look.bToon ? 0.f : Look.KeyIntensity) * K);
		LC->SetLightColor(Look.KeyColor);
		LC->SetVolumetricScatteringIntensity(Look.VolScatter);
	}
	if (FillLight)
	{
		ULightComponent* LC = FillLight->GetLightComponent();
		LC->SetIntensity((Look.bToon ? 0.f : Look.FillIntensity) * K);
		LC->SetLightColor(Look.FillColor);
		LC->SetVolumetricScatteringIntensity(0.f);
	}
	if (Fog)
	{
		UExponentialHeightFogComponent* FC = Fog->GetComponent();
		FC->SetFogInscatteringColor(Look.FogColor * K);
		FC->SetVolumetricFogEmissive(Look.bVolumetric ? Look.FogColor * (Look.VolEmissive * K) : FLinearColor::Black);
		FC->SetVolumetricFogAlbedo(FColor(140, 235, 215));
	}
	for (UMaterialInstanceDynamic* MID : BackdropMIDs)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Look.Backdrop * K);
	}
	if (Look.bToon)
	{
		// the toon bands are emissive, so they need the brightness the lights would have given (exposure is fixed)
		const float Glow = (bDark ? FMath::Max(Look.DarkFactor, 0.03f) : 1.f) * 3.0f;
		for (FDCPaint& P : Paints)
		{
			if (UMaterialInstanceDynamic* MID = P.MID.Get())
			{
				MID->SetScalarParameterValue(TEXT("Glow"), Glow);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------
// start
// ---------------------------------------------------------------------------------------------

void AMatchCutGameMode::StartPlay()
{
	Super::StartPlay();

	if (!CubeMesh || !SphereMesh || !CylinderMesh || !LitMaterial || !ToonMaterial || !UnlitMaterial)
	{
		UE_LOG(LogMatchCut, Error, TEXT("Deep Cut could not start: a shape or material asset is missing."));
		return;
	}
	if (!LoadLayout())
	{
		UE_LOG(LogMatchCut, Error, TEXT("Deep Cut could not start: no layout."));
		return;
	}

	GAreScreenMessagesEnabled = false; // engine debug text would sit on top of the picture

	SpawnWorld();

	FString CmdLook;
	if (FParse::Value(FCommandLine::Get(), TEXT("MCLook="), CmdLook))
	{
		LookName = CmdLook;
	}
	ApplyLook(LookName);
	GhostCapture->GetCaptureComponent2D()->HiddenActors = GhostHidden;

	Mask.SetNumZeroed(MaskW * MaskH);
	HeroDepth = StartDepth;
	bIntroSeen = IFileManager::Get().FileExists(*IntroFlagPath());

	UpdateMovers();
	SetShot(0, true);
	UE_LOG(LogMatchCut, Display, TEXT("DEEPCUT started: %d shots, part in shot %d, crew in shot %d, intro seen before: %d"), Shots.Num(), PartShot, CrewShot, bIntroSeen ? 1 : 0);

	if (FParse::Param(FCommandLine::Get(), TEXT("MCSkipFilm")))
	{
		State = EDCState::Playing;
		ApplyLighting();
	}
	else
	{
		StartFilm();
	}
	LoadScript();
}

void AMatchCutGameMode::SetView(ACameraActor* Cam)
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (Cam)
		{
			PC->SetViewTarget(Cam);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// shot maths
// ---------------------------------------------------------------------------------------------

FVector AMatchCutGameMode::CamPoint(const FVector& Pos, const FRotator& Rot, const FVector2D& Ndc, double Depth) const
{
	const FRotationMatrix M(Rot);
	return Pos + (M.GetUnitAxis(EAxis::X) + M.GetUnitAxis(EAxis::Y) * (Ndc.X * TanH) + M.GetUnitAxis(EAxis::Z) * (Ndc.Y * TanV)) * Depth;
}

void AMatchCutGameMode::ProjectToShot(int32 Shot, const FVector& World, FVector2D& OutNdc, double& OutDepth) const
{
	const FDCShot& S = Shots[Shot];
	const FRotationMatrix M(S.CamRot);
	const FVector D = World - S.CamPos;
	OutDepth = FMath::Max(FVector::DotProduct(D, M.GetUnitAxis(EAxis::X)), 1.0);
	OutNdc.X = FVector::DotProduct(D, M.GetUnitAxis(EAxis::Y)) / OutDepth / TanH;
	OutNdc.Y = FVector::DotProduct(D, M.GetUnitAxis(EAxis::Z)) / OutDepth / TanV;
}

FBox2D AMatchCutGameMode::RockRect(const FDCShot& Shot, const FDCRock& Rock) const
{
	// camera-plane rectangle in world units; a drifting plate is wherever the shot clock puts it
	FVector2D Off(0.0, 0.0);
	if (Rock.bMoves)
	{
		const double Alpha = 0.5 - 0.5 * FMath::Cos(2.0 * UE_DOUBLE_PI * ShotClock / FMath::Max(Rock.Period, 0.1f));
		Off = Rock.MoveTo * Alpha;
	}
	const double Sx = TanH * Shot.Ref, Sy = TanV * Shot.Ref;
	return FBox2D(FVector2D((Rock.Rect.X + Off.X) * Sx, (Rock.Rect.Y + Off.Y) * Sy), FVector2D((Rock.Rect.Z + Off.X) * Sx, (Rock.Rect.W + Off.Y) * Sy));
}

bool AMatchCutGameMode::Blocked(int32 ShotIndex, const FVector2D& Ndc, double Depth) const
{
	const FDCShot& Shot = Shots[ShotIndex];
	const FVector2D P(Ndc.X * TanH * Depth, Ndc.Y * TanV * Depth);
	for (const FDCRock& Rock : Shot.Rocks)
	{
		const FBox2D Box = RockRect(Shot, Rock);
		const FVector2D C(FMath::Clamp(P.X, Box.Min.X, Box.Max.X), FMath::Clamp(P.Y, Box.Min.Y, Box.Max.Y));
		if (FVector2D::DistSquared(P, C) < FMath::Square((double)HeroRadius))
		{
			return true;
		}
	}
	return false;
}

FDCCutEval AMatchCutGameMode::EvalCut(const FVector2D& Ndc, double Depth, bool bApplyLamp) const
{
	FDCCutEval E;
	E.Depth = Depth;

	if (InRestore())
	{
		// restore-by-cut: line the robot up with Unit 7 in the removed frame
		const double Dx = (Ndc.X - RestoreNdc.X) * AspectRatio;
		const double Dy = Ndc.Y - RestoreNdc.Y;
		E.bOk = FMath::Sqrt(Dx * Dx + Dy * Dy) <= RestoreTol;
		E.Reason = E.bOk ? TEXT("") : DCText::ReasonLineUp;
		return E;
	}

	const int32 Next = Cur + 1;
	if (!Shots.IsValidIndex(Next))
	{
		E.Reason = DCText::ReasonLast;
		return E;
	}
	const FDCShot& B = Shots[Next];
	// the match cut: same place on screen, same distance to the camera (so the same size), clamped to B's slab
	E.Depth = FMath::Clamp(Depth, B.DepthMin, B.DepthMax);
	E.Target = ShotPoint(Next, Ndc, E.Depth);

	if (FMath::Abs(Ndc.X) > 1.0 || FMath::Abs(Ndc.Y) > PictureHalf)
	{
		E.Reason = DCText::ReasonFrame;
	}
	else if (bApplyLamp && B.bDark && !bLampOn)
	{
		E.Reason = bHasPart ? DCText::ReasonDarkLamp : DCText::ReasonDark;
	}
	else if (Ndc.X < B.Swim.X || Ndc.X > B.Swim.Z || Ndc.Y < B.Swim.Y || Ndc.Y > B.Swim.W)
	{
		E.Reason = DCText::ReasonWater;
	}
	else if (Blocked(Next, Ndc, E.Depth))
	{
		E.Reason = DCText::ReasonRock;
	}
	else
	{
		E.bOk = true;
	}
	return E;
}

void AMatchCutGameMode::UpdateMask()
{
	for (int32 y = 0; y < MaskH; ++y)
	{
		for (int32 x = 0; x < MaskW; ++x)
		{
			const FVector2D Ndc(-1.0 + (x + 0.5) * 2.0 / MaskW, PictureHalf - (y + 0.5) * 2.0 * PictureHalf / MaskH);
			Mask[y * MaskW + x] = EvalCut(Ndc, HeroDepth, false).bOk ? 1 : 0;
		}
	}
	bMaskValid = true;
}

// ---------------------------------------------------------------------------------------------
// the cut
// ---------------------------------------------------------------------------------------------

bool AMatchCutGameMode::TryCut()
{
	if (State != EDCState::Playing)
	{
		return false;
	}

	FVector2D Ndc;
	double Depth;
	ProjectToShot(Cur, HeroPos, Ndc, Depth);
	const FDCCutEval E = EvalCut(Ndc, Depth, true);

	if (InRestore())
	{
		const double ErrPx = FVector2D((Ndc.X - RestoreNdc.X) * 960.0, (Ndc.Y - RestoreNdc.Y) * 540.0).Size();
		if (!E.bOk)
		{
			UE_LOG(LogMatchCut, Display, TEXT("RESTORE NG reason=\"%s\" ndc=(%.4f,%.4f) target=(%.4f,%.4f) off=%.0fpx@1080p take=%d"), E.Reason, Ndc.X, Ndc.Y, RestoreNdc.X, RestoreNdc.Y, ErrPx, Take);
			FailTake(E.Reason);
			return false;
		}
		UE_LOG(LogMatchCut, Display, TEXT("RESTORE OK ndc=(%.4f,%.4f) target=(%.4f,%.4f) off=%.0fpx@1080p"), Ndc.X, Ndc.Y, RestoreNdc.X, RestoreNdc.Y, ErrPx);
		Take = 1;
		StartFlashback();
		return true;
	}

	const int32 Next = Cur + 1;
	if (!E.bOk)
	{
		UE_LOG(LogMatchCut, Display, TEXT("CUT %d->%d NG reason=\"%s\" ndc=(%.4f,%.4f) depth=%.0f lamp=%d first=%d take=%d"), Cur, Next, E.Reason, Ndc.X, Ndc.Y, Depth, bLampOn ? 1 : 0, bFirstCutDone ? 0 : 1, Take);
		if (!Shots.IsValidIndex(Next))
		{
			return false;
		}
		if (!bFirstCutDone)
		{
			// the first cut cannot be failed: no clapper, no restart, just say where to go
			SoftFailTime = 2.5f;
			return false;
		}
		FailTake(E.Reason);
		return false;
	}

	HeroPos = E.Target;
	HeroDepth = E.Depth;
	// the velocity lives in screen axes, so it is already expressed in the new camera's frame (yaw and pitch)
	FVector2D After;
	double AfterDepth;
	ProjectToShot(Next, HeroPos, After, AfterDepth);
	const double ErrPx = FVector2D((After.X - Ndc.X) * 960.0, (After.Y - Ndc.Y) * 540.0).Size();
	UE_LOG(LogMatchCut, Display, TEXT("CUT %d->%d OK ndc=(%.4f,%.4f) depth=%.0f->%.0f ndcAfter=(%.4f,%.4f) err=%.2fpx@1080p vel2d=(%.0f,%.0f) lamp=%d"),
		Cur, Next, Ndc.X, Ndc.Y, Depth, AfterDepth, After.X, After.Y, ErrPx, Vel2D.X, Vel2D.Y, bLampOn ? 1 : 0);

	bFirstCutDone = true;
	SoftFailTime = 0.f;
	CutFlash = 0.5f;
	Take = 1;
	SetShot(Next, false);
	if (bCrewRescued)
	{
		CrewPos = HeroPos; // the rescued robot comes through the cut with us
	}
	if (Cur == Shots.Num() - 1)
	{
		EndTimer = 3.5f;
	}
	return true;
}

void AMatchCutGameMode::FailTake(const TCHAR* Reason)
{
	State = EDCState::TakeCard;
	StateTime = 0.f;
	TakeReason = Reason;
	++Take;
	Vel2D = FVector2D::ZeroVector;
}

void AMatchCutGameMode::PressCut()
{
	if (State == EDCState::Playing)
	{
		TryCut();
	}
}

void AMatchCutGameMode::PressLamp()
{
	if (State == EDCState::Playing && bHasPart)
	{
		bLampOn = !bLampOn;
		UE_LOG(LogMatchCut, Display, TEXT("LAMP %s"), bLampOn ? TEXT("on") : TEXT("off"));
	}
}

void AMatchCutGameMode::PressRetake()
{
	if (State == EDCState::EndCard)
	{
		RestartAll();
	}
	else if (State == EDCState::Playing)
	{
		++Take;
		UE_LOG(LogMatchCut, Display, TEXT("RETAKE from shot %d"), Cur);
		SetShot(Cur > 0 ? Cur - 1 : 0, true);
	}
}

void AMatchCutGameMode::PressAnyKey()
{
	if (State == EDCState::Film && bFilmSkippable && StateTime > 0.5f)
	{
		UE_LOG(LogMatchCut, Display, TEXT("FILM skipped at %.1fs"), FilmTime);
		EndFilm();
	}
}

void AMatchCutGameMode::RestartAll()
{
	bFirstCutDone = false;
	bHasPart = false;
	bLampOn = false;
	bCrewRescued = false;
	bGapFilled[0] = bGapFilled[1] = bGapFilled[2] = false;
	bThumbTaken = false;
	EndTimer = -1.f;
	Take = 1;
	HeroDepth = StartDepth;
	Vel2D = FVector2D::ZeroVector;
	BannerTime = 0.f;
	if (AActor* A = PartActor.Actor.Get())
	{
		A->SetActorHiddenInGame(false);
	}
	if (CrewShot != INDEX_NONE)
	{
		CrewPos = ShotPoint(CrewShot, Shots[CrewShot].CrewNdc, Shots[CrewShot].Ref);
		PlaceRobot(Crew, CrewPos, FVector2D(1.0, -0.3), Shots[CrewShot].CamRot, 35.f);
	}
	State = EDCState::Playing;
	SetShot(0, true);
	UE_LOG(LogMatchCut, Display, TEXT("RESTART"));
}

void AMatchCutGameMode::ShowBanner(const FString& Line1, const FString& Line2, float Seconds)
{
	Banner1 = Line1;
	Banner2 = Line2;
	BannerTime = Seconds;
}

// ---------------------------------------------------------------------------------------------
// flow
// ---------------------------------------------------------------------------------------------

void AMatchCutGameMode::SetShot(int32 Index, bool bRespawn)
{
	ReportFrameTime();
	Cur = FMath::Clamp(Index, 0, Shots.Num() - 1);
	SetView(Shots[Cur].Camera.Get());
	if (bRespawn)
	{
		RespawnAtStart();
	}
	PointGhost();
	ApplyLighting();
	bMaskValid = false;
	MaskTimer = 0.f;
	UE_LOG(LogMatchCut, Display, TEXT("SHOT %d \"%s\" dark=%d"), Cur, *Shots[Cur].Name, Shots[Cur].bDark ? 1 : 0);
}

void AMatchCutGameMode::PointGhost()
{
	if (!GhostCapture)
	{
		return;
	}
	USceneCaptureComponent2D* Cap = GhostCapture->GetCaptureComponent2D();
	// Mode 1 captures flat base colour: no lights and no fog reach it, so the water colour cannot wash it out.
	// Mode 2 is the brief-01 ghost (the lit frame, fog and all), kept for the wash-out comparison.
	Cap->CaptureSource = GhostMode == 2 ? ESceneCaptureSource::SCS_FinalColorLDR : ESceneCaptureSource::SCS_BaseColor;
	if (InRestore())
	{
		PoseFlashback(0.f);
		GhostCapture->SetActorLocationAndRotation(FlashCam->GetActorLocation(), FRotator::ZeroRotator);
	}
	else if (Shots.IsValidIndex(Cur + 1))
	{
		GhostCapture->SetActorLocationAndRotation(Shots[Cur + 1].CamPos, Shots[Cur + 1].CamRot);
	}
	GhostTimer = 0.f;
}

void AMatchCutGameMode::RespawnAtStart()
{
	const FDCShot& Shot = Shots[Cur];
	HeroDepth = FMath::Clamp(HeroDepth, Shot.DepthMin, Shot.DepthMax);
	HeroPos = ShotPoint(Cur, Shot.Start, HeroDepth);
	Vel2D = FVector2D::ZeroVector;
	if (bCrewRescued)
	{
		CrewPos = HeroPos;
	}
}

void AMatchCutGameMode::StartFilm()
{
	State = EDCState::Film;
	StateTime = 0.f;
	FilmTime = 0.f;
	bFilmSkippable = bIntroSeen;
	ApplyLighting();
	UE_LOG(LogMatchCut, Display, TEXT("FILM start length=%.1fs skippable=%d"), FilmLength, bFilmSkippable ? 1 : 0);
}

void AMatchCutGameMode::EndFilm()
{
	if (!bIntroSeen)
	{
		bIntroSeen = true;
		FFileHelper::SaveStringToFile(TEXT("seen"), *IntroFlagPath());
	}
	State = EDCState::Drop;
	StateTime = 0.f;
	Cur = 0;
	SetView(Shots[0].Camera.Get());
	ApplyLighting();
	UE_LOG(LogMatchCut, Display, TEXT("FILM end -> drop into shot 0"));
}

void AMatchCutGameMode::StartFlashback()
{
	State = EDCState::Flashback;
	StateTime = 0.f;
	bThumbTaken = false;
	SetView(FlashCam.Get());
	ApplyLighting();
	UE_LOG(LogMatchCut, Display, TEXT("FLASHBACK start (removed scene 1, %.0fs)"), FlashLength);
}

void AMatchCutGameMode::EndFlashback()
{
	bGapFilled[0] = true;
	State = EDCState::Playing;
	StateTime = 0.f;
	FlashCam->SetActorRotation(FRotator::ZeroRotator);
	SetShot(Cur, false);
	ShowBanner(FString::Printf(DCTEXT_RESTORED, 1, NumGaps), DCText::DarkHint, 7.f);
	UE_LOG(LogMatchCut, Display, TEXT("FLASHBACK end: gap 1 filled"));
}

void AMatchCutGameMode::UpdateMovers()
{
	for (FDCShot& Shot : Shots)
	{
		for (FDCRock& Rock : Shot.Rocks)
		{
			if (Rock.bMoves && Rock.Actor.IsValid())
			{
				const double Alpha = 0.5 - 0.5 * FMath::Cos(2.0 * UE_DOUBLE_PI * ShotClock / FMath::Max(Rock.Period, 0.1f));
				Rock.Actor->SetActorLocation(Rock.BaseLoc + Rock.MoveWorld * Alpha);
			}
		}
	}
}

void AMatchCutGameMode::PoseFilm(int32 Set, float T)
{
	const FVector O = FilmOrigin[Set];
	const FRotator CamRot = FRotator::ZeroRotator;
	for (int32 i = 0; i < FilmRobots.Num(); ++i)
	{
		FVector P = O;
		FVector2D Head(1.0, 0.0);
		float Roll = 0.f;
		bool bShow = true;
		FLinearColor Eye = EyeWarm;
		switch (Set)
		{
		case 0: // launch: they hang under the platform and drop one by one
		{
			const float Tau = FMath::Max(0.f, T - 0.6f - 0.45f * i);
			P += FVector(0.0, (i - 3) * 150.0, 150.0 - 130.0 * Tau * Tau - 20.0 * Tau);
			Head = FVector2D(1.0, Tau > 0.f ? -0.6 : 0.0);
			break;
		}
		case 1: // descent
			P += FVector(160.0 * FMath::Sin(i * 2.3f), (i - 3) * 135.0 + 12.0 * FMath::Sin(T * 1.3f + i), 330.0 - 95.0 * T + 60.0 * FMath::Sin(i * 1.7f));
			Head = FVector2D(i % 2 ? 1.0 : -1.0, -0.6);
			break;
		case 2: // on the sea floor, sweeping right
			P += FVector(120.0 * FMath::Sin(i * 1.9f), (i - 3) * 150.0 - 120.0 + 50.0 * T, -95.0 + 14.0 * FMath::Sin(T * 2.2f + i));
			Head = FVector2D(1.0, 0.1 * FMath::Sin(T + i));
			break;
		default: // Unit 7 alone, dark, lifted away on a cable
			bShow = i == 6;
			P += FVector(0.0, 0.0, -150.0 + 42.0 * T);
			Head = FVector2D(1.0, -0.35);
			Roll = 28.f + 6.f * FMath::Sin(T * 0.9f);
			Eye = FLinearColor::Black;
			break;
		}
		if (AActor* A = FilmRobots[i].Actor.Get())
		{
			A->SetActorHiddenInGame(!bShow);
		}
		SetPaintEmissive(FilmRobots[i].EyePaint, Eye);
		PlaceRobot(FilmRobots[i], P, Head, CamRot, Roll);
		if (Set == 3 && i == 6 && FilmCable.IsValid())
		{
			FilmCable->SetActorLocation(P + FVector(0.0, 0.0, 385.0));
		}
	}
}

void AMatchCutGameMode::PoseFlashback(float T)
{
	// Unit 7 holds still where the player matched it and keeps its lamp on the gap; three of the crew pass through the beam
	const FVector CamPos = FlashOrigin + FVector(-1000.0, 0.0, 0.0);
	const FVector U7 = CamPoint(CamPos, FRotator::ZeroRotator, RestoreNdc, HeroDepth);
	const float Sway = 0.12f * FMath::Sin(T * 1.4f);
	PlaceRobot(FlashUnit7, U7 + FVector(0.0, 0.0, 6.0 * FMath::Sin(T * 2.0f)), FVector2D(1.0, 0.22 + Sway), FRotator::ZeroRotator, 0.f);
	if (USpotLightComponent* L = FlashLamp.Get())
	{
		// the lamp stutters once near the end
		const bool bStutter = T > 9.2f && T < 9.9f && FMath::Fmod(T * 14.f, 2.f) < 1.f;
		L->SetIntensity(bStutter ? 6000.f : 60000.f);
	}
	for (int32 i = 0; i < FlashCrew.Num(); ++i)
	{
		const float Ti = T - 1.2f - 2.4f * i;
		const double X = -1.25 + 0.36 * FMath::Max(Ti, 0.f);
		const FVector2D N(RestoreNdc.X + 0.2 + X, RestoreNdc.Y + 0.42 + 0.05 * FMath::Sin(Ti * 2.f + i));
		PlaceRobot(FlashCrew[i], CamPoint(CamPos, FRotator::ZeroRotator, N, HeroDepth + 140.0 + 60.0 * i), FVector2D(1.0, 0.1 * FMath::Sin(Ti * 2.f)), FRotator::ZeroRotator, 0.f);
	}
}

void AMatchCutGameMode::TickFilm(float Dt)
{
	FilmTime += Dt;
	if (FilmTime >= FilmLength + FilmEndHold)
	{
		EndFilm();
		return;
	}
	for (const FDCReelSeg& S : Reel)
	{
		if (FilmTime >= S.T0 && FilmTime < S.T1 && S.Gap == INDEX_NONE)
		{
			SetView(FilmCam[S.Set].Get());
			PoseFilm(S.Set, FilmTime - S.T0);
		}
	}
}

void AMatchCutGameMode::TickDrop(float Dt)
{
	// the film lets go of the robot: it sinks into the first shot from above the frame
	const FDCShot& Shot = Shots[0];
	const float A = FMath::Clamp(StateTime / DropSeconds, 0.f, 1.f);
	const float Ease = 1.f - FMath::Square(1.f - A);
	const FVector2D N(Shot.Start.X, FMath::Lerp(1.05, Shot.Start.Y, (double)Ease));
	HeroPos = ShotPoint(0, N, HeroDepth);
	Heading = FVector2D(1.0, -0.7 * (1.0 - Ease));
	if (StateTime >= DropSeconds)
	{
		State = EDCState::Playing;
		StateTime = 0.f;
		SetShot(0, true);
		UE_LOG(LogMatchCut, Display, TEXT("DROP done: playing"));
	}
}

void AMatchCutGameMode::MoveHero(const FVector2D& Delta)
{
	// Everything the robot can touch is a rectangle in the camera plane, so the collision is done there:
	// a circle against rectangles, in world units at the robot's depth. (The engine cube's physics body is
	// hollow to overlap tests, and a 2D test is also exactly what Tools/cutmap.py mirrors.)
	const FDCShot& Shot = Shots[Cur];
	FVector2D Ndc;
	double Depth;
	ProjectToShot(Cur, HeroPos, Ndc, Depth);
	FVector2D P(Ndc.X * TanH * HeroDepth, Ndc.Y * TanV * HeroDepth);
	const double R = HeroRadius;

	const int32 Steps = FMath::Clamp(FMath::CeilToInt32(Delta.Size() / (R * 0.4)), 1, 12);
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		P += Delta / Steps;
		for (int32 Iter = 0; Iter < 3; ++Iter)
		{
			bool bHit = false;
			for (const FDCRock& Rock : Shot.Rocks)
			{
				const FBox2D Box = RockRect(Shot, Rock);
				const FVector2D C(FMath::Clamp(P.X, Box.Min.X, Box.Max.X), FMath::Clamp(P.Y, Box.Min.Y, Box.Max.Y));
				FVector2D D = P - C;
				const double Dist = D.Size();
				if (Dist >= R)
				{
					continue;
				}
				FVector2D N;
				if (Dist > 0.01)
				{
					N = D / Dist;
					P = C + N * R;
				}
				else
				{
					// the centre is inside (a drifting plate ran over us): leave through the nearest side
					const double L = P.X - Box.Min.X, Rt = Box.Max.X - P.X, B = P.Y - Box.Min.Y, T = Box.Max.Y - P.Y;
					const double M = FMath::Min(FMath::Min(L, Rt), FMath::Min(B, T));
					if (M == L) { N = FVector2D(-1, 0); P.X = Box.Min.X - R; }
					else if (M == Rt) { N = FVector2D(1, 0); P.X = Box.Max.X + R; }
					else if (M == B) { N = FVector2D(0, -1); P.Y = Box.Min.Y - R; }
					else { N = FVector2D(0, 1); P.Y = Box.Max.Y + R; }
				}
				const double Into = FVector2D::DotProduct(Vel2D, N);
				if (Into < 0.0)
				{
					Vel2D -= N * Into; // slide
				}
				bHit = true;
			}
			if (!bHit)
			{
				break;
			}
		}
	}

	// hard stop at the edge of the swim area (the soft push happens before this, in TickPlaying)
	FVector2D Out(P.X / (TanH * HeroDepth), P.Y / (TanV * HeroDepth));
	const FVector2D Clamped(FMath::Clamp(Out.X, Shot.Swim.X, Shot.Swim.Z), FMath::Clamp(Out.Y, Shot.Swim.Y, Shot.Swim.W));
	if (Clamped.X != Out.X) { Vel2D.X = 0.0; }
	if (Clamped.Y != Out.Y) { Vel2D.Y = 0.0; }
	HeroPos = ShotPoint(Cur, Clamped, HeroDepth);
}

void AMatchCutGameMode::UpdateHeroActor(float Dt)
{
	if (!Shots.IsValidIndex(Cur))
	{
		return;
	}
	const FDCShot& Shot = Shots[Cur];
	PlaceRobot(Hero, HeroPos, Heading, Shot.CamRot, 0.f);
	const bool bLamp = bLampOn && (State == EDCState::Playing || State == EDCState::TakeCard || State == EDCState::EndCard);
	if (USpotLightComponent* L = LampSpot.Get())
	{
		L->SetVisibility(bLamp);
	}
	if (UPointLightComponent* G = LampGlow.Get())
	{
		G->SetVisibility(bLamp);
		const FRotationMatrix M(Shot.CamRot);
		G->SetWorldLocation(HeroPos - M.GetUnitAxis(EAxis::X) * 240.0 + M.GetUnitAxis(EAxis::Z) * 40.0);
	}

	if (CrewShot != INDEX_NONE && bCrewRescued)
	{
		// simple follow: trail behind and a little above
		const FRotationMatrix M(Shot.CamRot);
		const FVector Want = HeroPos - M.GetUnitAxis(EAxis::Y) * (Heading.X >= 0.0 ? 95.0 : -95.0) + M.GetUnitAxis(EAxis::Z) * 45.0;
		CrewPos = FMath::VInterpTo(CrewPos, Want, Dt, 3.0f);
		PlaceRobot(Crew, CrewPos, FVector2D(Heading.X, 0.0), Shot.CamRot, 0.f);
	}
}

void AMatchCutGameMode::TickPlaying(float Dt)
{
	const FDCShot& Shot = Shots[Cur];

	FVector2D In = MoveInput;
	if (bScriptActive)
	{
		In = ScriptMoveLeft > 0.f ? ScriptMove : FVector2D::ZeroVector;
		if (bScriptSwimming || bScriptHold)
		{
			const FVector2D To((ScriptSwimTarget.X - HeroNdc.X) * AspectRatio, ScriptSwimTarget.Y - HeroNdc.Y);
			In = To.GetSafeNormal() * FMath::Clamp(To.Size() / 0.12, 0.2, 1.0);
		}
	}
	if (In.SizeSquared() > 1.0)
	{
		In.Normalize();
	}

	Vel2D += In * Accel * Dt;
	// with no input the robot brakes harder, so it settles within about one body length
	Vel2D -= Vel2D * FMath::Min((In.IsNearlyZero() ? Drag * 2.5f : Drag) * Dt, 1.f);
	if (Vel2D.Size() > MaxSpeed)
	{
		Vel2D = Vel2D.GetSafeNormal() * MaxSpeed;
	}
	// soft walls: the water pushes back near the edge of the swim area
	const double SoftX = 0.07, SoftY = 0.10, Push = 2600.0;
	if (HeroNdc.X > Shot.Swim.Z - SoftX) { Vel2D.X -= (HeroNdc.X - (Shot.Swim.Z - SoftX)) / SoftX * Push * Dt; }
	if (HeroNdc.X < Shot.Swim.X + SoftX) { Vel2D.X += ((Shot.Swim.X + SoftX) - HeroNdc.X) / SoftX * Push * Dt; }
	if (HeroNdc.Y > Shot.Swim.W - SoftY) { Vel2D.Y -= (HeroNdc.Y - (Shot.Swim.W - SoftY)) / SoftY * Push * Dt; }
	if (HeroNdc.Y < Shot.Swim.Y + SoftY) { Vel2D.Y += ((Shot.Swim.Y + SoftY) - HeroNdc.Y) / SoftY * Push * Dt; }

	MoveHero(Vel2D * Dt);

	if (!In.IsNearlyZero(0.05))
	{
		const FVector2D Want(In.X, In.Y);
		Heading = FMath::Vector2DInterpTo(Heading, FVector2D(FMath::Abs(Want.X) > 0.1 ? Want.X : Heading.X, Want.Y), Dt, 8.f);
	}
	else
	{
		Heading.Y = FMath::FInterpTo(Heading.Y, 0.0, Dt, 2.f);
	}

	double Depth;
	ProjectToShot(Cur, HeroPos, HeroNdc, Depth);

	// the part
	if (Cur == PartShot && !bHasPart && PartActor.Actor.IsValid())
	{
		const FVector P = ShotPoint(PartShot, Shot.PartNdc, HeroDepth);
		PartPulse += Dt;
		PartActor.Actor->SetActorLocationAndRotation(P + FVector(0, 0, 8.0 * FMath::Sin(PartPulse * 2.5f)), FRotator(0.0, PartPulse * 60.0, 0.0));
		if (FVector::Dist(P, HeroPos) < HeroRadius + 45.f)
		{
			bHasPart = true;
			PartActor.Actor->SetActorHiddenInGame(true);
			ShowBanner(DCText::PartFound, DCText::PartHow, 5.f);
			UE_LOG(LogMatchCut, Display, TEXT("PART picked up in shot %d: lamp granted, restore-by-cut armed"), Cur);
			PointGhost();
			bMaskValid = false;
			MaskTimer = 0.f;
		}
	}

	// the rescue
	if (Cur == CrewShot && !bCrewRescued && FVector::Dist(CrewPos, HeroPos) < HeroRadius * 2.f + 40.f)
	{
		bCrewRescued = true;
		ShowBanner(DCText::CrewFound, FString::Printf(DCTEXT_CREW, 1, CrewTotal), 4.f);
		UE_LOG(LogMatchCut, Display, TEXT("RESCUE crew robot freed in shot %d (lamp=%d): CREW 1/%d"), Cur, bLampOn ? 1 : 0, CrewTotal);
	}

	MaskTimer -= Dt;
	if (MaskTimer <= 0.f || !bMaskValid)
	{
		UpdateMask();
		MaskTimer = 0.15f;
	}
	CurEval = EvalCut(HeroNdc, HeroDepth, true);

	if (EndTimer > 0.f)
	{
		EndTimer -= Dt;
		if (EndTimer <= 0.f)
		{
			State = EDCState::EndCard;
			StateTime = 0.f;
			UE_LOG(LogMatchCut, Display, TEXT("END CARD scenes=%d/%d crew=%d/%d"), bGapFilled[0] ? 1 : 0, NumGaps, bCrewRescued ? 1 : 0, CrewTotal);
		}
	}
}

void AMatchCutGameMode::TickFlashback(float Dt)
{
	PoseFlashback(StateTime);
	// gate weave: the frame jitters a little, twelve times a second
	WeaveTimer -= Dt;
	if (WeaveTimer <= 0.f)
	{
		WeaveTimer = 1.f / 12.f;
		Weave = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f));
		FlashCam->SetActorRotation(FRotator(Weave.Y * 0.10, Weave.X * 0.10, Weave.X * 0.12));
	}
	if (!bThumbTaken && StateTime > 4.0f)
	{
		bThumbTaken = true;
		ThumbCapture->GetCaptureComponent2D()->CaptureScene(); // the thumbnail that fills the gap in the reel
	}
	if (StateTime >= FlashLength)
	{
		EndFlashback();
	}
}

void AMatchCutGameMode::ReportFrameTime()
{
	if (FrameCount > 0)
	{
		UE_LOG(LogMatchCut, Display, TEXT("FRAMETIME shot=%d look=%s state=%s ghost=%d lamp=%d frames=%d avg=%.2fms max=%.2fms"),
			Cur, *LookName, StateName(), GhostMode, bLampOn ? 1 : 0, FrameCount, FrameAccum / FrameCount * 1000.0, FrameMax * 1000.0);
	}
	FrameAccum = 0.0;
	FrameMax = 0.0;
	FrameCount = 0;
	ShotElapsed = 0.f;
}

const TCHAR* AMatchCutGameMode::StateName() const
{
	switch (State)
	{
	case EDCState::Film: return TEXT("Film");
	case EDCState::Drop: return TEXT("Drop");
	case EDCState::Playing: return TEXT("Playing");
	case EDCState::TakeCard: return TEXT("TakeCard");
	case EDCState::Flashback: return TEXT("Flashback");
	default: return TEXT("EndCard");
	}
}

void AMatchCutGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Shots.Num() == 0 || !Hero.Actor.IsValid())
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.1f); // long frames (shader hitches, background runs) must not tunnel the robot

	TickScript(DeltaSeconds);

	if (!bClockFrozen)
	{
		ShotClock += DeltaSeconds;
	}
	PlayClock += DeltaSeconds;
	UpdateMovers();

	// frame time, skipping the first second after each shot change (first-use shader hitches)
	ShotElapsed += DeltaSeconds;
	if (ShotElapsed > 1.f)
	{
		FrameAccum += DeltaSeconds;
		FrameMax = FMath::Max(FrameMax, (double)DeltaSeconds);
		++FrameCount;
	}

	CutFlash = FMath::Max(0.f, CutFlash - DeltaSeconds);
	SoftFailTime = FMath::Max(0.f, SoftFailTime - DeltaSeconds);
	if (BannerTime > 0.f)
	{
		BannerTime -= DeltaSeconds;
	}
	if (ScriptMoveLeft > 0.f)
	{
		ScriptMoveLeft -= DeltaSeconds;
	}
	StateTime += DeltaSeconds;

	switch (State)
	{
	case EDCState::Film:
		TickFilm(Dt);
		break;
	case EDCState::Drop:
		TickDrop(Dt);
		break;
	case EDCState::Playing:
		TickPlaying(Dt);
		break;
	case EDCState::TakeCard:
		if (StateTime >= TakeSeconds)
		{
			State = EDCState::Playing;
			StateTime = 0.f;
			RespawnAtStart();
		}
		break;
	case EDCState::Flashback:
		TickFlashback(Dt);
		break;
	default:
		break;
	}

	UpdateHeroActor(Dt);

	// refresh the ghost about 12 times a second; a second scene render every frame was the main cost in brief 01
	if (GhostCapture)
	{
		const bool bWant = GhostMode != 0 && (State == EDCState::Playing || State == EDCState::TakeCard || State == EDCState::Drop) && (InRestore() || Shots.IsValidIndex(Cur + 1));
		GhostTimer -= DeltaSeconds;
		if (bWant && GhostTimer <= 0.f)
		{
			GhostTimer = 0.08f;
			GhostCapture->GetCaptureComponent2D()->CaptureSceneDeferred();
		}
	}
}
