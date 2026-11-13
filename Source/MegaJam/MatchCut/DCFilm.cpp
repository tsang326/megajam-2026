// Deep Cut: the opening film and the flashback. Both are data: sets, clips of key poses and the reel order come
// from Content/Data/film.json (made by Tools/make_film_json.py). Nothing here knows what a clip shows.

#include "MatchCutGameMode.h"
#include "DCAudio.h"
#include "DeepCutText.h"
#include "Camera/CameraActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace DCFilmConst
{
	const float EndHold = 1.3f;
	const float CardTime = 3.2f; // the design-file card before the film
	FString IntroFlagPath()
	{
		return FPaths::ProjectSavedDir() / TEXT("DeepCut/intro_seen.txt");
	}
}

FDCRobot* AMatchCutGameMode::FilmActor(const FString& Name)
{
	return FilmActors.Find(Name);
}

void AMatchCutGameMode::PoseClip(const FString& ClipName, float T)
{
	const FDCClip* Clip = Clips.Find(ClipName);
	if (!Clip)
	{
		return;
	}
	const FDCFilmSet* Set = FilmSets.Find(Clip->Set);
	const FVector Origin = Set ? Set->Origin : FVector::ZeroVector;
	for (auto& Pair : FilmActors)
	{
		if (AActor* A = Pair.Value.Actor.Get())
		{
			A->SetActorHiddenInGame(!Clip->Tracks.ContainsByPredicate([&Pair](const FDCTrack& Tr) { return Tr.Actor == Pair.Key; }));
		}
	}
	for (const FDCTrack& Track : Clip->Tracks)
	{
		if (Track.Keys.Num() == 0)
		{
			continue;
		}
		int32 i = 0;
		while (i + 1 < Track.Keys.Num() && Track.Keys[i + 1].T <= T)
		{
			++i;
		}
		const int32 Last = Track.Keys.Num() - 1;
		const FDCKey& A = Track.Keys[i];
		const FDCKey& B = Track.Keys[FMath::Min(i + 1, Last)];
		const float Alpha = B.T > A.T ? FMath::Clamp((T - A.T) / (B.T - A.T), 0.f, 1.f) : 0.f;
		const FVector P = FMath::Lerp(A.P, B.P, (double)Alpha);
		const FVector2D Head = FMath::Lerp(A.Head, B.Head, (double)Alpha);
		const float Roll = FMath::Lerp(A.Roll, B.Roll, Alpha);
		// set space (x right, y up, z away) to world: the set camera looks down +X
		const FVector Loc = Origin + FVector(P.Z, P.X, P.Y);
		if (Track.Actor == TEXT("cam"))
		{
			// a head camera: the key is [t, x, y, z, yaw, pitch, roll]
			if (ACameraActor* Cam = Set ? Set->Camera.Get() : nullptr)
			{
				Cam->SetActorLocationAndRotation(Loc, FRotator(Head.Y, Head.X, Roll));
			}
			continue;
		}
		const FDCRobot* Robot = FilmActor(Track.Actor);
		if (!Robot)
		{
			continue;
		}
		if (Robot->bCreature)
		{
			// a creature faces the way it moves
			if (AActor* Body = Robot->Actor.Get())
			{
				const FVector D0 = B.P - A.P;
				const FVector D1 = Track.Keys[FMath::Min(i + 2, Last)].P - B.P;
				const FVector D = FMath::Lerp(D0, D1.IsNearlyZero() ? D0 : D1, (double)Alpha);
				FRotator Rot = Body->GetActorRotation();
				if (D.SizeSquared() > 0.25)
				{
					Rot = FRotationMatrix::MakeFromXZ(FVector(D.Z, D.X, D.Y), FVector::UpVector).Rotator();
					Rot.Roll += Roll;
				}
				Body->SetActorLocationAndRotation(Loc, Rot);
			}
			continue;
		}
		PlaceRobot(*Robot, Loc, Head, FRotator::ZeroRotator, Roll);
		if (Robot->EyePaint != INDEX_NONE && !Robot->bPlain)
		{
			SetPaintEmissive(Robot->EyePaint, FLinearColor(4.0f, 2.6f, 0.6f) * FMath::Lerp(A.Eye, B.Eye, Alpha));
		}
	}
	if (USpotLightComponent* L = FlashLamp.Get())
	{
		if (Clip->Lamp.Num() > 0)
		{
			float V = (float)Clip->Lamp[0].Y;
			for (const FVector2D& K : Clip->Lamp)
			{
				if (K.X <= T)
				{
					V = (float)K.Y; // stepped
				}
			}
			L->SetIntensity(V);
		}
	}
}

void AMatchCutGameMode::StartFilm()
{
	State = EDCState::Film;
	StateTime = 0.f;
	FilmTime = 0.f;
	bFilmSkippable = bIntroSeen;
	ApplyLighting();
	UE_LOG(LogMatchCut, Display, TEXT("FILM start length=%.1fs skippable=%d"), FilmLength, bFilmSkippable ? 1 : 0);
}

void AMatchCutGameMode::EndFilm()
{
	if (!bIntroSeen)
	{
		bIntroSeen = true;
		FFileHelper::SaveStringToFile(TEXT("seen"), *DCFilmConst::IntroFlagPath());
	}
	State = EDCState::Drop;
	StateTime = 0.f;
	Cur = 0;
	SetView(Shots[0].Camera.Get());
	ApplyLighting();
	UE_LOG(LogMatchCut, Display, TEXT("FILM end -> drop into shot 0"));
}

void AMatchCutGameMode::TickFilm(float Dt)
{
	if (StateTime < DCFilmConst::CardTime)
	{
		return; // the design-file card is up (drawn by the HUD)
	}
	FilmTime += Dt;
	if (FilmTime >= FilmLength + DCFilmConst::EndHold)
	{
		EndFilm();
		return;
	}
	for (const FDCReelSeg& S : Reel)
	{
		if (FilmTime >= S.T0 && FilmTime < S.T1 && S.Gap == INDEX_NONE)
		{
			if (const FDCClip* Clip = Clips.Find(S.Clip))
			{
				if (const FDCFilmSet* Set = FilmSets.Find(Clip->Set))
				{
					SetView(Set->Camera.Get());
				}
				PoseClip(S.Clip, (FilmTime - S.T0) * Clip->Length / FMath::Max(S.T1 - S.T0, 0.1f)); // the reel may play a clip faster than it was keyed
			}
		}
	}
}

void AMatchCutGameMode::StartClip(const FString& ClipName, bool bOfficial, int32 Gap)
{
	// A clip is played either whole (a removed scene that comes back) or as Fathom released it (without its cut seconds).
	const FDCClip* Clip = Clips.Find(ClipName);
	if (!Clip)
	{
		return;
	}
	FlashClip = ClipName;
	bFlashOfficial = bOfficial;
	FlashGap = Gap;
	FlashLength = bOfficial ? Clip->OfficialLength() : Clip->Length;
	FlashClipT = 0.f;
	State = EDCState::Flashback;
	StateTime = 0.f;
	bThumbTaken = false;
	PoseClip(ClipName, 0.f);
	SetView(ClipCamera(ClipName));
	ApplyLighting();
	if (Audio)
	{
		Audio->Play(bOfficial ? TEXT("cam_rec") : TEXT("reel_restore"));
	}
	UE_LOG(LogMatchCut, Display, TEXT("CLIP start \"%s\" version=%s gap=%d plays %.1fs of %.1fs (%d cuts)"), *ClipName, bOfficial ? TEXT("official") : TEXT("restored"), Gap + 1, FlashLength, Clip->Length, Clip->Cuts.Num());
}

void AMatchCutGameMode::StartFlashback()
{
	const FDCShot& Shot = Shots[Cur];
	const FString ClipName = Shot.bHasPart && !Shot.PartClip.IsEmpty() ? Shot.PartClip : FlashClip;
	UE_LOG(LogMatchCut, Display, TEXT("FLASHBACK start (removed scene %d, clip \"%s\")"), Shot.PartGap + 1, *ClipName);
	StartClip(ClipName, false, Shot.bHasPart ? Shot.PartGap : 0);
}

void AMatchCutGameMode::EndFlashback()
{
	const bool bOfficial = bFlashOfficial;
	const FDCClip* Clip = Clips.Find(FlashClip);
	const FDCFilmSet* Set = ClipSet(FlashClip);
	if (ACameraActor* Cam = ClipCamera(FlashClip))
	{
		if (!Set || !Set->bHeadCam)
		{
			Cam->SetActorRotation(FRotator::ZeroRotator);
		}
	}
	if (!bOfficial && FlashGap >= 0 && FlashGap < NumGaps)
	{
		bGapFilled[FlashGap] = true;
	}
	bFlashOfficial = false;
	State = EDCState::Playing;
	StateTime = 0.f;
	if (Shots[Cur].bHasPart)
	{
		FlashClip = Shots[Cur].PartClip;
	}
	SetView(Shots[Cur].Camera.Get());
	PointGhost();
	ApplyLighting();
	if (bOfficial)
	{
		ShowBanner(DCText::OfficialEnd, DCText::OfficialEndHint, 6.f);
		UE_LOG(LogMatchCut, Display, TEXT("CLIP official version ended"));
	}
	else
	{
		ShowBanner(FString::Printf(DCTEXT_RESTORED, ScenesCount(), NumGaps), Clip ? Clip->After : FString(), 7.f);
		UE_LOG(LogMatchCut, Display, TEXT("FLASHBACK end: gap %d filled"), FlashGap + 1);
	}
	UpdateGate(Cur, false);
}

void AMatchCutGameMode::TickFlashback(float Dt)
{
	const FDCClip* Clip = Clips.Find(FlashClip);
	const FDCFilmSet* Set = ClipSet(FlashClip);
	ACameraActor* Cam = ClipCamera(FlashClip);
	const float Prev = FlashClipT;
	FlashClipT = (Clip && bFlashOfficial) ? Clip->OfficialToClip(StateTime) : StateTime;
	PoseClip(FlashClip, FlashClipT);
	if (bFlashOfficial && FlashClipT - Prev > 1.f && Audio)
	{
		Audio->Play(TEXT("cut_splice"), 0.5f, 1.2f); // the join where seconds were taken out
	}
	// gate weave: the frame jitters a little, twelve times a second (a head camera moves by its own keys)
	WeaveTimer -= Dt;
	if (WeaveTimer <= 0.f && Cam && (!Set || !Set->bHeadCam))
	{
		WeaveTimer = 1.f / 12.f;
		Weave = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f));
		Cam->SetActorRotation(FRotator(Weave.Y * 0.10, Weave.X * 0.10, Weave.X * 0.12));
	}
	if (!bFlashOfficial && !bThumbTaken && Clip && Cam && FlashClipT > Clip->ThumbT && ThumbTargets.IsValidIndex(FlashGap))
	{
		// the thumbnail that fills the gap in the reel
		bThumbTaken = true;
		USceneCaptureComponent2D* Cap = ThumbCapture->GetCaptureComponent2D();
		Cap->TextureTarget = ThumbTargets[FlashGap];
		ThumbCapture->SetActorLocationAndRotation(Cam->GetActorLocation(), Cam->GetActorRotation());
		Cap->CaptureScene();
	}
	if (StateTime >= FlashLength)
	{
		EndFlashback();
	}
}
