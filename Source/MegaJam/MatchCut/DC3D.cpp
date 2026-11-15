// Deep Cut (brief 06, checkpoint M1): free 3D swimming, a camera behind Unit 0 that never sits inside rock,
// and one test area built from the art-drop models.
//   world space: z up, cm. The sea floor is z = 0.
//   movement: the keys push Unit 0 towards a speed, the water takes the speed away slowly when the keys are let go
//   collision: engine sweeps against the models themselves (their triangles), sliding along what is hit

#include "DC3D.h"
#include "ProceduralMeshComponent.h"
#include "DCDoc.h"
#include "MatchCutGameMode.h"
#include "DCAudio.h"
#include "DCMenu.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
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
	const float Swim = 660.f;        // cm/s
	const float Boost = 1150.f;
	const float Push = 2.4f;         // 1/s: how fast the thrusters reach the wanted speed
	const float Drag = 0.75f;        // 1/s: how fast the water takes speed away while gliding
	const float BoomLen = 440.f;
	const float BoomUp = 70.f;
	const float CamRadius = 26.f;
	const float AreaRadius = 25000.f; // a current turns Unit 0 back past this (brief 08: the wider sea)
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

// the soft things on the floor (kelp, grass, fans, anemones, tube worms) sway about their roots in a swell that rolls
// across the site, neighbours together, as grass does in wind. Each Place root is at the sand, so a turn of the actor is
// a bend from the root. One sea at a time: the list is kept here and starts again with a new world.
struct FDCSoft
{
	TWeakObjectPtr<AActor> A;
	FRotator Rot;
	FVector Base;
	float Amp;
	float Phase;
};
static TArray<FDCSoft> GSoft;
static TWeakObjectPtr<UWorld> GSoftWorld;
static void AddSoft(AActor* A, const TCHAR* Id, const FVector& Base, const FRotator& Rot)
{
	struct FKind { const TCHAR* Id; float Amp; };
	static const FKind Kinds[] = { { TEXT("scene:kelp"), 6.f }, { TEXT("scene:seagrass"), 9.f }, { TEXT("scene:seafan"), 3.5f },
		{ TEXT("scene:seapen"), 5.f }, { TEXT("scene:crinoid"), 4.f }, { TEXT("scene:anemone"), 3.f }, { TEXT("scene:tubeworms"), 3.f } };
	for (const FKind& K : Kinds)
	{
		if (FString(Id).StartsWith(K.Id))
		{
			if (GSoftWorld.Get() != A->GetWorld())
			{
				GSoft.Reset();
				GSoftWorld = A->GetWorld();
			}
			GSoft.Add({ A, Rot, Base, K.Amp, FMath::FRand() * 6.f });
			return;
		}
	}
}
static void TickSoft(UWorld* World, const FVector& Cam, float Time)
{
	if (GSoftWorld.Get() != World)
	{
		return;
	}
	for (const FDCSoft& S : GSoft)
	{
		AActor* A = S.A.Get();
		if (!A || FVector::DistSquared(S.Base, Cam) > 6000.0 * 6000.0)
		{
			continue; // far off, the haze has it: no need to move it
		}
		// the swell: a long wave along the current (+x, a little +y), a slower one across it, and each plant's own flutter
		const float W = (float)(S.Base.X * 0.0011 + S.Base.Y * 0.0005);
		const float Bend = FMath::Sin(Time * 0.9f - W) * 0.75f + FMath::Sin(Time * 2.3f + S.Phase) * 0.25f;
		const float Side = FMath::Sin(Time * 0.55f - (float)(S.Base.Y * 0.0009) + S.Phase * 0.3f);
		A->SetActorRotation(FQuat(FRotator(Bend * S.Amp, 0.f, Side * S.Amp * 0.6f)) * FQuat(S.Rot));
	}
}

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
		else if (Base.Size2D() > 7800.0)
		{
			// out in the wider sea: drawn only when near enough to matter (the haze has taken it long before)
			C->SetCullDistance(Size.GetMax() < 700.0 ? 9000.f : 20000.f);
		}
		if (bCollide)
		{
			C->SetCollisionObjectType(ECC_WorldStatic);
			C->SetCollisionResponseToAllChannels(ECR_Block);
			C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		else if (Size.GetMax() >= 140.0)
		{
			// loose set dressing was all swum through. What is big and hard (a rock, a buoy, a light pole, a bone pile, a big
			// sponge or coral) is in the way like everything else; what is soft and sways (weed, fans, worms, jelly) is not.
			// It turns solid only after the whole sea is laid out, so nothing that was placed by a sweep moves.
			static const TCHAR* Soft[] = { TEXT("creature:"), TEXT("scene:kelp"), TEXT("scene:seagrass"), TEXT("scene:seafan"), TEXT("scene:seapen"), TEXT("scene:crinoid"), TEXT("scene:anemone"), TEXT("scene:tubeworms") };
			bool bSoft = false;
			for (const TCHAR* S : Soft)
			{
				bSoft |= FString(Id).StartsWith(S);
			}
			if (!bSoft)
			{
				LateSolid.Add(C);
			}
		}
	}
	if (!bCollide)
	{
		AddSoft(A, Id, Base, Rot);
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
	double Z = Terrain(X, Y);
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

// the sea floor: flat round the station (everything there was placed on z = 0), a basin for each zone at its own depth,
// gentle slopes and ridges between, and the abyss a trench with walls you swim down
static const float GZoneFloor[] = { 1300.f, -1800.f, -2900.f, -4300.f, -5600.f, -9200.f };

float FDC3D::Terrain(double X, double Y) const
{
	const FVector2D P(X, Y);
	double WSum = 0.0, HSum = 0.0;
	auto Add = [&](const FVector2D& C, double Core, double H)
	{
		const double D = FMath::Max(FVector2D::Distance(P, C) - Core, 1.0);
		const double Wt = 1.0 / (D * D * D);
		WSum += Wt;
		HSum += Wt * H;
	};
	Add(FVector2D::ZeroVector, 6800.0, 0.0);
	const int32 NZ = FMath::Min(Zones.Num(), 5);
	for (int32 i = 0; i < NZ; ++i)
	{
		Add(Zones[i].At, Zones[i].Radius * 0.75, GZoneFloor[i]);
	}
	{
		const double Far = 9000.0; // the open sea between: a plain that leans down
		const double Wt = 1.0 / (Far * Far * Far);
		WSum += Wt;
		HSum += Wt * -1600.0;
	}
	double H = HSum / WSum;
	// ridges and hollows, none on the station's flat, less on a zone's floor
	const double Keep = FMath::SmoothStep(6600.0, 9500.0, P.Size());
	double Core = 0.0;
	for (int32 i = 0; i < Zones.Num(); ++i)
	{
		Core = FMath::Max(Core, 1.0 - FMath::SmoothStep(Zones[i].Radius * 0.5, Zones[i].Radius * 0.9, FVector2D::Distance(P, Zones[i].At)));
	}
	const double Relief = FMath::PerlinNoise2D(P / 10000.0 + FVector2D(3.7, 1.3)) * 900.0 + FMath::PerlinNoise2D(P / 3100.0 + FVector2D(0.4, 7.1)) * 260.0
		+ FMath::PerlinNoise2D(P / 950.0) * 45.0;
	H += Keep * Relief * (1.0 - 0.7 * Core);
	// the abyss: a trench whose walls drop almost straight down
	if (Zones.Num() > 5)
	{
		const FZone& A = Zones[5];
		const double D = FVector2D::Distance(P, A.At);
		const double Wobble = FMath::PerlinNoise1D(FMath::Atan2(P.Y - A.At.Y, P.X - A.At.X) * 2.3) * 900.0;
		const double S = 1.0 - FMath::SmoothStep(A.Radius * 0.85, A.Radius * 0.85 + 2400.0, D + Wobble);
		H = FMath::Lerp(H, (double)GZoneFloor[5] + FMath::PerlinNoise2D(P / 1800.0) * 220.0, FMath::Pow(S, 1.4));
	}
	return (float)H;
}

void FDC3D::BuildTerrain(UMaterialInterface* Mat)
{
	const double Half = 30000.0, Step = 250.0;
	const int32 N = FMath::RoundToInt(2.0 * Half / Step) + 1;
	TArray<FVector> V;
	TArray<FVector> Nrm;
	TArray<FVector2D> UV;
	TArray<int32> Tri;
	V.Reserve(N * N);
	Nrm.Reserve(N * N);
	UV.Reserve(N * N);
	TArray<float> Hs;
	Hs.SetNumUninitialized(N * N);
	for (int32 j = 0; j < N; ++j)
	{
		for (int32 i = 0; i < N; ++i)
		{
			Hs[j * N + i] = Terrain(-Half + i * Step, -Half + j * Step);
		}
	}
	for (int32 j = 0; j < N; ++j)
	{
		for (int32 i = 0; i < N; ++i)
		{
			const double X = -Half + i * Step, Y = -Half + j * Step;
			V.Add(FVector(X, Y, Hs[j * N + i]));
			const float Hx0 = Hs[j * N + FMath::Max(i - 1, 0)], Hx1 = Hs[j * N + FMath::Min(i + 1, N - 1)];
			const float Hy0 = Hs[FMath::Max(j - 1, 0) * N + i], Hy1 = Hs[FMath::Min(j + 1, N - 1) * N + i];
			Nrm.Add(FVector(Hx0 - Hx1, Hy0 - Hy1, 2.0 * Step).GetSafeNormal());
			UV.Add(FVector2D(X, Y) / 1000.0);
		}
	}
	Tri.Reserve((N - 1) * (N - 1) * 6);
	for (int32 j = 0; j < N - 1; ++j)
	{
		for (int32 i = 0; i < N - 1; ++i)
		{
			const int32 A = j * N + i, B = A + 1, C = A + N, D = C + 1;
			Tri.Append({ A, C, B, B, C, D });
		}
	}
	AActor* Owner = GM->SpawnGroup(FVector::ZeroVector);
	UProceduralMeshComponent* PM = NewObject<UProceduralMeshComponent>(Owner);
	PM->SetupAttachment(Owner->GetRootComponent());
	PM->bUseComplexAsSimpleCollision = true;
	PM->bUseAsyncCooking = false;
	PM->RegisterComponent();
	PM->SetWorldLocation(FVector::ZeroVector);
	PM->CreateMeshSection_LinearColor(0, V, Tri, Nrm, UV, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
	PM->SetCollisionObjectType(ECC_WorldStatic);
	PM->SetCollisionResponseToAllChannels(ECR_Block);
	PM->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	if (Mat)
	{
		PM->SetMaterial(0, Mat);
	}
	else
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(GM->LitMaterial, PM);
		PM->SetMaterial(0, M);
	}
	Ground = PM;
	float Lo = 1e9f, Hi = -1e9f;
	for (float H : Hs)
	{
		Lo = FMath::Min(Lo, H);
		Hi = FMath::Max(Hi, H);
	}
	UE_LOG(LogMatchCut, Display, TEXT("3D terrain: %d x %d, %d triangles, floor %.0f .. %.0f"), N, N, Tri.Num() / 3, Lo, Hi);
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
	// the body moves: a fin kick in the legs and a little paddle in the arms, in the vertex shader (the model has no bones).
	// The emissive map (eyes, lamp) is carried over; the kick follows the speed (Tick).
	if (!FParse::Param(FCommandLine::Get(), TEXT("MCNoHeroSwim")))
	{
		float Em = 14.f;
		FParse::Value(FCommandLine::Get(), TEXT("-MCEm="), Em);
		for (UStaticMeshComponent* Part : Parts)
		{
			if (!Part->GetStaticMesh())
			{
				continue;
			}
			DCApplySwim(Part, TEXT("creature:hero"), Part->GetRelativeRotation().Quaternion(), 1.f, 0.f);
			for (int32 m = 0; m < Part->GetNumMaterials(); ++m)
			{
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(m));
				if (MID && MID->Parent && MID->Parent->GetName() == TEXT("M_DC_Creature"))
				{
					MID->SetScalarParameterValue(TEXT("EmissiveStrength"), Em);
					HeroSwim.Add(MID);
				}
			}
		}
		UE_LOG(LogMatchCut, Display, TEXT("3D HERO swim materials: %d"), HeroSwim.Num());
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
	Snow = Make(GM->CubeMesh, 1100, 1.4f, 6.5f, 4711, SnowMID); // denser than it was: the water is never empty, it moves
	Bubbles = Make(GM->SphereMesh, 40, 3.5f, 8.f, 1213, BubbleMID);
}

void FDC3D::BuildArea()
{
	float ShaftPower = 14.f;
	FParse::Value(FCommandLine::Get(), TEXT("-MCShaft="), ShaftPower);
	const FLinearColor Sand(0.20f, 0.19f, 0.16f);
	AActor* FloorBlock = Block(FVector(0.0, 0.0, -14000.0), FRotator::ZeroRotator, FVector(64000.0, 64000.0, 300.0), Sand); // under everything: a last floor
	// sand and silt: colour from noise in world space, so it never repeats (Tools/make_sand_material.py)
	UMaterialInterface* SandMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DeepCut/M_DC_Sand.M_DC_Sand"));
	if (FParse::Param(FCommandLine::Get(), TEXT("MCPlainFloor")))
	{
		SandMat = nullptr;
	}
	if (SandMat)
	{
		FloorBlock->SetActorHiddenInGame(true);
	}
	BuildTerrain(SandMat);
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
	// (the old station model sat right under the base, in the way of the moon pool: left out, 2026-11-16)
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
		for (int32 Try = 0; Try < 3000 && Mounds.Num() < 230; ++Try)
		{
			const FVector2D At(MoundRand.FRandRange(-25000.f, 25000.f), MoundRand.FRandRange(-25000.f, 25000.f));
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
			M.CentreZ = Terrain(At.X, At.Y) + High - Deep;
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
		// a gap in the rim towards each zone of the wider sea
		bool bGate = false;
		for (const FZone& Z : Zones)
		{
			bGate |= FMath::Abs(FMath::FindDeltaAngleRadians(Ang, FMath::Atan2(Z.At.Y, Z.At.X))) < 0.30f;
		}
		if (bGate)
		{
			continue;
		}
		const float Rad = 7300.f + 350.f * FMath::Sin(i * 1.7f);
		const float S = 15.f + 4.f * FMath::Sin(i * 3.1f);
		Place(i % 3 == 2 ? Ledge : Boulder, 0, FVector(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang), Terrain(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang)) - 260.0), FRotator(0.f, 67.f * i, 0.f), i % 3 == 2 ? S * 1.4f : S);
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
			if (FMath::Abs(At.X - BaseC.X) < BaseHalf.X + 600.0 && FMath::Abs(At.Y - BaseC.Y) < BaseHalf.Y + 500.0)
			{
				continue; // the sand under the base and round its way in stays clear
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
			const double Floor = Terrain(At.X, At.Y);
			At.Z = FMath::Max(World->LineTraceSingleByChannel(Hit, FVector(At.X, At.Y, Floor + 2400.0), FVector(At.X, At.Y, Floor - 400.0), DC3::Channel, DC3::Query()) ? Hit.Location.Z : Floor, (double)GroundZ(At.X, At.Y)) - 8.0;
			if (At.Z - Floor > 900.0)
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

	// ---- the wider sea (brief 08): a zone in each direction, each built from its own models, with a trail to it
	{
		auto ZoneAt = [this](int32 i) { return FVector(Zones[i].At.X, Zones[i].At.Y, Terrain(Zones[i].At.X, Zones[i].At.Y)); };
		int32 Seed = 500;
		auto Z = [&](int32 Zi, const TCHAR* Id, int32 Count, float SMin, float SMax, const FVector2D& Off, float Spread, bool bCollide)
		{
			if (Zones.IsValidIndex(Zi))
			{
				Scatter(Id, Count, SMin, SMax, ZoneAt(Zi) + FVector(Off.X, Off.Y, 0.0), Spread, bCollide, false, ++Seed);
			}
		};
		const FVector2D O(0.0, 0.0);
		// 0 the kelp forest: tall kelp everywhere, coral and grass under it
		Z(0, TEXT("scene:kelp"), 46, 2.6f, 5.2f, O, 4800.f, false);
		Z(0, TEXT("scene:seagrass"), 40, 0.6f, 1.2f, O, 5000.f, false);
		Z(0, TEXT("scene:braincoral"), 8, 0.8f, 1.8f, O, 3800.f, true);
		Z(0, TEXT("scene:staghorn"), 10, 0.8f, 1.6f, O, 3800.f, false);
		Z(0, TEXT("scene:seafan"), 10, 0.8f, 1.6f, O, 4000.f, false);
		Z(0, TEXT("scene:coraltable"), 5, 1.4f, 2.4f, O, 3500.f, true);
		Z(0, TEXT("scene:barnaclerock"), 8, 1.2f, 3.0f, O, 4500.f, true);
		Z(0, TEXT("scene:boulder"), 10, 2.f, 6.f, O, 5200.f, true);
		Z(0, TEXT("creature:starfish"), 14, 0.2f, 0.35f, O, 4000.f, false);
		Z(0, TEXT("creature:urchin"), 14, 0.16f, 0.3f, O, 4000.f, false);
		// 1 the wreck graveyard: ships' parts on open sand
		Z(1, TEXT("scene:wreck"), 2, 7.f, 9.f, FVector2D(-900.0, 1200.0), 1800.f, true);
		Z(1, TEXT("scene:plane"), 1, 6.f, 6.f, FVector2D(1800.0, -1500.0), 400.f, true);
		Z(1, TEXT("scene:propeller"), 2, 1.6f, 2.2f, O, 3200.f, true);
		Z(1, TEXT("scene:mast"), 3, 3.5f, 4.5f, O, 3600.f, true);
		Z(1, TEXT("scene:lifeboat"), 3, 1.6f, 2.2f, O, 3600.f, true);
		Z(1, TEXT("scene:cannon"), 5, 1.0f, 1.4f, O, 3200.f, true);
		Z(1, TEXT("scene:wheel"), 2, 0.9f, 1.1f, O, 2800.f, false);
		Z(1, TEXT("scene:anchor"), 3, 1.6f, 2.4f, O, 3800.f, true);
		Z(1, TEXT("scene:container"), 6, 2.6f, 3.2f, O, 4200.f, true);
		Z(1, TEXT("scene:crates"), 6, 1.0f, 1.6f, O, 4000.f, true);
		Z(1, TEXT("scene:debris"), 6, 2.f, 3.5f, O, 4500.f, true);
		Z(1, TEXT("scene:seagrass"), 16, 0.5f, 1.0f, O, 4800.f, false);
		// 2 the sunken ruins: columns, stairs and an old face in the sand
		Z(2, TEXT("scene:ruins"), 4, 6.f, 8.f, O, 3600.f, true);
		Z(2, TEXT("scene:statue"), 1, 4.f, 4.f, FVector2D(600.0, -400.0), 300.f, true);
		Z(2, TEXT("scene:obelisk"), 4, 4.f, 7.f, O, 3800.f, true);
		Z(2, TEXT("scene:stairs"), 2, 4.f, 5.f, FVector2D(-800.0, 900.0), 1200.f, true);
		Z(2, TEXT("scene:pillar"), 7, 3.f, 4.5f, O, 4000.f, true);
		Z(2, TEXT("scene:arch"), 1, 7.f, 7.f, FVector2D(1800.0, 1600.0), 300.f, true);
		Z(2, TEXT("scene:sponge"), 10, 0.6f, 1.3f, O, 4200.f, false);
		Z(2, TEXT("scene:seafan"), 8, 0.7f, 1.4f, O, 4200.f, false);
		Z(2, TEXT("scene:anemone"), 10, 0.35f, 0.7f, O, 4000.f, false);
		// 3 the whale fall: a plain of grey silt, bones, and the things that live on them
		Z(3, TEXT("scene:whalefall"), 2, 6.f, 8.f, O, 2200.f, true);
		Z(3, TEXT("scene:bonepile"), 8, 1.2f, 2.4f, O, 4200.f, false);
		Z(3, TEXT("scene:crinoid"), 12, 0.8f, 1.5f, O, 4400.f, false);
		Z(3, TEXT("scene:seapen"), 16, 0.6f, 1.2f, O, 4600.f, false);
		Z(3, TEXT("scene:sponge"), 8, 0.6f, 1.2f, O, 4500.f, false);
		Z(3, TEXT("scene:boulder"), 6, 1.5f, 4.f, O, 5000.f, true);
		Z(3, TEXT("creature:brittlestar"), 14, 0.18f, 0.3f, O, 4000.f, false);
		// 4 the black smokers: chimneys, worms and lava
		Z(4, TEXT("scene:blacksmoker"), 6, 2.f, 3.4f, O, 3200.f, true);
		Z(4, TEXT("scene:chimney"), 6, 2.f, 3.4f, O, 3600.f, true);
		Z(4, TEXT("scene:pillowlava"), 10, 1.4f, 3.0f, O, 4400.f, true);
		Z(4, TEXT("scene:tubeworms"), 16, 0.8f, 1.6f, O, 3600.f, false);
		Z(4, TEXT("scene:clam"), 10, 0.4f, 0.9f, O, 3600.f, false);
		Z(4, TEXT("scene:vent"), 4, 1.6f, 2.8f, O, 3000.f, true);
		// 5 the abyss: black spires and a great arch, almost nothing else
		Z(5, TEXT("scene:abyssarch"), 1, 14.f, 14.f, O, 300.f, true);
		Z(5, TEXT("scene:spire"), 12, 6.f, 14.f, O, 5000.f, true);
		Z(5, TEXT("scene:cliff"), 6, 12.f, 18.f, O, 5600.f, true);
		Z(5, TEXT("scene:pillowlava"), 6, 2.f, 4.f, O, 4000.f, true);
		Z(5, TEXT("scene:seapen"), 10, 0.6f, 1.2f, O, 4000.f, false);
		Z(5, TEXT("scene:crinoid"), 8, 0.8f, 1.4f, O, 3800.f, false);
		if (Zones.IsValidIndex(5))
		{
			// the only light down here is alive: soft blue and violet points near the floor and drifting above it
			FRandomStream GR(5150);
			for (int32 g = 0; g < 16; ++g)
			{
				// a cluster round the arch, where you arrive, then a loose scatter further off
				const double A = GR.FRandRange(0.0, 2.0 * PI), R = g < 6 ? 350.0 + GR.FRandRange(0.0, 1100.0) : 1500.0 + GR.FRandRange(0.0, 3400.0);
				const FVector P = ZoneAt(5) + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, GR.FRandRange(60.f, 700.f));
				const FLinearColor C = (g % 3 == 0) ? FLinearColor(0.55f, 0.30f, 1.0f) : FLinearColor(0.15f, 0.65f, 1.0f);
				Glow(P, C, 2500.f + GR.FRandRange(0.f, 2000.f), 1000.f);
				Place(TEXT("creature:jelly"), 0, P - FVector(0.0, 0.0, 40.0), FRotator(0.0, GR.FRandRange(0.f, 360.f), 0.0), 0.3f + GR.FRandRange(0.f, 0.3f), false);
			}
		}
		// research outpost on the way to the smokers: the station's far end
		Z(4, TEXT("scene:dome"), 1, 5.f, 5.f, FVector2D(-4200.0, 3600.0), 300.f, true);
		Z(4, TEXT("scene:tanks"), 2, 1.4f, 1.8f, FVector2D(-4200.0, 3600.0), 1200.f, true);
		Z(4, TEXT("scene:antenna"), 1, 4.f, 4.f, FVector2D(-4200.0, 3600.0), 1300.f, true);
		Z(4, TEXT("scene:lightpole"), 3, 1.6f, 1.6f, FVector2D(-4200.0, 3600.0), 1300.f, false);

		// trails: beacons on the way out of the first site, and loose rock all along
		FRandomStream TR(7171);
		for (int32 i = 0; i < Zones.Num(); ++i)
		{
			const FVector To = ZoneAt(i);
			const FVector Dir = To.GetSafeNormal2D();
			const FVector Side(-Dir.Y, Dir.X, 0.0);
			const double Len = To.Size2D();
			for (double D = 8200.0; D < Len - 3500.0; D += 2400.0)
			{
				FVector At = Dir * D + Side * TR.FRandRange(-500.f, 500.f);
				At.Z = GroundZ(At.X, At.Y);
				FVector Off = Side * TR.FRandRange(900.f, 1600.f) * (TR.FRand() < 0.5f ? -1.0 : 1.0);
				Off.Z = GroundZ(At.X + Off.X, At.Y + Off.Y) - At.Z;
				if (GM->Art.Contains(TEXT("scene:buoy")) && FMath::RoundToInt(D / 2400.0) % 2 == 0)
				{
					Place(TEXT("scene:buoy"), 0, At, FRotator(0.f, TR.FRandRange(0.f, 360.f), 0.f), 1.3f, false);
					Glow(At + FVector(0.0, 0.0, 260.0), FLinearColor(1.f, 0.6f, 0.15f), 3000.f, 900.f);
				}
				Place(TEXT("scene:boulder"), 0, At + Off, FRotator(0.f, TR.FRandRange(0.f, 360.f), 0.f), TR.FRandRange(1.5f, 4.f));
			}
		}
		// the edge of the wider sea: a ring of cliffs, so the far water ends in rock and not in nothing
		for (int32 i = 0; i < 36; ++i)
		{
			const float Ang = 2.f * PI * i / 36.f;
			const float Rad = 26500.f + 700.f * FMath::Sin(i * 2.9f);
			Place(i % 2 ? TEXT("scene:cliff") : TEXT("scene:boulder"), 0, FVector(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang), Terrain(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang)) - 200.0),
				FRotator(0.f, FMath::RadiansToDegrees(Ang) + 90.f, 0.f), i % 2 ? 22.f : 26.f);
		}
		// open sand between the zones is not empty: stones, grass and the odd coral
		Scatter(TEXT("scene:boulder"), 120, 0.2f, 1.2f, Mid, 24000.f, false, false, 990);
		Scatter(TEXT("scene:seagrass"), 60, 0.5f, 1.1f, Mid, 24000.f, false, false, 991);
		Scatter(TEXT("scene:sponge"), 30, 0.5f, 1.1f, Mid, 24000.f, false, false, 992);
		Scatter(TEXT("scene:barnaclerock"), 25, 0.8f, 2.4f, Mid, 24000.f, true, false, 993);
	}

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

void FDC3D::SetupZones()
{
	// title, where, radius, extra depth (m), fog colour, fog density, key light, key colour, fill light, flecks
	Zones = {
		{ TEXT("THE KELP FOREST"), FVector2D(-13000.0, 7500.0), 4800.f, -95.f, FLinearColor(0.03f, 0.20f, 0.19f), 0.045f, 13.f, FLinearColor(0.60f, 0.95f, 0.80f), 3.2f, FLinearColor(0.30f, 0.42f, 0.34f) },
		{ TEXT("THE WRECK GRAVEYARD"), FVector2D(14500.0, -3900.0), 4800.f, 60.f, FLinearColor(0.02f, 0.10f, 0.20f), 0.055f, 7.f, FLinearColor(0.35f, 0.70f, 1.f), 2.2f, FLinearColor(0.20f, 0.30f, 0.40f) },
		{ TEXT("THE DROWNED CITY"), FVector2D(6800.0, 14500.0), 4600.f, 110.f, FLinearColor(0.035f, 0.085f, 0.17f), 0.05f, 6.f, FLinearColor(0.50f, 0.75f, 1.f), 2.0f, FLinearColor(0.26f, 0.30f, 0.40f) },
		{ TEXT("THE WHALE FALL"), FVector2D(-7500.0, -13000.0), 4600.f, 260.f, FLinearColor(0.030f, 0.045f, 0.060f), 0.07f, 3.f, FLinearColor(0.55f, 0.70f, 0.90f), 1.4f, FLinearColor(0.34f, 0.35f, 0.36f) },
		{ TEXT("THE BLACK SMOKERS"), FVector2D(6800.0, -14500.0), 4800.f, 380.f, FLinearColor(0.10f, 0.040f, 0.018f), 0.06f, 2.f, FLinearColor(1.f, 0.60f, 0.35f), 1.0f, FLinearColor(0.42f, 0.28f, 0.18f) },
		{ TEXT("THE ABYSS"), FVector2D(-5500.0, 19500.0), 5200.f, 900.f, FLinearColor(0.010f, 0.020f, 0.045f), 0.035f, 2.0f, FLinearColor(0.30f, 0.50f, 0.90f), 0.9f, FLinearColor(0.12f, 0.14f, 0.18f) } };
}

void FDC3D::TickZones(float Dt)
{
	const FDCLook* Base = GM->Looks.Find(TEXT("3D"));
	if (!Base || Zones.Num() == 0)
	{
		return;
	}
	float Sum = 0.f, BestW = 0.f;
	int32 Best = INDEX_NONE;
	float W[8] = {};
	for (int32 i = 0; i < Zones.Num() && i < 8; ++i)
	{
		const FZone& Z = Zones[i];
		const float D = FVector2D::Distance(FVector2D(Pos.X, Pos.Y), Z.At);
		W[i] = FMath::SmoothStep(0.f, 1.f, FMath::Clamp((1.6f * Z.Radius - D) / (0.6f * Z.Radius), 0.f, 1.f));
		Sum += W[i];
		if (W[i] > BestW)
		{
			BestW = W[i];
			Best = i;
		}
	}
	const float Norm = Sum > 1.f ? 1.f / Sum : 1.f;
	const float WBase = 1.f - FMath::Min(Sum, 1.f);
	FDCLook& L = GM->Look;
	L.FogColor = Base->FogColor * WBase;
	L.FogDensity = Base->FogDensity * WBase;
	L.KeyIntensity = Base->KeyIntensity * WBase;
	L.KeyColor = Base->KeyColor * WBase;
	L.FillIntensity = Base->FillIntensity * WBase;
	L.SnowColor = Base->SnowColor * WBase;
	for (int32 i = 0; i < Zones.Num() && i < 8; ++i)
	{
		const float K = W[i] * Norm;
		const FZone& Z = Zones[i];
		L.FogColor += Z.Fog * K;
		L.FogDensity += Z.FogDensity * K;
		L.KeyIntensity += Z.Key * K;
		L.KeyColor += Z.KeyColor * K;
		L.FillIntensity += Z.Fill * K;
		L.SnowColor += Z.Snow * K;
	}
	// the air of the base: no water to see through, the warm light of the room
	L.FogDensity *= 1.f - 0.985f * DryAmt;
	L.FogColor = FMath::Lerp(L.FogColor, FLinearColor(0.05f, 0.04f, 0.03f), DryAmt);
	ZoneDepth = DepthAt(Pos.Z);
	// the light and the water change slowly enough to read as swimming deeper, not as a switch
	LookTime += Dt;
	if (GM->Fog && LookTime > 0.1f)
	{
		LookTime = 0.f;
		GM->Fog->GetComponent()->SetFogDensity(L.FogDensity);
		if (GM->KeyLight)
		{
			GM->KeyLight->GetLightComponent()->SetLightColor(L.KeyColor);
		}
	}
	// the title card of a zone, once on the way in; the first site has its own when you come back to it
	const int32 Now = BestW > 0.6f ? Best : (Sum < 0.05f && FVector2D(Pos.X, Pos.Y).Size() < 7000.0 ? -2 : ZoneNow);
	if (Now != ZoneNow)
	{
		if (ZoneNow != INDEX_NONE || Now != -2)
		{
			ZoneTitle = Now >= 0 ? FString(Zones[Now].Title) : FString(TEXT("THE STATION"));
			ZoneTitleTime = 0.f;
		}
		ZoneNow = Now;
	}
	if (ZoneTitleTime >= 0.f)
	{
		ZoneTitleTime += Dt;
		if (ZoneTitleTime > 6.f)
		{
			ZoneTitleTime = -1.f;
		}
	}
}

void FDC3D::Start()
{
	UWorld* World = GM->GetWorld();
	GM->LoadArt();
	SetupZones();

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
	bFirstPerson = !FParse::Param(FCommandLine::Get(), TEXT("MCThirdPerson"));
	if (bFirstPerson)
	{
		SpawnArms();
	}
	SpawnFlecks();

	ACameraActor* C = GM->SpawnCamera(Pos - FVector(DC3::BoomLen, 0.0, 0.0), FRotator::ZeroRotator);
	C->GetCameraComponent()->SetFieldOfView(Fov);
	{
		// the film look: a lens that fringes a little at the edges and blooms on the lamps
		FPostProcessSettings& PP = C->GetCameraComponent()->PostProcessSettings;
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = 0.2f;
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
	L.Grain = 0.06f;
	L.Saturation = 0.88f;
	L.Contrast = 1.12f;
	L.Tint = FLinearColor(0.94f, 1.f, 1.07f);
	L.SnowColor = FLinearColor(0.30f, 0.45f, 0.55f);
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
	BuildBase();
	GM->ApplyLook(TEXT("3D")); // the base's walls and lamps came after the first look: paint them too
	Doc = new FDCDoc(GM, this);
	Doc->Start();
	Pivot = Pos + FVector(0.0, 0.0, 45.0);
	TickCamera(0.f);
	for (const TWeakObjectPtr<UStaticMeshComponent>& Late : LateSolid)
	{
		if (UStaticMeshComponent* Part = Late.Get())
		{
			Part->SetCollisionObjectType(ECC_WorldStatic);
			Part->SetCollisionResponseToAllChannels(ECR_Block);
			Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
	UE_LOG(LogMatchCut, Display, TEXT("3D big loose set dressing made solid: %d"), LateSolid.Num());
	LateSolid.Reset();

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

// the diver's breath: every few seconds a burst of bubbles from the regulator, under the chin, wobbling up past the
// view. The cheapest thing that makes first person under water feel alive (one diver: the component is kept here)
static TWeakObjectPtr<UInstancedStaticMeshComponent> GBreath;
static TWeakObjectPtr<UMaterialInstanceDynamic> GBreathMID;
void FDC3D::TickBreath(float Dt)
{
	constexpr int32 N = 16;
	UInstancedStaticMeshComponent* C = GBreath.Get();
	if (!C || C->GetWorld() != GM->GetWorld())
	{
		AActor* A = GM->SpawnGroup(FVector::ZeroVector);
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(GM->UnlitMaterial, A);
		C = NewObject<UInstancedStaticMeshComponent>(A);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(GM->SphereMesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetupAttachment(A->GetRootComponent());
		C->RegisterComponent();
		C->SetMaterial(0, MID);
		C->SetAbsolute(true, true, true);
		for (int32 i = 0; i < N; ++i)
		{
			C->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.001)));
		}
		GBreath = C;
		GBreathMID = MID;
	}
	if (UMaterialInstanceDynamic* MID = GBreathMID.Get())
	{
		MID->SetVectorParameterValue(TEXT("Color"), GM->Look.SnowColor * 1.4f);
	}
	const double Cycle = 4.3, T = FMath::Fmod((double)Time, Cycle);
	const FRotationMatrix M(FRotator(0.f, Yaw, 0.f));
	const FVector F = M.GetUnitAxis(EAxis::X), R = M.GetUnitAxis(EAxis::Y);
	// out of the regulator: a little forward and below the eyes, a hand's width to the side
	// (off to the right and low, so the bubbles stream up the edge of the view and never sit in the middle of it)
	const FVector Mouth = CamPos + F * 30.0 + R * 22.0 - FVector(0.0, 0.0, 26.0);
	FRandomStream Rand(7 + (int32)(Time / Cycle));
	for (int32 i = 0; i < N; ++i)
	{
		const double Age = T - i * 0.045 - Rand.FRandRange(0.f, 0.08f);
		const double Size = Rand.FRandRange(0.25f, 0.9f);
		const FVector Side = F * Rand.FRandRange(-4.f, 8.f) + R * Rand.FRandRange(2.f, 12.f);
		FTransform X(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.001));
		if (DryAmt < 0.5f && Age > 0.0 && Age < 2.2)
		{
			// rising faster as it goes (buoyancy), wobbling, spreading, growing as the pressure drops a little
			const double Up = 40.0 * Age + 55.0 * Age * Age;
			const FVector Wob = R * (5.0 * FMath::Sin(Age * 11.0 + i)) + F * (4.0 * FMath::Cos(Age * 9.0 + i * 1.7));
			const FVector At = Mouth + Side * (0.4 + Age) + Wob + FVector(0.0, 0.0, Up);
			const double S = Size * (0.8 + 0.25 * Age) / 100.0;
			X = FTransform(FRotator::ZeroRotator, At, FVector(S, S, S * 0.8));
		}
		C->UpdateInstanceTransform(i, X, true, i == N - 1, true);
	}
}

void FDC3D::MoveHero(const FVector& Delta, bool bStanding)
{
	UWorld* World = GM->GetWorld();
	// swimming, the diver is a ball; standing in the base, a body from the shins to over the head, so a table or a case
	// that is lower than the chest is in the way too (the feet are 145 cm under the middle: the lowest 40 cm step over)
	const FCollisionShape Ball = bStanding ? FCollisionShape::MakeCapsule(Radius, 105.f) : FCollisionShape::MakeSphere(Radius);
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
	FVector Boom = -F * (DC3::BoomLen + 60.f * Fast) + U * DC3::BoomUp;
	if (bFirstPerson)
	{
		// the diver's eyes: a little up and forward of the body's centre. They ride level: swimming, nothing moves them
		// but the diver; walking, a dip of a few millimetres at each footfall (the "Head bob" setting turns it off)
		Pivot = Pos + FVector(0.0, 0.0, 22.0 + (bHeadBobOn ? WalkDip - 2.0 * LandDip : 0.0));
		Boom = F * 4.0;
	}

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
	Fov = Dt > 0.f ? FMath::FInterpTo(Fov, BaseFov + 9.f * Fast, Dt, 3.f) : BaseFov;
	ViewFov = Fov;
	FRotator Rot = View;
	// first person: the horizon stays level and the view points where the player aims, always (no roll, no nodding)
	if (bFirstPerson)
	{
		const FVector Off = Pivot - (Pos + FVector(0.0, 0.0, 22.0));
		const FVector4 Now4(FVector::DotProduct(Off, FRotationMatrix(FRotator(0.f, Yaw, 0.f)).GetUnitAxis(EAxis::Y)), Off.Z, Rot.Roll, Rot.Pitch - Pitch);
		for (int32 i = 0; i < 4; ++i)
		{
			FeelMin[i] = FMath::Min(FeelMin[i], Now4[i]);
			FeelMax[i] = FMath::Max(FeelMax[i], Now4[i]);
		}
	}
	float FocusAt = 0.f; // 0: everything sharp
	bool bHide = bFirstPerson || FVector::Dist(CamPos, Pos) < 95.0; // too close to see anything but the diver's back
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
			FVector Chest = Pos + FVector(0.0, 0.0, 32.0) + F * (bFirstPerson ? 30.0 : 58.0);
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
	if (bTitleCam)
	{
		// the start menu: a slow circle round the base, from outside, a little below it
		const float A = Time * 0.045f + 2.2f;
		CamPos = BaseC + FVector(FMath::Cos(A) * 3000.0, FMath::Sin(A) * 3000.0, -120.0);
		CamPos.Z = FMath::Max(CamPos.Z, (double)GroundZ(CamPos.X, CamPos.Y) + 220.0);
		Rot = (BaseC + FVector(0.0, 0.0, -150.0) - CamPos).Rotation();
		ViewFov = 58.f;
		FocusAt = 0.f;
		bHide = true;
	}
	if (ACameraActor* C = Cam.Get())
	{
		C->SetActorLocationAndRotation(CamPos, Rot);
		C->GetCameraComponent()->SetFieldOfView(ViewFov);
		// a film lens: what it is not focused on goes soft, more so the longer the lens
		FPostProcessSettings& PP = C->GetCameraComponent()->PostProcessSettings;
		PP.bOverride_DepthOfFieldFocalDistance = PP.bOverride_DepthOfFieldFstop = PP.bOverride_DepthOfFieldSensorWidth = true;
		PP.DepthOfFieldFocalDistance = FocusAt;
		PP.DepthOfFieldFstop = 4.5f;
		PP.DepthOfFieldSensorWidth = 30.f;
	}
	if (AActor* A = GM->Hero.Actor.Get())
	{
		A->SetActorHiddenInGame(bHide);
	}
	if (bFirstPerson)
	{
		FVector PlayP;
		FRotator PlayR;
		float PlayFov = 0.f;
		const bool bPlaying = bTitleCam || (Doc && Doc->ViewOverride(PlayP, PlayR, PlayFov));
		TickArms(Dt, CamPos, Rot);
		if (UPoseableMeshComponent* PM = Arms.Get())
		{
			PM->SetVisibility(!bPlaying && (!Doc || Doc->AimBlend < 0.85f), true);
		}
		// the camera in the hand goes with the arms: up at the eye it would fill the picture
		const bool bHands = !bPlaying && (!Doc || Doc->AimBlend < 0.85f);
		if (USceneComponent* Hold = HandCamHolder.Get())
		{
			Hold->SetVisibility(bHands && DryAmt < 0.5f, true); // put away in the base
			if (UStaticMeshComponent* L = HandCamRec.Get())
			{
				L->SetVisibility(bHands && DryAmt < 0.5f && Doc && Doc->bRecording && FMath::Frac(Time * 1.2f) < 0.6f);
			}
		}
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
	if (bFrozen)
	{
		In = FVector::ZeroVector;
		bFast = false;
		Turn = FVector2D::ZeroVector;
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
		const AActor* Tracked = Doc->TrackActor.Get();
		const FRotator To = ((Tracked ? Tracked->GetActorLocation() : Doc->SubPos[Doc->TrackSub]) - CamPos).Rotation();
		Yaw = DC3::TurnTo(Yaw, To.Yaw, 9.f, Dt);
		Pitch = DC3::TurnTo(Pitch, FMath::Clamp(To.Pitch, -80.f, 80.f), 9.f, Dt);
	}
	Turn *= FMath::Lerp(1.f, ViewFov / BaseFov, Aim); // a long lens turns slowly
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
	const FVector StartPos = Pos;
	// the dry base: walking on the deck, the hatch a hole down into the sea
	const double Floor = BaseFloor();
	const bool bInBox = FMath::Abs(Pos.X - BaseC.X) < BaseHalf.X && FMath::Abs(Pos.Y - BaseC.Y) < BaseHalf.Y && Pos.Z < BaseC.Z + BaseHalf.Z + 40.0;
	const bool bWasDry = bDry;
	// under the moon pool: as in a habitat's wet porch (or a game's moon pool), coming home is easy. Near the shaft and
	// below it, swimming up (SPACE) lines the diver up with the opening and lifts them in; no hunting for a narrow hole
	bHatchAssist = false;
	// a moment after going through the hatch either way, it does not take the diver straight back (one diver, one hero)
	static float HatchCool = 0.f;
	HatchCool = FMath::Max(0.f, HatchCool - Dt);
	if (!bDry)
	{
		const FVector2D ToHatch(HatchAt.X - Pos.X, HatchAt.Y - Pos.Y);
		const double Below = HatchAt.Z - Pos.Z;
		if (ToHatch.Size() < 700.0 && Below > -20.0 && Below < 1100.0)
		{
			bHatchAssist = true;
			if (In.Z > 0.f || (Vel.Z > 60.0 && Below < 400.0))
			{
				const double K = FMath::Min(1.0, 6.0 * Dt);
				Pos.X += ToHatch.X * K;
				Pos.Y += ToHatch.Y * K;
				Vel.X *= 1.0 - K;
				Vel.Y *= 1.0 - K;
				Vel.Z = FMath::Max(Vel.Z, 320.0);
			}
		}
	}
	// up: anywhere in the trunk under the opening (it is 2.3 m deep), rising, and the diver is out on the deck
	if (!bDry && bInBox && OverHatch(Pos, -20.0) && Pos.Z > HatchAt.Z - 260.0 && (Vel.Z > 0.0 || In.Z > 0.f) && HatchCool <= 0.f)
	{
		HatchCool = 0.8f;
		// up through the surface: the hands on the rim, out onto the deck
		Pos = FVector(HatchAt.X - HatchHalf - 90.0, HatchAt.Y, Floor + Stand);
		Vel = FVector::ZeroVector;
		Yaw = 180.f;
		Pitch = -5.f;
		bDry = true;
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("climb_out"), 1.f, 1.f, 0.04f);
		}
		LandDip = 1.f;
		UE_LOG(LogMatchCut, Display, TEXT("3D BASE climbed out onto the deck"));
	}
	else if (bDry && (!bInBox || Pos.Z < HatchAt.Z || (OverHatch(Pos, 30.0) && HatchCool <= 0.f)))
	{
		// through the surface: into the sea. Stepping over the rim drops the diver straight down the middle of the trunk,
		// so the standing body never catches on the edge of the deck
		if (bInBox && OverHatch(Pos, 30.0))
		{
			Pos = FVector(HatchAt.X, HatchAt.Y, HatchAt.Z - 150.0);
			Vel = FVector(0.0, 0.0, -260.0);
			HatchCool = 0.8f;
		}
		bDry = false;
		Vel *= 0.35;
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("splash_in"), 1.f, 1.f, 0.05f);
		}
		UE_LOG(LogMatchCut, Display, TEXT("3D BASE dropped into the hatch"));
	}
	else if (!bDry && bInBox && Pos.Z > Floor + 30.0)
	{
		bDry = true; // woke in the base, or a script put the diver there
	}
	if (bDry)
	{
		const FVector Fwd = FRotationMatrix(FRotator(0.f, Yaw, 0.f)).GetUnitAxis(EAxis::X);
		FVector Walk = Fwd * In.X + R * In.Y;
		if (Walk.SizeSquared() > 1.0)
		{
			Walk.Normalize();
		}
		const FVector WantV = Walk * ((bFast ? 500.0 : 335.0) * FMath::Lerp(1.f, 0.6f, Aim));
		Vel.X += (WantV.X - Vel.X) * (1.0 - FMath::Exp(-10.0 * Dt));
		Vel.Y += (WantV.Y - Vel.Y) * (1.0 - FMath::Exp(-10.0 * Dt));
		Vel.Z -= 1800.0 * Dt; // air: things fall
		IdleTime = Walk.SizeSquared() > 0.01 || !Turn.IsNearlyZero() ? 0.f : IdleTime + Dt;
		MoveHero(Vel * Dt, true);
		if (!OverHatch(Pos, 30.0) && Pos.Z < Floor + Stand)
		{
			Pos.Z = Floor + Stand;
			Vel.Z = 0.0;
		}
		// steps: the phase moves with the distance walked, so the bob and the footfalls always match the speed
		const float HSpeed = (float)FVector2D(Vel.X, Vel.Y).Size();
		const float StepLen = bFast ? 92.f : 72.f;
		const float Was = StepPhase;
		StepAmt = FMath::FInterpTo(StepAmt, FMath::Clamp(HSpeed / 280.f, 0.f, 1.5f), Dt, 8.f);
		StepPhase += HSpeed * Dt / StepLen;
		if (HSpeed > 60.f && FMath::FloorToInt(StepPhase) != FMath::FloorToInt(Was) && GM->Audio)
		{
			static int32 Foot = 0;
			static const TCHAR* Steps[3] = { TEXT("step_deck1"), TEXT("step_deck2"), TEXT("step_deck3") };
			Foot = (Foot + 1 + FMath::RandRange(0, 1)) % 3;
			GM->Audio->Play(Steps[Foot], FMath::Lerp(0.7f, 1.f, FMath::Min(StepAmt, 1.f)), bFast ? 1.05f : 1.f, 0.06f);
		}
		if (HSpeed < 20.f)
		{
			StepPhase = FMath::FInterpTo(StepPhase, FMath::RoundToFloat(StepPhase), Dt, 4.f); // settle on both feet
		}
		// straight down and back, 5 mm walking and 8 mm running, smooth at both ends
		const float Amp = StepAmt > 1.f ? FMath::Lerp(0.5f, 0.8f, (StepAmt - 1.f) * 2.f) : 0.5f * StepAmt;
		WalkDip = -Amp * (0.5f + 0.5f * FMath::Cos(StepPhase * 2.f * PI));
	}
	else
	{
		StepAmt = FMath::FInterpTo(StepAmt, 0.f, Dt, 6.f);
		WalkDip = FMath::FInterpTo(WalkDip, 0.f, Dt, 6.f);
	}
	LandDip = FMath::FInterpTo(LandDip, 0.f, Dt, 5.f);
	const bool bThrust = !bDry && Wish.SizeSquared() > 0.01;
	if (!bDry)
	{
	IdleTime = bThrust || !Turn.IsNearlyZero() ? 0.f : IdleTime + Dt;
	if (bThrust)
	{
		Vel += (Wish * ((bFast ? DC3::Boost : DC3::Swim) * FMath::Lerp(1.f, 0.5f, Aim) * (Doc ? Doc->SpeedMul() : 1.f)) - Vel) * (1.0 - FMath::Exp(-DC3::Push * Dt)); // slower with the camera up
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
	if (Pos.Z < SandTop + Radius)
	{
		Pos.Z = SandTop + Radius;
		Vel.Z = FMath::Max(Vel.Z, 0.0);
	}
	}
	TickRings(StartPos);
	(void)bWasDry;

	ThrustTime -= Dt;
	if (bThrust && ThrustTime <= 0.f && GM->Audio && !bFirstPerson)
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
		// the kick: soft while floating, full while swimming, harder on boost. Only the amounts change, never the beat,
		// so the legs never jump.
		const float Speed = FMath::Clamp((float)(Vel.Size() / DC3::Swim), 0.f, 1.6f);
		KickAmt = FMath::FInterpTo(KickAmt, 0.25f + 0.75f * FMath::Min(Speed, 1.f) + 0.4f * FMath::Max(0.f, Speed - 1.f), Dt, 3.f);
		for (const TWeakObjectPtr<UMaterialInstanceDynamic>& W : HeroSwim)
		{
			if (UMaterialInstanceDynamic* MID = W.Get())
			{
				MID->SetVectorParameterValue(TEXT("SwimTail"), FLinearColor(0.16f * KickAmt, 2.4f, 0.55f, 2.0f));
				MID->SetVectorParameterValue(TEXT("SwimWing"), FLinearColor(0.20f * (0.5f + 0.5f * KickAmt), 0.f, 0.6f, 0.f));
			}
		}
	}

	if (Doc && !bFrozen)
	{
		Doc->Tick(Dt);
		// a big animal is in the way: the diver is moved out to its skin and slides along it (through the world's own
		// sweep, so never into rock). Only in the live sea: while a take plays the animals are where they were then.
		FVector Push, PushN;
		if (bShoot && Doc->State == EDCDocState::Shooting && Doc->Blocked(Pos, Radius, Push, PushN) && !bDry)
		{
			MoveHero(Push);
			Vel -= PushN * FMath::Min(0.0, FVector::DotProduct(Vel, PushN));
		}
	}
	TickCamera(Dt);
	TickZones(Dt);
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
	auto Around = [this, P](const FVector& Drift)
	{
		const FVector C = CamPos + Drift;
		return FVector(FMath::RoundToDouble(C.X / P) * P + P * 0.5 - Drift.X, FMath::RoundToDouble(C.Y / P) * P + P * 0.5 - Drift.Y, FMath::RoundToDouble(C.Z / P) * P + P * 0.5 - Drift.Z);
	};
	DryAmt = Dt > 0.f ? FMath::FInterpTo(DryAmt, bDry ? 1.f : 0.f, Dt, 5.f) : (bDry ? 1.f : 0.f);
	if (GM->Fog)
	{
		GM->Fog->GetComponent()->SetVolumetricFogExtinctionScale(GM->Look.VolExtinction * (1.f - 0.95f * DryAmt));
	}
	if (AActor* A = Snow.Get())
	{
		A->SetActorHiddenInGame(DryAmt > 0.5f);
	}
	if (AActor* A = Bubbles.Get())
	{
		A->SetActorHiddenInGame(DryAmt > 0.5f);
	}
	if (AActor* A = Snow.Get())
	{
		// marine snow: sinking, and carried sideways by a slow current that breathes (the stream's petals on the wind)
		const double Cur = Time * 26.0 + 120.0 * FMath::Sin(Time * 0.21);
		A->SetActorLocation(Around(FVector(-FMath::Fmod(Cur, P), -FMath::Fmod(Cur * 0.45 + 60.0 * FMath::Sin(Time * 0.33), P), FMath::Fmod(Time * 14.0, P))));
		if (UMaterialInstanceDynamic* MID = SnowMID.Get())
		{
			MID->SetVectorParameterValue(TEXT("Color"), GM->Look.SnowColor);
		}
	}
	if (AActor* A = Bubbles.Get())
	{
		A->SetActorLocation(Around(FVector(0.0, 0.0, -FMath::Fmod(Time * 80.0, P))));
		if (UMaterialInstanceDynamic* MID = BubbleMID.Get())
		{
			MID->SetVectorParameterValue(TEXT("Color"), GM->Look.SnowColor * 1.6f);
		}
	}
	TickBreath(Dt);
	TickSoft(GM->GetWorld(), CamPos, Time);
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
	if (const int32 Done = DCMenuRunLine(GM, T))
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
	if (Cmd == TEXT("hold3"))
	{
		// hold3 FORWARD RIGHT UP SECONDS [boost]: as key3, but the script goes on at once (snapshots while moving)
		ScriptMove = FVector(Num(1), Num(2), Num(3));
		ScriptMoveLeft = Num(4);
		bScriptBoost = T.IsValidIndex(5) && T[5] == TEXT("boost");
		return 1;
	}
	if (Cmd == TEXT("bob3"))
	{
		bHeadBobOn = Num(1) > 0.5f; // the "Head bob" setting, for a test
		return 1;
	}
	if (Cmd == TEXT("feel3"))
	{
		// feel3 [LABEL]: how far the eyes swayed since the last feel3, then start again
		UE_LOG(LogMatchCut, Display, TEXT("3D FEEL %s right [%.2f, %.2f] cm  up [%.2f, %.2f] cm  roll [%.2f, %.2f] deg  pitch [%.2f, %.2f] deg  speed %.0f dry %d bob %d"), T.IsValidIndex(1) ? *T[1] : TEXT(""),
			FeelMin.X, FeelMax.X, FeelMin.Y, FeelMax.Y, FeelMin.Z, FeelMax.Z, FeelMin.W, FeelMax.W, Vel.Size(), bDry ? 1 : 0, bHeadBobOn ? 1 : 0);
		FeelMin = FeelMax = FVector4(0.0, 0.0, 0.0, 0.0);
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
		FVector StatPush, StatN;
		if (Doc)
		{
			Doc->Blocked(Pos, Radius, StatPush, StatN); // for "animal": how near the nearest big animal is now
		}
		UE_LOG(LogMatchCut, Display, TEXT("3D STAT pos=(%.0f,%.0f,%.0f) speed=%.0f yaw=%.0f pitch=%.0f boom=%.2f cam=(%.0f,%.0f,%.0f) rings=%d/%d dry=%d animal=%.2f"), Pos.X, Pos.Y, Pos.Z, Vel.Size(), Yaw, Pitch,
			BoomFrac, CamPos.X, CamPos.Y, CamPos.Z, RingsDone, Rings.Num(), bDry ? 1 : 0, Doc ? Doc->SolidNear : 99.f);
		return 1;
	}
	return 0;
}
