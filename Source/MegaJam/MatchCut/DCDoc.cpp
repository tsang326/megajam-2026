// Deep Cut (brief 07): the documentary. See DCDoc.h.

#include "DCDoc.h"
#include "DCDocText.h"
#include "DC3D.h"
#include "MatchCutGameMode.h"
#include "DCAudio.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"

namespace DCD
{
	const FVector Wreck(3300.0, -700.0, 0.0);
	const FVector MantaC(1500.0, -700.0, 1650.0);
	const double MantaA = 1800.0, MantaB = 1500.0;
	const float MantaPeriod = 40.f;
	const FVector Reef(-2900.0, 2250.0, 720.0);
	const FVector Shaft(-2500.0, -2700.0, 0.0);
	const FVector AnglerAt(4050.0, 2390.0, 250.0);
	const FVector CrewAt(700.0, -560.0, 100.0);
	const float CrewYaw = 145.f; // towards where the dive starts
	const FVector GiantA(-7000.0, 2600.0, 2250.0), GiantB(7000.0, -2200.0, 2250.0);
	const float GiantPeriod = 120.f, GiantPass = 80.f, GiantLead = 85.f;
	const float TakeLen = 3.f;
	const int32 NumFish = 130, NumJelly = 7;
	const float Aspect = 16.f / 9.f;
	const float MaxDist[DCSub::Num] = { 3600.f, 2200.f, 1500.f, 2600.f, 5200.f, 1800.f };
	const int32 Need[DCSub::Num] = { 1, 12, 1, 2, 1, 1 };

	const int32 Order[DCSub::Num] = { DCSub::Crew, DCSub::Shoal, DCSub::Manta, DCSub::Jelly, DCSub::Giant, DCSub::Angler };

	float Ease(float Current, float Target, float Rate, float Dt)
	{
		return Current + (Target - Current) * (1.f - FMath::Exp(-Rate * Dt));
	}
}

// ---------------------------------------------------------------------------------------------
// spawning
// ---------------------------------------------------------------------------------------------

// ---- the swim: the models have no bones, so the body is moved in the material (M_DC_Creature, see
// Tools/make_creature_material.py). Each kind of animal gets its own numbers; each animal its own phase.
namespace DCSwim
{
	struct FKind
	{
		const TCHAR* Ids;   // ids of this kind, each between spaces
		int32 Mode;         // 0 tail beats sideways, 1 tail beats up and down, 2 hangs: the wave runs downwards
		float TailAmp, TailPow, TailWaves, Beats;
		float WingAmp, WingWaves, WingRate;
		float PulseAmp, PulseWaves, PulseRate;
	};
	static const FKind Kinds[] = {
		{ TEXT(" shark goblin hammerhead barracuda "), 0, 0.055f, 1.8f, 0.7f, 4.0f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" eel oarfish frilled gulper dragonfish viperfish "), 0, 0.065f, 1.1f, 1.6f, 4.5f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" whale dolphin "), 1, 0.035f, 1.8f, 0.6f, 1.5f, 0.10f, 0.2f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" manta stingray "), 1, 0.02f, 2.0f, 0.5f, 2.4f, 0.30f, 0.30f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" cuttle "), 0, 0.01f, 2.0f, 0.5f, 6.0f, 0.10f, 1.4f, 1.f, 0.02f, 0.6f, 0.5f },
		{ TEXT(" seaangel "), 0, 0.02f, 1.5f, 0.6f, 5.0f, 0.30f, 0.2f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" jelly lionsmane siphon atolla "), 2, 0.06f, 2.0f, 1.2f, 2.2f, 0.f, 0.f, 1.f, 0.07f, 0.5f, 1.f },
		{ TEXT(" squid vampsquid octopus nautilus giantsquid "), 0, 0.03f, 2.0f, 1.0f, 3.0f, 0.f, 0.f, 1.f, 0.05f, 0.7f, 1.f },
		{ TEXT(" crab isopod lobster brittlestar yeticrab "), 0, 0.f, 1.f, 1.f, 11.f, 0.10f, 0.5f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" seapig nudibranch "), 1, 0.025f, 1.0f, 1.5f, 2.5f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" urchin starfish "), 0, 0.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" angler sunfish seahorse seadragon seatoad "), 0, 0.04f, 1.8f, 0.8f, 4.0f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f },
		{ TEXT(" turtle turtle2 giant "), 0, 0.f, 1.f, 1.f, 2.6f, 0.28f, 0.15f, 1.f, 0.f, 0.f, 1.f },
		// Unit 0: the axis runs from the head down to the feet, the swing is forward and back: a fin kick in the legs,
		// the arms paddle a little. The game turns the amounts up and down with the speed (FDC3D::Tick).
		{ TEXT(" hero "), 2, 0.10f, 2.6f, 0.55f, 2.0f, 0.12f, 0.f, 1.f, 0.f, 0.f, 1.f } };
	static const FKind Fish = { TEXT(""), 0, 0.06f, 1.6f, 0.9f, 4.5f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f };

	static const FKind& KindOf(const FString& Id)
	{
		FString Name = Id;
		Name.RemoveFromStart(TEXT("creature:"));
		const FString Key = FString::Printf(TEXT(" %s "), *Name);
		for (const FKind& K : Kinds)
		{
			if (FString(K.Ids).Contains(Key))
			{
				return K;
			}
		}
		return Fish;
	}

	// models that came with the nose at the other end (seen in the parade, Content/Data/tests/d5_facing.txt)
	static float FrontFix(const FString& Id)
	{
		return FString(TEXT(" creature:octopus creature:squid creature:vampsquid creature:stingray creature:nautilus ")).Contains(FString::Printf(TEXT(" %s "), *Id)) ? 180.f : 0.f;
	}

	static float Ext(const FVector& V, const FVector& E)
	{
		return FMath::Abs(V.X) * E.X + FMath::Abs(V.Y) * E.Y + FMath::Abs(V.Z) * E.Z;
	}

	/** Gives the part the swimming material, with the model's own textures. Q turns the mesh so its nose is along x. */
	static void Apply(UStaticMeshComponent* C, const FString& Id, const FQuat& Q, float Pace, float Phase)
	{
		static bool bOff = FParse::Param(FCommandLine::Get(), TEXT("MCNoSwim"));
		UMaterialInterface* Base = bOff ? nullptr : LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DeepCut/M_DC_Creature.M_DC_Creature"));
		if (!Base || !C || !C->GetStaticMesh())
		{
			return;
		}
		const FKind& K = KindOf(Id);
		const FBox B = C->GetStaticMesh()->GetBoundingBox();
		const FVector Cn = B.GetCenter(), E = B.GetExtent();
		const FVector Nose = Q.UnrotateVector(FVector::XAxisVector), Across = Q.UnrotateVector(FVector::YAxisVector), Up = FVector::ZAxisVector;
		const FVector Axis = K.Mode == 2 ? -Up : -Nose;
		const FVector Side = K.Mode == 0 ? Across : K.Mode == 1 ? Up : Nose;
		const float Len = 2.f * Ext(Axis, E), Head = FVector::DotProduct(Cn, Axis) - Ext(Axis, E);
		for (int32 m = 0; m < C->GetNumMaterials(); ++m)
		{
			UMaterialInterface* Src = C->GetMaterial(m);
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, C);
			if (!MID)
			{
				continue;
			}
			for (const TCHAR* Tex : { TEXT("BaseColorTexture"), TEXT("MetallicRoughnessTexture"), TEXT("NormalTexture"), TEXT("EmissiveTexture") })
			{
				UTexture* T = nullptr;
				if (Src && Src->GetTextureParameterValue(FHashedMaterialParameterInfo(FName(Tex)), T) && T)
				{
					MID->SetTextureParameterValue(Tex, T);
				}
			}
			MID->SetVectorParameterValue(TEXT("SwimAxis"), FLinearColor(Axis.X, Axis.Y, Axis.Z, Head));
			MID->SetVectorParameterValue(TEXT("SwimSide"), FLinearColor(Side.X, Side.Y, Side.Z, Len));
			MID->SetVectorParameterValue(TEXT("SwimLat"), FLinearColor(Across.X, Across.Y, Across.Z, FVector::DotProduct(Cn, Across)));
			MID->SetVectorParameterValue(TEXT("SwimUp"), FLinearColor(Up.X, Up.Y, Up.Z, Ext(Across, E)));
			MID->SetVectorParameterValue(TEXT("SwimCentre"), FLinearColor(Cn.X, Cn.Y, Cn.Z, Phase));
			MID->SetVectorParameterValue(TEXT("SwimTail"), FLinearColor(K.TailAmp, K.TailPow, K.TailWaves, K.Beats * Pace));
			MID->SetVectorParameterValue(TEXT("SwimWing"), FLinearColor(K.WingAmp, K.WingWaves, K.WingRate, 0.f));
			MID->SetVectorParameterValue(TEXT("SwimPulse"), FLinearColor(K.PulseAmp, K.PulseWaves, K.PulseRate, 0.f));
			C->SetMaterial(m, MID);
		}
	}
}

void DCApplySwim(UStaticMeshComponent* C, const FString& Id, const FQuat& Q, float Pace, float Phase)
{
	DCSwim::Apply(C, Id, Q, Pace, Phase);
}

static int32 GFewFrames = 0; // frames of the running take with some of a group in the picture, but too few
static int32 GDangerFrames = 0; // frames of the take with something that bites close by

AActor* FDCDoc::SpawnCreature(const TCHAR* Id, float Length, float FrontYaw, bool bLampOnly, float* OutHalfHeight, float Pace)
{
	static FRandomStream SwimRand(4177);
	AActor* A = GM->SpawnGroup(FVector::ZeroVector);
	const TArray<FDCArtRef>* Set = GM->Art.Find(Id);
	UStaticMeshComponent* C = Set && Set->Num() > 0 ? GM->AddArt(A, Id, FVector::ZeroVector, FVector(Length), 0, false) : nullptr;
	if (!C)
	{
		// no model yet: a plain body of about the right size
		GM->AddPart(A, GM->SphereMesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(Length / 100.0, Length / 180.0, Length / 300.0), FLinearColor(0.5f, 0.55f, 0.6f), FLinearColor::Black);
		return A;
	}
	const FBox B = C->GetStaticMesh()->GetBoundingBox();
	const double S = C->GetRelativeScale3D().X;
	const FQuat Q(FRotator(0.f, FrontYaw > 500.f ? (*Set)[0].Yaw : FrontYaw + DCSwim::FrontFix(Id), 0.f));
	if (FrontYaw < 500.f)
	{
		DCSwim::Apply(C, Id, Q, Pace, SwimRand.FRandRange(0.f, 2.f * PI)); // not the crewmate: a diver does not ripple
	}
	// its middle on the actor's middle, its nose along the actor's x
	C->SetRelativeLocationAndRotation(-Q.RotateVector(B.GetCenter() * S), Q);
	if (OutHalfHeight)
	{
		*OutHalfHeight = B.GetSize().Z * S * 0.5;
	}
	if (FString(Id) == TEXT("creature:jelly") || FString(Id) == TEXT("creature:lionsmane") || FString(Id) == TEXT("creature:siphon") || FString(Id) == TEXT("creature:atolla"))
	{
		// the bells came out like polished metal: jelly is matte. The roughness map is multiplied up until nothing glints.
		for (int32 m = 0; m < C->GetNumMaterials(); ++m)
		{
			if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(C->GetMaterial(m)))
			{
				MID->SetScalarParameterValue(TEXT("RoughnessFactor"), 8.f);
			}
		}
	}
	if (bLampOnly)
	{
		C->SetLightingChannels(false, false, true);
		C->SetCastShadow(false);
	}
	UE_LOG(LogMatchCut, Display, TEXT("DOC creature %s: model %.0fx%.0fx%.0f, scale %.2f"), Id, B.GetSize().X, B.GetSize().Y, B.GetSize().Z, S);
	return A;
}

void FDCDoc::SpawnShoal()
{
	AActor* A = GM->SpawnGroup(FVector::ZeroVector);
	UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(A);
	C->SetMobility(EComponentMobility::Movable);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetupAttachment(A->GetRootComponent());
	const TArray<FDCArtRef>* Set = GM->Art.Find(TEXT("creature:fish"));
	const float Length = 52.f;
	if (Set && Set->Num() > 0 && GM->ArtMeshes.IsValidIndex((*Set)[0].Mesh))
	{
		UStaticMesh* Mesh = GM->ArtMeshes[(*Set)[0].Mesh];
		C->SetStaticMesh(Mesh);
		C->RegisterComponent();
		FishScale = Length / FMath::Max(1.0, Mesh->GetBoundingBox().GetSize().GetMax());
		FishFix = FQuat(FRotator(0.f, -90.f, 0.f));
		for (int32 m = 0; m < C->GetNumMaterials(); ++m)
		{
			// as the other models: drawn as non-metal, the game has no reflections
			if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(m))
			{
				TArray<FMaterialParameterInfo> Infos;
				TArray<FGuid> Ids;
				MID->GetAllScalarParameterInfo(Infos, Ids);
				for (const FMaterialParameterInfo& I : Infos)
				{
					const FString N = I.Name.ToString();
					if (N.Contains(TEXT("Metallic")) && !N.Contains(TEXT("Tex")))
					{
						MID->SetScalarParameterValue(I.Name, 0.f);
					}
				}
			}
		}
		DCSwim::Apply(C, TEXT("creature:fish"), FishFix, 0.75f, 0.f); // each fish of the shoal beats on its own phase (per-instance random)
	}
	else
	{
		C->SetStaticMesh(GM->SphereMesh);
		C->RegisterComponent();
		FishScale = Length / 100.f;
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(GM->LitMaterial, A);
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.7f, 0.75f));
		C->SetMaterial(0, MID);
	}
	FRandomStream Rand(2207);
	for (int32 i = 0; i < DCD::NumFish; ++i)
	{
		// ring radius, height, where on the ring, how fast round it
		FishSeed.Add(FVector4(Rand.FRandRange(150.f, 380.f), Rand.FRandRange(-150.f, 150.f), Rand.FRandRange(0.f, 2.f * PI), Rand.FRandRange(0.45f, 0.8f)));
		FishPos.Add(DCD::Reef);
		C->AddInstance(FTransform(DCD::Reef));
	}
	C->SetHasPerInstancePrevTransforms(true);
	Fish = C;
}

AActor* FDCDoc::SpawnPickup(const FVector& At, const FLinearColor& Color, TWeakObjectPtr<UPointLightComponent>& OutLight)
{
	AActor* A = GM->SpawnGroup(At);
	GM->AddPart(A, GM->CubeMesh, FVector::ZeroVector, FRotator(0.f, 0.f, 0.f), FVector(0.34, 0.22, 0.22), FLinearColor(0.12f, 0.13f, 0.14f), FLinearColor::Black);
	GM->AddPart(A, GM->SphereMesh, FVector(20.0, 0.0, 0.0), FRotator::ZeroRotator, FVector(0.2), Color, Color * 8.f);
	UPointLightComponent* L = NewObject<UPointLightComponent>(A);
	L->SetMobility(EComponentMobility::Movable);
	L->SetupAttachment(A->GetRootComponent());
	L->RegisterComponent();
	L->SetRelativeLocation(FVector(0.0, 0.0, 60.0));
	L->SetIntensityUnits(ELightUnits::Unitless);
	L->SetIntensity(5000.f);
	L->SetLightColor(Color);
	L->SetAttenuationRadius(800.f);
	L->SetCastShadows(false);
	L->SetVolumetricScatteringIntensity(2.f);
	OutLight = L;
	return A;
}


UPointLightComponent* FDCDoc::SoftLight(const FLinearColor& Color, float Power, float Reach)
{
	AActor* A = GM->SpawnGroup(FVector::ZeroVector);
	UPointLightComponent* L = NewObject<UPointLightComponent>(A);
	L->SetMobility(EComponentMobility::Movable);
	L->SetupAttachment(A->GetRootComponent());
	L->RegisterComponent();
	L->SetIntensityUnits(ELightUnits::Unitless);
	L->bUseInverseSquaredFalloff = false;
	L->SetLightFalloffExponent(2.f);
	L->SetIntensity(Power);
	L->SetLightColor(Color);
	L->SetAttenuationRadius(Reach);
	L->SetCastShadows(false);
	L->SetVolumetricScatteringIntensity(0.4f);
	L->MarkRenderStateDirty();
	return L;
}

void FDCDoc::SpawnAmbient()
{
	// kind: 0 swims a loop in open water, 1 crawls on the floor, 2 passes far off in the haze
	struct FRow { const TCHAR* Id; int32 Kind; int32 Count; float Length; FVector Centre; float Spread; float RMin, RMax; float ZMin, ZMax; float Speed; };
	static const FRow Rows[] = {
		{ TEXT("creature:squid"), 0, 3, 130.f, FVector(2600.0, -600.0, 0.0), 1500.f, 300.f, 800.f, 500.f, 1300.f, 90.f },
		{ TEXT("creature:nautilus"), 0, 3, 60.f, FVector(-3000.0, 2200.0, 0.0), 1200.f, 150.f, 400.f, 300.f, 900.f, 25.f },
		{ TEXT("creature:crab"), 1, 3, 230.f, FVector(1300.0, -2600.0, 0.0), 1500.f, 150.f, 400.f, 0.f, 0.f, 14.f },
		{ TEXT("creature:isopod"), 1, 4, 55.f, FVector(900.0, 2300.0, 0.0), 1100.f, 80.f, 260.f, 0.f, 0.f, 9.f },
		{ TEXT("creature:lantern"), 0, 12, 26.f, FVector(2400.0, 2390.0, 0.0), 1300.f, 60.f, 150.f, 140.f, 420.f, 45.f },
		{ TEXT("creature:whale"), 2, 1, 1500.f, FVector(0.0, 0.0, 0.0), 0.f, 5600.f, 6000.f, 1900.f, 2200.f, 160.f },
		{ TEXT("creature:shark"), 0, 1, 400.f, FVector(2400.0, 900.0, 0.0), 300.f, 1300.f, 1700.f, 900.f, 1100.f, 110.f },
		{ TEXT("creature:eel"), 1, 1, 220.f, FVector(3900.0, -100.0, 0.0), 200.f, 250.f, 400.f, 0.f, 0.f, 30.f },
		// on their way to the art drop: each joins in when its model is imported
		{ TEXT("creature:vampsquid"), 0, 2, 90.f, FVector(3200.0, 1300.0, 0.0), 900.f, 200.f, 500.f, 300.f, 700.f, 35.f },
		{ TEXT("creature:hatchet"), 0, 10, 22.f, FVector(-800.0, -1800.0, 0.0), 900.f, 80.f, 240.f, 500.f, 1100.f, 50.f },
		{ TEXT("creature:frilled"), 0, 1, 330.f, FVector(-1200.0, 2600.0, 0.0), 300.f, 1100.f, 1500.f, 350.f, 500.f, 70.f },
		{ TEXT("creature:seapig"), 1, 5, 40.f, FVector(-2500.0, -2700.0, 0.0), 1300.f, 60.f, 200.f, 0.f, 0.f, 5.f },
		{ TEXT("creature:barreleye"), 0, 3, 45.f, FVector(400.0, 1200.0, 0.0), 900.f, 150.f, 400.f, 400.f, 900.f, 30.f },
		{ TEXT("creature:coelacanth"), 0, 1, 190.f, FVector(-2600.0, 600.0, 0.0), 300.f, 500.f, 900.f, 300.f, 450.f, 40.f },
		{ TEXT("creature:oarfish"), 2, 1, 900.f, FVector(-500.0, -500.0, 0.0), 0.f, 4200.f, 4600.f, 1500.f, 1700.f, 120.f },
		{ TEXT("creature:stingray"), 0, 2, 190.f, FVector(-400.0, -2200.0, 0.0), 900.f, 400.f, 900.f, 130.f, 200.f, 60.f },
		{ TEXT("creature:cuttle"), 0, 2, 70.f, FVector(-3300.0, 2700.0, 0.0), 700.f, 150.f, 350.f, 350.f, 650.f, 30.f },
		{ TEXT("creature:brittlestar"), 1, 6, 45.f, FVector(-3300.0, 2400.0, 0.0), 1300.f, 20.f, 60.f, 0.f, 0.f, 3.f },
		{ TEXT("creature:siphon"), 0, 4, 60.f, FVector(-2500.0, -2700.0, 0.0), 800.f, 100.f, 300.f, 900.f, 1500.f, 15.f },
		{ TEXT("creature:seaangel"), 0, 5, 30.f, FVector(-1800.0, -300.0, 0.0), 1100.f, 60.f, 200.f, 400.f, 1100.f, 18.f },
		{ TEXT("creature:octopus"), 0, 2, 60.f, FVector(2600.0, -2300.0, 0.0), 900.f, 120.f, 320.f, 350.f, 800.f, 16.f },
		{ TEXT("creature:goblin"), 0, 1, 380.f, FVector(3600.0, 1000.0, 0.0), 300.f, 1000.f, 1500.f, 500.f, 700.f, 75.f },
		{ TEXT("creature:gulper"), 0, 1, 170.f, FVector(4300.0, -2300.0, 0.0), 500.f, 400.f, 800.f, 400.f, 800.f, 35.f },
		{ TEXT("creature:sunfish"), 2, 1, 420.f, FVector(300.0, 300.0, 0.0), 0.f, 3300.f, 3700.f, 1500.f, 1900.f, 55.f },
		{ TEXT("creature:seadragon"), 0, 3, 45.f, FVector(-3000.0, 2300.0, 0.0), 800.f, 80.f, 220.f, 250.f, 550.f, 10.f },
		{ TEXT("creature:seahorse"), 0, 4, 24.f, FVector(-3500.0, 2600.0, 0.0), 800.f, 30.f, 90.f, 250.f, 500.f, 5.f },
		{ TEXT("creature:grouper"), 0, 1, 190.f, FVector(2700.0, -200.0, 0.0), 300.f, 500.f, 800.f, 250.f, 400.f, 30.f },
		{ TEXT("creature:lobster"), 1, 3, 50.f, FVector(1800.0, -1900.0, 0.0), 1200.f, 60.f, 200.f, 0.f, 0.f, 8.f },
		{ TEXT("creature:lionsmane"), 0, 2, 150.f, FVector(-900.0, -3500.0, 0.0), 1100.f, 150.f, 400.f, 700.f, 1300.f, 12.f },
		{ TEXT("creature:nudibranch"), 1, 5, 22.f, FVector(-3200.0, 2000.0, 0.0), 1200.f, 15.f, 50.f, 0.f, 0.f, 2.f },
		{ TEXT("creature:krill"), 0, 10, 12.f, FVector(-200.0, 1100.0, 0.0), 700.f, 60.f, 180.f, 500.f, 900.f, 30.f },
		{ TEXT("creature:turtle2"), 0, 1, 170.f, FVector(-1700.0, -1500.0, 0.0), 400.f, 900.f, 1300.f, 800.f, 1100.f, 55.f },
		// ---- the wider sea (brief 08): each zone its own life (centres as in FDC3D::SetupZones)
		// the kelp forest
		{ TEXT("creature:turtle2"), 0, 2, 170.f, FVector(-13000.0, 7500.0, 0.0), 2500.f, 900.f, 1600.f, 600.f, 1200.f, 55.f },
		{ TEXT("creature:dolphin"), 0, 3, 260.f, FVector(-13000.0, 7500.0, 0.0), 1500.f, 1400.f, 2400.f, 900.f, 1600.f, 260.f },
		{ TEXT("creature:parrotfish"), 0, 5, 70.f, FVector(-13000.0, 7500.0, 0.0), 3500.f, 200.f, 600.f, 150.f, 500.f, 45.f },
		{ TEXT("creature:barracuda"), 0, 4, 150.f, FVector(-12000.0, 8500.0, 0.0), 2500.f, 600.f, 1200.f, 400.f, 900.f, 110.f },
		{ TEXT("creature:seahorse"), 0, 6, 24.f, FVector(-13000.0, 7500.0, 0.0), 3500.f, 30.f, 90.f, 200.f, 600.f, 5.f },
		{ TEXT("creature:seadragon"), 0, 4, 45.f, FVector(-13000.0, 7500.0, 0.0), 3500.f, 80.f, 220.f, 250.f, 600.f, 10.f },
		{ TEXT("creature:grouper"), 0, 2, 190.f, FVector(-13500.0, 6500.0, 0.0), 2500.f, 400.f, 900.f, 200.f, 400.f, 30.f },
		{ TEXT("creature:krill"), 0, 8, 12.f, FVector(-13000.0, 7500.0, 0.0), 3000.f, 60.f, 180.f, 400.f, 1000.f, 30.f },
		// the wreck graveyard
		{ TEXT("creature:hammerhead"), 0, 2, 380.f, FVector(14500.0, -3900.0, 0.0), 1500.f, 1500.f, 2400.f, 800.f, 1300.f, 120.f },
		{ TEXT("creature:shark"), 0, 1, 400.f, FVector(14500.0, -3900.0, 0.0), 1000.f, 1800.f, 2400.f, 600.f, 900.f, 110.f },
		{ TEXT("creature:barracuda"), 0, 4, 150.f, FVector(14500.0, -3900.0, 0.0), 3000.f, 500.f, 1100.f, 300.f, 800.f, 110.f },
		{ TEXT("creature:grouper"), 0, 2, 200.f, FVector(14500.0, -3900.0, 0.0), 3000.f, 400.f, 800.f, 200.f, 400.f, 30.f },
		{ TEXT("creature:lobster"), 1, 5, 50.f, FVector(14500.0, -3900.0, 0.0), 3500.f, 60.f, 200.f, 0.f, 0.f, 8.f },
		{ TEXT("creature:octopus"), 0, 2, 60.f, FVector(14500.0, -3900.0, 0.0), 3000.f, 120.f, 320.f, 300.f, 700.f, 16.f },
		// the drowned city
		{ TEXT("creature:sunfish"), 0, 2, 380.f, FVector(6800.0, 14500.0, 0.0), 1500.f, 1200.f, 2000.f, 900.f, 1500.f, 50.f },
		{ TEXT("creature:stingray"), 0, 4, 190.f, FVector(6800.0, 14500.0, 0.0), 3000.f, 400.f, 1000.f, 120.f, 220.f, 60.f },
		{ TEXT("creature:cuttle"), 0, 3, 70.f, FVector(6800.0, 14500.0, 0.0), 3000.f, 150.f, 350.f, 300.f, 700.f, 30.f },
		{ TEXT("creature:coelacanth"), 0, 2, 190.f, FVector(6800.0, 14500.0, 0.0), 2500.f, 600.f, 1000.f, 300.f, 500.f, 40.f },
		{ TEXT("creature:nautilus"), 0, 4, 60.f, FVector(6800.0, 14500.0, 0.0), 3000.f, 150.f, 400.f, 300.f, 900.f, 25.f },
		{ TEXT("creature:oarfish"), 0, 1, 700.f, FVector(6800.0, 14500.0, 0.0), 500.f, 2200.f, 2800.f, 1300.f, 1600.f, 90.f },
		// the whale fall
		{ TEXT("creature:isopod"), 1, 8, 55.f, FVector(-7500.0, -13000.0, 0.0), 3000.f, 80.f, 260.f, 0.f, 0.f, 9.f },
		{ TEXT("creature:crab"), 1, 4, 230.f, FVector(-7500.0, -13000.0, 0.0), 3500.f, 150.f, 400.f, 0.f, 0.f, 14.f },
		{ TEXT("creature:seapig"), 1, 8, 40.f, FVector(-7500.0, -13000.0, 0.0), 3500.f, 60.f, 200.f, 0.f, 0.f, 5.f },
		{ TEXT("creature:hatchet"), 0, 10, 22.f, FVector(-7500.0, -13000.0, 0.0), 3000.f, 80.f, 240.f, 400.f, 1000.f, 50.f },
		{ TEXT("creature:frilled"), 0, 1, 330.f, FVector(-7500.0, -13000.0, 0.0), 1000.f, 1200.f, 1800.f, 300.f, 500.f, 70.f },
		// the black smokers
		{ TEXT("creature:yeticrab"), 1, 10, 45.f, FVector(6800.0, -14500.0, 0.0), 3000.f, 30.f, 120.f, 0.f, 0.f, 4.f },
		{ TEXT("creature:ventshrimp"), 0, 6, 40.f, FVector(6800.0, -14500.0, 0.0), 2500.f, 60.f, 200.f, 150.f, 500.f, 12.f },
		{ TEXT("creature:lobster"), 1, 4, 50.f, FVector(6800.0, -14500.0, 0.0), 3000.f, 60.f, 200.f, 0.f, 0.f, 8.f },
		{ TEXT("creature:nudibranch"), 1, 6, 22.f, FVector(6800.0, -14500.0, 0.0), 3000.f, 15.f, 50.f, 0.f, 0.f, 2.f },
		{ TEXT("creature:octopus"), 0, 2, 60.f, FVector(6800.0, -14500.0, 0.0), 2500.f, 120.f, 320.f, 300.f, 700.f, 16.f },
		// the abyss
		{ TEXT("creature:giantsquid"), 0, 1, 900.f, FVector(-5500.0, 19500.0, 0.0), 800.f, 2000.f, 2800.f, 900.f, 1400.f, 80.f },
		{ TEXT("creature:dragonfish"), 0, 5, 50.f, FVector(-5500.0, 19500.0, 0.0), 3500.f, 100.f, 300.f, 300.f, 1100.f, 30.f },
		{ TEXT("creature:viperfish"), 0, 5, 45.f, FVector(-5500.0, 19500.0, 0.0), 3500.f, 100.f, 300.f, 300.f, 1100.f, 35.f },
		{ TEXT("creature:atolla"), 0, 6, 70.f, FVector(-5500.0, 19500.0, 0.0), 3500.f, 100.f, 300.f, 500.f, 1500.f, 12.f },
		{ TEXT("creature:seatoad"), 1, 5, 40.f, FVector(-5500.0, 19500.0, 0.0), 3500.f, 40.f, 140.f, 0.f, 0.f, 3.f },
		{ TEXT("creature:vampsquid"), 0, 3, 90.f, FVector(-5500.0, 19500.0, 0.0), 3000.f, 200.f, 500.f, 300.f, 900.f, 35.f },
		{ TEXT("creature:gulper"), 0, 2, 170.f, FVector(-5500.0, 19500.0, 0.0), 3000.f, 400.f, 800.f, 400.f, 1000.f, 35.f },
		{ TEXT("creature:goblin"), 0, 1, 380.f, FVector(-5500.0, 19500.0, 0.0), 1000.f, 1500.f, 2200.f, 500.f, 900.f, 75.f },
		{ TEXT("creature:barreleye"), 0, 4, 45.f, FVector(-5500.0, 19500.0, 0.0), 3000.f, 150.f, 400.f, 400.f, 1000.f, 30.f },
		{ TEXT("creature:siphon"), 0, 5, 60.f, FVector(-5500.0, 19500.0, 0.0), 3000.f, 100.f, 300.f, 600.f, 1500.f, 15.f },
		// the things the official film says are not there: one place each, deeper is worse
		{ TEXT("creature:ribray"), 0, 1, 520.f, FVector(-13000.0, 7500.0, 0.0), 2500.f, 900.f, 1400.f, 400.f, 900.f, 90.f },
		{ TEXT("creature:hollowdiver"), 0, 2, 210.f, FVector(14500.0, -3900.0, 0.0), 2500.f, 300.f, 700.f, 150.f, 400.f, 45.f },
		{ TEXT("creature:drowner"), 0, 2, 230.f, FVector(6800.0, 14500.0, 0.0), 2500.f, 300.f, 700.f, 150.f, 500.f, 50.f },
		{ TEXT("creature:choir"), 0, 1, 320.f, FVector(-7500.0, -13000.0, 0.0), 1500.f, 400.f, 800.f, 300.f, 700.f, 30.f },
		{ TEXT("creature:mawworm"), 1, 2, 700.f, FVector(-7500.0, -13000.0, 0.0), 2500.f, 300.f, 600.f, 0.f, 0.f, 40.f },
		{ TEXT("creature:stilt"), 1, 1, 650.f, FVector(6800.0, -14500.0, 0.0), 2500.f, 600.f, 1200.f, 0.f, 0.f, 35.f },
		{ TEXT("creature:widow"), 0, 2, 340.f, FVector(-5500.0, 19500.0, 0.0), 3000.f, 500.f, 1200.f, 300.f, 800.f, 60.f },
		{ TEXT("creature:warden"), 0, 1, 1100.f, FVector(-5500.0, 19500.0, 0.0), 1500.f, 1500.f, 2200.f, 900.f, 1400.f, 70.f } };
	float Front = -90.f;
	FParse::Value(FCommandLine::Get(), TEXT("-MCFront="), Front);
	float Much = 1.f; // -MCLife=0.5 halves the ambient life, 0 leaves only the shot list
	FParse::Value(FCommandLine::Get(), TEXT("-MCLife="), Much);
	FRandomStream Rand(60613);
	UWorld* World = GM->GetWorld();
	const FCollisionQueryParams Query(FName(TEXT("DCDoc")), true);
	int32 Total = 0;
	for (const FRow& Row : Rows)
	{
		if (!GM->Art.Contains(Row.Id))
		{
			continue;
		}
		const int32 Count = Row.Count > 1 ? FMath::RoundToInt(Row.Count * Much) : (Much > 0.f ? 1 : 0);
		for (int32 i = 0; i < Count; ++i)
		{
			FAmbient One;
			float Half = 0.f;
			One.Actor = SpawnCreature(Row.Id, Row.Length * Rand.FRandRange(0.85f, 1.15f), Front, false, &Half, FMath::Clamp(Row.Speed / Row.Length * 4.f, 0.6f, 1.6f));
			One.Radius = Row.Length * 0.5f;
			for (int32 s = DCSub::Num; s < Subjects.Num(); ++s)
			{
				One.Subject = Subjects[s].Id == Row.Id ? s : One.Subject;
			}
			One.Surge = DCSwim::KindOf(Row.Id).PulseAmp > 0.f ? 0.5f : 0.f; // what swims by a jet goes in surges
			// ambient life casts no shadow: 20k triangles each, and every caster is drawn again for each shadow cascade
			TInlineComponentArray<UStaticMeshComponent*> Parts; One.Actor->GetComponents(Parts);
			for (UStaticMeshComponent* Part : Parts)
			{
				Part->SetCastShadow(false);
				if (Row.Length < 130.f)
				{
					Part->SetCullDistance(FMath::Clamp(Row.Length * 70.f, 1800.f, 5200.f)); // too small to see from further off
				}
			}
			One.Ra = Rand.FRandRange(Row.RMin, Row.RMax);
			One.Rb = Row.Kind == 2 ? One.Ra : One.Ra * Rand.FRandRange(0.5f, 1.f);
			One.Phase = Rand.FRandRange(0.f, 2.f * PI);
			One.Omega = Row.Speed / FMath::Max(1.f, 0.5f * (One.Ra + One.Rb)) * (Rand.FRand() < 0.5f ? -1.f : 1.f);
			One.Bob = Row.Kind == 1 ? 0.f : FMath::Min(40.f, Row.Length * 0.25f);
			One.Wiggle = Row.Kind == 1 ? 2.f : 5.f;
			// a free place: off the rock, and for a crawler on whatever lies there
			for (int32 Try = 0; Try < 12; ++Try)
			{
				const float Ang = Rand.FRandRange(0.f, 2.f * PI);
				const float Rad = Row.Spread * FMath::Sqrt(Rand.FRand());
				One.Centre = Row.Centre + FVector(Rad * FMath::Cos(Ang), Rad * FMath::Sin(Ang), Row.Kind == 1 ? Half : Rand.FRandRange(Row.ZMin, Row.ZMax));
				if (Row.Kind != 1)
				{
					One.Centre.Z += W->Terrain(One.Centre.X, One.Centre.Y); // swimmers live over the floor where they are
				}
				if (Row.Kind == 2 || Row.Id == FString(TEXT("creature:lantern")))
				{
					if (Row.Kind == 0)
					{
						One.Centre.Y = Row.Centre.Y + Rand.FRandRange(-90.f, 90.f); // in the trench: a long narrow loop
						One.Ra = Rand.FRandRange(500.f, 1100.f);
						One.Omega = Row.Speed / (0.5f * (One.Ra + One.Rb)) * (i % 2 ? -1.f : 1.f);
					}
					break;
				}
				FHitResult Hit;
				const double Floor = W->Terrain(One.Centre.X, One.Centre.Y);
				const FVector Top(One.Centre.X, One.Centre.Y, Floor + 2500.0);
				const float Reach = FMath::Max(One.Ra, One.Rb) + Row.Length;
				if (Row.Kind == 1)
				{
					// a crawler needs bare floor under its whole loop
					if (!World->SweepSingleByChannel(Hit, Top, FVector(One.Centre.X, One.Centre.Y, Floor + Half + Reach + 30.0), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Reach), Query))
					{
						break;
					}
				}
				else if (!World->OverlapAnyTestByChannel(One.Centre, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Reach), Query)
					&& !World->SweepSingleByChannel(Hit, One.Centre, One.Centre + FVector(0.0, 0.0, 1.0), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(FMath::Min(Reach, 600.f)), Query))
				{
					break;
				}
			}
			Ambient.Add(One);
			++Total;
		}
	}
	UE_LOG(LogMatchCut, Display, TEXT("DOC ambient life: %d creatures"), Total);
}

void FDCDoc::Start()
{
	SetupSubjects();
	Seen.SetNum(Subjects.Num());
	Tally.SetNum(Subjects.Num());
	for (int32 i = 0; i < DCSub::Num; ++i)
	{
		Best[i] = 0;
		Pick[i] = INDEX_NONE;
		SubPos[i] = FVector::ZeroVector;
	}
	SubRadius[DCSub::Manta] = 250.f;
	SubRadius[DCSub::Shoal] = 300.f;
	SubRadius[DCSub::Angler] = 90.f;
	SubRadius[DCSub::Jelly] = 300.f;
	SubRadius[DCSub::Giant] = 380.f;
	SubRadius[DCSub::Crew] = 90.f;

	float Front = -90.f; // the models from the art drop have their nose along +y
	FParse::Value(FCommandLine::Get(), TEXT("-MCFront="), Front);
	Manta = SpawnCreature(TEXT("creature:manta"), 520.f, Front);
	Giant = SpawnCreature(TEXT("creature:giant"), 760.f, Front);
	Angler = SpawnCreature(TEXT("creature:angler"), 180.f, Front, true);
	Crew = SpawnCreature(TEXT("hero"), 190.f, 1000.f); // a second diver in the same suit
	for (int32 i = 0; i < DCD::NumJelly; ++i)
	{
		Jellies.Add(SpawnCreature(TEXT("creature:jelly"), 110.f + 22.f * (i % 3), Front));
		JellyPos.Add(DCD::Shaft);
	}
	SpawnShoal();

	if (AActor* A = Angler.Get())
	{
		// the lure: a bead that glows and a small light that reaches the rock, not the fish itself
		GM->AddPart(A, GM->SphereMesh, FVector(78.0, 0.0, 62.0), FRotator::ZeroRotator, FVector(0.13), FLinearColor(0.5f, 1.f, 0.7f), FLinearColor(3.f, 9.f, 5.f));
		UPointLightComponent* L = NewObject<UPointLightComponent>(A);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(A->GetRootComponent());
		L->RegisterComponent();
		L->SetRelativeLocation(FVector(95.0, 0.0, 70.0));
		L->SetIntensityUnits(ELightUnits::Unitless);
		L->SetLightColor(FLinearColor(0.45f, 1.f, 0.65f));
		L->SetAttenuationRadius(520.f);
		L->SetCastShadows(false);
		L->SetVolumetricScatteringIntensity(3.f);
		Lure = L;
	}
	if (AActor* A = Crew.Get())
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(A);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(A->GetRootComponent());
		L->RegisterComponent();
		L->SetRelativeLocation(FVector(60.0, 0.0, 90.0));
		L->SetIntensityUnits(ELightUnits::Unitless);
		L->SetLightColor(FLinearColor(1.f, 0.75f, 0.35f));
		L->SetAttenuationRadius(700.f);
		L->SetCastShadows(false);
		CrewLight = L;
	}
	// the headlamp lies on the wreck, the long lens on the reef: each on top of whatever is there
	auto OnTop = [this](double X, double Y)
	{
		FHitResult Hit;
		const bool bHit = GM->GetWorld()->SweepSingleByChannel(Hit, FVector(X, Y, 2400.0), FVector(X, Y, 60.0), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(40.f),
			FCollisionQueryParams(FName(TEXT("DCDoc")), true));
		return FVector(X, Y, (bHit ? Hit.Location.Z : 60.0) + 70.0);
	};
	LampAt = OnTop(3300.0, -700.0);
	LensAt = OnTop(-3600.0, 2700.0);
	UE_LOG(LogMatchCut, Display, TEXT("DOC headlamp at (%.0f,%.0f,%.0f), lens at (%.0f,%.0f,%.0f)"), LampAt.X, LampAt.Y, LampAt.Z, LensAt.X, LensAt.Y, LensAt.Z);
	// the headlamp and the lens are gear now: bought at the locker, not found on the floor
	if (USpotLightComponent* L = W->Lamp.Get())
	{
		L->SetLightingChannels(true, false, true); // the anglerfish is lit by nothing else
		L->SetAbsolute(true, true, false);
		// a beam that carries: the engine's true falloff leaves nothing of it ten metres out
		L->bUseInverseSquaredFalloff = false;
		L->SetLightFalloffExponent(3.f);
		L->SetAttenuationRadius(3000.f);
		L->SetOuterConeAngle(30.f);
		L->SetInnerConeAngle(12.f);
		L->SetIntensity(0.f);
		L->MarkRenderStateDirty();
	}
	FParse::Value(FCommandLine::Get(), TEXT("-MCLampPower="), LampPower);
	if (FParse::Param(FCommandLine::Get(), TEXT("MCLamp")))
	{
		bLampOwned = bLampOn = true;
	}
	SpawnAmbient();
	ShoalLight = SoftLight(FLinearColor(0.75f, 0.9f, 1.f), 7.f, 900.f);
	float JellyPower = 26.f; // enough to lift the bells out of the rock's shadow; they have no glow of their own
	FParse::Value(FCommandLine::Get(), TEXT("-MCJelly="), JellyPower);
	JellyLight = SoftLight(FLinearColor(1.f, 0.72f, 0.82f), JellyPower, 1000.f);
	if (Legend != INDEX_NONE)
	{
		Hunter = SpawnCreature(TEXT("creature:giantsquid"), 1800.f, Front);
		TickHunter(0.f, true);
	}
	StartDay();
	Now = Clock;
	TickCreatures(0.f, W->Pos, true);
	UE_LOG(LogMatchCut, Display, TEXT("DOC started: %d fish, %d jellyfish"), FishPos.Num(), Jellies.Num());
}

// ---------------------------------------------------------------------------------------------
// creatures: where each one is at the time Now
// ---------------------------------------------------------------------------------------------

void FDCDoc::TickCreatures(float Dt, const FVector& HeroPos, bool bLive)
{
	const double T = Now;

	// ---- the manta: one slow loop that takes it over the wreck
	{
		const double Ang = 2.0 * PI * T / DCD::MantaPeriod;
		const FVector P = DCD::MantaC + FVector(DCD::MantaA * FMath::Cos(Ang), DCD::MantaB * FMath::Sin(Ang), 120.0 * FMath::Sin(2.0 * Ang + 1.0));
		const FVector Dir(-DCD::MantaA * FMath::Sin(Ang), DCD::MantaB * FMath::Cos(Ang), 0.0);
		SubPos[DCSub::Manta] = P;
		if (AActor* A = Manta.Get())
		{
			A->SetActorLocationAndRotation(P + FVector(0.0, 0.0, 25.0 * FMath::Sin(T * 1.1)),
				FRotator(5.f * FMath::Sin(T * 1.1 + 1.2), Dir.Rotation().Yaw, -12.f + 7.f * FMath::Sin(T * 0.8)));
		}
	}

	// ---- the giant: a long straight pass high over the middle, then a long time away
	{
		const float U = FMath::Fmod((float)T + DCD::GiantLead, DCD::GiantPeriod) / DCD::GiantPass;
		const bool bHere = U <= 1.f;
		const FVector P = bHere ? FMath::Lerp(DCD::GiantA, DCD::GiantB, (double)U) + FVector(0.0, 0.0, 70.0 * FMath::Sin(T * 0.35)) : FVector(0.0, 0.0, -90000.0);
		SubPos[DCSub::Giant] = P;
		if (AActor* A = Giant.Get())
		{
			A->SetActorHiddenInGame(!bHere);
			A->SetActorLocationAndRotation(P, FRotator(4.f * FMath::Sin(T * 0.35 + 1.5), (DCD::GiantB - DCD::GiantA).Rotation().Yaw, 5.f * FMath::Sin(T * 0.5)));
		}
		if (bLive && State == EDCDocState::Shooting)
		{
			if (U > 0.28f && U < 0.5f && !bGiantCalled && Best[DCSub::Giant] == 0)
			{
				bGiantCalled = true;
				Say(DCDocText::GiantCall, 7.f);
			}
			else if (!bHere)
			{
				bGiantCalled = false;
			}
		}
	}

	// ---- the anglerfish: hangs at the end of the trench, its lure breathing
	{
		const FVector P = DCD::AnglerAt + FVector(18.0 * FMath::Sin(T * 0.4), 28.0 * FMath::Sin(T * 0.3), 16.0 * FMath::Sin(T * 0.5));
		SubPos[DCSub::Angler] = P;
		if (AActor* A = Angler.Get())
		{
			A->SetActorLocationAndRotation(P, FRotator(4.f * FMath::Sin(T * 0.5), 180.f + 9.f * FMath::Sin(T * 0.3), 3.f * FMath::Sin(T * 0.7)));
		}
		if (UPointLightComponent* L = Lure.Get())
		{
			L->SetIntensity(1500.f + 900.f * FMath::Sin(T * 2.1) + 300.f * FMath::Sin(T * 7.3));
		}
	}

	// ---- jellyfish: up and down in the shaft of light, each on its own beat
	{
		FVector Sum = FVector::ZeroVector;
		for (int32 i = 0; i < Jellies.Num(); ++i)
		{
			const double Ph = i * 0.9;
			const double Rad = 110.0 + 55.0 * (i % 4);
			const FVector P = DCD::Shaft + FVector(Rad * FMath::Cos(i * 2.4 + T * 0.05), Rad * FMath::Sin(i * 2.4 + T * 0.05),
				330.0 + 42.0 * i + 200.0 * (0.5 + 0.5 * FMath::Sin(T * 0.22 + Ph)));
			JellyPos[i] = P;
			Sum += P;
			if (AActor* A = Jellies[i].Get())
			{
				const float Pulse = FMath::Sin(T * 2.2 + Ph * 2.0);
				A->SetActorLocationAndRotation(P, FRotator(6.f * FMath::Sin(T * 0.6 + Ph), 40.f * i + 6.f * T, 6.f * FMath::Cos(T * 0.5 + Ph)));
				A->SetActorScale3D(FVector(1.0 + 0.07 * Pulse, 1.0 + 0.07 * Pulse, 1.0 - 0.10 * Pulse)); // the bell's own beat
			}
		}
		SubPos[DCSub::Jelly] = Jellies.Num() > 0 ? Sum / Jellies.Num() : DCD::Shaft;
	}

	// ---- the shoal: rings round a slow centre; it opens when something swims into it fast, and closes again
	{
		const FVector Centre = DCD::Reef + FVector(200.0 * FMath::Cos(T * 0.10), 200.0 * FMath::Sin(T * 0.13), 60.0 * FMath::Sin(T * 0.2));
		if (bLive)
		{
			// it opens as soon as you are close to it, and stays open a while after you leave
			const bool bInto = FVector::Dist(HeroPos, Centre) < 1000.0;
			Split = DCD::Ease(Split, bInto ? 1.f : 0.f, bInto ? 3.f : 0.25f, Dt);
			// where it opens from follows you softly: a fish never jumps because you turned
			SplitFrom = FVector::DistSquared(SplitFrom, HeroPos) > 4000.0 * 4000.0 ? HeroPos : FMath::Lerp(SplitFrom, HeroPos, 1.0 - FMath::Exp(-4.0 * Dt));
		}
		SubPos[DCSub::Shoal] = Centre;
		TArray<FTransform> Xf;
		Xf.Reserve(FishPos.Num());
		for (int32 i = 0; i < FishPos.Num(); ++i)
		{
			const FVector4& S = FishSeed[i];
			const double Th = S.Z + T * S.W;
			FVector P = Centre + FVector(S.X * FMath::Cos(Th), S.X * FMath::Sin(Th), S.Y + 18.0 * FMath::Sin(T * 1.3 + S.Z));
			// the way round the ring, with a small slow weave (the tail beat is the material's)
			float Yaw = FMath::UnwindDegrees(FMath::RadiansToDegrees(Th) + 90.f + 3.f * FMath::Sin(T * 1.7 + S.Z * 5.0));
			if (Split > 0.01f)
			{
				const FVector Away = P - SplitFrom;
				const double D = FMath::Max(1.0, Away.Size());
				const double Push = Split * FMath::Max(0.0, 600.0 - D) * 0.95;
				P += Away / D * Push;
				// turn the short way towards fleeing, never more than half a turn
				const float Flee = FMath::Clamp((float)Push / 260.f, 0.f, 0.7f);
				Yaw += Flee * FMath::FindDeltaAngleDegrees(Yaw, (float)Away.Rotation().Yaw);
			}
			FishPos[i] = P;
			Xf.Add(FTransform(FQuat(FRotator(0.f, Yaw, 0.f)) * FishFix, P, FVector(FishScale)));
		}
		if (UInstancedStaticMeshComponent* C = Fish.Get())
		{
			// last frame's places too, so the picture knows how each fish moved (no smear or flicker when it is close)
			if (FishPrevXf.Num() != Xf.Num())
			{
				FishPrevXf = Xf;
			}
			C->BatchUpdateInstancesTransforms(0, Xf, FishPrevXf, true, true, false);
			FishPrevXf = MoveTemp(Xf);
		}
	}

	// ---- the crewmate: when you come close in front of it, it rocks and blinks its light
	{
		if (bLive)
		{
			const FVector To = HeroPos - DCD::CrewAt;
			const bool bGreets = To.Size() < 800.0 && FVector::DotProduct(To.GetSafeNormal(), FRotator(0.f, DCD::CrewYaw, 0.f).Vector()) > 0.15;
			Wave = DCD::Ease(Wave, bGreets ? 1.f : 0.f, bGreets ? 4.f : 2.f, Dt);
		}
		SubPos[DCSub::Crew] = DCD::CrewAt + FVector(0.0, 0.0, 10.0);
		if (AActor* A = Crew.Get())
		{
			const float Turn = FMath::Clamp(FMath::FindDeltaAngleDegrees(DCD::CrewYaw, (float)(HeroPos - DCD::CrewAt).Rotation().Yaw), -55.f, 55.f);
			A->SetActorLocationAndRotation(DCD::CrewAt + FVector(0.0, 0.0, Wave * 9.0 * FMath::Abs(FMath::Sin(T * 7.0)) + 4.0 * FMath::Sin(T * 1.1)),
				FRotator(0.f, DCD::CrewYaw + Wave * Turn, Wave * 17.f * FMath::Sin(T * 7.0)));
		}
		if (UPointLightComponent* L = CrewLight.Get())
		{
			L->SetIntensity(1200.f + Wave * 7000.f * (0.5f + 0.5f * FMath::Sin(T * 14.0)));
		}
	}

	// ---- the rest of the sea: every one a loop in time
	for (FAmbient& One : Ambient)
	{
		if (AActor* A = One.Actor.Get())
		{
			// the wider sea: what lives in a zone far off is not moved or drawn (brief 08)
			const bool bAway = FVector::Dist2D(One.Centre, HeroPos) > FMath::Max(One.Ra, One.Rb) + 11000.0;
			if (bAway != One.bAway)
			{
				One.bAway = bAway;
				A->SetActorHiddenInGame(bAway);
			}
			if (bAway)
			{
				continue;
			}
			// a jet swimmer's speed swells and dies with its pulse (never backwards: the surge is less than the pace)
			const double Th = One.Phase + One.Omega * (T + One.Surge * 0.9 * FMath::Sin(T * 1.1 + One.Phase * 5.0));
			FVector P = One.Centre + FVector(One.Ra * FMath::Cos(Th), One.Rb * FMath::Sin(Th), One.Bob * FMath::Sin(T * 0.6 + One.Phase * 3.0));
			if (One.Bob <= 0.f)
			{
				P.Z += W->GroundZ(P.X, P.Y); // a crawler walks over the mounds
			}
			const FVector Dir = FVector(-One.Ra * FMath::Sin(Th), One.Rb * FMath::Cos(Th), 0.0) * (One.Omega < 0.f ? -1.0 : 1.0);
			A->SetActorLocationAndRotation(P, FRotator(0.f, Dir.Rotation().Yaw + One.Wiggle * FMath::Sin(T * 4.0 + One.Phase * 7.0), One.Bob > 0.f ? 4.f * FMath::Sin(T * 0.9 + One.Phase) + FMath::Clamp(One.Omega * 70.f, -14.f, 14.f) : 0.f)); // it banks into its turn
		}
	}
	if (UPointLightComponent* L = ShoalLight.Get())
	{
		L->SetWorldLocation(SubPos[DCSub::Shoal] + FVector(0.0, 0.0, 120.0));
	}
	if (UPointLightComponent* L = JellyLight.Get())
	{
		L->SetWorldLocation(SubPos[DCSub::Jelly]);
	}

	// ---- what can be picked up turns slowly and breathes
	if (AActor* A = LampItem.Get())
	{
		A->SetActorRotation(FRotator(0.f, 40.f * Clock, 0.f));
	}
	if (AActor* A = LensItem.Get())
	{
		A->SetActorRotation(FRotator(0.f, -40.f * Clock, 0.f));
	}
}

// ---------------------------------------------------------------------------------------------
// the lens
// ---------------------------------------------------------------------------------------------

bool FDCDoc::Project(const FVector& P, FVector2D& Out, float& Depth) const
{
	const FVector V = CamR.UnrotateVector(P - CamP);
	Depth = V.X;
	if (V.X < 20.0)
	{
		return false;
	}
	const double TanH = FMath::Tan(FMath::DegreesToRadians(CamFov * 0.5f));
	Out = FVector2D(V.Y / (V.X * TanH), V.Z / (V.X * TanH) * DCD::Aspect);
	return true;
}

void FDCDoc::Look(const FVector& P, const FRotator& R, float Fov)
{
	CamP = P;
	CamR = R;
	CamFov = Fov;
	const float TanH = FMath::Tan(FMath::DegreesToRadians(Fov * 0.5f));
	UWorld* World = GM->GetWorld();
	const FCollisionQueryParams Query(FName(TEXT("DCDoc")), true);
	Lock = INDEX_NONE;
	float BestOff = 10.f;
	// the ambient life and the legend: for each species, the one nearest the middle of the picture
	const int32 NumSub = FMath::Min(Subjects.Num(), Seen.Num());
	TArray<FVector> FarAt;
	TArray<float> FarR, FarOff;
	FarAt.Init(FVector::ZeroVector, NumSub);
	FarR.Init(0.f, NumSub);
	FarOff.Init(10.f, NumSub);
	auto MaxFar = [](float Radius) { return FMath::Clamp(Radius * 14.f, 1200.f, 4500.f); };
	auto Consider = [&](int32 Sub, const FVector& At, float Radius)
	{
		FVector2D In;
		float D = 0.f;
		if (Sub >= DCSub::Num && Sub < NumSub && Project(At, In, D) && FMath::Abs(In.X) < 1.0 && FMath::Abs(In.Y) < 1.0 && D < MaxFar(Radius) && In.Size() < FarOff[Sub])
		{
			FarOff[Sub] = (float)In.Size();
			FarAt[Sub] = At;
			FarR[Sub] = Radius;
		}
	};
	for (const FAmbient& One : Ambient)
	{
		const AActor* A = One.Actor.Get();
		if (!One.bAway && A && !A->IsHidden())
		{
			Consider(One.Subject, A->GetActorLocation(), One.Radius);
		}
	}
	if (Subjects.IsValidIndex(Legend))
	{
		const AActor* H = Hunter.Get();
		if (H && !H->IsHidden())
		{
			Consider(Legend, H->GetActorLocation(), Subjects[Legend].Radius);
		}
	}
	for (int32 s = 0; s < NumSub; ++s)
	{
		FDCSeen& S = Seen[s];
		S = FDCSeen();
		const bool bFar = s >= DCSub::Num;
		if (bFar && FarOff[s] >= 10.f)
		{
			continue;
		}
		FVector Mid = bFar ? FarAt[s] : SubPos[s];
		const float SubR = bFar ? FarR[s] : SubRadius[s];
		const float MaxD = bFar ? MaxFar(SubR) : DCD::MaxDist[s];
		float Depth = 0.f;
		const bool bGroup = s == DCSub::Shoal || s == DCSub::Jelly;
		if (bGroup)
		{
			const TArray<FVector>& Items = s == DCSub::Shoal ? FishPos : JellyPos;
			FVector Sum = FVector::ZeroVector;
			int32 Count = 0;
			for (const FVector& Item : Items)
			{
				FVector2D At;
				float D = 0.f;
				if (Project(Item, At, D) && FMath::Abs(At.X) < 1.0 && FMath::Abs(At.Y) < 1.0 && D < MaxD)
				{
					Sum += Item;
					++Count;
				}
			}
			if (Count < DCD::Need[s])
			{
				GFewFrames += bRecording && Count > 0 ? 1 : 0;
				continue;
			}
			Mid = Sum / Count;
		}
		if (!Project(Mid, S.At, Depth) || Depth > MaxD)
		{
			continue;
		}
		S.Dist = Depth;
		S.Size = FMath::Min(1.f, SubR / (Depth * TanH) * DCD::Aspect);
		const float SizeX = S.Size / DCD::Aspect;
		if (FMath::Abs(S.At.X) > 1.0 || FMath::Abs(S.At.Y) > 1.0)
		{
			continue;
		}
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, P, Mid - (Mid - P).GetSafeNormal() * SubR * 0.8, ECC_Visibility, Query))
		{
			continue; // rock in the way
		}
		S.bIn = true;
		S.bWhole = bGroup || (FMath::Abs(S.At.X) + SizeX < 1.03 && FMath::Abs(S.At.Y) + S.Size < 1.03);
		switch (bFar ? -1 : s)
		{
		case -1: S.bAction = true; break;
		case DCSub::Manta: S.bAction = FVector::Dist2D(SubPos[s], DCD::Wreck) < 1150.0; break;
		case DCSub::Shoal: S.bAction = Split > 0.3f; break;
		case DCSub::Angler: S.bAction = bLampOn || (bHasPlay && Play.bLamp); break;
		case DCSub::Jelly: S.bAction = true; break;
		case DCSub::Giant: S.bAction = SubPos[s].Z - P.Z > 450.0 && R.Pitch > 10.f; break;
		default: S.bAction = Wave > 0.5f; break;
		}
		const float Off = (float)S.At.Size();
		if (Off < BestOff)
		{
			BestOff = Off;
			Lock = s;
		}
	}
}

// ---------------------------------------------------------------------------------------------
// shooting
// ---------------------------------------------------------------------------------------------

int32 FDCDoc::ShotsDone() const
{
	int32 N = 0;
	for (int32 i = 0; i < DCSub::Num; ++i)
	{
		N += Best[i] > 0 ? 1 : 0;
	}
	return N;
}

int32 FDCDoc::StarsTotal() const
{
	int32 N = 0;
	for (int32 i = 0; i < DCSub::Num; ++i)
	{
		N += Takes.IsValidIndex(Pick[i]) ? Takes[Pick[i]].Stars : 0;
	}
	return N;
}

int32 FDCDoc::TakesOf(int32 Shot, TArray<int32>* Out) const
{
	int32 N = 0;
	for (int32 i = 0; i < Takes.Num(); ++i)
	{
		if (Takes[i].Shot == Shot)
		{
			++N;
			if (Out)
			{
				Out->Add(i);
			}
		}
	}
	return N;
}

void FDCDoc::Say(const FString& Line, float Seconds)
{
	Note = Line;
	NoteTime = Seconds;
	NoteAge = 0.f;
}

bool FDCDoc::Bearing(const FVector& P, FVector2D& Out) const
{
	const FVector V = CamR.UnrotateVector(P - CamP);
	if (V.X > 20.0)
	{
		const double TanH = FMath::Tan(FMath::DegreesToRadians(CamFov * 0.5f));
		Out = FVector2D(V.Y / (V.X * TanH), V.Z / (V.X * TanH) * DCD::Aspect);
		return true;
	}
	Out = FVector2D(V.Y, V.Z).GetSafeNormal() * 50.0; // behind: far off the edge, the way to turn
	if (Out.IsNearlyZero())
	{
		Out = FVector2D(50.0, 0.0);
	}
	return false;
}

static const TCHAR* TipFor(const FString& Reason, int32 Sub)
{
	if (Reason == DCDocText::TooFar) return DCDocText::TipFar;
	if (Reason == DCDocText::CutOff) return DCDocText::TipCutOff;
	if (Reason == DCDocText::Shaky) return DCDocText::TipShaky;
	if (Reason == DCDocText::OffCentre) return DCDocText::TipCentre;
	if (Reason == DCDocText::Lost) return DCDocText::TipLost;
	if (Reason == DCDocText::Nothing) return DCDocText::TipNothing;
	if (Reason == DCDocText::Few) return DCDocText::TipFew;
	if ((Reason == DCDocText::Missed || Reason == DCDocText::TooDark) && Sub >= 0 && Sub < DCSub::Num) return DCDocText::TipMoment[Sub];
	return TEXT("");
}

void FDCDoc::EndTake()
{
	bRecording = false;
	const int32 Few = GFewFrames;
	GFewFrames = 0;
	const int32 N = FMath::Max(1, RecFrames);
	// what the clip is of: of all that stayed in the picture most of the take, the one worth most; else what was in it longest
	int32 Sub = INDEX_NONE;
	for (int32 s = 0; s < Tally.Num(); ++s)
	{
		if (Tally[s].In > 0 && (Sub == INDEX_NONE || Tally[s].In > Tally[Sub].In))
		{
			Sub = s;
		}
	}
	int32 Worth = INDEX_NONE;
	for (int32 s = 0; s < Tally.Num() && s < Subjects.Num(); ++s)
	{
		if (Tally[s].In * 2 >= N && (Worth == INDEX_NONE || Subjects[s].Value > Subjects[Worth].Value))
		{
			Worth = s;
		}
	}
	Sub = Worth != INDEX_NONE ? Worth : Sub;
	ResultTime = 5.f;
	ResultShot = Sub;
	ResultStars = 0;
	if (Sub == INDEX_NONE)
	{
		ResultReason = Few * 3 > N ? DCDocText::Few : DCDocText::Nothing; // one fish of a shoal is not the shoal
	}
	else
	{
		const FTally& T = Tally[Sub];
		const float In = (float)T.In / N, Whole = (float)T.Whole / T.In, Act = (float)T.Action / N; // Whole: of the time it was in the picture
		const float Size = T.Size / T.In, Off = T.Off / T.In;
		const float Shake = RecShake / N; // parts of the picture's width per second
		const bool bGroup = Sub == DCSub::Shoal || Sub == DCSub::Jelly;
		const float MinSize = bGroup ? 0.10f : 0.05f, GoodSize = bGroup ? 0.28f : 0.13f;
		if (Size < MinSize)
		{
			ResultReason = DCDocText::TooFar;
		}
		else if (In < 0.4f)
		{
			ResultReason = DCDocText::Lost;
		}
		else if (Whole < 0.4f)
		{
			ResultReason = DCDocText::CutOff;
		}
		else if (Act < 0.25f)
		{
			ResultReason = Sub == DCSub::Angler ? DCDocText::TooDark : DCDocText::Missed;
		}
		else
		{
			const bool bFramed = Size >= GoodSize && Whole >= 0.85f;
			const bool bMoment = Act >= 0.7f && In >= 0.85f;
			const bool bSteady = Shake < 0.30f;
			const bool bPlaced = Off < 0.5f;
			const int32 Good = (bFramed ? 1 : 0) + (bMoment ? 1 : 0) + (bSteady && bPlaced ? 1 : 0);
			ResultStars = Good >= 3 ? 3 : Good == 2 ? 2 : 1;
			ResultReason = !bFramed ? (Size < GoodSize ? DCDocText::TooFar : DCDocText::CutOff) : !bMoment ? (In < 0.85f ? DCDocText::Lost : DCDocText::Missed)
				: !bSteady ? DCDocText::Shaky : !bPlaced ? DCDocText::OffCentre : DCDocText::Perfect;
		}
		UE_LOG(LogMatchCut, Display, TEXT("DOC TAKE shot=%d stars=%d reason=\"%s\" in=%.2f whole=%.2f action=%.2f size=%.3f off=%.2f shake=%.2f frames=%d"),
			Sub, ResultStars, *ResultReason, In, Whole, Act, Size, Off, Shake, N);
	}
	Tip = ResultStars <= 1 ? TipFor(ResultReason, Sub) : TEXT("");
	if (ResultStars <= 1)
	{
		ResultTime = 8.f; // time to read the tip
	}
	if (ResultStars == 0)
	{
		if (Sub == INDEX_NONE)
		{
			UE_LOG(LogMatchCut, Display, TEXT("DOC TAKE shot=-1 stars=0 reason=\"%s\""), *ResultReason);
		}
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("cut_splice"), 0.5f, 0.7f);
		}
		return;
	}
	Rec.Shot = Sub;
	Rec.Stars = ResultStars;
	Rec.bDanger = GDangerFrames * 4 > N;
	GDangerFrames = 0;
	Takes.Add(Rec);
	UE_LOG(LogMatchCut, Display, TEXT("DOC CLIP %d/%d: %s, %d stars%s"), Takes.Num(), TapeCap(), *SubjectName(Sub), ResultStars, Rec.bDanger ? TEXT(", danger") : TEXT(""));
	if (GM->Audio)
	{
		GM->Audio->Play(TEXT("take_clapper"), 0.8f, 1.f);
	}
	if (Takes.Num() >= TapeCap())
	{
		Alert = TEXT("TAPE FULL. Go home and upload.");
		AlertTime = 5.f;
	}
}

void FDCDoc::TickShooting(float Dt)
{
	bAim = bAimHeld || bAimToggle;
	bEverAimed |= bAim;
	AimBlend = DCD::Ease(AimBlend, bAim ? 1.f : 0.f, 12.f, Dt);
	if (ZoomIn != 0.f && bAim)
	{
		Zoom = FMath::Clamp(Zoom * FMath::Pow(0.9f, ZoomIn), MinFov, 72.f);
		bEverZoomed = true;
	}
	ZoomIn = 0.f;

	if (bLampPressed && bLampOwned)
	{
		bLampOn = !bLampOn;
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("lamp_on"), 0.8f, bLampOn ? 1.f : 0.8f);
		}
	}
	TickDive(Dt);
	NoteAge += Dt;

	// focus: on what the lens is locked to, else on whatever is straight ahead
	float Want = 1500.f;
	if (Lock != INDEX_NONE)
	{
		Want = Seen[Lock].Dist;
	}
	else
	{
		FHitResult Hit;
		if (GM->GetWorld()->LineTraceSingleByChannel(Hit, CamP, CamP + CamR.Vector() * 4000.0, ECC_Visibility, FCollisionQueryParams(FName(TEXT("DCDoc")), true)))
		{
			Want = Hit.Distance;
		}
	}
	Focus = DCD::Ease(Focus, FMath::Max(80.f, Want), 6.f, Dt);

	if (bRecordPressed && bAim && !bRecording && !bBlackout && Takes.Num() >= TapeCap())
	{
		Alert = TEXT("TAPE FULL. Go home and upload.");
		AlertTime = 3.f;
	}
	else if (bRecordPressed && bAim && !bRecording && !bBlackout)
	{
		bRecording = true;
		bEverRecorded = true;
		RecTime = 0.f;
		RecFrames = 0;
		RecShake = 0.f;
		LastRot = CamR;
		Rec = FDCTake();
		Rec.T0 = Clock;
		Rec.Len = DCD::TakeLen;
		for (FTally& Each : Tally)
		{
			Each = FTally();
		}
		GDangerFrames = 0;
		ResultTime = 0.f;
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("cam_rec"), 0.8f, 1.f);
		}
	}
	if (bRecording)
	{
		FDCDocFrame F;
		F.Cam = CamP;
		F.Rot = CamR;
		F.Fov = CamFov;
		F.Focus = Focus;
		F.Hero = W->Pos;
		F.Split = Split;
		F.Wave = Wave;
		F.bLamp = bLampOn;
		F.Hunt = HuntPos;
		F.HuntYaw = HuntYaw;
		Rec.Frames.Add(F);
		++RecFrames;
		GDangerFrames += DangerNear > 0.f ? 1 : 0;
		for (int32 s = 0; s < FMath::Min(Seen.Num(), Tally.Num()); ++s)
		{
			const FDCSeen& S = Seen[s];
			if (!S.bIn)
			{
				continue;
			}
			FTally& T = Tally[s];
			++T.In;
			T.Whole += S.bWhole ? 1 : 0;
			T.Action += S.bAction ? 1 : 0;
			T.Size += S.Size;
			// from the middle, or from the nearest of the four points where the thirds cross
			const FVector2D A(FMath::Abs(S.At.X), FMath::Abs(S.At.Y));
			T.Off += FMath::Min((float)A.Size(), (float)(A - FVector2D(1.0 / 3.0, 1.0 / 3.0)).Size());
		}
		if (Dt > 0.f)
		{
			const float Deg = FMath::RadiansToDegrees((float)CamR.Quaternion().AngularDistance(LastRot.Quaternion()));
			RecShake += Deg / Dt / FMath::Max(10.f, CamFov);
		}
		LastRot = CamR;
		RecTime += Dt;
		if (RecTime >= DCD::TakeLen || !bAim)
		{
			if (RecTime < DCD::TakeLen * 0.6f)
			{
				// the camera came down too soon: not a take
				bRecording = false;
			}
			else
			{
				Rec.Len = RecTime;
				EndTake();
			}
		}
	}
	bRecordPressed = false;
	bLampPressed = false;
	ResultTime = FMath::Max(0.f, ResultTime - Dt);
	ListTime = FMath::Max(0.f, ListTime - Dt);
	NoteTime = FMath::Max(0.f, NoteTime - Dt);

}

// ---------------------------------------------------------------------------------------------
// the cutting room and the film
// ---------------------------------------------------------------------------------------------

void FDCDoc::SetState(EDCDocState S)
{
	State = S;
	StateTime = 0.f;
	bAimToggle = false;
	bRecording = false;
	UE_LOG(LogMatchCut, Display, TEXT("DOC state %d, day %d, clips %d, money %d, proofs %d/%d"), (int32)S, Day, Takes.Num(), Money, EvidenceCount(), EvidenceTotal());
	if (S == EDCDocState::Shooting)
	{
		bHasPlay = false;
		PlayTake = INDEX_NONE;
		Now = Clock;
		W->Vel = FVector::ZeroVector;
	}
	else if (S == EDCDocState::Edit)
	{
		EditShot = FMath::Clamp(EditShot, 0, FMath::Max(0, Takes.Num() - 1));
		StartPlayTake(EditShot);
	}
	else if (S == EDCDocState::Film)
	{
		FilmPart = 0;
		FilmTime = 0.f;
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("take_clapper"), 0.9f, 0.9f);
		}
	}
}

void FDCDoc::StartPlayTake(int32 TakeIndex)
{
	PlayTake = TakeIndex;
	PlayT = 0.f;
	if (GM->Audio && Takes.IsValidIndex(TakeIndex))
	{
		GM->Audio->Play(TEXT("cut_splice"), 0.6f, 1.f);
	}
}

void FDCDoc::TickPlay(float Dt, bool bLoop)
{
	if (!Takes.IsValidIndex(PlayTake) || Takes[PlayTake].Frames.Num() == 0)
	{
		return;
	}
	const FDCTake& T = Takes[PlayTake];
	PlayT += Dt;
	if (PlayT > T.Len)
	{
		PlayT = bLoop ? 0.f : T.Len;
	}
	const float X = FMath::Clamp(PlayT / T.Len, 0.f, 1.f) * (T.Frames.Num() - 1);
	const int32 i = FMath::Min(FMath::FloorToInt(X), T.Frames.Num() - 1);
	const int32 j = FMath::Min(i + 1, T.Frames.Num() - 1);
	const float A = X - i;
	const FDCDocFrame& F0 = T.Frames[i];
	const FDCDocFrame& F1 = T.Frames[j];
	Play = F0;
	Play.Cam = FMath::Lerp(F0.Cam, F1.Cam, (double)A);
	Play.Rot = FQuat::Slerp(F0.Rot.Quaternion(), F1.Rot.Quaternion(), A).Rotator();
	Play.Fov = FMath::Lerp(F0.Fov, F1.Fov, A);
	Play.Focus = FMath::Lerp(F0.Focus, F1.Focus, A);
	Play.Hero = FMath::Lerp(F0.Hero, F1.Hero, (double)A);
	Play.Split = FMath::Lerp(F0.Split, F1.Split, A);
	Play.Wave = FMath::Lerp(F0.Wave, F1.Wave, A);
	bHasPlay = true;
	Now = T.T0 + PlayT;
	Split = Play.Split;
	Wave = Play.Wave;
	// a clip is what the lens saw: what only the lens sees is in it
	HuntPos = FMath::Lerp(F0.Hunt, F1.Hunt, (double)A);
	HuntYaw = F0.HuntYaw;
	TickHunter(0.f, false);
	for (const FAmbient& One : Ambient)
	{
		AActor* Act = One.Actor.Get();
		if (Act && !One.bAway && Subjects.IsValidIndex(One.Subject) && Subjects[One.Subject].bLensOnly)
		{
			Act->SetActorHiddenInGame(false);
		}
	}
	if (AActor* H = Hunter.Get())
	{
		H->SetActorHiddenInGame(Play.Hunt.IsZero());
	}
}

bool FDCDoc::ViewOverride(FVector& OutPos, FRotator& OutRot, float& OutFov) const
{
	if (!bHasPlay || State == EDCDocState::Shooting)
	{
		return false;
	}
	OutPos = Play.Cam;
	OutRot = Play.Rot;
	OutFov = Play.Fov;
	return true;
}

void FDCDoc::Tick(float Dt)
{
	Clock += Dt;
	StateTime += Dt;
	if (State == EDCDocState::Shooting)
	{
		Now = Clock;
		TickCreatures(Dt, W->Pos, true);
		Look(W->CamPos, FRotator(W->Pitch, W->Yaw, 0.f), W->ViewFov);
		TickShooting(Dt);
	}
	else
	{
		if (State == EDCDocState::Edit)
		{
			// the cutting room: A/D a clip, W the thumbnail, S in or out, ENTER uploads, ESC back to the base
			if (NavX != 0 && Takes.Num() > 0)
			{
				EditShot = ((EditShot + NavX) % Takes.Num() + Takes.Num()) % Takes.Num();
				StartPlayTake(EditShot);
			}
			if (NavY < 0 && Takes.IsValidIndex(EditShot))
			{
				Headline = EditShot;
				Takes[EditShot].bKeep = true;
			}
			if (NavY > 0 && Takes.IsValidIndex(EditShot))
			{
				Takes[EditShot].bKeep = !Takes[EditShot].bKeep;
				if (!Takes[EditShot].bKeep && Headline == EditShot)
				{
					Headline = INDEX_NONE;
				}
			}
			TickPlay(Dt, true);
			if (bEnterPressed && StateTime > 0.2f)
			{
				if (ClipsKept() == 0)
				{
					Alert = TEXT("Every clip is cut. Keep at least one (S).");
					AlertTime = 3.f;
				}
				else
				{
					MakeUpload();
					SetState(EDCDocState::Upload);
				}
			}
			else if (bBackPressed)
			{
				SetState(EDCDocState::Shooting);
			}
		}
		else if (State == EDCDocState::Upload)
		{
			bHasPlay = false;
			if ((bEnterPressed || bBackPressed) && StateTime > 1.f)
			{
				SetState(EDCDocState::Shooting);
			}
		}
		else if (State == EDCDocState::Shop)
		{
			bHasPlay = false;
			ShopNoteTime = FMath::Max(0.f, ShopNoteTime - Dt);
			if (NavY != 0)
			{
				ShopSel = ((ShopSel + NavY) % DCGear::Num + DCGear::Num) % DCGear::Num;
			}
			if (bEnterPressed && StateTime > 0.2f)
			{
				Buy(ShopSel);
			}
			else if (bBackPressed)
			{
				SetState(EDCDocState::Shooting);
			}
		}
		else if (State == EDCDocState::Film)
		{
			// the Director's Cut: title, every proof in the order found, the closing card
			FilmTime += Dt;
			const int32 NumClips = Takes.Num();
			const float PartLen = FilmPart == 0 ? 5.f : FilmPart <= NumClips ? Takes[FilmPart - 1].Len + 0.5f : 7.f;
			if (FilmPart >= 1 && FilmPart <= NumClips)
			{
				TickPlay(Dt, false);
			}
			if (FilmTime >= PartLen || (bEnterPressed && StateTime > 1.f))
			{
				FilmTime = 0.f;
				++FilmPart;
				if (FilmPart <= NumClips)
				{
					StartPlayTake(FilmPart - 1);
					TickPlay(0.f, false);
				}
				else
				{
					bHasPlay = false;
					if (FilmPart > NumClips + 1)
					{
						SetState(EDCDocState::Rating);
					}
				}
			}
		}
		else if (State == EDCDocState::Rating)
		{
			bHasPlay = false;
			if (bEnterPressed && StateTime > 2.f)
			{
				// once more from the start of the film
				SetState(EDCDocState::Film);
			}
		}
		if (State != EDCDocState::Shooting)
		{
			TickCreatures(Dt, bHasPlay ? Play.Hero : W->Pos, false);
			if (bHasPlay)
			{
				Look(Play.Cam, Play.Rot, Play.Fov);
				Focus = Play.Focus;
			}
		}
	}
	bEnterPressed = bBackPressed = bUsePressed = false;
	NavX = NavY = 0;

	// the headlamp looks where the lens looks
	if (USpotLightComponent* L = W->Lamp.Get())
	{
		const bool bOn = State == EDCDocState::Shooting ? bLampOn : (bHasPlay && Play.bLamp);
		L->SetIntensity(bOn ? LampPower * (State == EDCDocState::Shooting ? 1.f - 0.6f * W->DryAmt : 1.f) : 0.f); // the base has its own lights: the headlamp only glares on its pale walls
		const FRotator R = State == EDCDocState::Shooting ? FRotator(W->Pitch, W->Yaw, 0.f) : Play.Rot;
		const FVector From = State == EDCDocState::Shooting ? W->Pos + FVector(0.0, 0.0, 40.0) : Play.Cam - FVector(0.0, 0.0, 20.0);
		L->SetWorldLocationAndRotation(From + R.Vector() * 62.0, R);
	}
}

// ---------------------------------------------------------------------------------------------
// test script
// ---------------------------------------------------------------------------------------------

int32 FDCDoc::RunLine(const TArray<FString>& T)
{
	const FString& Cmd = T[0];
	auto Num = [&T](int32 i) { return T.IsValidIndex(i) ? FCString::Atof(*T[i]) : 0.f; };
	auto Sub = [&T](int32 i)
	{
		static const TCHAR* Names[] = { TEXT("manta"), TEXT("shoal"), TEXT("angler"), TEXT("jelly"), TEXT("giant"), TEXT("crew") };
		for (int32 s = 0; s < DCSub::Num; ++s)
		{
			if (T.IsValidIndex(i) && T[i] == Names[s])
			{
				return s;
			}
		}
		return (int32)INDEX_NONE;
	};
	// the expedition, for test scripts
	if (Cmd == TEXT("use"))
	{
		UseSpot((int32)Num(1));
		return 1;
	}
	if (Cmd == TEXT("base"))
	{
		W->Pos = W->BaseWake;
		W->Vel = FVector::ZeroVector;
		return 1;
	}
	if (Cmd == TEXT("money"))
	{
		Money = (int32)Num(1);
		return 1;
	}
	if (Cmd == TEXT("buy"))
	{
		Buy((int32)Num(1));
		return 1;
	}
	if (Cmd == TEXT("o2"))
	{
		O2 = Num(1);
		return 1;
	}
	if (Cmd == TEXT("day"))
	{
		Day = (int32)Num(1);
		return 1;
	}
	if (Cmd == TEXT("subject"))
	{
		// subject NAME DIST: put the diver DIST from the nearest one of that species, looking at it, camera up
		const FString Want = FString(TEXT("creature:")) + (T.IsValidIndex(1) ? T[1] : FString());
		const AActor* Best1 = nullptr;
		FVector Home = FVector::ZeroVector;
		for (const FAmbient& One : Ambient)
		{
			const AActor* A = One.Actor.Get();
			if (A && Subjects.IsValidIndex(One.Subject) && Subjects[One.Subject].Id == Want && (!Best1 || FVector::Dist(One.Centre, W->Pos) < FVector::Dist(Home, W->Pos)))
			{
				Best1 = A;
				Home = One.Centre;
			}
		}
		if (Best1)
		{
			// where it lives: being there wakes it
			W->Pos = Home;
			TickCreatures(0.f, W->Pos, true);
		}
		if (Want == TEXT("creature:leviathan"))
		{
			Best1 = Hunter.Get();
		}
		if (Best1)
		{
			// go there once so it wakes and moves to where the clock says, then stand off from where it is now
			W->Pos = Best1->GetActorLocation();
			TickCreatures(0.f, W->Pos, true);
			TickHunter(0.f, true);
			const FVector At = Best1->GetActorLocation();
			W->Pos = At + FVector(-Num(2), 0.0, 60.0);
			W->Vel = FVector::ZeroVector;
			W->ScriptMoveLeft = 0.f;
			const FRotator Aim = (At - W->Pos).Rotation();
			W->Yaw = W->BodyYaw = Aim.Yaw;
			W->Pitch = Aim.Pitch;
			W->Pivot = W->Pos;
			TrackActor = const_cast<AActor*>(Best1);
			TrackSub = 0;
			TrackLeft = T.IsValidIndex(3) ? Num(3) : 5.f;
		}
		UE_LOG(LogMatchCut, Display, TEXT("DOC SUBJECT %s %s"), *Want, Best1 ? TEXT("found") : TEXT("missing"));
		return 1;
	}
	if (Cmd == TEXT("career"))
	{
		UE_LOG(LogMatchCut, Display, TEXT("DOC CAREER day %d/%d state %d base %d spot %d money %d views %lld subs %d air %.0f/%.0f depth %.0f hull %.0f tape %d/%d proofs %d/%d hunt %.2f ending %d gear %d%d%d%d%d%d"),
			Day, Days, (int32)State, bInBase ? 1 : 0, Spot, Money, TotalViews, Subscribers, O2, O2Max, W->ZoneDepth, HullLimit(), Takes.Num(), TapeCap(), EvidenceCount(), EvidenceTotal(), HuntAlert, Ending,
			Gear[0], Gear[1], Gear[2], Gear[3], Gear[4], Gear[5]);
		return 1;
	}
	if (Cmd == TEXT("aim"))
	{
		bAimToggle = Num(1) > 0.f;
		return 1;
	}
	if (Cmd == TEXT("zoom"))
	{
		Zoom = FMath::Clamp(Num(1), 10.f, 72.f);
		return 1;
	}
	if (Cmd == TEXT("rec"))
	{
		// rec: the script waits for the take. rec 0: it goes on at once (to move during the take)
		bRecordPressed = true;
		GM->ScriptWait = T.Num() > 1 ? Num(1) : DCD::TakeLen + 0.4f;
		return 1;
	}
	if (Cmd == TEXT("lamp"))
	{
		bLampOwned = true;
		bLampOn = Num(1) > 0.f;
		return 1;
	}
	if (Cmd == TEXT("clock"))
	{
		Clock = Num(1);
		return 1;
	}
	if (Cmd == TEXT("track"))
	{
		// track NAME SECONDS: the lens follows the subject; the script goes on at once
		TrackSub = Sub(1);
		TrackLeft = Num(2);
		return 1;
	}
	if (Cmd == TEXT("parade"))
	{
		// every creature model in rows high in open water, all at one length, noses along +x: seen from the +y side
		// (yaw -90) every nose must point to the right of the picture
		TArray<FString> Ids;
		for (const TPair<FString, TArray<FDCArtRef>>& Pair : GM->Art)
		{
			if (Pair.Key.StartsWith(TEXT("creature:")))
			{
				Ids.Add(Pair.Key);
			}
		}
		Ids.Sort();
		float Front = -90.f;
		FParse::Value(FCommandLine::Get(), TEXT("-MCFront="), Front);
		for (int32 i = 0; i < Ids.Num(); ++i)
		{
			const int32 Page = i / 8, Slot = i % 8;
			const FVector At(-6000.0 + 3000.0 * Page + 420.0 * (Slot % 4) - 630.0, 0.0, 3100.0 - 330.0 * (Slot / 4));
			if (AActor* A = SpawnCreature(*Ids[i], 260.f, Front, false, nullptr))
			{
				A->SetActorLocation(At);
			}
			UE_LOG(LogMatchCut, Display, TEXT("DOC PARADE page %d row %d column %d: %s"), Page, Slot / 4, Slot % 4, *Ids[i]);
		}
		return 1;
	}
	if (Cmd == TEXT("near"))
	{
		// near NAME DX DY DZ: put the diver that far from the subject, looking at it
		const int32 s = Sub(1);
		if (s != INDEX_NONE)
		{
			W->Pos = SubPos[s] + FVector(Num(2), Num(3), Num(4));
			W->ScriptMoveLeft = 0.f;
			W->Vel = FVector::ZeroVector;
			const FRotator Aim = (SubPos[s] - W->Pos).Rotation();
			W->Yaw = Aim.Yaw;
			W->Pitch = Aim.Pitch;
			W->BodyYaw = Aim.Yaw;
			W->Pivot = W->Pos + FVector(0.0, 0.0, 45.0);
			W->BoomFrac = 1.f;
		}
		return 1;
	}
	if (Cmd == TEXT("enter"))
	{
		bEnterPressed = true;
		return 1;
	}
	if (Cmd == TEXT("back"))
	{
		bBackPressed = true;
		return 1;
	}
	if (Cmd == TEXT("nav"))
	{
		NavX = (int32)Num(1);
		NavY = (int32)Num(2);
		return 1;
	}
	if (Cmd == TEXT("stat4"))
	{
		FString S;
		for (int32 s = 0; s < DCSub::Num; ++s)
		{
			S += FString::Printf(TEXT(" %d:%d(%d)"), s, Best[s], TakesOf(s));
		}
		UE_LOG(LogMatchCut, Display, TEXT("DOC CAM hero=(%.0f,%.0f,%.0f) cam=(%.0f,%.0f,%.0f) rot=(%.0f,%.0f) fov=%.0f track=%d"), W->Pos.X, W->Pos.Y, W->Pos.Z, CamP.X, CamP.Y, CamP.Z, CamR.Pitch, CamR.Yaw, CamFov, TrackSub);
		UE_LOG(LogMatchCut, Display, TEXT("DOC STAT state=%d clock=%.1f shots=%d stars=%d lamp=%d/%d lens=%d aim=%d zoom=%.0f lock=%d split=%.2f wave=%.2f best(takes):%s"), (int32)State, Clock,
			ShotsDone(), StarsTotal(), bLampOwned ? 1 : 0, bLampOn ? 1 : 0, bLens ? 1 : 0, bAim ? 1 : 0, Zoom, Lock, Split, Wave, *S);
		for (int32 s = 0; s < DCSub::Num; ++s)
		{
			UE_LOG(LogMatchCut, Display, TEXT("DOC SEEN %d in=%d whole=%d action=%d at=(%.2f,%.2f) size=%.3f dist=%.0f pos=(%.0f,%.0f,%.0f)"), s, Seen[s].bIn ? 1 : 0, Seen[s].bWhole ? 1 : 0,
				Seen[s].bAction ? 1 : 0, Seen[s].At.X, Seen[s].At.Y, Seen[s].Size, Seen[s].Dist, SubPos[s].X, SubPos[s].Y, SubPos[s].Z);
		}
		return 1;
	}
	return 0;
}
