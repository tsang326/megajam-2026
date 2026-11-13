// Deep Cut - stage 1 (brief 03): state machine, tick, input, shot changes, retake history.

#include "MatchCutGameMode.h"
#include "MatchCutPlayerController.h"
#include "MatchCutHUD.h"
#include "DCAudio.h"
#include "DeepCutText.h"

#include "Camera/CameraActor.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogMatchCut);

namespace DCFlow
{
	const float TakeSeconds = 0.85f;
	const float DropSeconds = 2.2f;
}

AMatchCutGameMode::AMatchCutGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = nullptr; // the robot is a plain actor moved by this class
	PlayerControllerClass = AMatchCutPlayerController::StaticClass();
	HUDClass = AMatchCutHUD::StaticClass();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = CubeFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	SphereMesh = SphereFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	CylinderMesh = CylinderFinder.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LitFinder(TEXT("/Game/DeepCut/M_DC_Lit.M_DC_Lit"));
	LitMaterial = LitFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ToonFinder(TEXT("/Game/DeepCut/M_DC_Toon.M_DC_Toon"));
	ToonMaterial = ToonFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> UnlitFinder(TEXT("/Game/DeepCut/M_DC_Unlit.M_DC_Unlit"));
	UnlitMaterial = UnlitFinder.Object;
}

void AMatchCutGameMode::StartPlay()
{
	Super::StartPlay();

	if (!CubeMesh || !SphereMesh || !CylinderMesh || !LitMaterial || !ToonMaterial || !UnlitMaterial)
	{
		UE_LOG(LogMatchCut, Error, TEXT("Deep Cut could not start: a shape or material asset is missing."));
		return;
	}
	if (!LoadLayout() || !LoadFilm())
	{
		UE_LOG(LogMatchCut, Error, TEXT("Deep Cut could not start: no layout or no film data."));
		Shots.Reset();
		return;
	}

	GAreScreenMessagesEnabled = false; // engine debug text would sit on top of the picture

	SpawnWorld();
	Audio = NewObject<UDCAudio>(this);
	Audio->Init(GetWorld());

	// the frame to match in the restore-by-cut is where Unit 7 stands when the removed scene starts
	if (const FDCClip* Clip = Clips.Find(FlashClip))
	{
		for (const FDCTrack& Track : Clip->Tracks)
		{
			if (Track.Actor.StartsWith(TEXT("unit7")) && Track.Keys.Num() > 0)
			{
				const FVector P = Track.Keys[0].P;
				RestoreNdc = FVector2D(P.X / (P.Z * TanH), P.Y / (P.Z * TanV));
			}
		}
	}

	FString CmdLook;
	if (FParse::Value(FCommandLine::Get(), TEXT("MCLook="), CmdLook))
	{
		LookName = CmdLook;
	}
	ApplyLook(LookName);
	GhostCapture->GetCaptureComponent2D()->HiddenActors = GhostHidden;

	// a test script always sees the film as a first viewing, so its result does not depend on earlier runs
	bIntroSeen = !FCString::Strifind(FCommandLine::Get(), TEXT("MCScript=")) && IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir() / TEXT("DeepCut/intro_seen.txt")));

	EnterShot(0, Shots[0].Start, true);
	UE_LOG(LogMatchCut, Display, TEXT("DEEPCUT started: %d shots, part in shot %d, crew in shot %d, restore frame (%.3f,%.3f), intro seen before: %d"), Shots.Num(), PartShot, CrewShot, RestoreNdc.X, RestoreNdc.Y, bIntroSeen ? 1 : 0);

	int32 StartShot = INDEX_NONE;
	if (FParse::Value(FCommandLine::Get(), TEXT("MCShot="), StartShot) && Shots.IsValidIndex(StartShot))
	{
		// quick preview of one shot: Tools/preview_shot.sh
		State = EDCState::Playing;
		History.Reset();
		EnterShot(StartShot, Shots[StartShot].Start, true);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("MCSkipFilm")))
	{
		State = EDCState::Playing;
		ApplyLighting();
	}
	else
	{
		StartFilm();
	}
	LoadScript();
}

void AMatchCutGameMode::SetView(ACameraActor* Cam)
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (Cam)
		{
			PC->SetViewTarget(Cam);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// input
// ---------------------------------------------------------------------------------------------

void AMatchCutGameMode::PressCut()
{
	if (State == EDCState::Playing)
	{
		TryCut();
	}
}

void AMatchCutGameMode::PressLamp()
{
	if (State == EDCState::Playing && bHasPart)
	{
		bLampOn = !bLampOn;
		UE_LOG(LogMatchCut, Display, TEXT("LAMP %s"), bLampOn ? TEXT("on") : TEXT("off"));
		if (Audio)
		{
			Audio->Play(TEXT("lamp_on"), 1.f, bLampOn ? 1.f : 0.8f);
		}
	}
}

void AMatchCutGameMode::PressRetake()
{
	if (State == EDCState::EndCard)
	{
		RestartAll();
	}
	else if (State == EDCState::Playing && History.Num() > 0)
	{
		// back one shot, with every loose thing where it was when that shot was entered
		++Take;
		if (History.Num() > 1)
		{
			History.Pop();
		}
		const FDCSnap Snap = History.Last();
		UE_LOG(LogMatchCut, Display, TEXT("RETAKE from shot %d back to shot %d"), Cur, Snap.Shot);
		RestoreSnap(Snap);
		VelS = FVector::ZeroVector;
		EndTimer = -1.f;
		EnterShot(Snap.Shot, Snap.HeroS, false);
		if (Audio)
		{
			Audio->Play(TEXT("take_clapper"), 0.7f);
		}
	}
}

void AMatchCutGameMode::PressAnyKey()
{
	if (State == EDCState::Film && bFilmSkippable && StateTime > 0.5f)
	{
		UE_LOG(LogMatchCut, Display, TEXT("FILM skipped at %.1fs"), FilmTime);
		EndFilm();
	}
}

void AMatchCutGameMode::RestartAll()
{
	bFirstCutDone = false;
	bHasPart = false;
	bLampOn = false;
	bCrewRescued = false;
	bDepthUsed = false;
	bGapFilled[0] = bGapFilled[1] = bGapFilled[2] = false;
	bThumbTaken = false;
	EndTimer = -1.f;
	Take = 1;
	VelS = FVector::ZeroVector;
	BannerTime = 0.f;
	for (FDCProp& P : Props)
	{
		P.Shot = P.HomeShot;
		P.S = P.HomeS;
		P.bCarried = P.bLocked = P.bEverGrabbed = false;
	}
	for (FDCSocket& S : Sockets)
	{
		S.bFilled = false;
		SetPaintEmissive(S.Paint, S.Accepts == TEXT("cell") ? FLinearColor(5.f, 0.6f, 0.3f) : FLinearColor(0.5f, 1.4f, 1.4f));
	}
	for (FDCShot& Shot : Shots)
	{
		Shot.bGateOpen = false;
		Shot.Switch.bOn = false;
		Shot.Switch.Turn = 0.f;
		for (FDCSolid& Solid : Shot.Solids)
		{
			Solid.Open = 0.f;
			Solid.World.SetLocation(Solid.BaseLoc);
			if (AActor* A = Solid.Actor.Get())
			{
				A->SetActorLocation(Solid.BaseLoc);
			}
		}
	}
	if (AActor* A = PartActor.Actor.Get())
	{
		A->SetActorHiddenInGame(false);
	}
	if (CrewShot != INDEX_NONE)
	{
		CrewPos = ShotToWorld(CrewShot, Shots[CrewShot].CrewS);
		PlaceRobot(Crew, CrewPos, FVector2D(1.0, -0.3), Shots[CrewShot].CamRot, 35.f);
	}
	History.Reset();
	State = EDCState::Playing;
	EnterShot(0, Shots[0].Start, true);
	UE_LOG(LogMatchCut, Display, TEXT("RESTART"));
}

void AMatchCutGameMode::ShowBanner(const FString& Line1, const FString& Line2, float Seconds)
{
	Banner1 = Line1;
	Banner2 = Line2;
	BannerTime = Seconds;
}

// ---------------------------------------------------------------------------------------------
// flow
// ---------------------------------------------------------------------------------------------

void AMatchCutGameMode::EnterShot(int32 Index, const FVector& HeroAtS, bool bPushHistory)
{
	ReportFrameTime();
	Cur = FMath::Clamp(Index, 0, Shots.Num() - 1);
	FDCShot& Shot = Shots[Cur];
	Shot.Truck = Shot.Rail.Z; // a shot always starts with its camera at the start of the rail
	HeroS = HeroAtS;
	HeroPos = ShotToWorld(Cur, HeroS);
	HeroNdc = ViewNdc(Cur, HeroS, false);
	UpdateRail(0.f, true);
	SetView(Shot.Camera.Get());
	GhostExit = 0;
	UpdateGate(Cur, true); // a shot with no puzzle shows its marks at once
	CurEval = EvalCut();
	PointGhost();
	ApplyLighting();
	if (bPushHistory)
	{
		TakeSnap(History.AddDefaulted_GetRef());
	}
	UE_LOG(LogMatchCut, Display, TEXT("SHOT %d \"%s\" dark=%d gate=\"%s\" open=%d exits=%d hero=(%.0f,%.0f,%.0f)"), Cur, *Shot.Name, Shot.bDark ? 1 : 0, *Shot.Gate, Shot.bGateOpen ? 1 : 0, Shot.Exits.Num(), HeroS.X, HeroS.Y, HeroS.Z);

	// a last shot with nothing to plug in ends the stage by itself
	if (Shot.Exits.Num() == 0 && !Sockets.ContainsByPredicate([this](const FDCSocket& S) { return S.Shot == Cur && S.bEnds; }))
	{
		EndTimer = 3.5f;
	}
}

void AMatchCutGameMode::PointGhost()
{
	if (!GhostCapture)
	{
		return;
	}
	if (InRestore())
	{
		PoseClip(FlashClip, 0.f);
		if (FlashCam.IsValid())
		{
			GhostCapture->SetActorLocationAndRotation(FlashCam->GetActorLocation(), FRotator::ZeroRotator);
		}
	}
	else if (Shots[Cur].Exits.IsValidIndex(GhostExit))
	{
		const FDCShot& B = Shots[Shots[Cur].Exits[GhostExit].To];
		GhostCapture->SetActorLocationAndRotation(B.CamPos + B.AxR * B.Rail.Z, B.CamRot);
	}
	GhostTimer = 0.f;
}

void AMatchCutGameMode::TickDrop(float Dt)
{
	// the film lets go of the robot: it sinks into the first shot from above the frame
	const FDCShot& Shot = Shots[0];
	const float A = FMath::Clamp(StateTime / DCFlow::DropSeconds, 0.f, 1.f);
	const float Ease = 1.f - FMath::Square(1.f - A);
	HeroS = FVector(Shot.Start.X, FMath::Lerp(1.05 * Shot.Start.Z * TanV, Shot.Start.Y, (double)Ease), Shot.Start.Z);
	HeroPos = ShotToWorld(0, HeroS);
	HeroNdc = ViewNdc(0, HeroS, false);
	Heading = FVector2D(1.0, -0.7 * (1.0 - Ease));
	if (StateTime >= DCFlow::DropSeconds)
	{
		State = EDCState::Playing;
		StateTime = 0.f;
		VelS = FVector::ZeroVector;
		ApplyLighting();
		UE_LOG(LogMatchCut, Display, TEXT("DROP done: playing"));
	}
}

void AMatchCutGameMode::TickPlaying(float Dt)
{
	TickMovement(Dt);
	TickPuzzles(Dt);
	CurEval = EvalCut();
	if (!InRestore() && Shots[Cur].Exits.IsValidIndex(CurEval.Exit) && CurEval.Exit != GhostExit)
	{
		// two cameras: the ghost shows the one whose mark is nearest
		GhostExit = CurEval.Exit;
		PointGhost();
	}

	if (EndTimer > 0.f)
	{
		EndTimer -= Dt;
		if (EndTimer <= 0.f)
		{
			State = EDCState::EndCard;
			StateTime = 0.f;
			UE_LOG(LogMatchCut, Display, TEXT("END CARD scenes=%d/%d crew=%d/%d"), bGapFilled[0] ? 1 : 0, NumGaps, bCrewRescued ? 1 : 0, CrewTotal);
		}
	}
}

void AMatchCutGameMode::UpdateHeroActor(float Dt)
{
	if (!Shots.IsValidIndex(Cur))
	{
		return;
	}
	const FDCShot& Shot = Shots[Cur];
	PlaceRobot(Hero, HeroPos, Heading, Shot.CamRot, 0.f);
	const bool bLamp = bLampOn && (State == EDCState::Playing || State == EDCState::TakeCard || State == EDCState::EndCard);
	if (USpotLightComponent* L = LampSpot.Get())
	{
		L->SetVisibility(bLamp);
	}
	if (UPointLightComponent* G = LampGlow.Get())
	{
		G->SetVisibility(bLamp);
		G->SetWorldLocation(HeroPos - Shot.AxF * 240.0 + Shot.AxU * 40.0);
	}

	if (CrewShot != INDEX_NONE && bCrewRescued)
	{
		// simple follow: trail behind and a little above
		const FVector Want = HeroPos - Shot.AxR * (Heading.X >= 0.0 ? 95.0 : -95.0) + Shot.AxU * 45.0;
		CrewPos = FMath::VInterpTo(CrewPos, Want, Dt, 3.0f);
		PlaceRobot(Crew, CrewPos, FVector2D(Heading.X, 0.0), Shot.CamRot, 0.f);
	}
}

void AMatchCutGameMode::UpdateMusic()
{
	if (!Audio)
	{
		return;
	}
	const bool bFilm = State == EDCState::Film;
	const bool bFlash = State == EDCState::Flashback;
	Audio->SetLoop(TEXT("mus_opening"), bFilm ? 1.f : 0.f, 1.5f);
	Audio->SetLoop(TEXT("projector"), bFilm || bFlash ? 1.f : 0.f, 0.6f);
	Audio->SetLoop(TEXT("mus_flashback"), bFlash ? 1.f : 0.f, 1.0f);
	Audio->SetLoop(TEXT("amb_underwater"), bFilm ? 0.f : 1.f, 2.0f);
	Audio->SetLoop(TEXT("mus_explore"), bFilm ? 0.f : (State == EDCState::EndCard ? 0.6f : 1.f), 3.0f);
	Audio->SetDuck(bFlash); // the gameplay music goes under the recovered film
}

void AMatchCutGameMode::ReportFrameTime()
{
	if (FrameCount > 0)
	{
		UE_LOG(LogMatchCut, Display, TEXT("FRAMETIME shot=%d look=%s state=%s ghost=%d lamp=%d frames=%d avg=%.2fms max=%.2fms"),
			Cur, *LookName, StateName(), GhostMode, bLampOn ? 1 : 0, FrameCount, FrameAccum / FrameCount * 1000.0, FrameMax * 1000.0);
	}
	FrameAccum = 0.0;
	FrameMax = 0.0;
	FrameCount = 0;
	ShotElapsed = 0.f;
}

const TCHAR* AMatchCutGameMode::StateName() const
{
	switch (State)
	{
	case EDCState::Film: return TEXT("Film");
	case EDCState::Drop: return TEXT("Drop");
	case EDCState::Playing: return TEXT("Playing");
	case EDCState::TakeCard: return TEXT("TakeCard");
	case EDCState::Flashback: return TEXT("Flashback");
	default: return TEXT("EndCard");
	}
}

void AMatchCutGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Shots.Num() == 0 || !Hero.Actor.IsValid())
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.1f); // long frames (shader hitches, background runs) must not tunnel the robot

	TickScript(DeltaSeconds);
	PlayClock += DeltaSeconds;

	// frame time, skipping the first second after each shot change (first-use shader hitches)
	ShotElapsed += DeltaSeconds;
	if (ShotElapsed > 1.f)
	{
		FrameAccum += DeltaSeconds;
		FrameMax = FMath::Max(FrameMax, (double)DeltaSeconds);
		++FrameCount;
	}

	CutFlash = FMath::Max(0.f, CutFlash - DeltaSeconds);
	SoftFailTime = FMath::Max(0.f, SoftFailTime - DeltaSeconds);
	if (BannerTime > 0.f)
	{
		BannerTime -= DeltaSeconds;
	}
	if (ScriptMoveLeft > 0.f)
	{
		ScriptMoveLeft -= DeltaSeconds;
	}
	StateTime += DeltaSeconds;

	switch (State)
	{
	case EDCState::Film:
		TickFilm(Dt);
		break;
	case EDCState::Drop:
		TickDrop(Dt);
		break;
	case EDCState::Playing:
		TickPlaying(Dt);
		break;
	case EDCState::TakeCard:
		if (StateTime >= DCFlow::TakeSeconds && History.Num() > 0)
		{
			// back to where this shot was entered; loose things and solved puzzles stay as they are
			State = EDCState::Playing;
			StateTime = 0.f;
			HeroS = History.Last().HeroS;
			Shots[Cur].Truck = Shots[Cur].Rail.Z;
			UpdateRail(0.f, true);
			HeroPos = ShotToWorld(Cur, HeroS);
			HeroNdc = ViewNdc(Cur, HeroS, false);
			if (bCrewRescued)
			{
				CrewPos = HeroPos;
			}
		}
		break;
	case EDCState::Flashback:
		TickFlashback(Dt);
		break;
	default:
		break;
	}

	UpdateProps(Dt);
	UpdateCurrents(Dt);
	UpdateHeroActor(Dt);
	UpdateMusic();
	if (Audio)
	{
		Audio->Tick(DeltaSeconds);
	}

	// refresh the ghost about 12 times a second; a second scene render every frame was the main cost in brief 01
	if (GhostCapture)
	{
		const bool bWant = GhostMode != 0 && (State == EDCState::Playing || State == EDCState::TakeCard || State == EDCState::Drop) && (InRestore() || Shots[Cur].Exits.Num() > 0);
		GhostTimer -= DeltaSeconds;
		if (bWant && GhostTimer <= 0.f)
		{
			GhostTimer = 0.08f;
			GhostCapture->GetCaptureComponent2D()->CaptureSceneDeferred();
		}
	}
}
