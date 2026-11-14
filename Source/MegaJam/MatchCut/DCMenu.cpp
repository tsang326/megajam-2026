// Deep Cut: the menus, the settings and the Vietnamese text (see DCMenu.h).

#include "DCMenu.h"
#include "DC3D.h"
#include "DCAudio.h"
#include "DCDoc.h"
#include "MatchCutGameMode.h"

#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FDCSettings& DCSettings()
{
	static FDCSettings S;
	return S;
}

FDCMenu& DCMenu()
{
	static FDCMenu M;
	return M;
}

FDCTutorial& DCTutorial()
{
	static FDCTutorial T;
	return T;
}

// ---------------------------------------------------------------------------------------------
// the tutorial
// ---------------------------------------------------------------------------------------------

bool FDCTutorial::IsActive() const
{
	return bRunning && DCSettings().bTutorial && Step < NumSteps && !DCMenu().IsOpen();
}

void FDCTutorial::Card(int32 S, FString& Head, FString& Body)
{
	static const TCHAR* Heads[NumSteps] = { TEXT("LOOK AROUND"), TEXT("WALK"), TEXT("THE BOARD"), TEXT("DIVE"), TEXT("SWIM"),
		TEXT("RAISE THE CAMERA"), TEXT("RECORD"), TEXT("COME BACK"), TEXT("UPLOAD"), TEXT("GEAR AND SLEEP") };
	static const TCHAR* Bodies[NumSteps] = {
		TEXT("Move the mouse to look. No mouse: the arrow keys."),
		TEXT("W A S D to walk. SHIFT to run."),
		TEXT("The board by the table lists what to film. Look at it and press E."),
		TEXT("Walk into the moon pool, the yellow ring in the floor, to dive."),
		TEXT("SPACE up, Q down, SHIFT fast. Watch the AIR bar at the top."),
		TEXT("Hold RIGHT MOUSE to raise the camera (or press C)."),
		TEXT("LEFT MOUSE or E to record. Keep the subject in the frame."),
		TEXT("Follow the BASE marker and swim up into the moon pool before the air runs out."),
		TEXT("At the edit desk, press E to upload the tape. Views bring money."),
		TEXT("The locker sells gear. Sleep in the bunk to start the next dive.") };
	const int32 i = FMath::Clamp(S, 0, NumSteps - 1);
	Head = Heads[i];
	Body = Bodies[i];
}

void FDCTutorial::Tick(FDC3D& W, float Dt)
{
	Flash = FMath::Max(0.f, Flash - Dt);
	bRunning = true;
	if (!DCSettings().bTutorial || !W.Doc || W.bFrozen)
	{
		return;
	}
	const FDCDoc& D = *W.Doc;
	if (bHadYaw)
	{
		Looked += FMath::Abs(FMath::FindDeltaAngleDegrees(LastYaw, W.Yaw));
	}
	LastYaw = W.Yaw;
	bHadYaw = true;
	if (W.bDry)
	{
		Walked += (float)FVector2D(W.Vel.X, W.Vel.Y).Size() * Dt;
	}
	else
	{
		bOut = true;
		if (W.Move.SizeSquared() > 0.01)
		{
			Swum += Dt;
		}
	}
	bool Now[NumSteps];
	Now[0] = Looked > 75.f;
	Now[1] = Walked > 260.f;
	Now[2] = D.bBoardOpen;
	Now[3] = bOut || D.bLeftBase;
	Now[4] = Swum > 4.f;
	Now[5] = D.bEverAimed;
	Now[6] = D.bEverRecorded;
	Now[7] = bOut && W.bDry && D.bInBase;
	Now[8] = D.bUploadedToday || D.Day > 1;
	Now[9] = D.Day > 1;
	// a step done marks the ones before it done too: the player may skip ahead
	for (int32 s = NumSteps - 1; s >= 0; --s)
	{
		if (Now[s] && !Done[s])
		{
			for (int32 e = 0; e <= s; ++e)
			{
				if (!Done[e] && e == Step)
				{
					Flash = 1.2f;
					FlashStep = e;
				}
				Done[e] = true;
			}
			break;
		}
	}
	const int32 Was = Step;
	Step = 0;
	while (Step < NumSteps && Done[Step])
	{
		++Step;
	}
	if (Step != Was)
	{
		Shown = 0.f;
		if (W.GM && W.GM->GetAudio())
		{
			W.GM->GetAudio()->Play(TEXT("ui_select"), 0.6f, 1.2f);
		}
		UE_LOG(LogMatchCut, Display, TEXT("TUTORIAL step %d -> %d"), Was, Step);
	}
	Shown += Dt;
}

// ---------------------------------------------------------------------------------------------
// settings
// ---------------------------------------------------------------------------------------------

namespace
{
	FString SettingsPath() { return FPaths::ProjectSavedDir() / TEXT("DeepCutSettings.txt"); }
}

void FDCSettings::Load()
{
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *SettingsPath()))
	{
		return;
	}
	for (const FString& L : Lines)
	{
		FString Key, Val;
		if (!L.Split(TEXT("="), &Key, &Val))
		{
			continue;
		}
		const float V = FCString::Atof(*Val);
		if (Key == TEXT("lang")) { Lang = FMath::Clamp((int32)V, 0, 1); }
		else if (Key == TEXT("sens")) { Sens = FMath::Clamp(V, 0.03f, 0.6f); }
		else if (Key == TEXT("turn")) { TurnSpeed = FMath::Clamp(V, 0.5f, 2.5f); }
		else if (Key == TEXT("invert")) { bInvertY = V > 0.5f; }
		else if (Key == TEXT("bob")) { bHeadBob = V > 0.5f; }
		else if (Key == TEXT("fov")) { Fov = FMath::Clamp(V, 60.f, 100.f); }
		else if (Key == TEXT("master")) { Master = FMath::Clamp(V, 0.f, 1.f); }
		else if (Key == TEXT("music")) { Music = FMath::Clamp(V, 0.f, 1.f); }
		else if (Key == TEXT("fullscreen")) { bFullscreen = V > 0.5f; }
		else if (Key == TEXT("quality")) { Quality = FMath::Clamp((int32)V, 0, 3); }
		else if (Key == TEXT("tutorial")) { bTutorial = V > 0.5f; }
	}
}

void FDCSettings::Save() const
{
	const FString Out = FString::Printf(TEXT("lang=%d\nsens=%.4f\nturn=%.2f\ninvert=%d\nbob=%d\nfov=%.0f\nmaster=%.2f\nmusic=%.2f\nfullscreen=%d\nquality=%d\ntutorial=%d\n"),
		Lang, Sens, TurnSpeed, bInvertY ? 1 : 0, bHeadBob ? 1 : 0, Fov, Master, Music, bFullscreen ? 1 : 0, Quality, bTutorial ? 1 : 0);
	FFileHelper::SaveStringToFile(Out, *SettingsPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void FDCSettings::Apply(AMatchCutGameMode* GM, bool bDisplay) const
{
	if (GM)
	{
		if (FDC3D* W = GM->World3D())
		{
			W->Sens = Sens;
			W->bInvertY = bInvertY;
			W->BaseFov = Fov;
			W->bHeadBobOn = bHeadBob;
		}
		if (GM->GetAudio())
		{
			GM->GetAudio()->SetMix(Master, Music);
		}
	}
	if (bDisplay && GEngine)
	{
		if (UGameUserSettings* U = GEngine->GetGameUserSettings())
		{
			U->SetFullscreenMode(bFullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
			if (!bFullscreen)
			{
				U->SetScreenResolution(FIntPoint(1600, 900));
			}
			U->SetOverallScalabilityLevel(Quality);
			U->ApplySettings(false);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// the pages
// ---------------------------------------------------------------------------------------------

int32 FDCMenu::NumRows() const
{
	switch (Page)
	{
	case EDCMenuPage::Title: return 4;
	case EDCMenuPage::Pause: return 4;
	case EDCMenuPage::Settings: return 12;
	case EDCMenuPage::HowTo: return 1;
	default: return 0;
	}
}

void FDCMenu::Open(EDCMenuPage P)
{
	if (P == EDCMenuPage::Settings || P == EDCMenuPage::HowTo)
	{
		From = Page == EDCMenuPage::None ? EDCMenuPage::Pause : Page;
	}
	Page = P;
	Sel = 0;
	Time = 0.f;
	Rows.Reset();
}

void FDCMenu::Move(int32 D, AMatchCutGameMode* GM)
{
	const int32 N = NumRows();
	if (N <= 0 || D == 0)
	{
		return;
	}
	Sel = ((Sel + D) % N + N) % N;
	if (GM && GM->GetAudio())
	{
		GM->GetAudio()->Play(TEXT("ui_move"), 1.f, 1.f, 0.04f);
	}
}

void FDCMenu::Change(int32 D, AMatchCutGameMode* GM)
{
	if (Page != EDCMenuPage::Settings || D == 0)
	{
		return;
	}
	FDCSettings& S = DCSettings();
	switch (Sel)
	{
	case 0: S.Lang = 1 - S.Lang; break;
	case 1: S.Sens = FMath::Clamp(S.Sens * (D > 0 ? 1.15f : 1.f / 1.15f), 0.03f, 0.6f); break;
	case 2: S.TurnSpeed = FMath::Clamp(S.TurnSpeed + 0.1f * D, 0.5f, 2.5f); break;
	case 3: S.bInvertY = !S.bInvertY; break;
	case 4: S.bHeadBob = !S.bHeadBob; break;
	case 5: S.Fov = FMath::Clamp(S.Fov + 2.f * D, 60.f, 100.f); break;
	case 6: S.Master = FMath::Clamp(FMath::RoundToFloat((S.Master + 0.1f * D) * 10.f) / 10.f, 0.f, 1.f); break;
	case 7: S.Music = FMath::Clamp(FMath::RoundToFloat((S.Music + 0.1f * D) * 10.f) / 10.f, 0.f, 1.f); break;
	case 8: S.bFullscreen = !S.bFullscreen; break;
	case 9: S.Quality = ((S.Quality + D) % 4 + 4) % 4; break;
	case 10: S.bTutorial = !S.bTutorial; break;
	default: return;
	}
	S.Apply(GM, Sel == 8 || Sel == 9);
	S.Save();
	if (GM && GM->GetAudio())
	{
		GM->GetAudio()->Play(TEXT("ui_move"), 1.f, D > 0 ? 1.08f : 0.94f);
	}
}

void FDCMenu::Activate(AMatchCutGameMode* GM)
{
	if (GM && GM->GetAudio())
	{
		GM->GetAudio()->Play(TEXT("ui_select"));
	}
	if (Page == EDCMenuPage::Title || Page == EDCMenuPage::Pause)
	{
		switch (Sel)
		{
		case 0: Page = EDCMenuPage::None; bStarted = true; Rows.Reset(); break;
		case 1: Open(EDCMenuPage::Settings); break;
		case 2: Open(EDCMenuPage::HowTo); break;
		case 3: bQuit = true; break;
		default: break;
		}
	}
	else if (Page == EDCMenuPage::Settings)
	{
		if (Sel == NumRows() - 1)
		{
			Back(GM);
		}
		else
		{
			Change(1, GM);
		}
	}
	else if (Page == EDCMenuPage::HowTo)
	{
		Back(GM);
	}
}

void FDCMenu::Back(AMatchCutGameMode* GM)
{
	if (Page == EDCMenuPage::Settings || Page == EDCMenuPage::HowTo)
	{
		const EDCMenuPage To = From;
		const int32 Was = Page == EDCMenuPage::Settings ? 1 : 2;
		Open(To);
		Sel = Was;
	}
	else if (Page == EDCMenuPage::Pause)
	{
		Page = EDCMenuPage::None; // back into the dive
		Rows.Reset();
	}
	if (GM && GM->GetAudio())
	{
		GM->GetAudio()->Play(TEXT("ui_move"), 1.f, 0.9f);
	}
}

void FDCMenu::Row(int32 i, FString& Label, FString& Value) const
{
	const FDCSettings& S = DCSettings();
	Value.Reset();
	if (Page == EDCMenuPage::Title || Page == EDCMenuPage::Pause)
	{
		static const TCHAR* TitleRows[] = { TEXT("Start the dive"), TEXT("Settings"), TEXT("How to play"), TEXT("Quit") };
		static const TCHAR* PauseRows[] = { TEXT("Resume"), TEXT("Settings"), TEXT("How to play"), TEXT("Quit the game") };
		Label = Page == EDCMenuPage::Title ? (i == 0 && bStarted ? TEXT("Continue") : TitleRows[FMath::Clamp(i, 0, 3)]) : PauseRows[FMath::Clamp(i, 0, 3)];
		return;
	}
	if (Page == EDCMenuPage::HowTo)
	{
		Label = TEXT("Back");
		return;
	}
	static const TCHAR* Names[] = { TEXT("Language"), TEXT("Mouse sensitivity"), TEXT("Turn speed (keys, pad)"), TEXT("Invert look"), TEXT("Head bob"),
		TEXT("Field of view"), TEXT("Master volume"), TEXT("Music volume"), TEXT("Display"), TEXT("Graphics"), TEXT("Tutorial hints"), TEXT("Back") };
	static const TCHAR* Quality[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };
	Label = Names[FMath::Clamp(i, 0, 11)];
	switch (i)
	{
	case 0: Value = S.Lang ? TEXT("Tiếng Việt") : TEXT("English"); break;
	case 1: Value = FString::Printf(TEXT("%.0f"), S.Sens * 100.f); break;
	case 2: Value = FString::Printf(TEXT("%.1fx"), S.TurnSpeed); break;
	case 3: case 4: case 10: Value = (i == 3 ? S.bInvertY : i == 4 ? S.bHeadBob : S.bTutorial) ? TEXT("On") : TEXT("Off"); break;
	case 5: Value = FString::Printf(TEXT("%.0f°"), S.Fov); break;
	case 6: Value = FString::Printf(TEXT("%.0f%%"), S.Master * 100.f); break;
	case 7: Value = FString::Printf(TEXT("%.0f%%"), S.Music * 100.f); break;
	case 8: Value = S.bFullscreen ? TEXT("Fullscreen") : TEXT("Window"); break;
	case 9: Value = Quality[FMath::Clamp(S.Quality, 0, 3)]; break;
	default: break;
	}
}

int32 DCMenuRunLine(AMatchCutGameMode* GM, const TArray<FString>& T)
{
	const FString& Cmd = T[0];
	const FString Arg = T.IsValidIndex(1) ? T[1] : FString();
	FDCMenu& M = DCMenu();
	if (Cmd == TEXT("menu"))
	{
		if (Arg == TEXT("title")) { M.Page = EDCMenuPage::Title; M.bStarted = false; M.Sel = 0; }
		else if (Arg == TEXT("pause")) { M.bStarted = true; M.Open(EDCMenuPage::Pause); }
		else if (Arg == TEXT("settings")) { M.Open(EDCMenuPage::Settings); }
		else if (Arg == TEXT("howto")) { M.Open(EDCMenuPage::HowTo); }
		else { M.Page = EDCMenuPage::None; M.bStarted = true; }
		UE_LOG(LogMatchCut, Display, TEXT("MENU page %s"), *Arg);
		return 1;
	}
	if (Cmd == TEXT("menusel"))
	{
		M.Sel = FCString::Atoi(*Arg);
		return 1;
	}
	if (Cmd == TEXT("lang"))
	{
		DCSettings().Lang = Arg == TEXT("vi") ? 1 : 0;
		UE_LOG(LogMatchCut, Display, TEXT("MENU lang %s: '%s'"), *Arg, *DCLoc::Tr(TEXT("Start the dive")));
		return 1;
	}
	if (Cmd == TEXT("tut"))
	{
		FDCTutorial& Tut = DCTutorial();
		Tut.bForced = Arg != TEXT("off");
		DCSettings().bTutorial = Tut.bForced;
		return 1;
	}
	(void)GM;
	return 0;
}

// ---------------------------------------------------------------------------------------------
// the Vietnamese text
// ---------------------------------------------------------------------------------------------

namespace
{
	struct FLocPattern
	{
		TArray<FString> Parts; // the English text around each {}
		FString Vi;            // with {} in the same order
	};
	// FString keys hash without case in a plain TMap: "record" and "RECORD" are different rows here
	struct FCaseKeyFuncs : BaseKeyFuncs<TPair<FString, FString>, FString, false>
	{
		static const FString& GetSetKey(const TPair<FString, FString>& E) { return E.Key; }
		static bool Matches(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
		static uint32 GetKeyHash(const FString& Key) { return FCrc::StrCrc32(*Key); }
	};
	using FLocMap = TMap<FString, FString, FDefaultSetAllocator, FCaseKeyFuncs>;
	FLocMap GExact;
	FLocMap GUpper;   // the same rows in capitals, for text the game shows upper-cased
	TArray<FLocPattern> GPatterns;
	FLocMap GCache;
	bool bLocLoaded = false;

	bool MatchPattern(const FLocPattern& P, const FString& S, TArray<FString>& Caps)
	{
		Caps.Reset();
		// UE's StartsWith / EndsWith say no to an empty part: a pattern that starts or ends with {} has one
		if ((!P.Parts[0].IsEmpty() && !S.StartsWith(P.Parts[0], ESearchCase::CaseSensitive))
			|| (!P.Parts.Last().IsEmpty() && !S.EndsWith(P.Parts.Last(), ESearchCase::CaseSensitive)))
		{
			return false;
		}
		int32 At = P.Parts[0].Len();
		const int32 End = S.Len() - P.Parts.Last().Len();
		if (End < At)
		{
			return false;
		}
		for (int32 i = 1; i < P.Parts.Num(); ++i)
		{
			if (i == P.Parts.Num() - 1)
			{
				Caps.Add(S.Mid(At, End - At));
				return true;
			}
			const int32 Found = P.Parts[i].IsEmpty() ? INDEX_NONE : S.Find(P.Parts[i], ESearchCase::CaseSensitive, ESearchDir::FromStart, At + 1);
			if (Found == INDEX_NONE || Found > End)
			{
				return false;
			}
			Caps.Add(S.Mid(At, Found - At));
			At = Found + P.Parts[i].Len();
		}
		return false;
	}
}

namespace DCLoc
{
	void Load()
	{
		bLocLoaded = true;
		GExact.Reset();
		GUpper.Reset();
		GPatterns.Reset();
		GCache.Reset();
		TArray<FString> Lines;
		const FString Path = FPaths::ProjectContentDir() / TEXT("Data/loc_vi.txt");
		if (!FFileHelper::LoadFileToStringArray(Lines, *Path))
		{
			UE_LOG(LogMatchCut, Warning, TEXT("LOC no %s"), *Path);
			return;
		}
		for (const FString& L : Lines)
		{
			FString En, Vi;
			if (L.StartsWith(TEXT("#")) || !L.Split(TEXT("\t"), &En, &Vi) || En.IsEmpty() || Vi.IsEmpty())
			{
				continue;
			}
			Vi.ReplaceInline(TEXT("\\n"), TEXT("\n"));
			En.ReplaceInline(TEXT("\\n"), TEXT("\n"));
			if (En.Contains(TEXT("{}")))
			{
				FLocPattern P;
				En.ParseIntoArray(P.Parts, TEXT("{}"), false);
				if (En.EndsWith(TEXT("{}")) && (P.Parts.Num() == 0 || !P.Parts.Last().IsEmpty()))
				{
					P.Parts.Add(FString());
				}
				if (En.StartsWith(TEXT("{}")) && P.Parts.Num() > 0 && !P.Parts[0].IsEmpty())
				{
					P.Parts.Insert(FString(), 0);
				}
				P.Vi = Vi;
				int32 Lit = 0;
				for (const FString& Part : P.Parts) { Lit += Part.Len(); }
				if (Lit >= 2 && P.Parts.Num() >= 2)
				{
					GPatterns.Add(MoveTemp(P));
				}
			}
			else
			{
				GExact.Add(En, Vi);
				if (!GExact.Contains(En.ToUpper()))
				{
					GUpper.Add(En.ToUpper(), Vi.ToUpper());
				}
			}
		}
		// the longest English first: the most particular pattern wins
		GPatterns.Sort([](const FLocPattern& A, const FLocPattern& B)
		{
			int32 La = 0, Lb = 0;
			for (const FString& P : A.Parts) { La += P.Len(); }
			for (const FString& P : B.Parts) { Lb += P.Len(); }
			return La > Lb;
		});
		UE_LOG(LogMatchCut, Display, TEXT("LOC vi: %d lines, %d patterns"), GExact.Num(), GPatterns.Num());
	}

	bool IsVi()
	{
		return DCSettings().Lang == 1;
	}

	FString Tr(const FString& En)
	{
		if (!IsVi() || En.IsEmpty())
		{
			return En;
		}
		if (!bLocLoaded)
		{
			Load();
		}
		if (const FString* Hit = GExact.Find(En))
		{
			return *Hit;
		}
		if (const FString* Hit = GCache.Find(En))
		{
			return *Hit;
		}
		FString Out = En;
		bool bDone = false;
		if (const FString* Up = GUpper.Find(En))
		{
			Out = *Up;
			bDone = true;
		}
		TArray<FString> Caps;
		for (int32 p = 0; !bDone && p < GPatterns.Num(); ++p)
		{
			if (MatchPattern(GPatterns[p], En, Caps))
			{
				FString Built;
				int32 c = 0;
				const FString& V = GPatterns[p].Vi;
				for (int32 i = 0; i < V.Len(); ++i)
				{
					if (V[i] == TEXT('{') && i + 1 < V.Len() && V[i + 1] == TEXT('}'))
					{
						Built += Caps.IsValidIndex(c) ? Tr(Caps[c]) : FString();
						++c;
						++i;
					}
					else
					{
						Built.AppendChar(V[i]);
					}
				}
				Out = Built;
				bDone = true;
			}
		}
		if (GCache.Num() > 6000)
		{
			GCache.Reset();
		}
		GCache.Add(En, Out);
		return Out;
	}
}
