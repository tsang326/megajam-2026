// Deep Cut: first person. The camera is the diver's eyes; the engine's mannequin (Epic's template content) gives the two
// arms, lying prone behind the eyes as a swimmer does. No animation asset: each arm is solved every frame with two-bone IK
// toward a hand target that the swim stroke, the camera grip, a reach or a shield puts in front of the eyes.

#include "DC3D.h"
#include "DCDoc.h"
#include "MatchCutGameMode.h"
#include "DCAudio.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"

namespace DCArmsK
{
	// where the eyes sit in the mannequin (component space, cm). The body lies along the view, so this puts the shoulders
	// a hand's breadth behind the eyes and well below them: the arms come into the picture from its lower corners, as a
	// diver's own arms do, and never pass close in front of the lens.
	FVector EyeInMesh(0.0, -20.0, 152.0);

	// the hand's own frame in the reference pose, from where its knuckles are: x along the fingers, y toward the thumb's
	// side, z = x cross y (the palm of a right hand, the back of a left one)
	FQuat HandFrame(UPoseableMeshComponent* PM, const TCHAR* Side)
	{
		auto At = [PM, Side](const TCHAR* Bone) { return PM->GetBoneTransformByName(FName(*FString::Printf(TEXT("%s_%s"), Bone, Side)), EBoneSpaces::ComponentSpace).GetLocation(); };
		if (PM->GetBoneIndex(FName(*FString::Printf(TEXT("middle_01_%s"), Side))) == INDEX_NONE || PM->GetBoneIndex(FName(*FString::Printf(TEXT("index_01_%s"), Side))) == INDEX_NONE
			|| PM->GetBoneIndex(FName(*FString::Printf(TEXT("pinky_01_%s"), Side))) == INDEX_NONE)
		{
			return FQuat::Identity;
		}
		const FVector Dir = (At(TEXT("middle_01")) - At(TEXT("hand"))).GetSafeNormal();
		FVector Across = At(TEXT("index_01")) - At(TEXT("pinky_01"));
		Across = (Across - Dir * FVector::DotProduct(Across, Dir)).GetSafeNormal();
		return FMatrix(Dir, Across, FVector::CrossProduct(Dir, Across), FVector::ZeroVector).ToQuat();
	}

	// closes the fingers of one hand toward its palm, each joint by its own angle. Set once: the fingers then go with the hand.
	void CurlFingers(UPoseableMeshComponent* PM, const TCHAR* Side, const FQuat& Frame, bool bRight, const float Deg[3], float ThumbDeg)
	{
		// toward the palm: about the hand's y, one way for the right hand and the other for the left
		const FVector Axis = Frame.GetAxisY() * (bRight ? -1.0 : 1.0);
		static const TCHAR* Fingers[] = { TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky"), TEXT("thumb") };
		struct FJoint { FName Bone; FVector Local; float Deg; };
		TArray<FJoint> Joints;
		for (int32 f = 0; f < 5; ++f)
		{
			for (int32 j = 0; j < 3; ++j)
			{
				const FName Bone(*FString::Printf(TEXT("%s_%02d_%s"), Fingers[f], j + 1, Side));
				if (PM->GetBoneIndex(Bone) != INDEX_NONE)
				{
					// the thumb closes across the palm: about the fingers' own direction
					const FVector A = f == 4 ? Frame.GetAxisX() * (bRight ? 1.0 : -1.0) : Axis;
					Joints.Add({ Bone, PM->GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace).GetRotation().UnrotateVector(A), f == 4 ? (j == 0 ? ThumbDeg : ThumbDeg * 0.4f) : Deg[j] * (1.f + 0.08f * f) });
				}
			}
		}
		for (const FJoint& J : Joints)
		{
			const FQuat Now = PM->GetBoneTransformByName(J.Bone, EBoneSpaces::ComponentSpace).GetRotation();
			PM->SetBoneRotationByName(J.Bone, (FQuat(Now.RotateVector(J.Local), FMath::DegreesToRadians(J.Deg)) * Now).Rotator(), EBoneSpaces::ComponentSpace);
		}
	}
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
	FParse::Value(FCommandLine::Get(), TEXT("-MCEyeY="), DCArmsK::EyeInMesh.Y);
	FParse::Value(FCommandLine::Get(), TEXT("-MCEyeZ="), DCArmsK::EyeInMesh.Z);
	// the hands: the right one closed round the camera's grip, the left one loosely cupped (a swimmer's hand)
	HandFrameL = DCArmsK::HandFrame(PM, TEXT("l"));
	HandFrameR = DCArmsK::HandFrame(PM, TEXT("r"));
	{
		float Grip = 1.f, Thumb = 25.f;
		FParse::Value(FCommandLine::Get(), TEXT("-MCCurl="), Grip);
		FParse::Value(FCommandLine::Get(), TEXT("-MCThumb="), Thumb);
		const float Closed[3] = { 55.f * Grip, 65.f * Grip, 40.f * Grip }, Cupped[3] = { 12.f, 16.f, 10.f };
		DCArmsK::CurlFingers(PM, TEXT("r"), HandFrameR, true, Closed, Thumb);
		DCArmsK::CurlFingers(PM, TEXT("l"), HandFrameL, false, Cupped, 8.f);
	}
	// the camera in the right hand: the camcorder model (scene:camcorder), on a holder of its own so the model can be
	// turned and centred inside it; a box when the art is missing
	USceneComponent* Holder = NewObject<USceneComponent>(Owner);
	Holder->SetupAttachment(Owner->GetRootComponent());
	Holder->RegisterComponent();
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	C->SetupAttachment(Holder);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	UStaticMesh* CamMesh = nullptr;
	float CamYaw = -90.f; // the model's lens looks along its +Y
	if (const TArray<FDCArtRef>* Set = GM->Art.Find(TEXT("scene:camcorder")))
	{
		if (Set->Num() > 0 && GM->ArtMeshes.IsValidIndex((*Set)[0].Mesh))
		{
			CamMesh = GM->ArtMeshes[(*Set)[0].Mesh];
		}
	}
	FParse::Value(FCommandLine::Get(), TEXT("-MCCamYaw="), CamYaw);
	if (CamMesh)
	{
		C->SetStaticMesh(CamMesh);
		const FBox B = CamMesh->GetBoundingBox();
		const double S = 26.0 / FMath::Max(B.GetSize().GetMax(), 1.0); // about 26 cm long
		const FQuat Q(FRotator(0.f, CamYaw, 0.f));
		C->SetRelativeScale3D(FVector(S));
		C->SetRelativeRotation(Q);
		C->SetRelativeLocation(-Q.RotateVector(B.GetCenter() * S));
		CamHalf = FBox(B.Min * S, B.Max * S).TransformBy(FTransform(Q)).GetExtent(); // in the holder: x along the lens, y to the right, z up
	}
	else
	{
		C->SetStaticMesh(GM->CubeMesh);
		UMaterialInstanceDynamic* CM = UMaterialInstanceDynamic::Create(GM->LitMaterial, C);
		CM->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.32f, 0.05f));
		C->SetMaterial(0, CM);
		C->SetRelativeScale3D(FVector(0.17, 0.08, 0.10));
		CamHalf = FVector(8.5, 4.0, 5.0);
	}
	C->RegisterComponent();
	HandCam = C;
	HandCamHolder = Holder;
	UStaticMeshComponent* L = NewObject<UStaticMeshComponent>(Owner);
	L->SetupAttachment(Holder);
	L->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	L->SetCastShadow(false);
	L->SetStaticMesh(GM->SphereMesh);
	UMaterialInstanceDynamic* LM = UMaterialInstanceDynamic::Create(GM->UnlitMaterial ? GM->UnlitMaterial.Get() : GM->LitMaterial.Get(), L);
	LM->SetVectorParameterValue(TEXT("Color"), FLinearColor(6.f, 0.2f, 0.1f));
	L->SetMaterial(0, LM);
	L->RegisterComponent();
	L->SetRelativeLocation(FVector(4.0, 3.0, 9.0)); // on top, near the front, toward the eye
	L->SetRelativeScale3D(FVector(0.012));
	HandCamRec = L;
	Arms = PM;
	UE_LOG(LogMatchCut, Display, TEXT("3D arms: shoulder L (%.0f,%.0f,%.0f) R (%.0f,%.0f,%.0f), hand L (%.0f,%.0f,%.0f), camera half (%.1f,%.1f,%.1f), fingers %d"), ArmL.U.GetLocation().X, ArmL.U.GetLocation().Y, ArmL.U.GetLocation().Z,
		ArmR.U.GetLocation().X, ArmR.U.GetLocation().Y, ArmR.U.GetLocation().Z, ArmL.H.GetLocation().X, ArmL.H.GetLocation().Y, ArmL.H.GetLocation().Z, CamHalf.X, CamHalf.Y, CamHalf.Z,
		HandFrameR.Equals(FQuat::Identity) ? 0 : 1);
}

FVector FDC3D::SolveArm(const FArmRef& A, const FVector& Target, const FVector& Pole, const FQuat& HandTurn)
{
	UPoseableMeshComponent* PM = Arms.Get();
	if (!PM)
	{
		return FVector::ForwardVector;
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
	return (H - E).GetSafeNormal();
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
	{
		// one stroke sound per pull of the arm
		const float Was = StrokeSoundPhase;
		StrokeSoundPhase = StrokePhase;
		if (StrokeAmt > 0.25f && Was < 0.15f && StrokePhase >= 0.15f && GM->Audio)
		{
			GM->Audio->Play(TEXT("swim_stroke"), FMath::Lerp(0.6f, 1.f, FMath::Min(StrokeAmt, 1.f)), 1.f, 0.08f);
		}
	}
	ShieldAmt = FMath::Max(0.f, ShieldAmt - Dt * 1.5f);
	const float Th = StrokePhase * 2.f * PI;
	const float Idle = 0.4f * FMath::Sin(Time * 1.3f);
	const double Cs = FMath::Cos(Th), Sn = FMath::Sin(Th);

	// ---- the camera, low and to the right of the picture; raising it (aim) brings it up to the eye. It trails a turn of
	// the head a little and breathes; it does not swing with the steps or the strokes.
	static FRotator Lag = FRotator::ZeroRotator;
	static FRotator LastView = View;
	const FRotator DV = (View - LastView).GetNormalized();
	LastView = View;
	Lag = FMath::RInterpTo(Lag + FRotator(-DV.Pitch * 0.25f, -DV.Yaw * 0.25f, 0.f), FRotator::ZeroRotator, Dt, 10.f);
	Lag.Pitch = FMath::Clamp(Lag.Pitch, -3.f, 3.f);
	Lag.Yaw = FMath::Clamp(Lag.Yaw, -3.f, 3.f);
	static FVector LowK(38.0, 14.0, -13.5), GripK(-2.0, 1.0, -9.0);
	static float GripPitch = 58.f, GripIn = 12.f;
	static bool bTuned = false;
	if (!bTuned)
	{
		bTuned = true;
		FParse::Value(FCommandLine::Get(), TEXT("-MCLowF="), LowK.X);
		FParse::Value(FCommandLine::Get(), TEXT("-MCLowR="), LowK.Y);
		FParse::Value(FCommandLine::Get(), TEXT("-MCLowU="), LowK.Z);
		FParse::Value(FCommandLine::Get(), TEXT("-MCGripX="), GripK.X);
		FParse::Value(FCommandLine::Get(), TEXT("-MCGripY="), GripK.Y);
		FParse::Value(FCommandLine::Get(), TEXT("-MCGripZ="), GripK.Z);
		FParse::Value(FCommandLine::Get(), TEXT("-MCGripPitch="), GripPitch);
		FParse::Value(FCommandLine::Get(), TEXT("-MCGripIn="), GripIn);
	}
	const FVector Low = F * LowK.X + R * LowK.Y + U * (LowK.Z + Idle);
	const FVector Up = F * 30.0 + R * 6.0 + U * -7.0;
	const FVector HoldAt = Eye + FMath::Lerp(Low, Up, (double)Aim);
	const FQuat HoldQ(FRotator(View.Pitch + Lag.Pitch - 4.f * (1.f - Aim), View.Yaw + Lag.Yaw - 6.f * (1.f - Aim), 0.f));
	if (USceneComponent* Hold = HandCamHolder.Get())
	{
		Hold->SetWorldLocationAndRotation(HoldAt, HoldQ);
	}

	// ---- left arm: one-armed breaststroke (reach forward, sweep out and back, come in under the chest, slide forward
	// again), or resting below the picture
	const FVector LRest = F * 30.0 + R * -24.0 + U * (-44.0 + Idle);
	const FVector LStroke = F * (31.0 + 16.0 * Cs) + R * (-17.0 - 13.0 * Sn) + U * (-22.0 + 7.0 * Cs - 3.0 * Sn);
	const FVector LReach = F * 54.0 + R * -5.0 + U * -8.0;
	const FVector LShield = F * 34.0 + R * -9.0 + U * -3.0;
	FVector LT = FMath::Lerp(LRest, LStroke, (double)FMath::Min(1.f, StrokeAmt));
	LT = FMath::Lerp(LT, LReach, (double)ReachAmt);
	LT = FMath::Lerp(LT, LShield, (double)ShieldAmt);
	// ---- right arm: the hand is on the camera's grip, on its right side, the fingers over its top; in the base the camera
	// is put away and the hand hangs below the picture
	const FVector Wrist = HoldAt + HoldQ.RotateVector(FVector(GripK.X, CamHalf.Y + GripK.Y, CamHalf.Z + GripK.Z));
	const FVector RDry = Eye + F * 28.0 + R * 24.0 + U * (-46.0 + Idle);
	const FVector RShield = Eye + F * 34.0 + R * 9.0 + U * -3.0;
	FVector RT = FMath::Lerp(Wrist, RDry, (double)DryAmt);
	RT = FMath::Lerp(RT, RShield, (double)ShieldAmt * (double)DryAmt);

	const FVector Base = Eye;
	const FVector ForeL = SolveArm(ArmL, Base + LT, Base + R * -60.0 + U * -70.0 - F * 10.0, FQuat::Identity);
	SolveArm(ArmR, RT, Base + R * 55.0 + U * -75.0 - F * 5.0, FQuat::Identity);
	if (!HandFrameR.Equals(FQuat::Identity))
	{
		// a hand is turned by what it does, not by where the elbow happens to be
		auto Turn = [PM](const FArmRef& A, const FQuat& Frame, const FVector& Fingers, const FVector& ThumbSide)
		{
			const FVector X = Fingers.GetSafeNormal();
			const FVector Y = (ThumbSide - X * FVector::DotProduct(ThumbSide, X)).GetSafeNormal();
			const FQuat Want = FMatrix(X, Y, FVector::CrossProduct(X, Y), FVector::ZeroVector).ToQuat();
			PM->SetBoneRotationByName(A.Hand, (Want * Frame.Inverse() * A.H.GetRotation()).Rotator(), EBoneSpaces::WorldSpace);
		};
		// left: flat, the palm down and a little outward (it pushes water), the fingers along the forearm
		Turn(ArmL, HandFrameL, ForeL + F * 0.2 + U * 0.3, R - U * 0.25);
		if (DryAmt < 0.5f)
		{
			// right: the palm against the camera's side (the palm is x cross y: it looks left), the fingers up over its top
			// and a little forward, the thumb behind
			const float P = FMath::DegreesToRadians(GripPitch), In = FMath::DegreesToRadians(GripIn);
			const FVector Fingers = HoldQ.RotateVector(FVector(FMath::Cos(P), -FMath::Sin(P) * FMath::Sin(In), FMath::Sin(P) * FMath::Cos(In)));
			const FVector Palm = HoldQ.RotateVector(FVector(0.0, -FMath::Cos(In), -FMath::Sin(In)));
			Turn(ArmR, HandFrameR, Fingers, FVector::CrossProduct(Palm, Fingers));
		}
	}

	static float LogT = 0.f;
	LogT += Dt;
	if (LogT > 2.f)
	{
		LogT = 0.f;
		const FVector HL = PM->GetBoneLocationByName(ArmL.Hand, EBoneSpaces::WorldSpace), HR = PM->GetBoneLocationByName(ArmR.Hand, EBoneSpaces::WorldSpace);
		const FVector SL = PM->GetBoneLocationByName(ArmL.Upper, EBoneSpaces::WorldSpace);
		const FVector EL = PM->GetBoneLocationByName(ArmL.Lower, EBoneSpaces::WorldSpace), ER = PM->GetBoneLocationByName(ArmR.Lower, EBoneSpaces::WorldSpace);
		auto Loc = [&](const FVector& P) { const FVector V = P - Eye; return FVector(FVector::DotProduct(V, F), FVector::DotProduct(V, R), FVector::DotProduct(V, U)); };
		const FVector a = Loc(HL), b = Loc(HR), c = Loc(SL), d = Loc(Base + LT), e = Loc(EL), g = Loc(ER), h = Loc(HoldAt);
		UE_LOG(LogMatchCut, Display, TEXT("3D ARMS (eye frame f,r,u) handL (%.0f,%.0f,%.0f) wantL (%.0f,%.0f,%.0f) elbowL (%.0f,%.0f,%.0f) handR (%.0f,%.0f,%.0f) elbowR (%.0f,%.0f,%.0f) camera (%.0f,%.0f,%.0f) shoulderL (%.0f,%.0f,%.0f) vis=%d"),
			a.X, a.Y, a.Z, d.X, d.Y, d.Z, e.X, e.Y, e.Z, b.X, b.Y, b.Z, g.X, g.Y, g.Z, h.X, h.Y, h.Z, c.X, c.Y, c.Z, PM->IsVisible() ? 1 : 0);
	}
}
