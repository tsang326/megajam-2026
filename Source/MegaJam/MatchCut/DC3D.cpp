// Deep Cut (brief 06, checkpoint M1): free 3D swimming, a camera behind Unit 0 that never sits inside rock,
// and one test area built from the art-drop models.
//   world space: z up, cm. The sea floor is z = 0.
//   movement: the keys push Unit 0 towards a speed, the water takes the speed away slowly when the keys are let go
//   collision: engine sweeps against the models themselves (their triangles), sliding along what is hit

#include "DC3D.h"
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
	const float AreaRadius = 5400.f; // a current turns Unit 0 back past this
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
		if (bCollide)
		{
			C->SetCollisionObjectType(ECC_WorldStatic);
			C->SetCollisionResponseToAllChannels(ECR_Block);
			C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
	return A;
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
				bool bMap = false;
				FGuid Guid;
				MID->GetStaticSwitchParameterValue(FName(TEXT("bHasEmissiveTexture")), bMap, Guid);
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
	USpotLightComponent* L = NewObject<USpotLightComponent>(A);
	L->SetMobility(EComponentMobility::Movable);
	L->SetupAttachment(A->GetRootComponent());
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
	const FLinearColor Sand(0.20f, 0.19f, 0.16f);
	Block(FVector(0.0, 0.0, -150.0), FRotator::ZeroRotator, FVector(22000.0, 22000.0, 300.0), Sand);

	// the arch straight ahead of the start: two pillars and a slab across them
	Place(TEXT("solid:rock"), 1, FVector(1500.0, -440.0, -60.0), FRotator(0.f, 20.f, 0.f), 5.2f);
	Place(TEXT("solid:rock"), 1, FVector(1500.0, 440.0, -60.0), FRotator(0.f, 200.f, 0.f), 5.6f);
	Place(TEXT("solid:rock"), 2, FVector(1500.0, 0.0, 880.0), FRotator(0.f, 90.f, 0.f), 6.4f);
	AddRing(FVector(1500.0, 0.0, 470.0), 0.f, 0.f);

	// the wreck, with what fell off it
	Place(TEXT("solid:hull"), 0, FVector(3300.0, -700.0, -80.0), FRotator(0.f, 35.f, 0.f), 8.f);
	Place(TEXT("solid:metal"), 0, FVector(2500.0, -1500.0, -30.0), FRotator(0.f, 70.f, 0.f), 3.0f);
	Place(TEXT("solid:metal"), 0, FVector(3900.0, 500.0, -40.0), FRotator(0.f, 160.f, 0.f), 2.2f);
	Place(TEXT("plate"), 0, FVector(2550.0, -350.0, 0.0), FRotator(0.f, 25.f, 0.f), 1.6f);
	Place(TEXT("weight"), 0, FVector(2900.0, 300.0, 0.0), FRotator(0.f, 110.f, 0.f), 1.4f);
	Place(TEXT("socket:DOCK"), 0, FVector(2300.0, 620.0, 0.0), FRotator(0.f, -60.f, 0.f), 2.4f);
	Place(TEXT("cell"), 0, FVector(2300.0, 620.0, 150.0), FRotator(0.f, -60.f, 0.f), 0.8f, false);
	Place(TEXT("crew"), 0, FVector(2620.0, 880.0, 0.0), FRotator(0.f, 200.f, 0.f), 0.75f);
	Glow(FVector(2300.0, 620.0, 330.0), FLinearColor(0.2f, 1.f, 0.85f), 5000.f, 900.f);
	AddRing(FVector(3300.0, -700.0, 1420.0), 35.f, 0.f);

	// the vent field: warm light low in the water
	const FVector Vents[] = { FVector(900.0, -2300.0, 0.0), FVector(1500.0, -2700.0, 0.0), FVector(1750.0, -2050.0, 0.0), FVector(700.0, -3000.0, 0.0) };
	const float VentScale[] = { 2.8f, 2.2f, 1.8f, 3.2f };
	for (int32 i = 0; i < 4; ++i)
	{
		Place(TEXT("socket:VENT"), 0, Vents[i] - FVector(0.0, 0.0, 20.0), FRotator(0.f, 75.f * i, 0.f), VentScale[i]);
		Glow(Vents[i] + FVector(0.0, 0.0, 150.0 * VentScale[i]), FLinearColor(1.f, 0.45f, 0.12f), 4200.f, 1100.f);
	}
	AddRing(FVector(1250.0, -2450.0, 620.0), 90.f, 0.f);

	// the gully: boulders on both sides and slabs over it. The tight spot for the camera.
	for (int32 i = 0; i < 4; ++i)
	{
		const double X = 500.0 + 760.0 * i;
		Place(TEXT("solid:rock"), 0, FVector(X, 1700.0 - 40.0 * (i % 2), -90.0), FRotator(0.f, 90.f * i, 0.f), 5.6f);
		Place(TEXT("solid:rock"), 0, FVector(X + 120.0, 3080.0 + 50.0 * (i % 2), -90.0), FRotator(0.f, 45.f + 90.f * i, 0.f), 5.8f);
	}
	for (int32 i = 0; i < 3; ++i)
	{
		Place(TEXT("solid:rock"), 2, FVector(760.0 + 900.0 * i, 2390.0, 560.0 + 30.0 * (i % 2)), FRotator(0.f, 80.f + 15.f * i, 0.f), 7.4f);
	}
	AddRing(FVector(1650.0, 2390.0, 300.0), 0.f, 0.f);
	Glow(FVector(1650.0, 2390.0, 380.0), FLinearColor(0.25f, 0.9f, 1.f), 2600.f, 900.f);

	// a spire to climb, with a ring high up
	Place(TEXT("solid:rock"), 1, FVector(-1500.0, 900.0, -100.0), FRotator(0.f, 40.f, 0.f), 11.f);
	Place(TEXT("solid:rock"), 0, FVector(-1750.0, 520.0, -80.0), FRotator(0.f, 10.f, 0.f), 4.f);
	AddRing(FVector(-900.0, 420.0, 1750.0), 150.f, 20.f);
	AddRing(FVector(-1900.0, -1300.0, 260.0), 200.f, 0.f);

	// loose boulders, and a rim of big rock so the area reads as a bowl in the sea floor
	const FVector Keep[] = { FVector(0.0, 0.0, 700.0), FVector(1500.0, 0.0, 900.0), FVector(3300.0, -700.0, 1500.0), FVector(1250.0, -2500.0, 1300.0),
		FVector(1650.0, 2390.0, 1900.0), FVector(-1500.0, 800.0, 1100.0), FVector(-1900.0, -1300.0, 500.0), FVector(2500.0, 600.0, 800.0) };
	FRandomStream Rand(9041);
	int32 Loose = 0;
	for (int32 Try = 0; Try < 400 && Loose < 26; ++Try)
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
		const int32 V = Rand.RandRange(0, 2);
		const float S = V == 1 ? Rand.FRandRange(1.6f, 4.2f) : Rand.FRandRange(1.2f, 3.4f);
		Place(TEXT("solid:rock"), V, At - FVector(0.0, 0.0, 12.0 * S), FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), S);
		++Loose;
	}
	for (int32 i = 0; i < 16; ++i)
	{
		const float Ang = 2.f * PI * i / 16.f + 0.11f * FMath::Sin(i * 2.3f);
		const float Rad = 6500.f + 350.f * FMath::Sin(i * 1.7f);
		const float S = 15.f + 4.f * FMath::Sin(i * 3.1f);
		Place(TEXT("solid:rock"), i % 3 == 2 ? 2 : 0, FVector(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang), -260.0), FRotator(0.f, 67.f * i, 0.f), i % 3 == 2 ? S * 1.4f : S);
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
		FRandomStream Rand(77);
		for (int32 i = 0; i < 150; ++i)
		{
			const FVector At(Rand.FRandRange(-8500.f, 8500.f), Rand.FRandRange(-8500.f, 8500.f), 3600.0 + Rand.FRandRange(0.f, 500.f));
			AActor* A = GM->SpawnBox(At, FRotator(0.f, Rand.FRandRange(0.f, 180.f), 0.f), FVector(Rand.FRandRange(500.f, 1700.f), Rand.FRandRange(160.f, 420.f), 30.0), FLinearColor::Black, false);
			if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(A))
			{
				SMA->GetStaticMeshComponent()->SetCastShadow(true);
				SMA->GetStaticMeshComponent()->bCastHiddenShadow = true;
				SMA->GetStaticMeshComponent()->MarkRenderStateDirty();
			}
			A->SetActorHiddenInGame(true);
			Slats.Add(A);
			SlatBase.Add(A->GetActorLocation());
		}
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
	Pivot = Pos + FVector(0.0, 0.0, 45.0);
	TickCamera(0.f);

	// does the hero's sweep see the models? One line trace straight down onto the arch's slab says so in the log.
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, FVector(1500.0, 0.0, 2500.0), FVector(1500.0, 0.0, 500.0), DC3::Channel, DC3::Query());
	UE_LOG(LogMatchCut, Display, TEXT("3D started: %d rings, %d paints, collision probe on the arch: %s at z=%.0f"), Rings.Num(), GM->Paints.Num(),
		bHit ? TEXT("HIT") : TEXT("NO HIT"), bHit ? Hit.Location.Z : 0.0);
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
	CamPos.Z = FMath::Max(CamPos.Z, (double)DC3::CamRadius);
	Fov = FMath::FInterpTo(Fov, 72.f + 9.f * Fast, Dt, 3.f);
	if (ACameraActor* C = Cam.Get())
	{
		C->SetActorLocationAndRotation(CamPos, View);
		C->GetCameraComponent()->SetFieldOfView(Fov);
	}
	// too close to see anything but the robot's back: it steps out of the picture
	if (AActor* A = GM->Hero.Actor.Get())
	{
		A->SetActorHiddenInGame(FVector::Dist(CamPos, Pos) < 95.0);
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
		Vel += (Wish * (bFast ? DC3::Boost : DC3::Swim) - Vel) * (1.0 - FMath::Exp(-DC3::Push * Dt));
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

	TickCamera(Dt);
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
	if (Cmd == TEXT("tp"))
	{
		// tp X Y Z [YAW PITCH]
		Pos = FVector(Num(1), Num(2), Num(3));
		Vel = FVector::ZeroVector;
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
