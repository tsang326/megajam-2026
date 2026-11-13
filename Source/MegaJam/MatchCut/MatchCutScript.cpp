// Deep Cut: script runner for offscreen verification.  -MCScript=<file or name> [-MCOut=<dir>]
// A bare name (no slash) is looked up in Content/Data/tests/<name>.txt, which is staged with the packaged build.
// One command per line (# starts a comment). Shot space: x right, y up, z distance from the camera, in cm.
//   skipfilm              leave the opening film now and start playing in shot 0
//   filmtime T            jump the opening film to T seconds
//   card                  show the design-file card before the film again
//   goto N                play shot N from its start point
//   tp X Y Z              put the robot at a shot-space point
//   tpn NX NY [Z]         put the robot at a screen position (x, y in [-1,1]), keeping or setting its depth
//   swimto X Y Z          swim there with the normal movement (stops at rock like a player would)
//   swimn NX NY [Z]       the same, to a screen position
//   move X Y Z SECONDS    hold a direction (x right, y up, z away)
//   use | cut | lamp 1|0 | retake | restart | givepart | opengate | anykey
//   wait S | waitstate NAME | waitok (until C would work)
//   look A|B|C | ghost 0|1 | snap NAME | stat | quit

#include "MatchCutGameMode.h"
#include "DC3D.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void AMatchCutGameMode::LoadScript()
{
	FString Path;
	if (!FParse::Value(FCommandLine::Get(), TEXT("MCScript="), Path))
	{
		return;
	}
	FString Text;
	if (!Path.Contains(TEXT("/")) && !Path.Contains(TEXT("\\")))
	{
		Path = FPaths::ProjectContentDir() / TEXT("Data/tests") / Path + (Path.EndsWith(TEXT(".txt")) ? TEXT("") : TEXT(".txt"));
	}
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogMatchCut, Error, TEXT("Cannot read script %s"), *Path);
		return;
	}
	Text.ParseIntoArrayLines(ScriptLines, true);
	if (!FParse::Value(FCommandLine::Get(), TEXT("MCOut="), ScriptOutDir))
	{
		ScriptOutDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("DeepCut"));
	}
	bScriptActive = true;
	ScriptWait = 3.f; // let the first shaders settle
	UE_LOG(LogMatchCut, Display, TEXT("Script %s: %d lines, output %s"), *Path, ScriptLines.Num(), *ScriptOutDir);
}

void AMatchCutGameMode::TickScript(float DeltaSeconds)
{
	if (!bScriptActive)
	{
		return;
	}
	if (bScriptSwimming)
	{
		const FVector To = ScriptSwimTarget - HeroS;
		ScriptSwimTime += DeltaSeconds;
		if ((To.Size() < 22.0 && VelS.Size() < 90.0) || ScriptSwimTime > 14.f || State != EDCState::Playing)
		{
			UE_LOG(LogMatchCut, Display, TEXT("SCRIPT swim ended at (%.0f,%.0f,%.0f) after %.2fs, wanted (%.0f,%.0f,%.0f)%s"), HeroS.X, HeroS.Y, HeroS.Z, ScriptSwimTime,
				ScriptSwimTarget.X, ScriptSwimTarget.Y, ScriptSwimTarget.Z, To.Size() < 22.0 ? TEXT("") : TEXT("  NOT REACHED"));
			bScriptSwimming = false;
			bScriptHold = State == EDCState::Playing && To.Size() < 22.0;
		}
		return;
	}
	if (State != EDCState::Playing)
	{
		bScriptHold = false;
	}
	if (!ScriptWaitState.IsEmpty())
	{
		if (ScriptWaitState != StateName())
		{
			return;
		}
		ScriptWaitState.Empty();
	}
	if (ScriptWait > 0.f)
	{
		ScriptWait -= DeltaSeconds;
		return;
	}
	if (State == EDCState::TakeCard && ScriptPc < ScriptLines.Num() && !ScriptLines[ScriptPc].StartsWith(TEXT("snap")) && !ScriptLines[ScriptPc].StartsWith(TEXT("wait")))
	{
		return; // let the clapper card finish before acting again
	}
	while (ScriptPc < ScriptLines.Num() && ScriptWait <= 0.f && !bScriptSwimming && ScriptWaitState.IsEmpty())
	{
		const FString Line = ScriptLines[ScriptPc++].TrimStartAndEnd();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		if (!RunScriptLine(Line))
		{
			break;
		}
	}
	if (ScriptPc >= ScriptLines.Num() && ScriptWait <= 0.f)
	{
		bScriptActive = false;
	}
}

bool AMatchCutGameMode::RunScriptLine(const FString& Line)
{
	TArray<FString> T;
	Line.ParseIntoArrayWS(T);
	const FString& Cmd = T[0];
	auto Num = [&T](int32 i) { return T.IsValidIndex(i) ? FCString::Atof(*T[i]) : 0.f; };

	UE_LOG(LogMatchCut, Display, TEXT("SCRIPT %s   [state=%s shot=%d s=(%.0f,%.0f,%.0f) ndc=(%.3f,%.3f) gate=%d ok=%d reason=\"%s\"]"), *Line, StateName(), Cur, HeroS.X, HeroS.Y, HeroS.Z,
		HeroNdc.X, HeroNdc.Y, Shots[Cur].bGateOpen ? 1 : 0, CurEval.bOk ? 1 : 0, CurEval.Reason);

	if (const int32 Done = D3 ? D3->RunLine(T) : 0)
	{
		return Done == 1;
	}
	if (Cmd == TEXT("skipfilm"))
	{
		if (State == EDCState::Film)
		{
			EndFilm();
		}
		State = EDCState::Playing;
		StateTime = 0.f;
		History.Reset();
		EnterShot(0, Shots[0].Start, true);
	}
	else if (Cmd == TEXT("filmtime"))
	{
		FilmTime = Num(1);
	}
	else if (Cmd == TEXT("card"))
	{
		StateTime = 0.f; // show the design-file card again from its start
	}
	else if (Cmd == TEXT("anykey"))
	{
		PressAnyKey();
	}
	else if (Cmd == TEXT("goto"))
	{
		const int32 N = FMath::Clamp((int32)Num(1), 0, Shots.Num() - 1);
		State = EDCState::Playing;
		StateTime = 0.f;
		TeachUpTo(N);
		EnterShot(N, Shots[N].Start, true);
	}
	else if (Cmd == TEXT("swimrel") || Cmd == TEXT("swimmark") || Cmd == TEXT("swimsock"))
	{
		// swimrel ID DX DY [DZ]: to a point beside a loose thing.   swimsock ID DX DY [DZ]: beside the place of a socket
		// (seen from this shot).   swimmark EXIT MARK [Z]: onto a tape mark, at its depth unless Z is given.
		bool bFound = false;
		if (Cmd == TEXT("swimmark"))
		{
			const int32 e = (int32)Num(1), m = (int32)Num(2);
			if (Shots[Cur].Exits.IsValidIndex(e) && Shots[Cur].Exits[e].Marks.IsValidIndex(m))
			{
				const FDCExit& Exit = Shots[Cur].Exits[e];
				ScriptSwimTarget = NdcToShot(Cur, ViewNdc(Exit.To, Exit.Marks[m], true), T.IsValidIndex(3) ? Num(3) : Exit.Marks[m].Z, false);
				bFound = true;
			}
		}
		else if (Cmd == TEXT("swimrel"))
		{
			for (const FDCProp& P : Props)
			{
				if (T.IsValidIndex(1) && P.Id == T[1] && P.Shot == Cur)
				{
					ScriptSwimTarget = P.S + FVector(Num(2), Num(3), Num(4));
					bFound = true;
				}
			}
		}
		else
		{
			for (const FDCSocket& S : Sockets)
			{
				if (T.IsValidIndex(1) && S.Id == T[1])
				{
					// where the socket's place in its own frame is in this shot, at the depth given (default: the socket's)
					const double Z = T.IsValidIndex(4) ? Num(4) : S.S.Z;
					ScriptSwimTarget = NdcToShot(Cur, ViewNdc(S.Shot, S.S, S.Shot != Cur), Z, false) + FVector(Num(2), Num(3), 0.0);
					bFound = true;
				}
			}
		}
		if (bFound)
		{
			ScriptSwimTime = 0.f;
			bScriptSwimming = true;
		}
		else
		{
			UE_LOG(LogMatchCut, Warning, TEXT("SCRIPT target not found: %s"), *Line);
		}
	}
	else if (Cmd == TEXT("wheel"))
	{
		bScriptHold = false;
		AddDepthStep(Num(1));
	}
	else if (Cmd == TEXT("stuck"))
	{
		StuckTime = Num(1); // jump the no-progress clock
	}
	else if (Cmd == TEXT("tp") || Cmd == TEXT("tpn"))
	{
		bScriptHold = false;
		HeroS = Cmd == TEXT("tp") ? FVector(Num(1), Num(2), Num(3)) : NdcToShot(Cur, FVector2D(Num(1), Num(2)), T.IsValidIndex(3) ? Num(3) : HeroS.Z, false);
		VelS = FVector::ZeroVector;
		HeroPos = ShotToWorld(Cur, HeroS);
		HeroNdc = ViewNdc(Cur, HeroS, false);
	}
	else if (Cmd == TEXT("swimto") || Cmd == TEXT("swimn"))
	{
		ScriptSwimTarget = Cmd == TEXT("swimto") ? FVector(Num(1), Num(2), Num(3)) : NdcToShot(Cur, FVector2D(Num(1), Num(2)), T.IsValidIndex(3) ? Num(3) : HeroS.Z, false);
		ScriptSwimTime = 0.f;
		bScriptSwimming = true;
	}
	else if (Cmd == TEXT("move"))
	{
		bScriptHold = false;
		ScriptMove = FVector(Num(1), Num(2), Num(3));
		ScriptMoveLeft = Num(4);
		ScriptWait = Num(4);
	}
	else if (Cmd == TEXT("wait"))
	{
		ScriptWait = Num(1);
	}
	else if (Cmd == TEXT("waitstate"))
	{
		ScriptWaitState = T.IsValidIndex(1) ? T[1] : FString();
	}
	else if (Cmd == TEXT("waitok"))
	{
		// wait until C would work (a mark that comes and goes), at most 12 s
		const bool bWant = !T.IsValidIndex(1) || Num(1) > 0.f; // "waitok 0" waits until C would NOT work
		if (CurEval.bOk != bWant && ScriptOkWait < 12.f)
		{
			ScriptOkWait += 0.1f;
			ScriptWait = 0.1f;
			--ScriptPc;
			return false;
		}
		ScriptOkWait = 0.f;
	}
	else if (Cmd == TEXT("snap"))
	{
		const FString File = ScriptOutDir / (T.IsValidIndex(1) ? T[1] : TEXT("snap")) + TEXT(".png");
		FScreenshotRequest::RequestScreenshot(File, true, false);
		ScriptWait = 0.3f;
	}
	else if (Cmd == TEXT("use"))
	{
		PressUse();
		return false;
	}
	else if (Cmd == TEXT("cut"))
	{
		bScriptHold = false;
		TryCut();
		return false; // let a frame pass
	}
	else if (Cmd == TEXT("lamp"))
	{
		if (bFeature[0] && bLampOn != (Num(1) > 0.f))
		{
			PressLamp();
		}
	}
	else if (Cmd == TEXT("next"))
	{
		ContinueStage(); // C on the end card
	}
	else if (Cmd == TEXT("skipto"))
	{
		// start at shot N with everything the earlier stages give
		const int32 N = FMath::Clamp((int32)Num(1), 0, Shots.Num() - 1);
		if (State == EDCState::Film)
		{
			EndFilm();
		}
		GrantEarlierStages(N);
		TeachUpTo(N);
		State = EDCState::Playing;
		StateTime = 0.f;
		History.Reset();
		EnterShot(N, Shots[N].Start, true);
		return false;
	}
	else if (Cmd == TEXT("waitpred"))
	{
		// wait until the predator is at least in phase N (1 warning, 2 coming), at most 40 s
		if (PredPhase < (int32)Num(1) && ScriptPredWait < 40.f)
		{
			ScriptPredWait += 0.1f;
			ScriptWait = 0.1f;
			--ScriptPc;
			return false;
		}
		ScriptPredWait = 0.f;
	}
	else if (Cmd == TEXT("givepart"))
	{
		// "givepart" = the headlamp, "givepart 1" = the gripper; the restore is skipped as well
		const int32 Feature = FMath::Clamp((int32)Num(1), 0, 2);
		bFeature[Feature] = true;
		for (FDCShot& S : Shots)
		{
			if (S.bHasPart && S.PartFeature == Feature)
			{
				bGapFilled[S.PartGap] = true;
				if (AActor* A = S.PartActor.Get())
				{
					A->SetActorHiddenInGame(true);
				}
			}
		}
		UpdateGate(Cur, true);
		PointGhost();
	}
	else if (Cmd == TEXT("opengate"))
	{
		Shots[Cur].bGateOpen = true; // preview only: show the marks without solving the shot
		RevealMarks(Cur);
	}
	else if (Cmd == TEXT("retake"))
	{
		bScriptHold = false;
		PressRetake();
	}
	else if (Cmd == TEXT("restart"))
	{
		RestartAll();
	}
	else if (Cmd == TEXT("look"))
	{
		ReportFrameTime();
		ApplyLook(T.IsValidIndex(1) ? T[1] : TEXT("A"));
	}
	else if (Cmd == TEXT("ghost"))
	{
		ReportFrameTime();
		GhostMode = (int32)Num(1);
		PointGhost();
	}
	else if (Cmd == TEXT("stat"))
	{
		ReportFrameTime();
	}
	else if (Cmd == TEXT("quit"))
	{
		ReportFrameTime();
		FPlatformMisc::RequestExit(false);
		return false;
	}
	else
	{
		UE_LOG(LogMatchCut, Warning, TEXT("Unknown script command: %s"), *Line);
	}
	return true;
}
