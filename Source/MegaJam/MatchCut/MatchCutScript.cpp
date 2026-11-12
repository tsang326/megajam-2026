// Deep Cut slice: script runner for offscreen verification.  -MCScript=<file> [-MCOut=<dir>]
// One command per line (# starts a comment). Screen units: x and y in [-1,1] of the 16:9 frame; the picture is |y| <= 0.75.
//   skipfilm              leave the opening film now and start playing in shot 0
//   filmtime T            jump the opening film to T seconds
//   goto N                play shot N from its start mark
//   tpn X Y               put the robot at a screen position
//   swimto X Y            swim there with the normal movement (stops at rock like a player would)
//   move X Y SECONDS      hold a direction (x right, y up)
//   cut | lamp 1|0 | retake | restart | givepart | anykey
//   wait S | waitstate NAME | waitclock T PERIOD | mtime T | freeze 1|0
//   look A|B|C | ghost 0|1|2 | snap NAME | stat | quit

#include "MatchCutGameMode.h"
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
		const FVector2D To((ScriptSwimTarget.X - HeroNdc.X) * 16.0 / 9.0, ScriptSwimTarget.Y - HeroNdc.Y);
		ScriptSwimTime += DeltaSeconds;
		if (To.Size() < 0.035 || ScriptSwimTime > 10.f || State != EDCState::Playing)
		{
			UE_LOG(LogMatchCut, Display, TEXT("SCRIPT swimto ended at (%.3f,%.3f) after %.2fs, wanted (%.3f,%.3f)"), HeroNdc.X, HeroNdc.Y, ScriptSwimTime, ScriptSwimTarget.X, ScriptSwimTarget.Y);
			bScriptSwimming = false;
			bScriptHold = State == EDCState::Playing;
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
	if (ScriptClockTarget >= 0.f)
	{
		const float Phase = FMath::Fmod(ShotClock, ScriptClockPeriod);
		if (Phase < ScriptClockTarget || Phase > ScriptClockTarget + 0.25f)
		{
			return;
		}
		ScriptClockTarget = -1.f;
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
	while (ScriptPc < ScriptLines.Num() && ScriptWait <= 0.f && !bScriptSwimming && ScriptClockTarget < 0.f && ScriptWaitState.IsEmpty())
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

	UE_LOG(LogMatchCut, Display, TEXT("SCRIPT %s   [state=%s shot=%d ndc=(%.3f,%.3f) ok=%d reason=\"%s\"]"), *Line, StateName(), Cur, HeroNdc.X, HeroNdc.Y, CurEval.bOk ? 1 : 0, CurEval.Reason);

	if (Cmd == TEXT("skipfilm"))
	{
		if (State == EDCState::Film)
		{
			EndFilm();
		}
		State = EDCState::Playing;
		StateTime = 0.f;
		SetShot(0, true);
	}
	else if (Cmd == TEXT("filmtime"))
	{
		FilmTime = Num(1);
	}
	else if (Cmd == TEXT("anykey"))
	{
		PressAnyKey();
	}
	else if (Cmd == TEXT("goto"))
	{
		State = EDCState::Playing;
		StateTime = 0.f;
		SetShot((int32)Num(1), true);
	}
	else if (Cmd == TEXT("tpn"))
	{
		HeroPos = ShotPoint(Cur, FVector2D(Num(1), Num(2)), HeroDepth);
		Vel2D = FVector2D::ZeroVector;
		HeroNdc = FVector2D(Num(1), Num(2));
	}
	else if (Cmd == TEXT("swimto"))
	{
		ScriptSwimTarget = FVector2D(Num(1), Num(2));
		ScriptSwimTime = 0.f;
		bScriptSwimming = true;
	}
	else if (Cmd == TEXT("move"))
	{
		bScriptHold = false;
		ScriptMove = FVector2D(Num(1), Num(2));
		ScriptMoveLeft = Num(3);
		ScriptWait = Num(3);
	}
	else if (Cmd == TEXT("wait"))
	{
		ScriptWait = Num(1);
	}
	else if (Cmd == TEXT("waitstate"))
	{
		ScriptWaitState = T.IsValidIndex(1) ? T[1] : FString();
	}
	else if (Cmd == TEXT("snap"))
	{
		const FString File = ScriptOutDir / (T.IsValidIndex(1) ? T[1] : TEXT("snap")) + TEXT(".png");
		FScreenshotRequest::RequestScreenshot(File, true, false);
		ScriptWait = 0.3f;
	}
	else if (Cmd == TEXT("cut"))
	{
		bScriptHold = false;
		TryCut();
		return false; // let a frame pass
	}
	else if (Cmd == TEXT("lamp"))
	{
		if (bHasPart)
		{
			bLampOn = Num(1) > 0.f;
		}
	}
	else if (Cmd == TEXT("givepart"))
	{
		bHasPart = true;
		bGapFilled[0] = true; // skip the restore as well
		if (AActor* A = PartActor.Actor.Get())
		{
			A->SetActorHiddenInGame(true);
		}
		PointGhost();
	}
	else if (Cmd == TEXT("retake"))
	{
		PressRetake();
	}
	else if (Cmd == TEXT("restart"))
	{
		RestartAll();
	}
	else if (Cmd == TEXT("mtime"))
	{
		ShotClock = Num(1);
		UpdateMovers();
	}
	else if (Cmd == TEXT("freeze"))
	{
		bClockFrozen = Num(1) > 0.f; // hold the movers still so a capture and the cut after it see the same thing
	}
	else if (Cmd == TEXT("waitclock"))
	{
		ScriptClockTarget = Num(1);
		ScriptClockPeriod = FMath::Max(Num(2), 0.1f);
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
