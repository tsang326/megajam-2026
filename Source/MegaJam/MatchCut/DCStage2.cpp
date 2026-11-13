// Deep Cut, stage 2 (brief 04): the predator, the way from one stage to the next, and the counts the HUD shows.
// The predator only comes into a shot whose puzzle is solved. It cannot be fought or hidden from:
// the only way out is to cut to another shot. Caught = the signal is lost and the take starts again.

#include "MatchCutGameMode.h"
#include "DCAudio.h"
#include "DeepCutText.h"
#include "Camera/CameraActor.h"

AActor* AMatchCutGameMode::SpawnPredator(const FVector& Loc, bool bOnFilm)
{
	// thin, dark, fast: a line of dark segments, a long jaw and cold eyes. Local +X is forward, Z up.
	// (The gentle creature is the opposite on purpose: big, round, warm, with soft lights.)
	AActor* A = SpawnGroup(Loc);
	// on the old diver film it is a little lighter, or the torch would not find it against the dark water
	const FLinearColor Skin = bOnFilm ? FLinearColor(0.07f, 0.075f, 0.085f) : FLinearColor(0.012f, 0.014f, 0.02f);
	for (int32 i = 0; i < 10; ++i)
	{
		const double W = FMath::Max(0.56 - 0.052 * i, 0.08);
		AddPart(A, SphereMesh, FVector(-i * 50.0, 0, 5.0 * FMath::Sin(i * 0.9)), FRotator::ZeroRotator, FVector(1.1, W * 0.6, W), Skin, FLinearColor::Black);
	}
	AddPart(A, SphereMesh, FVector(75, 0, -4), FRotator::ZeroRotator, FVector(1.5, 0.30, 0.34), Skin, FLinearColor::Black);   // long jaw
	AddPart(A, SphereMesh, FVector(-130, 0, 42), FRotator(-30, 0, 0), FVector(1.5, 0.05, 0.42), Skin, FLinearColor::Black); // back spine
	AddPart(A, SphereMesh, FVector(-250, 0, -30), FRotator(25, 0, 0), FVector(1.0, 0.05, 0.30), Skin, FLinearColor::Black); // belly spine
	AddPart(A, SphereMesh, FVector(-520, 0, 0), FRotator::ZeroRotator, FVector(1.3, 0.04, 0.85), Skin, FLinearColor::Black); // tail blade
	const FLinearColor Eye(0.80f, 0.88f, 1.f);
	const FLinearColor EyeGlow(9.f, 11.f, 16.f);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		AddPart(A, SphereMesh, FVector(98, Side * 15.0, 12), FRotator::ZeroRotator, FVector(0.15, 0.10, 0.11), Eye, EyeGlow);
		AddPart(A, SphereMesh, FVector(70, Side * 17.0, 17), FRotator::ZeroRotator, FVector(0.09, 0.08, 0.07), Eye, EyeGlow);
	}
	return A;
}

void AMatchCutGameMode::ResetPredator()
{
	PredPhase = 0;
	PredClock = 0.f;
	PredChase = 0.f;
	if (AActor* A = PredActor.Get())
	{
		A->SetActorHiddenInGame(true);
	}
}

void AMatchCutGameMode::UpdatePredator(float Dt)
{
	AActor* A = PredActor.Get();
	if (!A || !Shots.IsValidIndex(Cur))
	{
		return;
	}
	const FDCShot& Shot = Shots[Cur];
	if (State != EDCState::Playing || !Shot.bPredator || !Shot.bGateOpen || EndTimer > 0.f)
	{
		A->SetActorHiddenInGame(true);
		return;
	}
	PredClock += Dt;
	if (PredPhase == 0)
	{
		if (PredClock < Shot.PredDelay)
		{
			A->SetActorHiddenInGame(true);
			return;
		}
		PredPhase = 1;
		PredS = Shot.PredFrom;
		UE_LOG(LogMatchCut, Display, TEXT("PREDATOR warning in shot %d \"%s\": delay=%.1fs warn=%.1fs speed=%.0f+%.0f*t cm/s, from (%.0f,%.0f,%.0f), %.0f cm from Unit 0"),
			Cur, *Shot.Name, Shot.PredDelay, Shot.PredWarn, Shot.PredSpeed, Shot.PredAccel, Shot.PredFrom.X, Shot.PredFrom.Y, Shot.PredFrom.Z, FVector::Dist(Shot.PredFrom, HeroS));
		if (Audio)
		{
			Audio->Play(TEXT("reel_restore"), 1.0f, 0.42f); // stand-in sting: the restore sound, far lower
		}
	}
	const FVector ToHero = HeroS - PredS;
	if (PredPhase == 1)
	{
		// eyes first: it slides in from outside the frame and waits at the edge
		const float U = (PredClock - Shot.PredDelay) / FMath::Max(Shot.PredWarn, 0.1f);
		const FVector In = FVector(HeroS.X - Shot.PredFrom.X, 0.0, 0.0).GetSafeNormal();
		PredS = Shot.PredFrom + In * (330.0 * FMath::Min(1.0, U / 0.35)) + FVector(0.0, 14.0 * FMath::Sin(PredClock * 2.2), 0.0);
		if (U >= 1.f)
		{
			PredPhase = 2;
			PredChase = 0.f;
			UE_LOG(LogMatchCut, Display, TEXT("PREDATOR attacks in shot %d: %.0f cm from Unit 0"), Cur, ToHero.Size());
			if (Audio)
			{
				Audio->Play(TEXT("take_clapper"), 0.7f, 0.55f);
			}
		}
	}
	else
	{
		PredChase += Dt;
		const double Speed = Shot.PredSpeed + Shot.PredAccel * PredChase;
		const double D = ToHero.Size();
		PredS += ToHero.GetSafeNormal() * FMath::Min(D, Speed * Dt);
		if (D < HeroRadius + 55.0)
		{
			PredatorCatch();
			return;
		}
	}
	const FVector Dir = (HeroS - PredS).GetSafeNormal();
	const FVector DirW = Shot.AxR * Dir.X + Shot.AxU * Dir.Y + Shot.AxF * Dir.Z;
	FRotator Rot = FRotationMatrix::MakeFromXZ(DirW.IsNearlyZero() ? Shot.AxR : DirW, Shot.AxU).Rotator();
	Rot.Roll += 7.0 * FMath::Sin(PredClock * 9.0);
	A->SetActorHiddenInGame(false);
	A->SetActorLocationAndRotation(ShotToWorld(Cur, PredS), Rot);
}

void AMatchCutGameMode::PredatorCatch()
{
	const FDCShot& Shot = Shots[Cur];
	UE_LOG(LogMatchCut, Display, TEXT("PREDATOR caught Unit 0 in shot %d \"%s\": %.1fs after the marks, %.1fs after the warning, %.1fs of chase"),
		Cur, *Shot.Name, PredClock, PredClock - Shot.PredDelay, PredChase);
	for (FDCProp& P : Props)
	{
		if (P.Shot == Cur && P.bCarried)
		{
			P.bCarried = false; // what was held is let go where it is
		}
	}
	bCaught = true;
	FailTake(DCText::ReasonCaught);
	ResetPredator();
	if (Audio)
	{
		Audio->Play(TEXT("cut_splice"), 1.0f, 0.5f);
	}
}

int32 AMatchCutGameMode::CrewCount() const
{
	int32 N = bCrewRescued ? 1 : 0;
	for (const FDCSocket& S : Sockets)
	{
		N += (S.bFilled && S.Accepts == TEXT("crew")) ? 1 : 0;
	}
	return N;
}

int32 AMatchCutGameMode::ScenesCount() const
{
	return (bGapFilled[0] ? 1 : 0) + (bGapFilled[1] ? 1 : 0) + (bGapFilled[2] ? 1 : 0);
}

bool AMatchCutGameMode::HasNextStage() const
{
	const int32 Next = Shots[Cur].Stage + 1;
	return Shots.ContainsByPredicate([Next](const FDCShot& S) { return S.Stage == Next; });
}

void AMatchCutGameMode::GrantEarlierStages(int32 ShotIndex)
{
	// starting in a later stage (-MCShot=N, script "skipto N"): what the earlier stages give is given
	const int32 Stage = Shots[ShotIndex].Stage;
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		FDCShot& S = Shots[i];
		if (S.Stage >= Stage)
		{
			continue;
		}
		S.bIntroPlayed = true;
		if (S.bHasPart)
		{
			bFeature[S.PartFeature] = true;
			bGapFilled[S.PartGap] = true;
			if (AActor* A = S.PartActor.Get())
			{
				A->SetActorHiddenInGame(true);
			}
		}
		if (i == CrewShot)
		{
			bCrewRescued = true;
		}
		bFirstCutDone = true;
	}
}

void AMatchCutGameMode::ContinueStage()
{
	const int32 Next = Shots[Cur].Stage + 1;
	const int32 Index = Shots.IndexOfByPredicate([Next](const FDCShot& S) { return S.Stage == Next; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	UE_LOG(LogMatchCut, Display, TEXT("STAGE %d starts at shot %d \"%s\""), Next, Index, *Shots[Index].Name);
	EndTimer = -1.f;
	Take = 1;
	VelS = FVector::ZeroVector;
	BannerTime = 0.f;
	History.Reset();
	State = EDCState::Playing;
	StateTime = 0.f;
	EnterShot(Index, Shots[Index].Start, true);
	if (bCrewRescued)
	{
		CrewPos = HeroPos;
	}
}

const FDCFilmSet* AMatchCutGameMode::ClipSet(const FString& ClipName) const
{
	const FDCClip* Clip = Clips.Find(ClipName);
	return Clip ? FilmSets.Find(Clip->Set) : nullptr;
}

ACameraActor* AMatchCutGameMode::ClipCamera(const FString& ClipName) const
{
	const FDCFilmSet* Set = ClipSet(ClipName);
	return Set ? Set->Camera.Get() : nullptr;
}
