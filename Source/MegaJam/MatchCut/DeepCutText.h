// Deep Cut slice (brief 02): every player-facing English line lives here.
// All of these are PLACEHOLDERS until the user approves them. Per-shot hints are in Content/Data/deepcut.json.
// The HUD font is the engine's built-in bitmap font: plain ASCII only (no en dash, no bullet).

#pragma once

#include "CoreMinimal.h"

// lines with numbers in them (FString::Printf needs a literal format)
#define DCTEXT_CAM_TAG TEXT("FATHOM CAM %02d")
#define DCTEXT_TAKE TEXT("TAKE %d")
#define DCTEXT_SCENE TEXT("SCENE %02d")
#define DCTEXT_RESTORED TEXT("SCENE RESTORED  %d/%d")
#define DCTEXT_CREW TEXT("CREW %d/%d")
#define DCTEXT_END_LINE TEXT("Scenes restored %d/%d.   Crew found %d/%d.")

namespace DCText
{
	// opening film
	static const TCHAR* FilmTitle = TEXT("FATHOM DYNAMICS  /  MISSION FILM  /  OFFICIAL RELEASE");
	static const TCHAR* FilmCap1 = TEXT("FATHOM DYNAMICS SURVEY CREW. LAUNCH DAY.");
	static const TCHAR* FilmCap2 = TEXT("DESCENT TO THE SEA FLOOR.");
	static const TCHAR* FilmCap3 = TEXT("EXPLORATION MISSION SUCCESSFUL.");
	static const TCHAR* FilmCap4 = TEXT("UNIT 7 - DEFECTIVE, DECOMMISSIONED.");
	static const TCHAR* GapStamp = TEXT("[SCENE REMOVED]");
	static const TCHAR* GapBy = TEXT("REMOVED BY: ________");
	static const TCHAR* FilmSkip = TEXT("PRESS ANY KEY TO SKIP");
	static const TCHAR* FilmEnd = TEXT("END OF FILM.  3 SCENES MISSING.");
	static const TCHAR* ReelGapLabel = TEXT("REMOVED");

	// survey camera frame
	static const TCHAR* Rec = TEXT("REC");
	static const TCHAR* Live = TEXT("LIVE");

	// cut preview and failed takes
	static const TCHAR* Tutorial = TEXT("Line up with the ghost. Press C to cut.");
	static const TCHAR* SoftFail = TEXT("Not here. Move to the green area.");
	static const TCHAR* RingOk = TEXT("C: CUT");
	static const TCHAR* ReasonRock = TEXT("ROCK IN THE WAY");
	static const TCHAR* ReasonDark = TEXT("TOO DARK");
	static const TCHAR* ReasonDarkLamp = TEXT("TOO DARK - F: LAMP");
	static const TCHAR* ReasonWater = TEXT("NO WATER THERE");
	static const TCHAR* ReasonFrame = TEXT("OUT OF FRAME");
	static const TCHAR* ReasonLineUp = TEXT("NOT LINED UP");
	static const TCHAR* ReasonLast = TEXT("NO NEXT SHOT");
	static const TCHAR* CutFlash = TEXT("CUT");

	// part, restore, rescue
	static const TCHAR* PartFound = TEXT("HEADLAMP FOUND");
	static const TCHAR* PartHow = TEXT("F: lamp on / off");
	static const TCHAR* RestoreHint = TEXT("A removed scene. Line up with the frame. Press C.");
	static const TCHAR* RestoreMark = TEXT("MATCH");
	static const TCHAR* Recovered = TEXT("RECOVERED FOOTAGE");
	static const TCHAR* DarkHint = TEXT("The next shot is dark. Turn on the lamp (F) and look for the signal.");
	static const TCHAR* CrewSignal = TEXT("CREW SIGNAL");
	static const TCHAR* CrewFound = TEXT("CREW FOUND");

	// general
	static const TCHAR* Controls = TEXT("WASD move    C cut    R retake    ESC quit");
	static const TCHAR* ControlsLamp = TEXT("WASD move    C cut    F lamp    R retake    ESC quit");
	static const TCHAR* EndTitle = TEXT("END OF SLICE");
	static const TCHAR* EndKeys = TEXT("R: play again     ESC: quit");
}
