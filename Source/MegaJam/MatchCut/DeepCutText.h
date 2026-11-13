// Deep Cut: player-facing English lines that live in code.
// All of these are PLACEHOLDERS until the user approves them. Per-shot hints and shot names are in
// Content/Data/deepcut.json, film captions in Content/Data/film.json. Lines new in brief 03 are marked NEW.

#pragma once

#include "CoreMinimal.h"
#include "DeepCutTypes.h"

// lines with numbers in them (FString::Printf needs a literal format)
#define DCTEXT_CAM_TAG TEXT("FATHOM CAM %02d")
#define DCTEXT_TAKE TEXT("TAKE %d")
#define DCTEXT_SCENE TEXT("SCENE %02d")
#define DCTEXT_RESTORED TEXT("SCENE RESTORED  %d/%d")
#define DCTEXT_CREW TEXT("CREW %d/%d")
#define DCTEXT_END_LINE TEXT("Scenes restored %d/%d.   Crew found %d/%d.")
#define DCTEXT_END_TITLE TEXT("END OF STAGE %d")           // NEW (brief 04)
#define DCTEXT_PART_FOUND TEXT("FEATURE CUT -> BUILT: %s") // NEW (brief 04): one line for every feature
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
	static const TCHAR* SoftFail = TEXT("Not here. Swim onto the tape mark. Then press C.");
	static const TCHAR* SoftFailNoMark = TEXT("No tape mark yet."); // followed by the shot's own hint (brief 05)
	static const TCHAR* RingOk = TEXT("C: CUT");
	static const TCHAR* ReasonRock = TEXT("ROCK IN THE WAY");
	static const TCHAR* ReasonDarkLamp = TEXT("TOO DARK - F: LAMP");
	static const TCHAR* ReasonLineUp = TEXT("NOT LINED UP");
	static const TCHAR* ReasonLast = TEXT("NO NEXT SHOT");
	static const TCHAR* ReasonNoMark = TEXT("NO MARK YET");       // NEW
	static const TCHAR* ReasonOffMark = TEXT("NOT ON A MARK");    // NEW
	static const TCHAR* ReasonTooClose = TEXT("TOO CLOSE - Q"); // NEW: nearer to the camera than the mark
	static const TCHAR* ReasonTooFar = TEXT("TOO FAR - Z");     // NEW
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
	static const TCHAR* PartHow = TEXT("F: lamp on / off");
	static const TCHAR* RestoreHint = TEXT("A removed scene. Line up with the frame. Press C.");
	static const TCHAR* RestoreMark = TEXT("MATCH");
	static const TCHAR* Recovered = TEXT("RECOVERED FOOTAGE");
	static const TCHAR* CrewSignal = TEXT("CREW SIGNAL");
	static const TCHAR* CrewFound = TEXT("CREW FOUND");

	// general
	// the controls line is put together from these: a key only shows once it has been taught (brief 05)
	static const TCHAR* Controls[DCTeach::Num] = { TEXT("WASD swim"), TEXT("C cut"), TEXT("R back one shot"), TEXT("swim into things: push"),
		TEXT("WHEEL or Q / Z farther / closer"), TEXT("F lamp"), TEXT("E grab") };
	static const TCHAR* ControlsQuit = TEXT("ESC quit");
	// one-time prompt, shown the first time the level needs the control (brief 05). All NEW.
	static const TCHAR* Prompts[DCTeach::Num] = {
		TEXT("W A S D  or arrow keys: swim"),
		TEXT("On the tape mark:  C  cuts to the next shot"),
		TEXT("R: go back one shot"),
		TEXT("Swim into a loose thing to push it"),
		TEXT("Mouse wheel or  Q / Z: farther / closer"),
		TEXT("F: lamp on / off"),
		TEXT("E: grab / drop. Now you can carry heavy things") };
	static const TCHAR* PushCue = TEXT("PUSH");
	static const TCHAR* DepthFirst = TEXT("Depth only matters at a mark with a white ring.");
	// a failed take says what to do next (brief 05). All NEW.
	static const TCHAR* HowOffMark = TEXT("Swim onto the tape mark. Then press C.");
	static const TCHAR* HowTooClose = TEXT("Go farther: wheel up or Q, until your ring fits the white ring.");
	static const TCHAR* HowTooFar = TEXT("Come closer: wheel down or Z, until your ring fits the white ring.");
	static const TCHAR* HowWait = TEXT("Wait until the mark is bright. Then press C.");
	static const TCHAR* HowDark = TEXT("Press F for the lamp. Then press C.");
	static const TCHAR* HowLineUp = TEXT("Swim into the MATCH ring. Then press C.");
	static const TCHAR* HowRock = TEXT("Rock is in the way in the next shot. Move a little on the mark.");
	static const TCHAR* HowCaught = TEXT("Get to the tape mark and press C before it reaches you.");
	static const TCHAR* ReasonCame = TEXT("IT CAME WITH YOU");
	static const TCHAR* HowCame = TEXT("Cut while it is outside your circle. Then it stays behind.");
	// brief 05, part B. All NEW.
	static const TCHAR* WrongShape = TEXT("WRONG SHAPE");
	static const TCHAR* PredHintTool = TEXT("Cut away while it is OUTSIDE your circle. Then it stays in this shot.");
	static const TCHAR* PredStayed = TEXT("IT STAYED BEHIND");
	static const TCHAR* PredStayedHow = TEXT("It is still in the last shot. R: back one shot. See what it did.");
	static const TCHAR* ShoalGone = TEXT("THE SHOAL IS GONE");
	static const TCHAR* BackMark = TEXT("BACK TO");
	static const TCHAR* LiftNeedsMore = TEXT("The lift needs one more thing.");
	static const TCHAR* StuckBack = TEXT("Nothing more to do here. R: back one shot.");
	static const TCHAR* EndKeys = TEXT("R: play again     ESC: quit");
	static const TCHAR* EndKeysNext = TEXT("C: next stage     R: play again     ESC: quit"); // NEW

	// stage 2 (brief 04). All NEW.
	static const TCHAR* PartHowGrip = TEXT("E: grab / drop. Now you can carry heavy things.");
	static const TCHAR* PropHeavy = TEXT("HEAVY - E");                 // a thing that only comes through the cut when it is held
	static const TCHAR* CrewCarry = TEXT("It cannot swim. Carry it: E.");
	static const TCHAR* CrewSafe = TEXT("CREW SAFE");
	static const TCHAR* PredHint = TEXT("Something is coming. You cannot fight it. Cut away: C.");
	static const TCHAR* ReasonCaught = TEXT("CAUGHT - CUT AWAY SOONER");
	static const TCHAR* SignalLost = TEXT("SIGNAL LOST");
	static const TCHAR* NextShotBody = TEXT("IN THE NEXT SHOT");
	static const TCHAR* OfficialRecord = TEXT("FATHOM DYNAMICS  /  DIVE RECORD  /  OFFICIAL RELEASE");
	static const TCHAR* OfficialTag = TEXT("OFFICIAL VERSION");
	static const TCHAR* OfficialEnd = TEXT("END OF RECORD");
	static const TCHAR* OfficialEndHint = TEXT("That is the official record. Some seconds are missing.");
	static const TCHAR* CutByFathom = TEXT("RESTORED SECONDS");
	static const TCHAR* DiverCam = TEXT("DIVER CAM  REC");
}
