// Deep Cut (brief 05, part C4): the water itself. Engine shapes only, no image or model files.
//   - marine snow that sinks and bubbles that rise, in front of whatever camera is live
//   - the slats that cut the key light into shafts sway, so the light on the floor moves like slow caustics

#include "MatchCutGameMode.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const double Period = 1200.0; // cm: the fleck pattern repeats in height, so it can scroll for ever without a jump

	UInstancedStaticMeshComponent* Flecks(AActor* Owner, UStaticMesh* Mesh, UMaterialInstanceDynamic* MID, int32 Count, float MinSize, float MaxSize, int32 Seed, double Dir, double TanH)
	{
		UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(Owner);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetupAttachment(Owner->GetRootComponent());
		C->RegisterComponent();
		C->SetMaterial(0, MID);
		FRandomStream Rand(Seed);
		for (int32 i = 0; i < Count; ++i)
		{
			const double X = Rand.FRandRange(380.f, 2700.f);
			const double Y = Rand.FRandRange(-1.15f, 1.15f) * TanH * X;
			const double Z = Rand.FRandRange(-0.5f, 0.5f) * Period;
			const FVector Scale = FVector(Rand.FRandRange(MinSize, MaxSize)) * FVector(1.0, Rand.FRandRange(0.6f, 1.4f), Rand.FRandRange(0.6f, 1.4f)) / 100.0;
			const FRotator Rot(Rand.FRandRange(0.f, 90.f), Rand.FRandRange(0.f, 90.f), 0.f);
			C->AddInstance(FTransform(Rot, FVector(X, Y, Z), Scale));
			C->AddInstance(FTransform(Rot, FVector(X, Y, Z + Dir * Period), Scale));
		}
		return C;
	}
}

void AMatchCutGameMode::SpawnWater()
{
	SnowMID = UMaterialInstanceDynamic::Create(UnlitMaterial, this);
	BubbleMID = UMaterialInstanceDynamic::Create(UnlitMaterial, this);
	AActor* Snow = SpawnGroup(FVector::ZeroVector);
	Flecks(Snow, CubeMesh, SnowMID, 330, 2.4f, 6.0f, 4711, 1.0, TanH);
	SnowActor = Snow;
	AActor* Bub = SpawnGroup(FVector::ZeroVector);
	Flecks(Bub, SphereMesh, BubbleMID, 46, 3.5f, 9.f, 1213, -1.0, TanH);
	BubbleActor = Bub;
	GhostHidden.Add(Snow);
	GhostHidden.Add(Bub);
	SlatBase.Reset();
	for (const TWeakObjectPtr<AActor>& Slat : ShaftSlats)
	{
		SlatBase.Add(Slat.IsValid() ? Slat->GetActorLocation() : FVector::ZeroVector);
	}
	UE_LOG(LogMatchCut, Display, TEXT("WATER snow=660 flecks bubbles=92 slats=%d"), SlatBase.Num());
}

void AMatchCutGameMode::TickWater(float Dt)
{
	WaterTime += Dt;
	if (!Shots.IsValidIndex(Cur))
	{
		return;
	}
	const FDCShot& Shot = Shots[Cur];
	const FVector Base = Shot.CamPos + Shot.AxR * Shot.Truck;
	const float Dim = (Shot.bDark ? 0.10f : 1.f) * Look.Snow;
	if (AActor* A = SnowActor.Get())
	{
		A->SetActorLocationAndRotation(Base - Shot.AxU * FMath::Fmod(WaterTime * 13.0, Period) + Shot.AxR * (30.0 * FMath::Sin(WaterTime * 0.21)), Shot.CamRot);
		A->SetActorHiddenInGame(Look.Snow <= 0.f);
		SnowMID->SetVectorParameterValue(TEXT("Color"), Look.SnowColor * Dim);
	}
	if (AActor* A = BubbleActor.Get())
	{
		A->SetActorLocationAndRotation(Base + Shot.AxU * FMath::Fmod(WaterTime * 85.0, Period) + Shot.AxR * (14.0 * FMath::Sin(WaterTime * 1.7)), Shot.CamRot);
		A->SetActorHiddenInGame(Look.Snow <= 0.f);
		BubbleMID->SetVectorParameterValue(TEXT("Color"), Look.SnowColor * (Dim * 1.6f));
	}
	if (Look.bShafts)
	{
		for (int32 i = 0; i < ShaftSlats.Num() && i < SlatBase.Num(); ++i)
		{
			if (AActor* A = ShaftSlats[i].Get())
			{
				A->SetActorLocation(SlatBase[i] + A->GetActorRightVector() * (75.0 * FMath::Sin(WaterTime * 0.23 + i * 1.7) + 30.0 * FMath::Sin(WaterTime * 0.61 + i * 0.9)));
			}
		}
	}
}
