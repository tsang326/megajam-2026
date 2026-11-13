// Deep Cut: the opening film and the flashback. Both are data: sets, clips of key poses and the reel order come
// from Content/Data/film.json (made by Tools/make_film_json.py). Nothing here knows what a clip shows.

#include "MatchCutGameMode.h"
#include "DCAudio.h"
#include "DeepCutText.h"
#include "Camera/CameraActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Engine/SceneCapture2D.h"
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
		const FDCRobot* Robot = FilmActor(Track.Actor);
		if (!Robot || Track.Keys.Num() == 0)
		{
			continue;
		}
		int32 i = 0;
		while (i + 1 < Track.Keys.Num() && Track.Keys[i + 1].T <= T)
		{
			++i;
		}
		const FDCKey& A = Track.Keys[i];
		const FDCKey& B = Track.Keys[FMath::Min(i + 1, Track.Keys.Num() - 1)];
		const float Alpha = B.T > A.T ? FMath::Clamp((T - A.T) / (B.T - A.T), 0.f, 1.f) : 0.f;
		const FVector P = FMath::Lerp(A.P, B.P, (double)Alpha);
		const FVector2D Head = FMath::Lerp(A.Head, B.Head, (double)Alpha);
		// set space (x right, y up, z away) to world: the set camera looks down +X
		PlaceRobot(*Robot, Origin + FVector(P.Z, P.X, P.Y), Head, FRotator::ZeroRotator, FMath::Lerp(A.Roll, B.Roll, Alpha));
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

void AMatchCutGameMode::StartFlashback()
{
	State = EDCState::Flashback;
	StateTime = 0.f;
	bThumbTaken = false;
	SetView(FlashCam.Get());
	ApplyLighting();
	if (Audio)
	{
		Audio->Play(TEXT("reel_restore"));
	}
	UE_LOG(LogMatchCut, Display, TEXT("FLASHBACK start (removed scene 1, clip \"%s\", %.0fs)"), *FlashClip, FlashLength);
}

void AMatchCutGameMode::EndFlashback()
{
	bGapFilled[0] = true;
	State = EDCState::Playing;
	StateTime = 0.f;
	if (FlashCam.IsValid())
	{
		FlashCam->SetActorRotation(FRotator::ZeroRotator);
	}
	SetView(Shots[Cur].Camera.Get());
	PointGhost();
	ApplyLighting();
	ShowBanner(FString::Printf(DCTEXT_RESTORED, 1, NumGaps), TEXT(""), 5.f);
	UE_LOG(LogMatchCut, Display, TEXT("FLASHBACK end: gap 1 filled"));
	UpdateGate(Cur, false);
}

void AMatchCutGameMode::TickFlashback(float Dt)
{
	PoseClip(FlashClip, StateTime);
	// gate weave: the frame jitters a little, twelve times a second
	WeaveTimer -= Dt;
	if (WeaveTimer <= 0.f && FlashCam.IsValid())
	{
		WeaveTimer = 1.f / 12.f;
		Weave = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f));
		FlashCam->SetActorRotation(FRotator(Weave.Y * 0.10, Weave.X * 0.10, Weave.X * 0.12));
	}
	if (!bThumbTaken && StateTime > 4.0f)
	{
		bThumbTaken = true;
		ThumbCapture->GetCaptureComponent2D()->CaptureScene(); // the thumbnail that fills the gap in the reel
	}
	if (StateTime >= FlashLength)
	{
		EndFlashback();
	}
}
