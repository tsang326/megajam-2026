// Deep Cut (brief 05): nobody gets stuck without being told what to do.
//   - a control is taught the first time the level needs it (one prompt), then it stays in the controls line
//   - a failed take says what to do next
//   - after 20 s without progress in a shot, the thing to use is pointed at

#include "MatchCutGameMode.h"
#include "DeepCutText.h"

namespace
{
	const float StuckAfter = 20.f;
}

void AMatchCutGameMode::Teach(int32 Control)
{
	if (Control < 0 || Control >= DCTeach::Num || bTaught[Control])
	{
		return;
	}
	bTaught[Control] = true;
	PromptId = Control;
	PromptTime = 8.f;
	UE_LOG(LogMatchCut, Display, TEXT("TEACH %d \"%s\" in shot %d"), Control, DCText::Prompts[Control], Cur);
}

void AMatchCutGameMode::TeachUpTo(int32 ShotIndex)
{
	// starting at a later shot (-MCShot=N, script goto / skipto): what the earlier shots teach counts as taught
	if (ShotIndex <= 0)
	{
		return;
	}
	bTaught[DCTeach::Move] = bTaught[DCTeach::Cut] = true;
	for (int32 i = 0; i < ShotIndex && i < Shots.Num(); ++i)
	{
		const FDCShot& S = Shots[i];
		for (const FDCProp& P : Props)
		{
			if (P.HomeShot == i && !P.bHeavy)
			{
				bTaught[DCTeach::Push] = true;
				bTaught[DCTeach::Back] = true; // the first carry is also where R is first needed
			}
		}
		for (const FDCExit& E : S.Exits)
		{
			bTaught[DCTeach::Depth] |= !E.bAnyDepth && i > 0;
		}
		if (S.bHasPart)
		{
			bTaught[S.PartFeature == 0 ? DCTeach::Lamp : DCTeach::Grab] |= S.PartFeature <= 1;
		}
	}
	PromptTime = 0.f;
}

FString AMatchCutGameMode::ControlsLine() const
{
	FString Line;
	for (int32 i = 0; i < DCTeach::Num; ++i)
	{
		if (bTaught[i])
		{
			Line += FString(DCText::Controls[i]) + TEXT("   ");
		}
	}
	return Line + DCText::ControlsQuit;
}

const TCHAR* AMatchCutGameMode::FailHow(const TCHAR* Reason) const
{
	if (!FCString::Strcmp(Reason, DCText::ReasonOffMark)) { return DCText::HowOffMark; }
	if (!FCString::Strcmp(Reason, DCText::ReasonTooClose)) { return DCText::HowTooClose; }
	if (!FCString::Strcmp(Reason, DCText::ReasonTooFar)) { return DCText::HowTooFar; }
	if (!FCString::Strcmp(Reason, DCText::ReasonWait)) { return DCText::HowWait; }
	if (!FCString::Strcmp(Reason, DCText::ReasonDarkLamp)) { return DCText::HowDark; }
	if (!FCString::Strcmp(Reason, DCText::ReasonLineUp)) { return DCText::HowLineUp; }
	if (!FCString::Strcmp(Reason, DCText::ReasonRock)) { return DCText::HowRock; }
	if (!FCString::Strcmp(Reason, DCText::ReasonCaught)) { return DCText::HowCaught; }
	if (!FCString::Strcmp(Reason, DCText::ReasonCame)) { return DCText::HowCame; }
	return TEXT("");
}

void AMatchCutGameMode::TickTeach(float Dt)
{
	const FDCShot& Shot = Shots[Cur];
	PushCue = FMath::Max(0.f, PushCue - Dt);

	// the prompt for swimming and cutting stays until the control has been used
	const bool bHold = (PromptId == DCTeach::Move && MovedFor < 0.5f) || (PromptId == DCTeach::Cut && !bFirstCutDone);
	if (PromptTime > 0.f && !(bHold && PromptTime < 1.5f))
	{
		PromptTime -= Dt;
	}

	Teach(DCTeach::Move);
	if (!bTaught[DCTeach::Cut] && Shot.bGateOpen && Shot.Exits.Num() > 0 && !InRestore() && MovedFor > 1.0f && PromptTime < 5.f)
	{
		Teach(DCTeach::Cut);
	}
	if (!bTaught[DCTeach::Push] && Props.ContainsByPredicate([this](const FDCProp& P) { return P.Shot == Cur && !P.bHeavy && !P.bLocked; }) && (bFirstCutDone || Cur > 0) && StuckTime > 1.2f)
	{
		Teach(DCTeach::Push);
	}
	if (!bTaught[DCTeach::Back] && bFirstCutDone && !Shot.bGateOpen && Shot.Gate.StartsWith(TEXT("socket:")) && StuckTime > 2.5f)
	{
		Teach(DCTeach::Back); // arrived without the thing this shot needs: R is the way out
	}
	if (!bTaught[DCTeach::Depth] && Shot.bGateOpen && !InRestore() && Shot.Exits.ContainsByPredicate([](const FDCExit& E) { return !E.bAnyDepth; }) && RevealTime <= 0.f)
	{
		Teach(DCTeach::Depth);
	}
	if (bFeature[0]) { Teach(DCTeach::Lamp); }
	if (bFeature[1]) { Teach(DCTeach::Grab); }

	// stuck: no progress for a while
	StuckTime += Dt;
	if (StuckTime > StuckAfter && !bStuckShown)
	{
		bStuckShown = true;
		FVector2D A, B;
		bool bTo = false;
		const bool bTarget = StuckTarget(A, B, bTo);
		UE_LOG(LogMatchCut, Display, TEXT("STUCK hint in shot %d \"%s\" after %.0fs: target=%d at (%.3f,%.3f) to=%d"), Cur, *Shot.Name, StuckTime, bTarget ? 1 : 0, A.X, A.Y, bTo ? 1 : 0);
	}
	if (StuckTime <= StuckAfter)
	{
		bStuckShown = false;
	}
}

bool AMatchCutGameMode::StuckTarget(FVector2D& OutNdc, FVector2D& OutToNdc, bool& bHasTo) const
{
	const FDCShot& Shot = Shots[Cur];
	bHasTo = false;
	if (InRestore())
	{
		OutNdc = RestoreNdc;
		return true;
	}
	if (Shot.bHasPart && !bFeature[Shot.PartFeature])
	{
		OutNdc = ViewNdc(Cur, Shot.PartS, false);
		return true;
	}
	// a loose thing of this shot that still has to go somewhere: a frame of the next shot, or a socket here
	for (int32 s = 0; s < Sockets.Num(); ++s)
	{
		const FDCSocket& Socket = Sockets[s];
		const bool bNext = Shot.Exits.ContainsByPredicate([&Socket](const FDCExit& E) { return E.To == Socket.Shot; });
		if (Socket.bFilled || (Socket.Shot != Cur && !bNext))
		{
			continue;
		}
		for (const FDCProp& P : Props)
		{
			if (P.Shot == Cur && !P.bLocked && P.Kind == Socket.Accepts && !PropFitsSocket(P, s))
			{
				if (P.bCarried && Socket.Shot != Cur)
				{
					continue; // held: the mark is what matters now
				}
				OutNdc = ViewNdc(Cur, P.S, false);
				OutToNdc = ViewNdc(Socket.Shot, Socket.S, Socket.Shot != Cur);
				bHasTo = !P.bHeavy || P.bCarried || Socket.Shot == Cur;
				if (Socket.Shot != Cur && Shot.bGateOpen && !Shot.Gate.StartsWith(TEXT("fits:")))
				{
					continue; // marks are showing and the thing is free to come or not: point at the mark instead
				}
				return true;
			}
		}
	}
	if (!Shot.bGateOpen)
	{
		if (Shot.Gate == TEXT("grab"))
		{
			for (const FDCProp& P : Props)
			{
				if (P.HomeShot == Cur && P.Shot == Cur)
				{
					OutNdc = ViewNdc(Cur, P.S, false);
					return true;
				}
			}
		}
		return false; // nothing here to use: R
	}
	if (Shot.Exits.IsValidIndex(CurEval.Exit))
	{
		const FDCExit& Exit = Shot.Exits[CurEval.Exit];
		if (Exit.Marks.Num() > 0)
		{
			OutNdc = ViewNdc(Exit.To, Exit.Marks[FMath::Clamp(CurEval.Mark, 0, Exit.Marks.Num() - 1)], true);
			return true;
		}
	}
	return false;
}

double AMatchCutGameMode::FloorUnder(int32 Shot, const FVector& S) const
{
	const double Bottom = Shots[Shot].VolMin.Y - 160.0;
	for (double Y = S.Y - HeroRadius; Y > Bottom; Y -= 6.0)
	{
		if (Blocked(Shot, FVector(S.X, Y, S.Z), 4.0))
		{
			return Y + 3.0;
		}
	}
	return Bottom;
}

void AMatchCutGameMode::AddDepthStep(float Steps)
{
	if (State == EDCState::Playing)
	{
		DepthQueue = FMath::Clamp(DepthQueue + Steps * 70.f, -280.f, 280.f);
	}
}
