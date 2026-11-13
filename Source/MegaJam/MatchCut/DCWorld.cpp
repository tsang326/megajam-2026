// Deep Cut: spawns the stage from the loaded data. Greybox: everything is made of engine shapes.

#include "MatchCutGameMode.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace DCColor
{
	const FLinearColor Rock(0.30f, 0.28f, 0.25f);
	const FLinearColor RockDark(0.20f, 0.20f, 0.19f);
	const FLinearColor Plate(0.95f, 0.50f, 0.06f); // safety orange: a loose thing never reads as rock (brief 05)
	const FLinearColor Hull(0.26f, 0.30f, 0.33f);
	const FLinearColor Decor(0.10f, 0.12f, 0.13f);
	const FLinearColor Hero(0.36f, 0.45f, 0.42f);
	const FLinearColor Crew(0.85f, 0.60f, 0.08f);
	const FLinearColor Unit7(0.80f, 0.78f, 0.72f);
	const FLinearColor DarkMetal(0.06f, 0.06f, 0.07f);
	const FLinearColor EyeCyan(0.2f, 3.0f, 3.5f);
	const FLinearColor EyeWarm(4.0f, 2.6f, 0.6f);
	const FLinearColor ValveRed(0.75f, 0.10f, 0.06f);

	FLinearColor ForKind(FName Kind)
	{
		if (Kind == TEXT("plate")) { return FLinearColor(0.50f, 0.26f, 0.10f); } // rusty metal scenery, not the loose plate
		if (Kind == TEXT("hull")) { return Hull; }
		if (Kind == TEXT("metal")) { return DarkMetal; }
		if (Kind == TEXT("dark")) { return RockDark; }
		return Rock;
	}
}

namespace
{
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
}

ACameraActor* AMatchCutGameMode::SpawnCamera(const FVector& Pos, const FRotator& Rot)
{
	ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(Pos, Rot);
	UCameraComponent* CC = Cam->GetCameraComponent();
	CC->SetFieldOfView(Fov);
	CC->SetAspectRatio(DC::Aspect);
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

AActor* AMatchCutGameMode::SpawnBox(const FVector& Center, const FRotator& Rot, const FVector& Size, const FLinearColor& Color, bool bHideFromGhost)
{
	AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Center, Rot);
	UStaticMeshComponent* SMC = Actor->GetStaticMeshComponent();
	SMC->SetMobility(EComponentMobility::Movable);
	SMC->SetStaticMesh(CubeMesh);
	Actor->SetActorScale3D(Size / 100.0);
	SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AddPaint(SMC, Color, FLinearColor::Black);
	if (bHideFromGhost)
	{
		GhostHidden.Add(Actor);
	}
	return Actor;
}

AActor* AMatchCutGameMode::SpawnGroup(const FVector& Loc)
{
	AActor* A = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Loc));
	USceneComponent* RootComp = NewObject<USceneComponent>(A);
	RootComp->SetMobility(EComponentMobility::Movable);
	A->SetRootComponent(RootComp);
	RootComp->RegisterComponent();
	A->SetActorLocation(Loc);
	return A;
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

AActor* AMatchCutGameMode::SpawnGentle(const FVector& Loc)
{
	// The gentle creature: big, slow, warm. Local frame: +X is forward, Z up; it is seen from the side.
	AActor* A = SpawnGroup(Loc);
	const FLinearColor Skin(0.95f, 0.62f, 0.28f);
	const FLinearColor Glow(1.5f, 0.75f, 0.2f);
	const FLinearColor Spot(7.f, 4.0f, 1.0f);
	AddPart(A, SphereMesh, FVector(0, 0, 0), FRotator::ZeroRotator, FVector(7.0, 2.6, 2.5), Skin, Glow);
	AddPart(A, SphereMesh, FVector(290, 0, -10), FRotator::ZeroRotator, FVector(3.0, 2.2, 1.9), Skin, Glow);
	AddPart(A, SphereMesh, FVector(-60, 0, 120), FRotator(-18, 0, 0), FVector(3.2, 0.5, 1.5), Skin, Glow * 0.8f); // soft back fin
	AddPart(A, SphereMesh, FVector(-380, 0, 10), FRotator(8, 0, 0), FVector(3.0, 1.2, 1.1), Skin, Glow * 0.8f);
	AddPart(A, SphereMesh, FVector(-560, 0, 30), FRotator(14, 0, 0), FVector(2.0, 0.7, 0.6), Skin, Glow * 0.7f);
	AddPart(A, SphereMesh, FVector(-700, 0, 55), FRotator(0, 0, 0), FVector(1.1, 0.4, 1.6), Skin, Glow * 0.7f); // tail fluke
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		AddPart(A, SphereMesh, FVector(60, Side * 150.0, -90), FRotator(0, 0, Side * 25.0), FVector(2.4, 1.6, 0.3), Skin, Glow * 0.8f); // side fin
		for (int32 i = 0; i < 7; ++i)
		{
			// a row of lights along each flank
			AddPart(A, SphereMesh, FVector(-250.0 + i * 85.0, Side * (118.0 + 8.0 * FMath::Sin(i * 1.3)), 25.0 - 6.0 * FMath::Abs(i - 3)), FRotator::ZeroRotator, FVector(0.22 - 0.015 * FMath::Abs(i - 3)), Spot.GetClamped(0.f, 1.f), Spot);
		}
		AddPart(A, SphereMesh, FVector(370, Side * 78.0, 20), FRotator::ZeroRotator, FVector(0.30, 0.16, 0.30), FLinearColor(0.02f, 0.03f, 0.05f), FLinearColor::Black); // calm dark eye
	}
	UPointLightComponent* G = NewObject<UPointLightComponent>(A);
	G->SetMobility(EComponentMobility::Movable);
	G->SetupAttachment(A->GetRootComponent());
	G->RegisterComponent();
	G->SetAbsolute(true, true, false); // placed by UpdateGentle, nearer to the camera than the body
	G->SetIntensityUnits(ELightUnits::Unitless);
	G->SetIntensity(GentleLightPower);
	G->SetLightColor(FLinearColor(1.f, 0.72f, 0.38f));
	G->SetAttenuationRadius(GentleLightRadius);
	G->SetCastShadows(false);
	GentleLight = G;
	return A;
}

FDCRobot AMatchCutGameMode::SpawnRobot(const FVector& Loc, const FLinearColor& Body, const FLinearColor& Eye, bool bBattered, const TCHAR* ArtId)
{
	FDCRobot Robot;
	AActor* A = SpawnGroup(Loc);
	const FLinearColor DarkMetal = DCColor::DarkMetal;
	// a model from the art drop stands upright in a box around the centre: about 96 cm tall, a little bigger than the
	// 36 cm ball that is Unit 0 for the rules
	if (ArtId[0] && AddArt(A, ArtId, FVector::ZeroVector, FVector(84.0, 84.0, 96.0)))
	{
		Robot.bArt = true;
		Robot.Actor = A;
		return Robot;
	}

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
	if (Robot.bPlain)
	{
		A->SetActorLocation(Loc);
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

void AMatchCutGameMode::SpawnSolid(FDCSolid& Solid, const FVector& CamPos, const FRotator& CamRot, bool bInGhost)
{
	const FRotationMatrix M(CamRot);
	const FVector Center = CamPos + M.GetUnitAxis(EAxis::X) * Solid.C.Z + M.GetUnitAxis(EAxis::Y) * Solid.C.X + M.GetUnitAxis(EAxis::Z) * Solid.C.Y;
	const FQuat Q = CamRot.Quaternion() * Solid.Rot.Quaternion();
	// actor axes: X = away from the camera, Y = right, Z = up
	const FVector Size(Solid.Size.Z, Solid.Size.X, Solid.Size.Y);
	if (Solid.Kind == TEXT("shoal"))
	{
		// not rock: a ball of small silver fish. The box is what Unit 0 bumps into.
		AActor* A = SpawnGroup(Center);
		A->SetActorRotation(Q);
		FRandomStream Rand(GetTypeHash(Center));
		const FLinearColor Fish(0.75f, 0.85f, 0.9f);
		for (int32 i = 0; i < 120; ++i)
		{
			const FVector P(Rand.FRandRange(-0.5f, 0.5f) * Size.X, Rand.FRandRange(-0.5f, 0.5f) * Size.Y, Rand.FRandRange(-0.5f, 0.5f) * Size.Z);
			AddPart(A, SphereMesh, P, FRotator(Rand.FRandRange(-15.f, 15.f), 90.f + Rand.FRandRange(-20.f, 20.f), 0.f), FVector(0.22, 0.045, 0.10), Fish, Fish * 0.9f);
		}
		GhostHidden.Add(A);
		Solid.Actor = A;
		Solid.World = FTransform(Q, Center);
		Solid.Half = Size * 0.5;
		Solid.BaseLoc = Center;
		return;
	}
	const FString ArtId = FString(TEXT("solid:")) + (Solid.Kind.IsNone() ? FString(TEXT("rock")) : Solid.Kind.ToString());
	if (Size.GetMax() < 2000.0 && Art.Contains(ArtId))
	{
		// a model stretched over the box, picked and turned by a hash of the place so shots do not look tiled
		AActor* A = SpawnGroup(Center);
		A->SetActorRotation(Q);
		AddArt(A, ArtId, FVector::ZeroVector, Size, GetTypeHash(FIntVector(FMath::RoundToInt(Solid.C.X), FMath::RoundToInt(Solid.C.Y), FMath::RoundToInt(Solid.C.Z))), true);
		if (!bInGhost)
		{
			GhostHidden.Add(A);
		}
		Solid.Actor = A;
	}
	else
	{
		// rock is not one colour: each block gets its own shade from where it stands
		FLinearColor Col = DCColor::ForKind(Solid.Kind);
		if (Solid.Kind.IsNone() || Solid.Kind == TEXT("rock"))
		{
			FRandomStream Rand(GetTypeHash(Center));
			const float V = Rand.FRandRange(0.78f, 1.12f);
			Col = FLinearColor(Col.R * V * Rand.FRandRange(0.92f, 1.08f), Col.G * V, Col.B * V * Rand.FRandRange(0.85f, 1.15f));
		}
		Solid.Actor = SpawnBox(Center, Q.Rotator(), Size, Col, !bInGhost);
	}
	Solid.World = FTransform(Q, Center);
	Solid.Half = Size * 0.5;
	Solid.BaseLoc = Center;
}

void AMatchCutGameMode::SpawnShot(FDCShot& Shot, int32 Index)
{
	Shot.Camera = SpawnCamera(Shot.CamPos + Shot.AxR * Shot.Rail.Z, Shot.CamRot);
	for (FDCSolid& Solid : Shot.Solids)
	{
		SpawnSolid(Solid, Shot.CamPos, Shot.CamRot, true);
	}

	// far backdrop and loose background rocks (no collision, never in the ghost)
	FRandomStream Rand(Index * 7919 + 13);
	const double Far = Shot.VolMax.Z;
	AStaticMeshActor* Back = GetWorld()->SpawnActor<AStaticMeshActor>(Shot.CamPos + Shot.AxF * (Far + 5200.0), Shot.CamRot);
	{
		UStaticMeshComponent* SMC = Back->GetStaticMeshComponent();
		SMC->SetMobility(EComponentMobility::Movable);
		SMC->SetStaticMesh(CubeMesh);
		SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SMC->SetCastShadow(false);
		Back->SetActorScale3D(FVector(0.2, 300.0, 140.0));
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(UnlitMaterial, Back);
		SMC->SetMaterial(0, MID);
		BackdropMIDs.Add(MID);
		GhostHidden.Add(Back);
	}
	const double Wide = 1.3 + (Shot.Rail.Y - Shot.Rail.X) / 1200.0;
	for (int32 i = 0; i < 18; ++i)
	{
		const double D = Far + 900.0 + Rand.FRandRange(0.f, 2600.f);
		const FVector S(Rand.FRandRange(-Wide, Wide) * TanH * D + 0.5 * (Shot.Rail.X + Shot.Rail.Y), Rand.FRandRange(-1.1f, 0.5f) * TanV * D, D);
		const FVector Size(Rand.FRandRange(150.f, 520.f), Rand.FRandRange(150.f, 520.f), Rand.FRandRange(200.f, 900.f));
		const FRotator Rot(Rand.FRandRange(-25.f, 25.f), Rand.FRandRange(0.f, 360.f), Rand.FRandRange(-25.f, 25.f));
		AActor* Decor = SpawnBox(Shot.CamPos + Shot.AxF * S.Z + Shot.AxR * S.X + Shot.AxU * S.Y, Rot, Size, DCColor::Decor, true);
		Cast<AStaticMeshActor>(Decor)->GetStaticMeshComponent()->SetCastShadow(false);
	}

	// slats high above the water: they only exist to cut the key light into shafts (look A)
	const FVector Mid = Shot.CamPos + Shot.AxF * 1300.0 + Shot.AxR * (0.5 * (Shot.Rail.X + Shot.Rail.Y));
	for (int32 i = 0; i < 11; ++i)
	{
		const FVector C = Mid + Shot.AxR * ((i - 5) * 230.0 + Rand.FRandRange(-40.f, 40.f)) + FVector(0, 0, 1100.0);
		AActor* Slat = SpawnBox(C, FRotator(0.0, Shot.CamRot.Yaw, 0.0), FVector(2800.0, Rand.FRandRange(70.f, 130.f), 20.0), DCColor::Decor, true);
		ShaftSlats.Add(Slat);
	}

	// the valve: a red wheel that faces the camera
	if (Shot.bHasSwitch)
	{
		AActor* A = SpawnGroup(Shot.CamPos + Shot.AxF * Shot.Switch.S.Z + Shot.AxR * Shot.Switch.S.X + Shot.AxU * Shot.Switch.S.Y);
		A->SetActorRotation(Shot.CamRot);
		AddPart(A, CylinderMesh, FVector(0, 0, 0), FRotator(90, 0, 0), FVector(0.78, 0.78, 0.10), DCColor::ValveRed, DCColor::ValveRed * 0.6f);
		AddPart(A, CubeMesh, FVector(-8, 0, 0), FRotator(0, 0, 0), FVector(0.10, 0.92, 0.12), DCColor::DarkMetal, FLinearColor::Black);
		AddPart(A, CubeMesh, FVector(-8, 0, 0), FRotator(0, 0, 0), FVector(0.10, 0.12, 0.92), DCColor::DarkMetal, FLinearColor::Black);
		AddPart(A, CylinderMesh, FVector(40, 0, 0), FRotator(90, 0, 0), FVector(0.16, 0.16, 0.9), DCColor::DarkMetal, FLinearColor::Black);
		Shot.Switch.Actor = A;
	}

	// moving water: pale streaks that run through the box
	for (FDCCurrent& C : Shot.Currents)
	{
		const FVector Dir = (Shot.AxF * C.Force.Z + Shot.AxR * C.Force.X + Shot.AxU * C.Force.Y).GetSafeNormal();
		for (int32 i = 0; i < 16; ++i)
		{
			AActor* Streak = SpawnBox(Shot.CamPos, Dir.Rotation(), FVector(70.0, 5.0, 5.0), FLinearColor(0.6f, 0.9f, 0.9f), true);
			Paints.Last().Emissive = FLinearColor(0.9f, 1.6f, 1.6f);
			Cast<AStaticMeshActor>(Streak)->GetStaticMeshComponent()->SetCastShadow(false);
			C.Streaks.Add(Streak);
		}
	}
}

void AMatchCutGameMode::SpawnFilmSets()
{
	// The official film and the removed scene are small sets far away from the stage, each shot by its own camera.
	// Set space is the same as shot space: x right, y up, z away from the set's camera.
	int32 k = 0;
	for (auto& Pair : FilmSets)
	{
		FDCFilmSet& Set = Pair.Value;
		Set.Origin = FVector(0.0, -200000.0 - 30000.0 * k++, 0.0);
		Set.Camera = SpawnCamera(Set.Origin, FRotator::ZeroRotator);
		AActor* Back = SpawnBox(Set.Origin + FVector(5900.0, 0, 0), FRotator::ZeroRotator, FVector(20.0, 20000.0, 12000.0), DCColor::Decor, true);
		Cast<AStaticMeshActor>(Back)->GetStaticMeshComponent()->SetCastShadow(false);
		if (TArray<FDCSolid>* Solids = FilmSetSolids.Find(Pair.Key))
		{
			for (FDCSolid& Solid : *Solids)
			{
				SpawnSolid(Solid, Set.Origin, FRotator::ZeroRotator, true);
			}
		}
		if (Set.TorchPower > 0.f)
		{
			// a torch on the head camera
			USpotLightComponent* L = NewObject<USpotLightComponent>(Set.Camera.Get());
			L->SetMobility(EComponentMobility::Movable);
			L->SetupAttachment(Set.Camera->GetRootComponent());
			L->RegisterComponent();
			L->SetRelativeLocation(FVector(10.0, 18.0, -12.0));
			L->SetIntensityUnits(ELightUnits::Unitless);
			L->SetIntensity(Set.TorchPower);
			L->SetLightColor(FLinearColor(0.85f, 0.95f, 1.f));
			L->SetInnerConeAngle(10.f);
			L->SetOuterConeAngle(26.f);
			L->SetAttenuationRadius(3200.f);
			L->SetVolumetricScatteringIntensity(3.f);
		}
		if (Set.bOldFilm)
		{
			if (!FlashCam.IsValid())
			{
				FlashCam = Set.Camera;
			}
			FPostProcessSettings& PP = Set.Camera->GetCameraComponent()->PostProcessSettings;
			PP.bOverride_ColorSaturation = true;
			PP.ColorSaturation = FVector4(0.18, 0.18, 0.18, 1.0);
			PP.bOverride_ColorGain = true;
			PP.ColorGain = FVector4(1.15, 1.02, 0.80, 1.0);
			PP.bOverride_ColorContrast = true;
			PP.ColorContrast = FVector4(1.15, 1.15, 1.15, 1.0);
			PP.VignetteIntensity = 1.0f;
			PP.FilmGrainIntensity = 1.0f;
		}
	}

	// every actor a clip names is made now, so the look can paint it
	for (const auto& Pair : Clips)
	{
		const FDCFilmSet* Set = FilmSets.Find(Pair.Value.Set);
		const FVector Origin = Set ? Set->Origin : FVector::ZeroVector;
		for (const FDCTrack& Track : Pair.Value.Tracks)
		{
			if (FilmActors.Contains(Track.Actor) || Track.Actor == TEXT("cam"))
			{
				continue;
			}
			FDCRobot Robot;
			if (Track.Actor.StartsWith(TEXT("gentle")))
			{
				// the same body as in play; its light rides with it here
				const TWeakObjectPtr<UPointLightComponent> Keep = GentleLight;
				Robot.Actor = SpawnGentle(Origin);
				if (UPointLightComponent* L = GentleLight.Get())
				{
					L->SetAbsolute(false, false, false);
					L->SetRelativeLocation(FVector(260.0, 0.0, 0.0));
					L->SetAttenuationRadius(1600.f);
					L->SetIntensity(16000.f);
				}
				GentleLight = Keep;
				Robot.bCreature = true;
			}
			else if (Track.Actor.StartsWith(TEXT("predator")))
			{
				Robot.Actor = SpawnPredator(Origin, true);
				Robot.bCreature = true;
			}
			else if (Track.Actor.StartsWith(TEXT("cable")))
			{
				Robot.Actor = SpawnBox(Origin, FRotator::ZeroRotator, FVector(6.0, 6.0, 700.0), DCColor::DarkMetal, true);
				Robot.bPlain = true;
			}
			else
			{
				const bool bUnit7 = Track.Actor.StartsWith(TEXT("unit7"));
				Robot = SpawnRobot(Origin, bUnit7 ? DCColor::Unit7 : DCColor::Crew, DCColor::EyeWarm, false);
				if (Track.Actor.EndsWith(TEXT("lamp")))
				{
					AActor* A = Robot.Actor.Get();
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
					G->SetAbsolute(true, true, false);
					G->SetIntensityUnits(ELightUnits::Unitless);
					G->SetIntensity(9000.f);
					G->SetLightColor(FLinearColor(1.f, 0.9f, 0.7f));
					G->SetAttenuationRadius(900.f);
					G->SetCastShadows(false);
					G->SetWorldLocation(Origin + FVector(700.0, -110.0, 20.0));
				}
			}
			if (AActor* A = Robot.Actor.Get())
			{
				A->SetActorHiddenInGame(true);
			}
			FilmActors.Add(Track.Actor, Robot);
		}
	}
}

void AMatchCutGameMode::SpawnWorld()
{
	UWorld* World = GetWorld();
	LoadArt();

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

	// loose things
	for (FDCProp& Prop : Props)
	{
		AActor* A = SpawnGroup(ShotToWorld(Prop.Shot, Prop.S));
		A->SetActorRotation(Shots[Prop.Shot].CamRot);
		if (Prop.Kind == TEXT("crew"))
		{
			// a crew robot that cannot swim: carried like any heavy thing
			A->Destroy();
			const FDCRobot Robot = SpawnRobot(ShotToWorld(Prop.Shot, Prop.S), DCColor::Crew, DCColor::EyeWarm, false, TEXT("crew"));
			PropRobots.Add((int32)(&Prop - Props.GetData()), Robot);
			Prop.Actor = Robot.Actor;
			continue;
		}
		const bool bLong = Prop.Kind == TEXT("long"), bRound = Prop.Kind == TEXT("round");
		const FString PropArt = bLong ? FString(TEXT("plate")) : (bRound ? FString(TEXT("weight")) : Prop.Kind.ToString());
		const FVector PropBox = Prop.Kind == TEXT("cell") ? FVector(64.0, 64.0, 88.0) : (bLong ? FVector(68.0, 190.0, 24.0) : (bRound ? FVector(116.0, 116.0, 34.0) : FVector(130.0, 150.0, Prop.Kind == TEXT("weight") ? 42.0 : 24.0)));
		// the long plate and the round one are the same two models as the plate and the lid, flattened to their boxes:
		// their outline is what the puzzle is about
		// the two shapes of the graphic match keep their plain orange shapes: outline and contrast matter more there
		if (!bLong && !bRound && AddArt(A, PropArt, FVector::ZeroVector, PropBox, 0, false))
		{
			// the model; its box is the one the shapes below fill
		}
		else if (bLong)
		{
			// a long plate: actor X is away from the camera, Y across the picture
			AddPart(A, CubeMesh, FVector(0, 0, 0), FRotator::ZeroRotator, FVector(0.68, 1.9, 0.24), DCColor::Plate, DCColor::Plate * 0.45f);
			AddPart(A, CubeMesh, FVector(0, 0, 10), FRotator::ZeroRotator, FVector(0.14, 1.6, 0.08), DCColor::DarkMetal, FLinearColor::Black);
		}
		else if (bRound)
		{
			AddPart(A, CylinderMesh, FVector(0, 0, 0), FRotator::ZeroRotator, FVector(1.16, 1.16, 0.24), DCColor::Plate, DCColor::Plate * 0.45f);
			AddPart(A, CylinderMesh, FVector(0, 0, 10), FRotator::ZeroRotator, FVector(0.5, 0.5, 0.08), DCColor::DarkMetal, FLinearColor::Black);
		}
		else if (Prop.Kind == TEXT("weight"))
		{
			// a heavy lid: too heavy to push, it needs the gripper
			AddPart(A, CubeMesh, FVector(0, 0, 0), FRotator::ZeroRotator, FVector(1.3, 1.5, 0.42), DCColor::DarkMetal, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(0, 0, 22), FRotator::ZeroRotator, FVector(1.32, 0.34, 0.04), FLinearColor(0.9f, 0.7f, 0.1f), FLinearColor(0.9f, 0.7f, 0.1f) * 0.6f);
			AddPart(A, CubeMesh, FVector(0, 0, 30), FRotator::ZeroRotator, FVector(0.5, 0.12, 0.12), DCColor::Hull, FLinearColor::Black);
		}
		else if (Prop.Kind == TEXT("cell"))
		{
			AddPart(A, CylinderMesh, FVector(0, 0, 0), FRotator::ZeroRotator, FVector(0.42, 0.42, 0.62), DCColor::DarkMetal, FLinearColor::Black);
			AddPart(A, CylinderMesh, FVector(0, 0, 0), FRotator::ZeroRotator, FVector(0.46, 0.46, 0.22), FLinearColor(0.2f, 1.f, 0.5f), FLinearColor(0.6f, 3.5f, 1.4f));
		}
		else
		{
			AddPart(A, CubeMesh, FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1.3, 1.5, 0.24), DCColor::Plate, DCColor::Plate * 0.45f);
			AddPart(A, CubeMesh, FVector(0, 0, 10), FRotator(0, 0, 0), FVector(0.2, 1.1, 0.08), DCColor::DarkMetal, FLinearColor::Black);
		}
		Prop.Actor = A;
	}
	for (FDCSocket& Socket : Sockets)
	{
		const FDCShot& Shot = Shots[Socket.Shot];
		AActor* A = SpawnGroup(ShotToWorld(Socket.Shot, Socket.S));
		A->SetActorRotation(Shot.CamRot);
		const FString SocketArt = TEXT("socket:") + Socket.Label;
		const bool bSocketArt = Art.Contains(SocketArt) && Socket.Accepts != TEXT("long");
		if (bSocketArt)
		{
			// the model, with the small light the game switches from red to green
			const bool bDock = Socket.Accepts == TEXT("cell"), bLift = Socket.Accepts == TEXT("crew");
			AddArt(A, SocketArt, bDock ? FVector(45, 0, -5) : (bLift ? FVector(0, 0, 5) : FVector(0, 0, 30)), bDock ? FVector(130, 130, 170) : (bLift ? FVector(130, 180, 150) : FVector(220, 200, 60)));
			AddPart(A, SphereMesh, bDock ? FVector(40, 0, 90) : (bLift ? FVector(-20, 0, 92) : FVector(-80, 0, 20)), FRotator::ZeroRotator, FVector(0.2, 0.2, 0.2), FLinearColor(1.f, 0.2f, 0.1f), FLinearColor(5.f, 0.6f, 0.3f));
			Socket.Paint = Paints.Num() - 1;
		}
		else if (Socket.Accepts == TEXT("cell"))
		{
			// a dock: a dark block with a light that turns green when the cell is in
			AddPart(A, CubeMesh, FVector(30, 0, -60), FRotator::ZeroRotator, FVector(1.0, 1.3, 0.5), DCColor::DarkMetal, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(60, 0, 20), FRotator::ZeroRotator, FVector(0.3, 1.3, 1.2), DCColor::Hull, FLinearColor::Black);
			AddPart(A, SphereMesh, FVector(40, 0, 70), FRotator::ZeroRotator, FVector(0.2, 0.2, 0.2), FLinearColor(1.f, 0.2f, 0.1f), FLinearColor(5.f, 0.6f, 0.3f));
			Socket.Paint = Paints.Num() - 1;
		}
		else if (Socket.Accepts == TEXT("crew"))
		{
			// a lift cradle: a base, two posts, a cable up out of the frame and a light that turns green when a unit is in
			AddPart(A, CubeMesh, FVector(0, 0, -62), FRotator::ZeroRotator, FVector(1.3, 1.7, 0.14), DCColor::DarkMetal, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(0, -80, 0), FRotator::ZeroRotator, FVector(0.12, 0.12, 1.4), DCColor::Hull, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(0, 80, 0), FRotator::ZeroRotator, FVector(0.12, 0.12, 1.4), DCColor::Hull, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(0, 0, 72), FRotator::ZeroRotator, FVector(0.12, 1.72, 0.12), DCColor::Hull, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(0, 0, 1080), FRotator::ZeroRotator, FVector(0.05, 0.05, 20.0), DCColor::DarkMetal, FLinearColor::Black);
			AddPart(A, SphereMesh, FVector(-20, 0, 92), FRotator::ZeroRotator, FVector(0.2, 0.2, 0.2), FLinearColor(1.f, 0.2f, 0.1f), FLinearColor(5.f, 0.6f, 0.3f));
			Socket.Paint = Paints.Num() - 1;
		}
		else
		{
			if (Socket.Accepts == TEXT("long"))
			{
				// a crack that runs across the picture: only a long thing covers it
				AddPart(A, CubeMesh, FVector(0, 0, 3), FRotator::ZeroRotator, FVector(1.0, 2.3, 0.10), DCColor::DarkMetal, FLinearColor::Black);
				AddPart(A, CubeMesh, FVector(0, 0, 9), FRotator::ZeroRotator, FVector(0.22, 1.9, 0.06), FLinearColor(0.5f, 0.9f, 0.9f), FLinearColor(0.5f, 1.4f, 1.4f));
			}
			else
			{
			// a vent: a long slot in the floor that runs away from the camera
			AddPart(A, CubeMesh, FVector(0, 0, 3), FRotator::ZeroRotator, FVector(9.0, 1.9, 0.10), DCColor::DarkMetal, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(0, 0, 9), FRotator::ZeroRotator, FVector(8.6, 0.5, 0.06), FLinearColor(0.5f, 0.9f, 0.9f), FLinearColor(0.5f, 1.4f, 1.4f));
			}
			Socket.Paint = Paints.Num() - 1;
		}
		Socket.Actor = A;
	}

	GentleActor = SpawnGentle(Shots[0].CamPos - FVector(0.0, 0.0, 90000.0));
	GentleActor->SetActorHiddenInGame(true);
	PredActor = SpawnPredator(Shots[0].CamPos - FVector(0.0, 0.0, 95000.0));
	PredActor->SetActorHiddenInGame(true);

	// the player's robot: battered, one thruster missing, number plate scraped off
	SpawnWater();
	Hero = SpawnRobot(Shots[0].CamPos, DCColor::Hero, DCColor::EyeCyan, true, TEXT("hero"));
	if (AActor* A = Hero.Actor.Get())
	{
		USpotLightComponent* L = NewObject<USpotLightComponent>(A);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(A->GetRootComponent());
		L->RegisterComponent();
		// the lamp sits at the head: for the model, a little under the top of its box and at its front
		L->SetRelativeLocation(Hero.bArt ? FVector(22.0, 0.0, 36.0) : FVector(36.0, 0.0, 8.0));
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

		// a soft light that only Unit 0 takes (lighting channel 1), from the camera side and above: it reads against
		// dark rock in every shot, lamp or no lamp (brief 05)
		TArray<UStaticMeshComponent*> Parts;
		A->GetComponents(Parts);
		for (UStaticMeshComponent* Part : Parts)
		{
			Part->SetLightingChannels(true, true, false);
		}
		UPointLightComponent* Rim = NewObject<UPointLightComponent>(A);
		Rim->SetMobility(EComponentMobility::Movable);
		Rim->SetupAttachment(A->GetRootComponent());
		Rim->RegisterComponent();
		Rim->SetAbsolute(false, true, false);
		Rim->SetIntensityUnits(ELightUnits::Unitless);
		Rim->SetIntensity(HeroRimPower);
		Rim->SetLightColor(FLinearColor(0.75f, 0.95f, 1.f));
		Rim->SetAttenuationRadius(700.f);
		Rim->SetCastShadows(false);
		Rim->SetLightingChannels(false, true, false);
		HeroRim = Rim;
	}

	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		// the parts: features that were cut from Unit 0 and built into Unit 7
		FDCShot& S = Shots[i];
		if (!S.bHasPart)
		{
			continue;
		}
		AActor* A = SpawnGroup(ShotToWorld(i, S.PartS));
		if (S.PartFeature == 1)
		{
			// the gripper: a wrist and two fingers
			AddPart(A, CylinderMesh, FVector(-16, 0, 0), FRotator(90, 0, 0), FVector(0.22, 0.22, 0.30), DCColor::Unit7, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(24, 0, 15), FRotator(25, 0, 0), FVector(0.44, 0.12, 0.08), DCColor::Unit7, FLinearColor::Black);
			AddPart(A, CubeMesh, FVector(24, 0, -15), FRotator(-25, 0, 0), FVector(0.44, 0.12, 0.08), DCColor::Unit7, FLinearColor::Black);
			AddPart(A, SphereMesh, FVector(2, 0, 0), FRotator::ZeroRotator, FVector(0.2, 0.2, 0.2), FLinearColor(1.f, 0.85f, 0.4f), FLinearColor(6.f, 4.5f, 1.2f));
		}
		else
		{
			// the headlamp
			AddPart(A, CylinderMesh, FVector(0, 0, 0), FRotator(90, 0, 0), FVector(0.30, 0.30, 0.34), DCColor::Unit7, FLinearColor::Black);
			AddPart(A, SphereMesh, FVector(18, 0, 0), FRotator::ZeroRotator, FVector(0.26, 0.26, 0.26), FLinearColor(1.f, 0.85f, 0.4f), FLinearColor(6.f, 4.5f, 1.2f));
		}
		S.PartActor = A;
	}
	if (CrewShot != INDEX_NONE)
	{
		CrewPos = ShotToWorld(CrewShot, Shots[CrewShot].CrewS);
		Crew = SpawnRobot(CrewPos, DCColor::Crew, DCColor::EyeWarm, false, TEXT("crew"));
		PlaceRobot(Crew, CrewPos, FVector2D(1.0, -0.3), Shots[CrewShot].CamRot, 35.f);
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
		Cap->CaptureSource = ESceneCaptureSource::SCS_BaseColor; // flat colour: no lights, no fog, so the water cannot wash it out
		Cap->bCaptureEveryFrame = false; // refreshed about 12 times a second from Tick
		Cap->bCaptureOnMovement = false;
		Cap->bAlwaysPersistRenderingState = true;
		SetupLook(Cap->PostProcessSettings, ExposureEV, 0.f, 0.f);
		Cap->PostProcessBlendWeight = 1.f;
	}

	for (int32 i = 0; i < NumGaps; ++i)
	{
		UTextureRenderTarget2D* Thumb = NewObject<UTextureRenderTarget2D>(this);
		Thumb->RenderTargetFormat = RTF_RGBA8_SRGB;
		Thumb->ClearColor = FLinearColor::Black;
		Thumb->InitAutoFormat(256, 144);
		Thumb->UpdateResourceImmediate(true);
		ThumbTargets.Add(Thumb);
	}
	const FVector FlashPos = FlashCam.IsValid() ? FlashCam->GetActorLocation() : FVector::ZeroVector;
	ThumbCapture = World->SpawnActor<ASceneCapture2D>(FlashPos, FRotator::ZeroRotator);
	{
		USceneCaptureComponent2D* Cap = ThumbCapture->GetCaptureComponent2D();
		Cap->TextureTarget = ThumbTargets[0];
		Cap->FOVAngle = Fov;
		Cap->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Cap->bCaptureEveryFrame = false;
		Cap->bCaptureOnMovement = false;
		if (FlashCam.IsValid())
		{
			Cap->PostProcessSettings = FlashCam->GetCameraComponent()->PostProcessSettings;
		}
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
		const ACameraActor* CamPtr = Cam.Get();
		bool bOld = false;
		for (const auto& SetPair : FilmSets)
		{
			bOld |= SetPair.Value.bOldFilm && SetPair.Value.Camera.Get() == CamPtr;
		}
		if (Cam.IsValid() && !bOld)
		{
			FPostProcessSettings& PP = Cam->GetCameraComponent()->PostProcessSettings;
			PP.VignetteIntensity = Look.Vignette;
			PP.FilmGrainIntensity = Look.Grain;
			PP.bOverride_ColorSaturation = PP.bOverride_ColorContrast = PP.bOverride_ColorGain = true;
			PP.ColorSaturation = FVector4(Look.Saturation, Look.Saturation, Look.Saturation, 1.0);
			PP.ColorContrast = FVector4(Look.Contrast, Look.Contrast, Look.Contrast, 1.0);
			PP.ColorGain = FVector4(Look.Tint.R, Look.Tint.G, Look.Tint.B, 1.0);
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
