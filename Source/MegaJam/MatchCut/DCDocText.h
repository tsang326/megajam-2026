// Deep Cut (brief 07): every English line of the documentary game, in one place.

#pragma once

#include "CoreMinimal.h"

namespace DCDocText
{
	static const TCHAR* ShotTitle[] = {
		TEXT("The manta glides over the wreck"),
		TEXT("The shoal splits around you"),
		TEXT("The anglerfish's light in the trench"),
		TEXT("Jellyfish rise in the light shaft"),
		TEXT("The giant passes overhead"),
		TEXT("Your crewmate waves") };
	static const TCHAR* ShotCaption[] = { TEXT("THE MANTA"), TEXT("THE SHOAL"), TEXT("THE ANGLERFISH"), TEXT("THE JELLYFISH"), TEXT("THE GIANT"), TEXT("THE CREW") };

	static const TCHAR* ShotList = TEXT("SHOT LIST");
	static const TCHAR* TeachAim = TEXT("Hold RIGHT MOUSE to raise the camera     (or press C)");
	static const TCHAR* TeachRecord = TEXT("LEFT MOUSE record a take     WHEEL zoom     (or E, and 1 / 2)");
	static const TCHAR* TeachEnd = TEXT("ENTER  end the shoot and cut your film");
	static const TCHAR* LampFound = TEXT("Headlamp fitted.  F switches it.");
	static const TCHAR* LensFound = TEXT("Long lens fitted.  The zoom goes further.");
	static const TCHAR* GiantCall = TEXT("Something big is passing overhead. Look up.");
	static const TCHAR* AllShots = TEXT("That is every shot.  ENTER: cut your film.");

	static const TCHAR* NoGood = TEXT("NO GOOD");
	static const TCHAR* Nothing = TEXT("Nothing in frame");
	static const TCHAR* TooFar = TEXT("Too far");
	static const TCHAR* CutOff = TEXT("Subject cut off");
	static const TCHAR* Missed = TEXT("Missed the moment");
	static const TCHAR* TooDark = TEXT("Too dark. It needs a lamp.");
	static const TCHAR* Shaky = TEXT("Too shaky");
	static const TCHAR* OffCentre = TEXT("Off centre");
	static const TCHAR* Lost = TEXT("Subject lost");
	static const TCHAR* Perfect = TEXT("A perfect take");

	static const TCHAR* EditTitle = TEXT("THE CUTTING ROOM");
	static const TCHAR* EditKeys = TEXT("A / D  shot     W / S  take     ENTER  play the film     BACKSPACE  keep shooting");
	static const TCHAR* NoTake = TEXT("NO TAKE");
	static const TCHAR* FilmTitle = TEXT("DEEP CUT");
	static const TCHAR* FilmSub = TEXT("a documentary from 212 metres down");
	static const TCHAR* Credits1 = TEXT("Camera, direction and cut:  you");
	static const TCHAR* Credits2 = TEXT("Filmed on location.  No animal was asked to do it twice.");
	static const TCHAR* RatingKeys = TEXT("ENTER  back to the cutting room     ESC  quit");
	static const TCHAR* Rating[] = {
		TEXT("STRAIGHT TO THE ARCHIVE"),
		TEXT("A LATE-NIGHT SLOT"),
		TEXT("PRIME TIME"),
		TEXT("FESTIVAL OPENER") };
}
