// Deep Cut: player-facing English lines that live in code.
// All of these are PLACEHOLDERS until the user approves them. Per-shot hints and shot names are in
// Content/Data/deepcut.json, film captions in Content/Data/film.json. Lines new in brief 03 are marked NEW.

#pragma once

#include "CoreMinimal.h"

// lines with numbers in them (FString::Printf needs a literal format)
#define DCTEXT_CAM_TAG TEXT("FATHOM CAM %02d")
#define DCTEXT_TAKE TEXT("TAKE %d")
#define DCTEXT_SCENE TEXT("SCENE %02d")
#define DCTEXT_RESTORED TEXT("SCENE RESTORED  %d/%d")
#define DCTEXT_CREW TEXT("CREW %d/%d")
#define DCTEXT_END_LINE TEXT("Scenes restored %d/%d.   Crew found %d/%d.")
#define DCTEXT_MARK_CAM TEXT("CAM %02d  %s") // label next to a tape mark when a shot has two cameras: camera number, shot name (brief 04)

namespace DCText
{
	// opening film
	static const TCHAR* FilmTitle = TEXT("FATHOM DYNAMICS  /  MISSION FILM  /  OFFICIAL RELEASE");
	static const TCHAR* GapStamp = TEXT("[SCENE REMOVED]");
	static const TCHAR* GapBy = TEXT("REMOVED BY: ________");
	static const TCHAR* FilmSkip = TEXT("PRESS ANY KEY TO SKIP");
	static const TCHAR* FilmEnd = TEXT("END OF FILM.  3 SCENES MISSING.");
	static const TCHAR* ReelGapLabel = TEXT("REMOVED");

	// design-file card before the film, and the feature list that stays in the top bar. All NEW.
	static const TCHAR* CardFile = TEXT("FATHOM DYNAMICS  /  DESIGN FILE");
	static const TCHAR* CardTitle = TEXT("UNIT 0 - PROTOTYPE");
	static const TCHAR* CardShelved = TEXT("SHELVED 02:00 AM");
	static const TCHAR* CardReason = TEXT("reason: over budget");
	static const TCHAR* CardFeatures = TEXT("FEATURES");
	static const TCHAR* CardCut = TEXT("CUT");
	static const TCHAR* CardBuilt = TEXT("BUILT");
	static const TCHAR* HudUnit = TEXT("UNIT 0");
	static const TCHAR* Features[3] = { TEXT("HEADLAMP"), TEXT("GRIPPER"), TEXT("MULTI-CAM") };

	// survey camera frame
	static const TCHAR* Rec = TEXT("REC");

	// cut preview and failed takes
	static const TCHAR* SoftFail = TEXT("Not here. Swim onto a tape mark.");              // NEW (was "Move to the green area.")
	static const TCHAR* SoftFailNoMark = TEXT("No tape mark yet. Solve this shot first."); // NEW
	static const TCHAR* RingOk = TEXT("C: CUT");
	static const TCHAR* ReasonRock = TEXT("ROCK IN THE WAY");
	static const TCHAR* ReasonDarkLamp = TEXT("TOO DARK - F: LAMP");
	static const TCHAR* ReasonLineUp = TEXT("NOT LINED UP");
	static const TCHAR* ReasonLast = TEXT("NO NEXT SHOT");
	static const TCHAR* ReasonNoMark = TEXT("NO MARK YET");       // NEW
	static const TCHAR* ReasonOffMark = TEXT("NOT ON A MARK");    // NEW
	static const TCHAR* ReasonTooClose = TEXT("TOO CLOSE - SHIFT"); // NEW: nearer to the camera than the mark
	static const TCHAR* ReasonTooFar = TEXT("TOO FAR - SPACE");     // NEW
	static const TCHAR* ReasonWait = TEXT("NOT YET - WAIT");         // NEW: a mark that comes and goes
	static const TCHAR* CutFlash = TEXT("CUT");

	// use key, props, sockets
	static const TCHAR* UseGrab = TEXT("E: GRAB");   // NEW
	static const TCHAR* UseDrop = TEXT("E: DROP");   // NEW
	static const TCHAR* UseTurn = TEXT("E: TURN");   // NEW
	static const TCHAR* PropStays = TEXT("STAYS");   // NEW: a loose thing too far away to come through the cut
	static const TCHAR* PropComes = TEXT("COMES");   // NEW
	static const TCHAR* PropLands = TEXT("FITS");    // NEW: it would land in the socket
	static const TCHAR* CurrentStopped = TEXT("CURRENT STOPPED"); // NEW
	static const TCHAR* PowerOn = TEXT("POWER ON");               // NEW
	static const TCHAR* MarksUp = TEXT("MARKS SET");              // NEW: shown for a moment when tape marks appear

	// part, restore, rescue
	static const TCHAR* PartFound = TEXT("FEATURE CUT -> BUILT: HEADLAMP"); // NEW (was HEADLAMP FOUND)
	static const TCHAR* PartHow = TEXT("F: lamp on / off");
	static const TCHAR* RestoreHint = TEXT("A removed scene. Line up with the frame. Press C.");
	static const TCHAR* RestoreMark = TEXT("MATCH");
	static const TCHAR* Recovered = TEXT("RECOVERED FOOTAGE");
	static const TCHAR* CrewSignal = TEXT("CREW SIGNAL");
	static const TCHAR* CrewFound = TEXT("CREW FOUND");

	// general
	// the controls line is put together from these: a key only shows once its feature is built (brief 04)
	static const TCHAR* ControlsMove = TEXT("WASD swim   SPACE / SHIFT closer / farther");
	static const TCHAR* ControlsGrab = TEXT("   E grab");
	static const TCHAR* ControlsLampKey = TEXT("   F lamp");
	static const TCHAR* ControlsEnd = TEXT("   C cut   R back one shot   ESC quit");
	static const TCHAR* EndTitle = TEXT("END OF STAGE 1"); // NEW (was END OF SLICE)
	static const TCHAR* EndKeys = TEXT("R: play again     ESC: quit");
}
