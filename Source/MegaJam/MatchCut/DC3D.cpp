// Deep Cut (brief 06, checkpoint M1): free 3D swimming, a camera behind Unit 0 that never sits inside rock,
// and one test area built from the art-drop models.
//   world space: z up, cm. The sea floor is z = 0.
//   movement: the keys push Unit 0 towards a speed, the water takes the speed away slowly when the keys are let go
//   collision: engine sweeps against the models themselves (their triangles), sliding along what is hit

#include "DC3D.h"
#include "DCDoc.h"
#include "MatchCutGameMode.h"
#include "DCAudio.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Engine/Texture.h"
#include "Misc/CommandLine.h"

namespace DC3
{
	const float Swim = 560.f;        // cm/s
	const float Boost = 980.f;
	const float Push = 2.4f;         // 1/s: how fast the thrusters reach the wanted speed
	const float Drag = 0.75f;        // 1/s: how fast the water takes speed away while gliding
	const float BoomLen = 440.f;
	const float BoomUp = 70.f;
	const float CamRadius = 26.f;
	const float AreaRadius = 5500.f; // a current turns Unit 0 back past this
	const float Ceiling = 2900.f;
	const double FleckPeriod = 1600.0;
	const ECollisionChannel Channel = ECC_Visibility;

	FCollisionQueryParams Query()
	{
		FCollisionQueryParams P(FName(TEXT("DC3D")), true); // against the triangles
		return P;
	}

	float TurnTo(float From, float To, float Rate, float Dt)
	{
		return From + FMath::FindDeltaAngleDegrees(From, To) * (1.f - FMath::Exp(-Rate * Dt));
	}
}

// ---------------------------------------------------------------------------------------------
// building
// ---------------------------------------------------------------------------------------------

AActor* FDC3D::Place(const TCHAR* Id, int32 Variant, const FVector& Base, const FRotator& Rot, float Scale, bool bCollide)
{
	const TArray<FDCArtRef>* Set = GM->Art.Find(Id);
	AActor* A = GM->SpawnGroup(Base);
	A->SetActorRotation(Rot);
	if (!Set || Set->Num() == 0)
	{
		// no model: a plain block of about the same size
		UStaticMeshComponent* C = GM->AddPart(A, GM->CubeMesh, FVector(0.0, 0.0, 60.0 * Scale), FRotator::ZeroRotator, FVector(1.5, 1.5, 1.2) * Scale, FLinearColor(0.28f, 0.27f, 0.25f), FLinearColor::Black);
		C->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		C->SetCollisionResponseToAllChannels(ECR_Block);
		return A;
	}
	const FDCArtRef& Ref = (*Set)[Variant % Set->Num()];
	const FBox B = GM->ArtMeshes[Ref.Mesh]->GetBoundingBox().TransformBy(FTransform(FQuat(FRotator(0.f, Ref.Yaw, 0.f))));
	const FVector Size = B.GetSize() * Scale;
	// one uniform scale: the model is never stretched
	if (UStaticMeshComponent* C = GM->AddArt(A, Id, FVector(0.0, 0.0, Size.Z * 0.5), Size, Variant, false))
	{
		if (!bCollide && Size.GetMax() < 320.0)
		{
			C->SetCullDistance(4200.f); // small loose things are not drawn across the whole site: the haze hides them there anyway
			C->SetCastShadow(false);
		}
		if (bCollide)
		{
			C->SetCollisionObjectType(ECC_WorldStatic);
			C->SetCollisionResponseToAllChannels(ECR_Block);
			C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
	return A;
}

UStaticMeshComponent* FDC3D::Bare(UStaticMesh* Mesh, UMaterialInterface* Mat, const FVector& At, const FRotator& Rot, const FVector& Scale, const FLinearColor& Color, UMaterialInstanceDynamic** OutMID)
{
	// a part with its own material and no paint: painted parts get the look's material again later (GM->Paints)
	AActor* A = GM->SpawnGroup(At);
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(A);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetupAttachment(A->GetRootComponent());
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Mat, C);
	M->SetVectorParameterValue(TEXT("Color"), Color);
	C->SetMaterial(0, M);
	C->RegisterComponent();
	C->SetRelativeRotation(Rot);
	C->SetRelativeScale3D(Scale);
	if (OutMID)
	{
		*OutMID = M;
	}
	return C;
}

float FDC3D::GroundZ(double X, double Y) const
{
	double Z = 0.0;
	for (const FMound& M : Mounds)
	{
		const FVector2D D = (FVector2D(X, Y) - M.At).GetRotated(-M.Yaw);
		const double Q = FMath::Square(D.X / M.Radii.X) + FMath::Square(D.Y / M.Radii.Y);
		if (Q < 1.0)
		{
			Z = FMath::Max(Z, M.CentreZ + M.Radii.Z * FMath::Sqrt(1.0 - Q));
		}
	}
	return (float)Z;
}

AActor* FDC3D::Block(const FVector& Centre, const FRotator& Rot, const FVector& Size, const FLinearColor& Color)
{
	AActor* A = GM->SpawnBox(Centre, Rot, Size, Color, false);
	if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(A))
	{
		UStaticMeshComponent* C = SMA->GetStaticMeshComponent();
		C->SetCollisionObjectType(ECC_WorldStatic);
		C->SetCollisionResponseToAllChannels(ECR_Block);
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	return A;
}

UPointLightComponent* FDC3D::Glow(const FVector& At, const FLinearColor& Color, float Power, float Reach)
{
	AActor* A = GM->SpawnGroup(At);
	UPointLightComponent* L = NewObject<UPointLightComponent>(A);
	L->SetMobility(EComponentMobility::Movable);
	L->SetupAttachment(A->GetRootComponent());
	L->RegisterComponent();
	L->SetIntensityUnits(ELightUnits::Unitless);
	L->SetIntensity(Power);
	L->SetLightColor(Color);
	L->SetAttenuationRadius(Reach);
	L->SetCastShadows(false);
	L->SetVolumetricScatteringIntensity(1.5f);
	Glows.Add(L);
	return L;
}

void FDC3D::AddRing(const FVector& At, float YawDeg, float PitchDeg)
{
	FDC3DRing& R = Rings.AddDefaulted_GetRef();
	R.Pos = At;
	const FRotator Rot(PitchDeg, YawDeg, 0.f);
	R.Axis = Rot.Vector();
	AActor* A = GM->SpawnGroup(At);
	A->SetActorRotation(Rot);
	const int32 N = 18;
	for (int32 i = 0; i < N; ++i)
	{
		const float Ang = 360.f * i / N;
		const FVector P = FVector(0.0, FMath::Cos(FMath::DegreesToRadians(Ang)), FMath::Sin(FMath::DegreesToRadians(Ang))) * R.Radius;
		GM->AddPart(A, GM->CubeMesh, P, FRotator(0.f, 0.f, Ang), FVector(0.10, 0.44, 0.10), FLinearColor(0.9f, 0.55f, 0.1f), FLinearColor(5.f, 2.6f, 0.4f));
		R.Paints.Add(GM->Paints.Num() - 1);
	}
}

void FDC3D::SpawnHero()
{
	GM->Hero = GM->SpawnRobot(Pos, FLinearColor(0.36f, 0.45f, 0.42f), FLinearColor(0.2f, 3.0f, 3.5f), true, TEXT("hero"));
	AActor* A = GM->Hero.Actor.Get();
	if (!A)
	{
		return;
	}
	TArray<UStaticMeshComponent*> Parts;
	A->GetComponents(Parts);
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetLightingChannels(true, true, false);
		// what the model lights up (eyes, headlamp, chest) comes from its emissive map; AddArt set a strength made for the
		// side view, where it washed the whole body pale
		for (int32 m = 0; m < Part->GetNumMaterials(); ++m)
		{
			if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(m)))
			{
				float Em = 14.f;
				FParse::Value(FCommandLine::Get(), TEXT("-MCEm="), Em);
				MID->SetScalarParameterValue(TEXT("EmissiveStrength"), Em);
				// the import switched every "emissive" number on, also the ones that say where on the model the map lies:
				// with those wrong the whole body glowed. The map uses the model's first UV set, unturned and unscaled.
				MID->SetScalarParameterValue(TEXT("EmissiveTexture_TexCoord"), 0.f);
				MID->SetScalarParameterValue(TEXT("EmissiveTexture_Rotation"), 0.f);
				MID->SetVectorParameterValue(TEXT("EmissiveTexture_OffsetScale"), FLinearColor(0.f, 0.f, 1.f, 1.f));
				MID->SetVectorParameterValue(TEXT("EmissiveTexture_TilingMethod"), FLinearColor(0.f, 0.f, 0.f, 0.f));
				// the switch can only be read in the editor; the hero model is imported with it on (Tools/import_assets.py)
				bool bMap = true;
#if WITH_EDITOR
				FGuid Guid;
				MID->GetStaticSwitchParameterValue(FName(TEXT("bHasEmissiveTexture")), bMap, Guid);
#endif
				// without the switch the material adds the emissive colour to the whole body: then it has to be black
				MID->SetVectorParameterValue(TEXT("EmissiveFactor"), bMap ? FLinearColor::White : FLinearColor::Black);
				UE_LOG(LogMatchCut, Display, TEXT("3D HERO emissive map in use: %d, strength %.1f"), bMap ? 1 : 0, Em);
			}
		}
	}
	// a light only Unit 0 takes (lighting channel 1), from behind and above where the camera is: its teal reads in green water
	UPointLightComponent* Rim = NewObject<UPointLightComponent>(A);
	Rim->SetMobility(EComponentMobility::Movable);
	Rim->SetupAttachment(A->GetRootComponent());
	Rim->RegisterComponent();
	Rim->SetAbsolute(false, true, false);
	Rim->SetIntensityUnits(ELightUnits::Unitless);
	float RimPower = 24000.f;
	FParse::Value(FCommandLine::Get(), TEXT("-MCRim="), RimPower);
	Rim->SetIntensity(RimPower);
	Rim->SetLightColor(FLinearColor(0.85f, 0.97f, 1.f));
	Rim->SetAttenuationRadius(900.f);
	Rim->SetCastShadows(false);
	Rim->SetLightingChannels(false, true, false);
	GM->HeroRim = Rim;

	// the headlamp: always on in the 3D game, it shows where Unit 0 is heading
	// on an actor of its own: when the diver is out of the picture (camera up) the light must stay
	AActor* LampActor = GM->SpawnGroup(Pos);
	USpotLightComponent* L = NewObject<USpotLightComponent>(LampActor);
	L->SetMobility(EComponentMobility::Movable);
	L->SetupAttachment(LampActor->GetRootComponent());
	L->RegisterComponent();
	L->SetRelativeLocation(FVector(62.0, 0.0, 30.0)); // in front of the body: a lamp inside the mesh lights it white from within
	L->SetIntensityUnits(ELightUnits::Unitless);
	L->SetIntensity(26000.f);
	L->SetLightColor(FLinearColor(1.f, 0.80f, 0.52f)); // a warm practical light in cold water
	L->SetInnerConeAngle(16.f);
	L->SetOuterConeAngle(34.f);
	L->SetAttenuationRadius(2600.f);
	L->SetCastShadows(false);
	L->SetVolumetricScatteringIntensity(1.2f);
	Lamp = L;
}

void FDC3D::SpawnFlecks()
{
	auto Make = [this](UStaticMesh* Mesh, int32 Count, float MinSize, float MaxSize, int32 Seed, TWeakObjectPtr<UMaterialInstanceDynamic>& OutMID)
	{
		AActor* A = GM->SpawnGroup(FVector::ZeroVector);
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(GM->UnlitMaterial, A);
		UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(A);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetupAttachment(A->GetRootComponent());
		C->RegisterComponent();
		C->SetMaterial(0, MID);
		FRandomStream Rand(Seed);
		const double P = DC3::FleckPeriod;
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector At(Rand.FRandRange(0.f, 1.f) * P, Rand.FRandRange(0.f, 1.f) * P, Rand.FRandRange(0.f, 1.f) * P);
			const FVector Scale = FVector(Rand.FRandRange(MinSize, MaxSize)) * FVector(1.0, Rand.FRandRange(0.6f, 1.4f), Rand.FRandRange(0.6f, 1.4f)) / 100.0;
			const FRotator Rot(Rand.FRandRange(0.f, 90.f), Rand.FRandRange(0.f, 90.f), 0.f);
			// the same pattern in all eight boxes around the camera: it can follow the camera in whole periods without a jump
			for (int32 k = 0; k < 8; ++k)
			{
				C->AddInstance(FTransform(Rot, At - FVector((k & 1) ? P : 0.0, (k & 2) ? P : 0.0, (k & 4) ? P : 0.0), Scale));
			}
		}
		OutMID = MID;
		return A;
	};
	Snow = Make(GM->CubeMesh, 420, 2.2f, 5.5f, 4711, SnowMID);
	Bubbles = Make(GM->SphereMesh, 40, 3.5f, 8.f, 1213, BubbleMID);
}

void FDC3D::BuildArea()
{
	float ShaftPower = 14.f;
	FParse::Value(FCommandLine::Get(), TEXT("-MCShaft="), ShaftPower);
	const FLinearColor Sand(0.20f, 0.19f, 0.16f);
	AActor* FloorBlock = Block(FVector(0.0, 0.0, -150.0), FRotator::ZeroRotator, FVector(22000.0, 22000.0, 300.0), Sand);
	// sand and silt: colour from noise in world space, so it never repeats (Tools/make_sand_material.py)
	UMaterialInterface* SandMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DeepCut/M_DC_Sand.M_DC_Sand"));
	if (FParse::Param(FCommandLine::Get(), TEXT("MCPlainFloor")))
	{
		SandMat = nullptr;
	}
	if (SandMat)
	{
		Bare(GM->CubeMesh, SandMat, FVector(0.0, 0.0, -50.0), FRotator::ZeroRotator, FVector(220.0, 220.0, 1.0), FLinearColor::White);
		FloorBlock->SetActorHiddenInGame(true); // still what things collide with
	}
	const TCHAR* Boulder = TEXT("scene:boulder");
	const TCHAR* Ledge = TEXT("scene:ledge");
	const TCHAR* Coral = TEXT("scene:coral");

	// the arch straight ahead of the start
	Place(TEXT("scene:arch"), 0, FVector(1500.0, 0.0, -40.0), FRotator(0.f, 90.f, 0.f), 8.f);
	Place(Coral, 0, FVector(1250.0, 820.0, -10.0), FRotator(0.f, 40.f, 0.f), 1.3f);

	// the wreck, with what fell off it
	Place(TEXT("scene:wreck"), 0, FVector(3300.0, -700.0, -60.0), FRotator(0.f, 35.f, 0.f), 8.f);
	Place(TEXT("scene:debris"), 0, FVector(2500.0, -1500.0, -30.0), FRotator(0.f, 70.f, 0.f), 3.0f);
	Place(TEXT("scene:debris"), 0, FVector(3950.0, 450.0, -40.0), FRotator(0.f, 160.f, 0.f), 2.4f);

	// the station, where the dive starts and the crewmate waits (the crewmate belongs to the documentary, DCDoc.cpp)
	Place(TEXT("scene:station"), 0, FVector(1150.0, -1150.0, 0.0), FRotator(0.f, 30.f, 0.f), 6.f);
	Glow(FVector(800.0, -700.0, 380.0), FLinearColor(1.f, 0.82f, 0.55f), 5000.f, 1100.f);

	// the vent field: warm light low in the water
	const FVector Vents[] = { FVector(900.0, -2700.0, 0.0), FVector(1500.0, -3100.0, 0.0), FVector(1750.0, -2500.0, 0.0), FVector(700.0, -3400.0, 0.0) };
	const float VentScale[] = { 2.6f, 2.0f, 1.6f, 3.0f };
	for (int32 i = 0; i < 4; ++i)
	{
		if (AActor* Made = Place(TEXT("scene:vent"), 0, Vents[i] - FVector(0.0, 0.0, 20.0), FRotator(0.f, 75.f * i, 0.f), VentScale[i]))
		{
			const FBox Box = Made->GetComponentsBoundingBox();
			PlumeAt.Add(FVector(Box.GetCenter().X, Box.GetCenter().Y, Box.Max.Z));
		}
		Glow(Vents[i] + FVector(120.0, 0.0, 120.0 * VentScale[i]), FLinearColor(1.f, 0.45f, 0.12f), 4200.f, 1100.f);
	}

	// the trench: boulders on both sides, ledges over it, a plug of rock at the far end. The sun does not reach in.
	for (int32 i = 0; i < 6; ++i)
	{
		const double X = 500.0 + 760.0 * i;
		Place(Boulder, 0, FVector(X, 1640.0 - 40.0 * (i % 2), -90.0), FRotator(0.f, 180.f * i, 0.f), 6.4f);
		Place(Boulder, 0, FVector(X + 120.0, 3140.0 + 50.0 * (i % 2), -90.0), FRotator(0.f, 40.f + 180.f * i, 0.f), 6.6f);
	}
	for (int32 i = 0; i < 5; ++i)
	{
		Place(Ledge, 0, FVector(760.0 + 900.0 * i, 2390.0, 600.0 + 30.0 * (i % 2)), FRotator(0.f, 85.f + 6.f * (i % 3), 0.f), 9.f);
	}
	Place(Boulder, 0, FVector(5000.0, 2390.0, -90.0), FRotator(0.f, 30.f, 0.f), 8.f);
	Place(Coral, 0, FVector(350.0, 2390.0, -10.0), FRotator(0.f, 10.f, 0.f), 1.2f);

	// the reef, where the shoal lives: shelves of rock stepping up, coral on them
	Place(Ledge, 0, FVector(-3600.0, 2700.0, -40.0), FRotator(0.f, 20.f, 0.f), 5.5f);
	Place(Ledge, 0, FVector(-3950.0, 2400.0, 110.0), FRotator(0.f, 75.f, 0.f), 4.2f);
	Place(Boulder, 0, FVector(-4150.0, 3000.0, -60.0), FRotator(0.f, 130.f, 0.f), 4.6f);
	Place(Coral, 0, FVector(-3550.0, 2750.0, 120.0), FRotator(0.f, 0.f, 0.f), 1.8f);
	Place(Coral, 0, FVector(-3900.0, 2250.0, 230.0), FRotator(0.f, 110.f, 0.f), 1.4f);
	Place(Coral, 0, FVector(-2850.0, 3200.0, -10.0), FRotator(0.f, 200.f, 0.f), 2.2f);
	Place(Coral, 0, FVector(-2650.0, 1750.0, -10.0), FRotator(0.f, 290.f, 0.f), 1.6f);
	Place(Boulder, 0, FVector(-2400.0, 3300.0, -50.0), FRotator(0.f, 250.f, 0.f), 2.4f);

	// the clearing with the shaft of light: a ring of low rock and coral round an open floor
	for (int32 i = 0; i < 7; ++i)
	{
		const float Ang = 2.f * PI * i / 7.f + 0.3f;
		Place(i % 3 == 1 ? Coral : i % 3 == 2 ? Ledge : Boulder, 0, FVector(-2500.0 + 950.0 * FMath::Cos(Ang), -2700.0 + 950.0 * FMath::Sin(Ang), -30.0),
			FRotator(0.f, 51.f * i, 0.f), i % 3 == 1 ? 1.5f : 2.4f);
	}
	{
		AActor* A = GM->SpawnGroup(FVector(-2500.0, -2700.0, 2600.0));
		USpotLightComponent* L = NewObject<USpotLightComponent>(A);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(A->GetRootComponent());
		L->RegisterComponent();
		L->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));
		L->SetIntensityUnits(ELightUnits::Unitless);
		L->bUseInverseSquaredFalloff = false; // a shaft of sun does not thin out on the way down
		L->SetLightFalloffExponent(1.f);
		L->SetIntensity(ShaftPower);
		L->SetLightColor(FLinearColor(0.62f, 0.95f, 1.f));
		L->SetInnerConeAngle(7.f);
		L->SetOuterConeAngle(11.f);
		L->SetAttenuationRadius(3200.f);
		L->SetCastShadows(false);
		L->SetVolumetricScatteringIntensity(6.f);
		L->MarkRenderStateDirty();
	}

	// light you can see in the water: faint additive columns. One stands in the clearing, the rest lean the way the key light falls.
	if (UMaterialInterface* AddMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DeepCut/M_DC_Add.M_DC_Add")))
	{
		float Rays = 1.f;
		FParse::Value(FCommandLine::Get(), TEXT("-MCRays="), Rays);
		auto Beam = [this, AddMat](const FVector& Mid, const FVector& Axis, float Wide, float Len, const FLinearColor& Col)
		{
			AActor* A = GM->SpawnGroup(Mid);
			// not AddPart: that paints the part, and painted parts get the look's material again later
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(A);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(GM->CylinderMesh);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->SetupAttachment(A->GetRootComponent());
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(AddMat, C);
			M->SetVectorParameterValue(TEXT("Color"), Col);
			C->SetMaterial(0, M);
			C->RegisterComponent();
			C->SetRelativeRotation(FRotationMatrix::MakeFromZ(Axis).Rotator());
			C->SetRelativeScale3D(FVector(Wide / 50.0, Wide / 50.0, Len / 100.0));
		};
		const FLinearColor Sun(0.030f, 0.075f, 0.085f);
		// the clearing's shaft: seven thin shells, so its edge is a gradient and not a step
		for (int32 i = 0; i < 7; ++i)
		{
			Beam(FVector(-2500.0, -2700.0, 1500.0), FVector::UpVector, 110.f + 60.f * i, 3000.f, Sun * 0.24f * Rays);
		}
		const FVector Down = FRotator(-62.0, 25.0, 0.0).Vector();
		FRandomStream RayRand(4127);
		for (int32 i = 0; i < 16; ++i)
		{
			const FVector Foot(RayRand.FRandRange(-5200.f, 5200.f), RayRand.FRandRange(-5200.f, 5200.f), 0.0);
			if (Foot.Y > 1300.0 && Foot.Y < 3500.0 && Foot.X > 800.0)
			{
				continue; // no sun in the trench
			}
			const float R = RayRand.FRandRange(110.f, 300.f);
			Beam(Foot - Down * 1900.0, -Down, R, 4000.f, Sun * RayRand.FRandRange(0.35f, 0.7f) * Rays);
			Beam(Foot - Down * 1900.0, -Down, R * 1.9f, 4000.f, Sun * 0.22f * Rays);
		}
	}

	// a stack of rock to swim round and over
	Place(Boulder, 0, FVector(-1500.0, 900.0, -100.0), FRotator(0.f, 40.f, 0.f), 7.f);
	Place(Ledge, 0, FVector(-1450.0, 850.0, 600.0), FRotator(0.f, 10.f, 0.f), 5.f);
	Place(Coral, 0, FVector(-1950.0, 450.0, -10.0), FRotator(0.f, 10.f, 0.f), 2.f);

	// loose rock and coral, and a rim of big rock so the site reads as a bowl in the sea floor
	const FVector Keep[] = { FVector(0.0, 0.0, 700.0), FVector(1500.0, 0.0, 1000.0), FVector(3300.0, -700.0, 1500.0), FVector(1250.0, -2900.0, 1300.0),
		FVector(-1500.0, 800.0, 1100.0), FVector(1050.0, -900.0, 1000.0), FVector(2600.0, 2390.0, 2900.0), FVector(-3400.0, 2500.0, 1300.0), FVector(-2500.0, -2700.0, 1300.0) };
	// low mounds of sand so the floor is not a table. They are seen, not solid: what stands on them asks GroundZ.
	if (SandMat)
	{
		FRandomStream MoundRand(3355);
		for (int32 Try = 0; Try < 600 && Mounds.Num() < 70; ++Try)
		{
			const FVector2D At(MoundRand.FRandRange(-7200.f, 7200.f), MoundRand.FRandRange(-7200.f, 7200.f));
			bool bFree = At.Size() > 800.0 && FVector2D::Distance(At, FVector2D(700.0, -560.0)) > 900.0;
			for (const FVector& K : Keep)
			{
				bFree &= FVector2D::Distance(At, FVector2D(K.X, K.Y)) > K.Z * 0.75;
			}
			if (!bFree)
			{
				continue;
			}
			const float High = MoundRand.FRandRange(22.f, 62.f), Deep = 260.f; // the part above the floor, and the ball's half height
			const float Part = FMath::Sqrt(1.f - FMath::Square((Deep - High) / Deep)); // how much of the ball's width shows at floor level
			const float Wide = MoundRand.FRandRange(650.f, 1700.f);
			FMound M;
			M.At = At;
			M.Yaw = MoundRand.FRandRange(0.f, 180.f);
			M.Radii = FVector(Wide / Part, Wide * MoundRand.FRandRange(0.5f, 1.f) / Part, Deep);
			M.CentreZ = High - Deep;
			Mounds.Add(M);
			Bare(GM->SphereMesh, SandMat, FVector(At.X, At.Y, M.CentreZ), FRotator(0.f, M.Yaw, 0.f), M.Radii / 50.0, FLinearColor::White);
		}
	}
	FRandomStream Rand(9041);
	int32 Loose = 0;
	for (int32 Try = 0; Try < 400 && Loose < 24; ++Try)
	{
		const float Ang = Rand.FRandRange(0.f, 2.f * PI);
		const float Rad = Rand.FRandRange(900.f, 5200.f);
		const FVector At(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang), 0.0);
		bool bFree = true;
		for (const FVector& K : Keep)
		{
			bFree &= FVector::Dist2D(At, K) > K.Z;
		}
		if (!bFree)
		{
			continue;
		}
		const int32 V = Rand.RandRange(0, 3);
		const float S = V == 1 ? Rand.FRandRange(0.8f, 1.8f) : Rand.FRandRange(1.2f, 3.4f);
		Place(V == 1 ? Coral : V == 2 ? Ledge : Boulder, 0, At + FVector(0.0, 0.0, GroundZ(At.X, At.Y) - 12.0 * S), FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), S);
		++Loose;
	}
	for (int32 i = 0; i < 16; ++i)
	{
		const float Ang = 2.f * PI * i / 16.f + 0.11f * FMath::Sin(i * 2.3f);
		const float Rad = 7300.f + 350.f * FMath::Sin(i * 1.7f);
		const float S = 15.f + 4.f * FMath::Sin(i * 3.1f);
		Place(i % 3 == 2 ? Ledge : Boulder, 0, FVector(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang), -260.0), FRotator(0.f, 67.f * i, 0.f), i % 3 == 2 ? S * 1.4f : S);
	}

	// set dressing from the art drop: each piece joins in when its model is there. It sits on whatever lies under it.
	UWorld* World = GM->GetWorld();
	auto Scatter = [this, World, &Keep](const TCHAR* Id, int32 Count, float SMin, float SMax, const FVector& Centre, float Spread, bool bCollide, bool bKeepClear, int32 Seed)
	{
		if (!GM->Art.Contains(Id))
		{
			return;
		}
		FRandomStream R(Seed);
		int32 Done = 0;
		for (int32 Try = 0; Try < Count * 12 && Done < Count; ++Try)
		{
			const float Ang = R.FRandRange(0.f, 2.f * PI);
			const float Rad = Spread * FMath::Sqrt(R.FRand());
			FVector At = Centre + FVector(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang), 0.0);
			if (FVector::Dist2D(At, FVector::ZeroVector) < 500.0 || (At.Y > 1900.0 && At.Y < 2900.0 && At.X > 200.0 && At.X < 4400.0 && bCollide))
			{
				continue; // not on the start, nothing solid in the trench
			}
			bool bFree = true;
			if (bKeepClear)
			{
				for (const FVector& K : Keep)
				{
					bFree &= FVector::Dist2D(At, K) > K.Z * 0.8;
				}
			}
			if (!bFree)
			{
				continue;
			}
			FHitResult Hit;
			At.Z = FMath::Max(World->LineTraceSingleByChannel(Hit, At + FVector(0.0, 0.0, 2400.0), At, DC3::Channel, DC3::Query()) ? Hit.Location.Z : 0.0, (double)GroundZ(At.X, At.Y)) - 8.0;
			if (At.Z > 900.0)
			{
				continue; // not on top of the big rock
			}
			AActor* Made = Place(Id, 0, At, FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), R.FRandRange(SMin, SMax), bCollide);
			if (Made && FString(Id) == TEXT("scene:chimney"))
			{
				const FBox Box = Made->GetComponentsBoundingBox();
				PlumeAt.Add(FVector(Box.GetCenter().X, Box.GetCenter().Y, Box.Max.Z));
			}
			++Done;
		}
	};
	const FVector Mid(0.0, 0.0, 0.0);
	const FVector ReefC(-3300.0, 2400.0, 0.0), ShaftC(-2500.0, -2700.0, 0.0), VentC(1200.0, -2900.0, 0.0), WreckC(3300.0, -700.0, 0.0), StationC(1150.0, -1150.0, 0.0);
	Scatter(TEXT("scene:kelp"), 10, 2.2f, 4.f, ReefC, 1500.f, false, false, 11);
	Scatter(TEXT("scene:kelp"), 8, 2.f, 3.4f, Mid, 4800.f, false, true, 12);
	Scatter(TEXT("scene:sponge"), 8, 0.6f, 1.3f, ReefC, 1400.f, false, false, 13);
	Scatter(TEXT("scene:sponge"), 6, 0.6f, 1.2f, Mid, 4500.f, false, false, 14);
	Scatter(TEXT("scene:anemone"), 8, 0.35f, 0.7f, ReefC, 1300.f, false, false, 15);
	Scatter(TEXT("scene:anemone"), 8, 0.35f, 0.7f, ShaftC, 1200.f, false, false, 16);
	Scatter(TEXT("scene:clam"), 6, 0.4f, 0.8f, Mid, 4200.f, false, false, 17);
	Scatter(TEXT("scene:tubeworms"), 8, 0.7f, 1.4f, VentC, 900.f, false, false, 18);
	Scatter(TEXT("scene:chimney"), 3, 2.f, 3.2f, VentC + FVector(500.0, -300.0, 0.0), 900.f, true, false, 19);
	Scatter(TEXT("scene:spire"), 3, 6.f, 10.f, FVector(-800.0, 3800.0, 0.0), 1500.f, true, true, 20);
	Scatter(TEXT("scene:cliff"), 4, 10.f, 14.f, FVector(0.0, -5600.0, 0.0), 2600.f, true, true, 21);
	Scatter(TEXT("scene:whalefall"), 1, 6.f, 6.f, FVector(-700.0, -1700.0, 0.0), 300.f, true, true, 22);
	Scatter(TEXT("scene:anchor"), 1, 1.8f, 1.8f, WreckC + FVector(-900.0, 900.0, 0.0), 200.f, true, false, 23);
	Scatter(TEXT("scene:container"), 2, 2.6f, 3.2f, WreckC + FVector(600.0, -1300.0, 0.0), 500.f, true, false, 24);
	Scatter(TEXT("scene:bell"), 1, 2.2f, 2.2f, StationC + FVector(-900.0, -500.0, 0.0), 150.f, true, false, 25);
	Scatter(TEXT("scene:module"), 1, 5.f, 5.f, StationC + FVector(300.0, -1000.0, 0.0), 150.f, true, false, 26);
	Scatter(TEXT("scene:pipe"), 2, 3.f, 4.f, StationC + FVector(900.0, -300.0, 0.0), 500.f, true, false, 27);
	Scatter(TEXT("scene:sonar"), 1, 1.4f, 1.4f, StationC + FVector(-500.0, 700.0, 0.0), 100.f, true, false, 28);
	Scatter(TEXT("scene:sub"), 1, 4.f, 4.f, FVector(-2300.0, -700.0, 0.0), 300.f, true, true, 29);
	Scatter(TEXT("scene:ruins"), 1, 7.f, 7.f, FVector(-400.0, 4000.0, 0.0), 300.f, true, true, 30);
	// third batch of set dressing
	Scatter(TEXT("scene:seafan"), 9, 0.7f, 1.4f, ReefC, 1400.f, false, false, 31);
	Scatter(TEXT("scene:coraltable"), 5, 1.2f, 2.2f, ReefC, 1500.f, true, false, 32);
	Scatter(TEXT("scene:seagrass"), 26, 0.5f, 1.1f, Mid, 5000.f, false, false, 33);
	Scatter(TEXT("scene:seagrass"), 10, 0.5f, 1.1f, ShaftC, 1300.f, false, false, 34);
	Scatter(TEXT("scene:propeller"), 1, 1.6f, 1.6f, WreckC + FVector(-700.0, -1000.0, 0.0), 200.f, true, false, 35);
	Scatter(TEXT("scene:mast"), 1, 4.f, 4.f, WreckC + FVector(1200.0, 300.0, 0.0), 300.f, true, false, 36);
	Scatter(TEXT("scene:plane"), 1, 6.f, 6.f, FVector(-4300.0, -900.0, 0.0), 400.f, true, true, 37);
	Scatter(TEXT("scene:crates"), 3, 1.1f, 1.6f, StationC + FVector(-300.0, 600.0, 0.0), 600.f, true, false, 38);
	Scatter(TEXT("scene:rov"), 1, 1.1f, 1.1f, StationC + FVector(600.0, 800.0, 0.0), 150.f, true, false, 39);
	Scatter(TEXT("scene:spool"), 2, 1.2f, 1.5f, StationC + FVector(700.0, -700.0, 0.0), 400.f, true, false, 40);
	Scatter(TEXT("scene:statue"), 1, 3.f, 3.f, FVector(-900.0, 3600.0, 0.0), 300.f, true, false, 41);
	Scatter(TEXT("scene:rigleg"), 1, 14.f, 14.f, FVector(4600.0, -3600.0, 0.0), 300.f, true, true, 42);
	Scatter(TEXT("scene:lightpole"), 3, 1.6f, 1.6f, StationC, 900.f, false, false, 43);
	// small things on the sand: stones everywhere, urchins in loose groups
	Scatter(Boulder, 46, 0.14f, 0.42f, Mid, 5600.f, false, false, 44);
	Scatter(TEXT("creature:urchin"), 12, 0.16f, 0.3f, ReefC, 1500.f, false, false, 45);
	Scatter(TEXT("creature:urchin"), 14, 0.16f, 0.3f, Mid, 4600.f, false, false, 46);

	// what the vents give off: warm specks that rise, swell and fade, and a still haze over each mouth. No particle system:
	// a dozen additive balls per mouth, moved by hand in Tick.
	if (UMaterialInterface* AddMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DeepCut/M_DC_Add.M_DC_Add")))
	{
		FRandomStream PR(8128);
		for (const FVector& Mouth : PlumeAt)
		{
			for (int32 i = 0; i < 3; ++i)
			{
				Bare(GM->SphereMesh, AddMat, Mouth + FVector(PR.FRandRange(-30.f, 30.f), PR.FRandRange(-30.f, 30.f), 90.0 + 170.0 * i), FRotator::ZeroRotator,
					FVector(1.6 + 1.3 * i, 1.6 + 1.3 * i, 2.6 + 1.2 * i), FLinearColor(0.050f, 0.030f, 0.016f) * (1.f - 0.25f * i));
			}
			for (int32 i = 0; i < 12; ++i)
			{
				FPlumeBit Bit;
				Bit.Base = Mouth + FVector(PR.FRandRange(-22.f, 22.f), PR.FRandRange(-22.f, 22.f), -10.0);
				Bit.Phase = PR.FRand();
				Bit.Speed = PR.FRandRange(0.11f, 0.2f);
				Bit.Size = PR.FRandRange(0.10f, 0.26f);
				UMaterialInstanceDynamic* M = nullptr;
				Bit.Part = Bare(GM->SphereMesh, AddMat, Bit.Base, FRotator::ZeroRotator, FVector(Bit.Size), FLinearColor::Black, &M);
				Bit.MID = M;
				PlumeBits.Add(Bit);
			}
		}
		UE_LOG(LogMatchCut, Display, TEXT("3D plumes: %d mouths, %d specks, %d mounds"), PlumeAt.Num(), PlumeBits.Num(), Mounds.Num());
	}

	if (GM->Art.Contains(TEXT("scene:buoy")))
	{
		// amber beacons: one by each place the shot list sends you
		const FVector Buoys[] = { FVector(500.0, 1500.0, 0.0), FVector(-2000.0, -1900.0, 0.0), FVector(-2300.0, 1700.0, 0.0) };
		for (const FVector& At : Buoys)
		{
			Place(TEXT("scene:buoy"), 0, At, FRotator::ZeroRotator, 1.4f);
			Glow(At + FVector(0.0, 0.0, 300.0), FLinearColor(1.f, 0.6f, 0.15f), 5000.f, 1000.f);
		}
	}
}

void FDC3D::Start()
{
	UWorld* World = GM->GetWorld();
	GM->LoadArt();

	GM->KeyLight = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-62.0, 25.0, 0.0));
	if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(GM->KeyLight->GetLightComponent()))
	{
		DLC->SetMobility(EComponentMobility::Movable);
		DLC->SetForwardShadingPriority(1);
		DLC->SetDynamicShadowDistanceMovableLight(12000.f);
		// sunlight broken up by the surface: a drifting net of bright lines on everything the key light reaches
		// off unless asked for (-MCCaustics=1): two layers of cell noise per lit pixel took the frame from 28 ms to 150 ms on this Mac
		int32 Caustics = 0;
		FParse::Value(FCommandLine::Get(), TEXT("-MCCaustics="), Caustics);
		if (UMaterialInterface* Net = Caustics ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DeepCut/M_DC_Caustics.M_DC_Caustics")) : nullptr)
		{
			DLC->SetLightFunctionMaterial(Net);
			DLC->SetLightFunctionFadeDistance(60000.f);
			UE_LOG(LogMatchCut, Display, TEXT("3D caustics on the key light"));
		}
	}
	GM->FillLight = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-20.0, 200.0, 0.0));
	if (ULightComponent* LC = GM->FillLight->GetLightComponent())
	{
		LC->SetMobility(EComponentMobility::Movable);
		LC->SetCastShadows(false);
	}
	GM->Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0, 0, 0), FRotator::ZeroRotator);

	BuildArea();
	// shafts of light: long bars high above, never seen, that cut the key light. They sway, so the light on the floor moves.
	{
		// flat pieces scattered high above, never seen: their shadows dapple the floor and cut the light into shafts
		// thousands of small flat pieces in one instanced mesh, not a few long bars: a bar's shadow is a hard rectangle on the floor,
		// small overlapping pieces read as light broken up by the water above. The whole sheet drifts.
		FRandomStream Rand(77);
		AActor* A = GM->SpawnGroup(FVector(0.0, 0.0, 3700.0));
		UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(A);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(GM->CubeMesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(A->GetRootComponent());
		C->RegisterComponent();
		// off unless asked for (-MCDapple=9000): every version tried drew a pattern on the floor that fought the creatures for the eye
		int32 Dapple = 0;
		FParse::Value(FCommandLine::Get(), TEXT("-MCDapple="), Dapple);
		for (int32 i = 0; i < Dapple; ++i)
		{
			C->AddInstance(FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f),
				FVector(Rand.FRandRange(-8800.f, 8800.f), Rand.FRandRange(-8800.f, 8800.f), Rand.FRandRange(-250.f, 250.f)),
				FVector(Rand.FRandRange(0.9f, 3.0f), Rand.FRandRange(0.6f, 1.7f), 0.1)));
		}
		C->SetCastShadow(true);
		C->bCastHiddenShadow = true;
		C->MarkRenderStateDirty();
		A->SetActorHiddenInGame(true);
		Slats.Add(A);
		SlatBase.Add(A->GetActorLocation());
	}
	SpawnHero();
	SpawnFlecks();

	ACameraActor* C = GM->SpawnCamera(Pos - FVector(DC3::BoomLen, 0.0, 0.0), FRotator::ZeroRotator);
	C->GetCameraComponent()->SetFieldOfView(Fov);
	{
		// the film look: a lens that fringes a little at the edges and blooms on the lamps
		FPostProcessSettings& PP = C->GetCameraComponent()->PostProcessSettings;
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = 0.9f;
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = 0.8f;
		PP.bOverride_FilmToe = PP.bOverride_FilmSlope = true;
		PP.FilmToe = 0.62f;
		PP.FilmSlope = 0.92f;
		bLumen = FParse::Param(FCommandLine::Get(), TEXT("MCLumen"));
		if (bLumen)
		{
			PP.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
			PP.ReflectionMethod = EReflectionMethod::Lumen;
		}
	}
	Cam = C;
	GM->SetView(C);

	// the 3D look: look A of the layout, with fog thin enough to see across the area
	FDCLook L = GM->Looks.Contains(TEXT("A")) ? GM->Looks[TEXT("A")] : FDCLook();
	// water takes the red out of light first: what the sun still gives down here is blue-green, and the far water is deep blue
	L.FogDensity = 0.05f;
	L.FogColor = FLinearColor(0.02f, 0.14f, 0.26f);
	L.VolExtinction = 0.35f;
	L.VolEmissive = 0.5f;
	L.VolScatter = 10.f;
	L.KeyIntensity = 9.f;
	L.KeyColor = FLinearColor(0.38f, 0.82f, 1.f);
	L.FillIntensity = 2.5f;
	L.FillColor = FLinearColor(0.12f, 0.35f, 0.75f);
	L.Ambient = 0.2f;
	FParse::Value(FCommandLine::Get(), TEXT("-MCScatter="), L.VolScatter);
	L.Vignette = 0.5f;
	L.Grain = 0.18f;
	L.Saturation = 0.88f;
	L.Contrast = 1.12f;
	L.Tint = FLinearColor(0.94f, 1.f, 1.07f);
	L.SnowColor = FLinearColor(0.22f, 0.36f, 0.46f);
	L.bShafts = false;
	FParse::Value(FCommandLine::Get(), TEXT("-MCFog="), L.FogDensity);
	FParse::Value(FCommandLine::Get(), TEXT("-MCVolExt="), L.VolExtinction);
	FParse::Value(FCommandLine::Get(), TEXT("-MCVolEm="), L.VolEmissive);
	FParse::Value(FCommandLine::Get(), TEXT("-MCAmbient="), L.Ambient);
	FParse::Value(FCommandLine::Get(), TEXT("-MCKey="), L.KeyIntensity);
	GM->Looks.Add(TEXT("3D"), L);
	GM->State = EDCState::Playing;
	GM->Cur = 0;
	GM->ApplyLook(TEXT("3D"));
	if (GM->Fog)
	{
		GM->Fog->GetComponent()->SetVolumetricFogDistance(9000.f);
	}

	FParse::Value(FCommandLine::Get(), TEXT("-MCSens="), Sens);
	Doc = new FDCDoc(GM, this);
	Doc->Start();
	Pivot = Pos + FVector(0.0, 0.0, 45.0);
	TickCamera(0.f);

	// do the sweeps see the models, and are the places the game needs free? The log says so.
	auto Free = [World](const TCHAR* Name, const FVector& A, const FVector& B, float R)
	{
		FHitResult Hit;
		const bool bHit = World->SweepSingleByChannel(Hit, A, B, FQuat::Identity, DC3::Channel, FCollisionShape::MakeSphere(R), DC3::Query());
		UE_LOG(LogMatchCut, Display, TEXT("3D PROBE %s: %s%s"), Name, bHit ? TEXT("BLOCKED") : TEXT("free"), bHit ? *FString::Printf(TEXT(" at (%.0f,%.0f,%.0f) by %s"), Hit.Location.X, Hit.Location.Y, Hit.Location.Z, *GetNameSafe(Hit.GetActor())) : TEXT(""));
	};
	Free(TEXT("arch top (wants BLOCKED)"), FVector(1500.0, 0.0, 2500.0), FVector(1500.0, 0.0, 20.0), 5.f);
	Free(TEXT("through the arch"), FVector(700.0, 0.0, 260.0), FVector(2300.0, 0.0, 260.0), 46.f);
	Free(TEXT("trench, mouth to anglerfish"), FVector(300.0, 2390.0, 250.0), FVector(4050.0, 2390.0, 250.0), 46.f);
	Free(TEXT("trench roof at x=2600 (wants BLOCKED)"), FVector(2600.0, 2390.0, 2000.0), FVector(2600.0, 2390.0, 300.0), 5.f);
	Free(TEXT("crewmate"), FVector(700.0, -560.0, 600.0), FVector(700.0, -560.0, 100.0), 46.f);
	Free(TEXT("shaft"), FVector(-2500.0, -2700.0, 1500.0), FVector(-2500.0, -2700.0, 200.0), 150.f);
	Free(TEXT("shoal"), FVector(-2900.0, 2250.0, 1500.0), FVector(-2900.0, 2250.0, 450.0), 400.f);
	UE_LOG(LogMatchCut, Display, TEXT("3D started: %d paints"), GM->Paints.Num());
}

// ---------------------------------------------------------------------------------------------
// every frame
// ---------------------------------------------------------------------------------------------

void FDC3D::MoveHero(const FVector& Delta)
{
	UWorld* World = GM->GetWorld();
	const FCollisionShape Ball = FCollisionShape::MakeSphere(Radius);
	FVector Left = Delta;
	for (int32 Step = 0; Step < 4 && Left.SizeSquared() > 0.0001; ++Step)
	{
		FHitResult Hit;
		if (!World->SweepSingleByChannel(Hit, Pos, Pos + Left, FQuat::Identity, DC3::Channel, Ball, DC3::Query()))
		{
			Pos += Left;
			return;
		}
		if (Hit.bStartPenetrating)
		{
			// inside something (a long frame, or a model that turned): step out along the way the engine says is shortest
			const FVector Out = Hit.Normal.IsNearlyZero() ? FVector::UpVector : Hit.Normal;
			Pos += Out * (Hit.PenetrationDepth + 1.0);
			Vel -= Out * FMath::Min(0.0, FVector::DotProduct(Vel, Out));
			continue;
		}
		const FVector N = Hit.ImpactNormal.IsNearlyZero() ? Hit.Normal : Hit.ImpactNormal;
		Pos += Left * FMath::Max(0.0, Hit.Time - 0.02) + N * 0.3;
		const double Into = FVector::DotProduct(Vel, N);
		if (Into < -260.0 && BumpTime <= 0.f && GM->Audio)
		{
			GM->Audio->Play(TEXT("plate_lift"), 0.35f, 0.7f, 0.1f);
			BumpTime = 0.5f;
		}
		// slide: what is left of the step, and of the speed, runs along the surface
		Left = Left * (1.0 - Hit.Time);
		Left -= N * FVector::DotProduct(Left, N);
		Vel -= N * FMath::Min(0.0, Into);
	}
}

void FDC3D::TickCamera(float Dt)
{
	const FRotator View(Pitch, Yaw, 0.f);
	const FRotationMatrix M(View);
	const FVector F = M.GetUnitAxis(EAxis::X);
	const FVector U = M.GetUnitAxis(EAxis::Z);
	const FVector Want = Pos + FVector(0.0, 0.0, 45.0);
	Pivot = Dt > 0.f ? FMath::Lerp(Pivot, Want, 1.0 - FMath::Exp(-14.0 * Dt)) : Want;
	const float Fast = FMath::Clamp((Vel.Size() - DC3::Swim) / (DC3::Boost - DC3::Swim), 0.0, 1.0);
	const FVector Boom = -F * (DC3::BoomLen + 60.f * Fast) + U * DC3::BoomUp;

	// never inside rock: a ball swept from Unit 0 to where the camera wants to be stops at the first thing in the way.
	// It comes in at once and goes back out slowly, so the picture does not pump next to a wall.
	float Free = 1.f;
	FHitResult Hit;
	if (GM->GetWorld()->SweepSingleByChannel(Hit, Pivot, Pivot + Boom, FQuat::Identity, DC3::Channel, FCollisionShape::MakeSphere(DC3::CamRadius), DC3::Query()))
	{
		Free = Hit.bStartPenetrating ? 0.05f : FMath::Max(0.05f, Hit.Time - 0.02f);
	}
	BoomFrac = Free < BoomFrac ? Free : FMath::Min(Free, BoomFrac + 1.4f * Dt);
	CamPos = Pivot + Boom * BoomFrac;
	CamPos.Z = FMath::Max(CamPos.Z, (double)DC3::CamRadius + GroundZ(CamPos.X, CamPos.Y));
	Fov = FMath::FInterpTo(Fov, 72.f + 9.f * Fast, Dt, 3.f);
	ViewFov = Fov;
	FRotator Rot = View;
	float FocusAt = 0.f; // 0: everything sharp
	bool bHide = FVector::Dist(CamPos, Pos) < 95.0; // too close to see anything but the diver's back
	if (Doc)
	{
		FVector P;
		FRotator R;
		float F2 = 0.f;
		if (Doc->ViewOverride(P, R, F2))
		{
			// a take is playing: the lens is where it was then
			CamPos = P;
			Rot = R;
			ViewFov = F2;
			FocusAt = Doc->Focus;
			bHide = true;
		}
		else if (Doc->AimBlend > 0.001f)
		{
			// the camera is up: the picture is the one the diver's own camera takes, held in front of the chest
			FVector Chest = Pos + FVector(0.0, 0.0, 32.0) + F * 58.0;
			if (GM->GetWorld()->SweepSingleByChannel(Hit, Pos, Chest, FQuat::Identity, DC3::Channel, FCollisionShape::MakeSphere(12.f), DC3::Query()))
			{
				Chest = Pos + (Chest - Pos) * FMath::Max(0.0, Hit.Time - 0.05);
			}
			CamPos = FMath::Lerp(CamPos, Chest, (double)Doc->AimBlend);
			ViewFov = FMath::Lerp(Fov, Doc->Zoom, Doc->AimBlend);
			bHide |= Doc->AimBlend > 0.35f;
			FocusAt = Doc->AimBlend > 0.6f ? Doc->Focus : 0.f;
		}
	}
	if (ACameraActor* C = Cam.Get())
	{
		C->SetActorLocationAndRotation(CamPos, Rot);
		C->GetCameraComponent()->SetFieldOfView(ViewFov);
		// a film lens: what it is not focused on goes soft, more so the longer the lens
		FPostProcessSettings& PP = C->GetCameraComponent()->PostProcessSettings;
		PP.bOverride_DepthOfFieldFocalDistance = PP.bOverride_DepthOfFieldFstop = PP.bOverride_DepthOfFieldSensorWidth = true;
		PP.DepthOfFieldFocalDistance = FocusAt;
		PP.DepthOfFieldFstop = 2.2f;
		PP.DepthOfFieldSensorWidth = 30.f;
	}
	if (AActor* A = GM->Hero.Actor.Get())
	{
		A->SetActorHiddenInGame(bHide);
	}
}

void FDC3D::TickRings(const FVector& From)
{
	for (FDC3DRing& R : Rings)
	{
		if (R.bDone)
		{
			continue;
		}
		const double A = FVector::DotProduct(From - R.Pos, R.Axis);
		const double B = FVector::DotProduct(Pos - R.Pos, R.Axis);
		if ((A < 0.0) == (B < 0.0))
		{
			continue;
		}
		const FVector Cross = From + (Pos - From) * (A / (A - B));
		if (FVector::Dist(Cross, R.Pos) < R.Radius)
		{
			R.bDone = true;
			++RingsDone;
			RingFlash = 1.f;
			for (int32 P : R.Paints)
			{
				if (GM->Paints.IsValidIndex(P))
				{
					GM->Paints[P].Color = FLinearColor(0.1f, 0.8f, 0.7f);
					if (UMaterialInstanceDynamic* MID = GM->Paints[P].MID.Get())
					{
						MID->SetVectorParameterValue(TEXT("Color"), GM->Paints[P].Color);
					}
				}
				GM->SetPaintEmissive(P, FLinearColor(0.3f, 4.f, 3.4f));
			}
			if (GM->Audio)
			{
				GM->Audio->Play(TEXT("part_pickup"), 0.7f, 1.f + 0.06f * RingsDone);
			}
			UE_LOG(LogMatchCut, Display, TEXT("3D RING %d/%d"), RingsDone, Rings.Num());
		}
	}
}

void FDC3D::Tick(float Dt)
{
	Time += Dt;
	for (const FPlumeBit& Bit : PlumeBits)
	{
		UStaticMeshComponent* Part = Bit.Part.Get();
		UMaterialInstanceDynamic* M = Bit.MID.Get();
		if (Part && M)
		{
			const float U = FMath::Frac(Time * Bit.Speed + Bit.Phase); // 0 at the mouth, 1 where it is gone
			const float Sway = 70.f * U * U;
			Part->SetWorldLocation(Bit.Base + FVector(Sway * FMath::Sin(Time * 0.7f + Bit.Phase * 20.f), Sway * FMath::Cos(Time * 0.5f + Bit.Phase * 33.f), 620.0 * U));
			Part->SetWorldScale3D(FVector(Bit.Size * (0.6f + 2.6f * U)));
			const float Shimmer = 0.6f + 0.4f * FMath::Sin(Time * 9.f + Bit.Phase * 50.f);
			M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.26f, 0.09f) * (FMath::Square(1.f - U) * FMath::Min(1.f, U * 8.f) * Shimmer));
		}
	}
	BumpTime = FMath::Max(0.f, BumpTime - Dt);
	RingFlash = FMath::Max(0.f, RingFlash - Dt);

	// ---- input: the player's, or the script's
	FVector In = Move;
	bool bFast = bBoost;
	FVector2D Turn = LookDelta + LookRate * Dt;
	LookDelta = FVector2D::ZeroVector;
	if (GM->bScriptActive)
	{
		In = FVector::ZeroVector;
		bFast = false;
		Turn = FVector2D::ZeroVector;
		if (ScriptMoveLeft > 0.f)
		{
			ScriptMoveLeft -= Dt;
			In = ScriptMove;
			bFast = bScriptBoost;
		}
		if (ScriptTurnLeft > 0.f)
		{
			ScriptTurnLeft -= Dt;
			Turn = ScriptTurn * Dt;
		}
		if (bSwimTo)
		{
			// look at the target and swim forward
			const FVector To = SwimTarget - Pos;
			SwimTime += Dt;
			const FRotator Aim = To.Rotation();
			Yaw = DC3::TurnTo(Yaw, Aim.Yaw, 5.f, Dt);
			Pitch = DC3::TurnTo(Pitch, FMath::Clamp(Aim.Pitch, -80.f, 80.f), 5.f, Dt);
			In = FVector(To.Size() > 260.0 || Vel.Size() < 150.0 ? 1.0 : 0.0, 0.0, 0.0);
			if (To.Size() < 90.0 || SwimTime > 30.f)
			{
				UE_LOG(LogMatchCut, Display, TEXT("3D swim ended at (%.0f,%.0f,%.0f) after %.1fs, wanted (%.0f,%.0f,%.0f)%s"), Pos.X, Pos.Y, Pos.Z, SwimTime,
					SwimTarget.X, SwimTarget.Y, SwimTarget.Z, To.Size() < 90.0 ? TEXT("") : TEXT("  NOT REACHED"));
				bSwimTo = false;
				SwimTime = -1.f; // tells the script line that its swim is over
			}
		}
	}
	const bool bShoot = !Doc || Doc->State == EDCDocState::Shooting;
	if (!bShoot)
	{
		In = FVector::ZeroVector;
		Turn = FVector2D::ZeroVector;
		Vel = FVector::ZeroVector;
	}
	const float Aim = Doc ? Doc->AimBlend : 0.f;
	if (Doc && Doc->TrackLeft > 0.f && Doc->TrackSub != INDEX_NONE)
	{
		// test script: the lens follows a subject
		Doc->TrackLeft -= Dt;
		const FRotator To = (Doc->SubPos[Doc->TrackSub] - CamPos).Rotation();
		Yaw = DC3::TurnTo(Yaw, To.Yaw, 9.f, Dt);
		Pitch = DC3::TurnTo(Pitch, FMath::Clamp(To.Pitch, -80.f, 80.f), 9.f, Dt);
	}
	Turn *= FMath::Lerp(1.f, ViewFov / 72.f, Aim); // a long lens turns slowly
	Yaw = FRotator::NormalizeAxis(Yaw + Turn.X);
	Pitch = FMath::Clamp(Pitch + Turn.Y, -82.f, 82.f);

	// ---- swimming
	const FRotationMatrix ViewM(FRotator(Pitch, Yaw, 0.f));
	const FVector F = ViewM.GetUnitAxis(EAxis::X);
	const FVector R = FRotationMatrix(FRotator(0.f, Yaw, 0.f)).GetUnitAxis(EAxis::Y);
	FVector Wish = F * In.X + R * In.Y + FVector::UpVector * In.Z;
	if (Wish.SizeSquared() > 1.0)
	{
		Wish.Normalize();
	}
	const bool bThrust = Wish.SizeSquared() > 0.01;
	IdleTime = bThrust || !Turn.IsNearlyZero() ? 0.f : IdleTime + Dt;
	if (bThrust)
	{
		Vel += (Wish * ((bFast ? DC3::Boost : DC3::Swim) * FMath::Lerp(1.f, 0.5f, Aim)) - Vel) * (1.0 - FMath::Exp(-DC3::Push * Dt)); // slower with the camera up
	}
	else
	{
		Vel *= FMath::Exp(-DC3::Drag * Dt); // the glide
		if (Vel.SizeSquared() < 16.0)
		{
			Vel = FVector::ZeroVector;
		}
	}
	// the edge of the area and the bright water high up: a current that grows the further out Unit 0 goes
	const double Out = FVector(Pos.X, Pos.Y, 0.0).Size() - DC3::AreaRadius;
	if (Out > 0.0)
	{
		Vel -= FVector(Pos.X, Pos.Y, 0.0).GetSafeNormal() * FMath::Min(Out, 900.0) * 3.0 * Dt;
	}
	if (Pos.Z > DC3::Ceiling)
	{
		Vel.Z -= FMath::Min(Pos.Z - DC3::Ceiling, 900.0) * 3.0 * Dt;
	}
	const FVector From = Pos;
	MoveHero(Vel * Dt);
	// the mounds are not solid: the diver is lifted over them here
	const double SandTop = GroundZ(Pos.X, Pos.Y);
	if (SandTop > 0.0 && Pos.Z < SandTop + Radius)
	{
		Pos.Z = SandTop + Radius;
		Vel.Z = FMath::Max(Vel.Z, 0.0);
	}
	TickRings(From);

	ThrustTime -= Dt;
	if (bThrust && ThrustTime <= 0.f && GM->Audio)
	{
		GM->Audio->Play(TEXT("thruster"), bFast ? 0.9f : 0.6f, bFast ? 1.15f : 1.f, 0.05f);
		ThrustTime = bFast ? 0.7f : 1.1f;
	}

	// ---- the body: turns to where it is pushed, leans into its speed, banks in a turn, bobs when it floats
	if (AActor* A = GM->Hero.Actor.Get())
	{
		const FVector Flat(Wish.X, Wish.Y, 0.0);
		float WantYaw = BodyYaw;
		if (Flat.SizeSquared() > 0.04)
		{
			WantYaw = Flat.Rotation().Yaw;
		}
		else if (bThrust)
		{
			WantYaw = Yaw; // straight up or down: the back stays to the camera
		}
		const float Before = BodyYaw;
		BodyYaw = DC3::TurnTo(BodyYaw, WantYaw, 7.f, Dt);
		const float YawRate = Dt > 0.f ? FMath::FindDeltaAngleDegrees(Before, BodyYaw) / Dt : 0.f;
		const FVector Local = FRotator(0.f, BodyYaw, 0.f).UnrotateVector(Vel) / DC3::Swim;
		// swimming, it lies forward into the water; floating, it stands
		const float WantPitch = FMath::Clamp(-36.f * (float)Local.X + 22.f * (float)Local.Z, -68.f, 40.f);
		const float WantRoll = FMath::Clamp(YawRate * 0.14f + 14.f * (float)Local.Y, -30.f, 30.f);
		BodyPitch = FMath::FInterpTo(BodyPitch, WantPitch, Dt, 5.f);
		BodyRoll = FMath::FInterpTo(BodyRoll, WantRoll, Dt, 5.f);
		const double Bob = 5.0 * FMath::Sin(Time * 1.3) * (1.0 - FMath::Clamp(Vel.Size() / 300.0, 0.0, 1.0));
		A->SetActorLocationAndRotation(Pos + FVector(0.0, 0.0, Bob), FRotator(BodyPitch, BodyYaw, BodyRoll));
	}

	if (Doc)
	{
		Doc->Tick(Dt);
	}
	TickCamera(Dt);
	{
		// the trench: the roof keeps the sun out, and the lens' eye opens no further. The deeper in, the less of the fill
		// light and of the water's own glow is left.
		const bool bIn = CamPos.Y > 1850.0 && CamPos.Y < 2950.0 && CamPos.Z < 620.0;
		const float Want = bIn ? FMath::Clamp(((float)CamPos.X - 1000.f) / 900.f, 0.f, 1.f) : 0.f;
		Dark = Dt > 0.f ? FMath::FInterpTo(Dark, Want, Dt, 2.5f) : Want;
		const float K = 1.f - 0.94f * Dark;
		if (GM->KeyLight)
		{
			GM->KeyLight->GetLightComponent()->SetIntensity(GM->Look.KeyIntensity * (1.f - 0.8f * Dark));
		}
		if (GM->FillLight)
		{
			GM->FillLight->GetLightComponent()->SetIntensity(GM->Look.FillIntensity * K);
		}
		if (GM->Fog)
		{
			GM->Fog->GetComponent()->SetFogInscatteringColor(GM->Look.FogColor * K);
			GM->Fog->GetComponent()->SetVolumetricFogEmissive(GM->Look.FogColor * (GM->Look.VolEmissive * K));
		}
	}
	if (UPointLightComponent* Rim = GM->HeroRim.Get())
	{
		// between the camera and Unit 0, a little above: never inside the body (the model is double-sided, a light inside turns it white)
		Rim->SetWorldLocation(Pos + (CamPos - Pos).GetSafeNormal() * 170.0 + FVector(0.0, 0.0, 110.0));
	}

	// ---- the water: flecks that sink and bubbles that rise, around the camera wherever it goes
	const double P = DC3::FleckPeriod;
	auto Around = [this, P](double Drift)
	{
		const FVector C = CamPos + FVector(0.0, 0.0, Drift);
		return FVector(FMath::RoundToDouble(C.X / P) * P + P * 0.5, FMath::RoundToDouble(C.Y / P) * P + P * 0.5, FMath::RoundToDouble(C.Z / P) * P + P * 0.5 - Drift);
	};
	if (AActor* A = Snow.Get())
	{
		A->SetActorLocation(Around(FMath::Fmod(Time * 14.0, P)));
		if (UMaterialInstanceDynamic* MID = SnowMID.Get())
		{
			MID->SetVectorParameterValue(TEXT("Color"), GM->Look.SnowColor);
		}
	}
	if (AActor* A = Bubbles.Get())
	{
		A->SetActorLocation(Around(-FMath::Fmod(Time * 80.0, P)));
		if (UMaterialInstanceDynamic* MID = BubbleMID.Get())
		{
			MID->SetVectorParameterValue(TEXT("Color"), GM->Look.SnowColor * 1.6f);
		}
	}
	for (int32 i = 0; i < Slats.Num() && i < SlatBase.Num(); ++i)
	{
		if (AActor* A = Slats[i].Get())
		{
			A->SetActorLocation(SlatBase[i] + A->GetActorRightVector() * (220.0 * FMath::Sin(Time * 0.31 + i * 1.7) + 90.0 * FMath::Sin(Time * 0.83 + i * 0.9)));
		}
	}
	for (int32 i = 0; i < Glows.Num(); ++i)
	{
		if (UPointLightComponent* L = Glows[i].Get())
		{
			// a slow breath, each light out of step with the next
			L->SetVolumetricScatteringIntensity(1.2f + 0.5f * FMath::Sin(Time * 0.9f + i * 1.9f));
		}
	}
}

// ---------------------------------------------------------------------------------------------
// test script
// ---------------------------------------------------------------------------------------------

int32 FDC3D::RunLine(const TArray<FString>& T)
{
	const FString& Cmd = T[0];
	auto Num = [&T](int32 i) { return T.IsValidIndex(i) ? FCString::Atof(*T[i]) : 0.f; };
	if (const int32 Done = Doc ? Doc->RunLine(T) : 0)
	{
		return Done;
	}
	if (Cmd == TEXT("tp"))
	{
		// tp X Y Z [YAW PITCH]
		Pos = FVector(Num(1), Num(2), Num(3));
		Vel = FVector::ZeroVector;
		ScriptMoveLeft = 0.f; // a long frame can leave a little of the last key3
		if (T.Num() > 4)
		{
			Yaw = Num(4);
			Pitch = Num(5);
			BodyYaw = Yaw;
		}
		Pivot = Pos + FVector(0.0, 0.0, 45.0);
		BoomFrac = 1.f;
		return 1;
	}
	if (Cmd == TEXT("look3"))
	{
		Yaw = Num(1);
		Pitch = Num(2);
		return 1;
	}
	if (Cmd == TEXT("turn3"))
	{
		// turn3 YAWRATE PITCHRATE SECONDS
		ScriptTurn = FVector2D(Num(1), Num(2));
		ScriptTurnLeft = Num(3);
		GM->ScriptWait = Num(3);
		return 1;
	}
	if (Cmd == TEXT("key3"))
	{
		// key3 FORWARD RIGHT UP SECONDS [boost]: hold the keys; the script goes on after SECONDS
		ScriptMove = FVector(Num(1), Num(2), Num(3));
		ScriptMoveLeft = Num(4);
		bScriptBoost = T.IsValidIndex(5) && T[5] == TEXT("boost");
		GM->ScriptWait = Num(4);
		return 1;
	}
	if (Cmd == TEXT("swim3"))
	{
		// swim3 X Y Z: look at the point and swim there; the script waits
		if (SwimTime < 0.f)
		{
			SwimTime = 0.f;
			return 1;
		}
		if (!bSwimTo)
		{
			bSwimTo = true;
			SwimTarget = FVector(Num(1), Num(2), Num(3));
			SwimTime = 0.f;
		}
		GM->ScriptWait = 0.05f;
		--GM->ScriptPc;
		return 2;
	}
	if (Cmd == TEXT("stat3"))
	{
		UE_LOG(LogMatchCut, Display, TEXT("3D STAT pos=(%.0f,%.0f,%.0f) speed=%.0f yaw=%.0f pitch=%.0f boom=%.2f cam=(%.0f,%.0f,%.0f) rings=%d/%d"), Pos.X, Pos.Y, Pos.Z, Vel.Size(), Yaw, Pitch,
			BoomFrac, CamPos.X, CamPos.Y, CamPos.Z, RingsDone, Rings.Num());
		return 1;
	}
	return 0;
}
