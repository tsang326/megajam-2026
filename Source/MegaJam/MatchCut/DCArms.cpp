// Deep Cut: first person. The camera is the diver's eyes; the engine's mannequin (Epic's template content) gives the two
// arms, lying prone behind the eyes as a swimmer does. No animation asset: each arm is solved every frame with two-bone IK
// toward a hand target that the swim stroke, the camera grip, a reach or a shield puts in front of the eyes.

#include "DC3D.h"
#include "DCDoc.h"
#include "MatchCutGameMode.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"

namespace DCArmsK
{
	// where the eyes sit in the mannequin (component space, cm): just in front of the head bone
	const FVector EyeInMesh(0.0, 2.0, 151.0);
}

void FDC3D::SpawnArms()
{
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (!Mesh)
	{
		UE_LOG(LogMatchCut, Warning, TEXT("3D arms: no mannequin mesh, first person without arms"));
		return;
	}
	AActor* Owner = GM->SpawnGroup(Pos);
	UPoseableMeshComponent* PM = NewObject<UPoseableMeshComponent>(Owner);
	PM->SetupAttachment(Owner->GetRootComponent());
	PM->SetSkinnedAssetAndUpdate(Mesh);
	PM->SetCastShadow(false);
	PM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PM->bUseBoundsFromLeaderPoseComponent = false;
	PM->SetBoundsScale(4.f); // the bones move far from the reference pose: never culled
	PM->RegisterComponent();
	// a dark wet-suit and its gloves
	for (int32 i = 0; i < PM->GetNumMaterials(); ++i)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(GM->LitMaterial, PM);
		M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.055f, 0.065f, 0.08f));
		PM->SetMaterial(i, M);
	}
	// nothing of the body but the arms: the head would fill the eyes, the legs trail out of sight anyway
	for (const TCHAR* Bone : { TEXT("neck_01"), TEXT("thigh_l"), TEXT("thigh_r") })
	{
		PM->HideBoneByName(FName(Bone), PBO_None);
	}
	auto Ref = [PM](const TCHAR* Side)
	{
		FArmRef A;
		A.Upper = FName(*FString::Printf(TEXT("upperarm_%s"), Side));
		A.Lower = FName(*FString::Printf(TEXT("lowerarm_%s"), Side));
		A.Hand = FName(*FString::Printf(TEXT("hand_%s"), Side));
		A.U = PM->GetBoneTransformByName(A.Upper, EBoneSpaces::ComponentSpace);
		A.L = PM->GetBoneTransformByName(A.Lower, EBoneSpaces::ComponentSpace);
		A.H = PM->GetBoneTransformByName(A.Hand, EBoneSpaces::ComponentSpace);
		return A;
	};
	ArmL = Ref(TEXT("l"));
	ArmR = Ref(TEXT("r"));
	// the camera in the right hand: a box for now, the camcorder model when the art drop has it
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	C->SetupAttachment(Owner->GetRootComponent());
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetStaticMesh(GM->CubeMesh);
	UMaterialInstanceDynamic* CM = UMaterialInstanceDynamic::Create(GM->LitMaterial, C);
	CM->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.32f, 0.05f));
	C->SetMaterial(0, CM);
	C->RegisterComponent();
	C->SetRelativeScale3D(FVector(0.17, 0.08, 0.10));
	HandCam = C;
	UStaticMeshComponent* L = NewObject<UStaticMeshComponent>(Owner);
	L->SetupAttachment(C);
	L->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	L->SetCastShadow(false);
	L->SetStaticMesh(GM->SphereMesh);
	UMaterialInstanceDynamic* LM = UMaterialInstanceDynamic::Create(GM->UnlitMaterial ? GM->UnlitMaterial.Get() : GM->LitMaterial.Get(), L);
	LM->SetVectorParameterValue(TEXT("Color"), FLinearColor(6.f, 0.2f, 0.1f));
	L->SetMaterial(0, LM);
	L->RegisterComponent();
	L->SetRelativeLocation(FVector(30.0, 0.0, 55.0));
	L->SetRelativeScale3D(FVector(0.1, 0.2, 0.15));
	HandCamRec = L;
	Arms = PM;
	UE_LOG(LogMatchCut, Display, TEXT("3D arms: shoulder L (%.0f,%.0f,%.0f) R (%.0f,%.0f,%.0f), hand L (%.0f,%.0f,%.0f)"), ArmL.U.GetLocation().X, ArmL.U.GetLocation().Y, ArmL.U.GetLocation().Z,
		ArmR.U.GetLocation().X, ArmR.U.GetLocation().Y, ArmR.U.GetLocation().Z, ArmL.H.GetLocation().X, ArmL.H.GetLocation().Y, ArmL.H.GetLocation().Z);
}

void FDC3D::SolveArm(const FArmRef& A, const FVector& Target, const FVector& Pole, const FQuat& HandTurn)
{
	UPoseableMeshComponent* PM = Arms.Get();
	if (!PM)
	{
		return;
	}
	const FTransform& CX = PM->GetComponentTransform();
	const FVector S = CX.TransformPosition(A.U.GetLocation());
	const FVector E0 = CX.TransformPosition(A.L.GetLocation());
	const FVector H0 = CX.TransformPosition(A.H.GetLocation());
	const double L1 = FVector::Dist(S, E0), L2 = FVector::Dist(E0, H0);
	FVector D = Target - S;
	const double Len = FMath::Clamp(D.Size(), FMath::Abs(L1 - L2) + 1.0, L1 + L2 - 0.5);
	const FVector Dir = D.GetSafeNormal();
	FVector Side = (Pole - S) - Dir * FVector::DotProduct(Pole - S, Dir);
	Side = Side.GetSafeNormal();
	const double Along = (L1 * L1 - L2 * L2 + Len * Len) / (2.0 * Len);
	const double Out = FMath::Sqrt(FMath::Max(0.0, L1 * L1 - Along * Along));
	const FVector E = S + Dir * Along + Side * Out;
	const FVector H = S + Dir * Len;
	const FQuat Q = CX.GetRotation();
	const FQuat DU = FQuat::FindBetweenNormals((E0 - S).GetSafeNormal(), (E - S).GetSafeNormal());
	const FQuat DL = FQuat::FindBetweenNormals(DU.RotateVector((H0 - E0).GetSafeNormal()), (H - E).GetSafeNormal()) * DU;
	PM->SetBoneRotationByName(A.Upper, (DU * Q * A.U.GetRotation()).Rotator(), EBoneSpaces::WorldSpace);
	PM->SetBoneRotationByName(A.Lower, (DL * Q * A.L.GetRotation()).Rotator(), EBoneSpaces::WorldSpace);
	PM->SetBoneRotationByName(A.Hand, (HandTurn * DL * Q * A.H.GetRotation()).Rotator(), EBoneSpaces::WorldSpace);
}

void FDC3D::TickArms(float Dt, const FVector& Eye, const FRotator& View)
{
	UPoseableMeshComponent* PM = Arms.Get();
	if (!PM)
	{
		return;
	}
	const FRotationMatrix M(FRotator(View.Pitch, View.Yaw, 0.f));
	const FVector F = M.GetUnitAxis(EAxis::X), R = M.GetUnitAxis(EAxis::Y), U = M.GetUnitAxis(EAxis::Z);
	// the body lies behind the eyes: the mannequin's up along the view, its chest facing down
	const FQuat Body = FRotationMatrix::MakeFromZY(F, -U).ToQuat();
	PM->GetOwner()->SetActorLocationAndRotation(Eye - Body.RotateVector(DCArmsK::EyeInMesh), Body);

	const float Aim = Doc ? Doc->AimBlend : 0.f;
	const float Speed = (float)Vel.Size();
	const float WantStroke = FMath::Clamp(Speed / 560.f, 0.f, 1.3f) * (1.f - Aim) * (1.f - DryAmt); // walking in the base: no strokes
	StrokeAmt = FMath::FInterpTo(StrokeAmt, WantStroke, Dt, 4.f);
	StrokePhase = FMath::Frac(StrokePhase + Dt * (0.25f + 0.55f * StrokeAmt));
	HeadBob = 1.6f * StrokeAmt * FMath::Sin(StrokePhase * 4.f * PI);
	ShieldAmt = FMath::Max(0.f, ShieldAmt - Dt * 1.5f);
	const float Th = StrokePhase * 2.f * PI;
	const float Idle = 0.6f * FMath::Sin(Time * 1.3f);

	// left hand: a breaststroke sweep (reach forward, sweep out and back, come in under the chest), or resting low
	const FVector LRest = F * 40.0 + R * -15.0 + U * (-13.0 + Idle);
	const FVector LStroke = F * (40.0 + 12.0 * FMath::Cos(Th)) + R * (-10.0 - 18.0 * FMath::Max(0.f, FMath::Sin(Th))) + U * (-12.0 + 6.0 * FMath::Sin(Th));
	const FVector LReach = F * 54.0 + R * -5.0 + U * -4.0;
	const FVector LShield = F * 32.0 + R * -8.0 + U * 2.0;
	FVector LT = FMath::Lerp(LRest, LStroke, (double)FMath::Min(1.f, StrokeAmt));
	LT = FMath::Lerp(LT, LReach, (double)ReachAmt);
	LT = FMath::Lerp(LT, LShield, (double)ShieldAmt);
	// right hand: holds the camera low and to the right; raises it to the eye to film
	const FVector RRest = F * 40.0 + R * 15.0 + U * (-13.0 + Idle * 0.7f) + F * 3.0 * StrokeAmt * FMath::Sin(Th + PI);
	const FVector RAim = F * 32.0 + R * 5.0 + U * -6.0;
	const FVector RShield = F * 32.0 + R * 8.0 + U * 2.0;
	FVector RT = FMath::Lerp(RRest, RAim, (double)Aim);
	RT = FMath::Lerp(RT, RShield, (double)ShieldAmt);

	const FVector Base = Eye;
	SolveArm(ArmL, Base + LT, Base + R * -70.0 + U * -45.0 - F * 10.0, FQuat::Identity);
	SolveArm(ArmR, Base + RT, Base + R * 70.0 + U * -45.0 - F * 10.0, FQuat::Identity);

	static float LogT = 0.f;
	LogT += Dt;
	if (LogT > 2.f)
	{
		LogT = 0.f;
		const FVector HL = PM->GetBoneLocationByName(ArmL.Hand, EBoneSpaces::WorldSpace), HR = PM->GetBoneLocationByName(ArmR.Hand, EBoneSpaces::WorldSpace);
		const FVector SL = PM->GetBoneLocationByName(ArmL.Upper, EBoneSpaces::WorldSpace);
		const FVector HeadW = PM->GetBoneLocationByName(FName(TEXT("head")), EBoneSpaces::WorldSpace);
		auto Loc = [&](const FVector& P) { const FVector V = P - Eye; return FVector(FVector::DotProduct(V, F), FVector::DotProduct(V, R), FVector::DotProduct(V, U)); };
		const FVector a = Loc(HL), b = Loc(HR), c = Loc(SL), d = Loc(Base + LT), e = Loc(HeadW);
		UE_LOG(LogMatchCut, Display, TEXT("3D ARMS (eye frame f,r,u) handL (%.0f,%.0f,%.0f) wantL (%.0f,%.0f,%.0f) handR (%.0f,%.0f,%.0f) shoulderL (%.0f,%.0f,%.0f) head (%.0f,%.0f,%.0f) vis=%d"),
			a.X, a.Y, a.Z, d.X, d.Y, d.Z, b.X, b.Y, b.Z, c.X, c.Y, c.Z, e.X, e.Y, e.Z, PM->IsVisible() ? 1 : 0);
	}
	if (UStaticMeshComponent* C = HandCam.Get())
	{
		// the camera sits in the right palm, its lens along the view
		const FVector HR = PM->GetBoneLocationByName(ArmR.Hand, EBoneSpaces::WorldSpace);
		C->SetWorldLocationAndRotation(HR + F * 7.0 + U * 3.0 - R * 2.0, FRotator(View.Pitch, View.Yaw, 0.f));
	}
	if (UStaticMeshComponent* L = HandCamRec.Get())
	{
		const bool bRec = Doc && Doc->bRecording;
		L->SetVisibility(bRec && FMath::Frac(Time * 1.2f) < 0.6f);
	}
}
