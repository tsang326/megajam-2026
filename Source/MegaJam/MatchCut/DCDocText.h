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

	// ---- the guided shoot: one shot at a time, in this order
	static const TCHAR* CurrentShot = TEXT("CURRENT SHOT");
	static const TCHAR* ShotWhere[] = {
		TEXT("Find the manta. It circles over the wreck."),
		TEXT("Find the shoal over the reef. Swim into it fast, then film it opening."),
		TEXT("Film the anglerfish at the end of the trench, with your lamp on it."),
		TEXT("Film the jellyfish rising in the shaft of light."),
		TEXT("The giant crosses high overhead every two minutes. Get under it and film upwards."),
		TEXT("Film your crewmate waving. Swim up in front of them.") };
	static const TCHAR* LampFirst = TEXT("The trench is dark. First fetch the headlamp: it lies on the wreck.");
	static const TCHAR* MarkLamp = TEXT("HEADLAMP");
	static const TCHAR* DirStart = TEXT("DIRECTOR:  Rolling. Start easy: film your crewmate, just ahead.");
	static const TCHAR* DirNext[] = {
		TEXT("DIRECTOR:  Good take. Next: the manta, over the wreck."),
		TEXT("DIRECTOR:  Good take. Next: the shoal over the reef."),
		TEXT("DIRECTOR:  Good take. Last one: the anglerfish, down in the trench."),
		TEXT("DIRECTOR:  Good take. Next: the jellyfish in the light shaft."),
		TEXT("DIRECTOR:  Good take. Next: the giant. Watch the water above you."),
		TEXT("DIRECTOR:  Good take. Next: your crewmate.") };
	static const TCHAR* DirEnough = TEXT("DIRECTOR:  That is enough for a film. ENTER ends the shoot, or keep going.");
	static const TCHAR* DirWrap = TEXT("DIRECTOR:  That's a wrap. Press ENTER to cut your film.");
	static const TCHAR* DirBetter = TEXT("DIRECTOR:  We have that one. A better take replaces it.");
	static const TCHAR* TutSwim = TEXT("Swim up to your crewmate     W A S D swim     MOUSE look");
	static const TCHAR* TutAim = TEXT("Hold RIGHT MOUSE to raise the camera     (or press C)");
	static const TCHAR* TutFrame = TEXT("Frame your crewmate");
	static const TCHAR* TutRecord = TEXT("LEFT MOUSE: record     Hold the frame for three seconds     (or E)");
	static const TCHAR* TutZoom = TEXT("WHEEL zooms the lens     (or 1 / 2)");
	static const TCHAR* TipFar = TEXT("TIP: swim closer, or zoom in with the WHEEL.");
	static const TCHAR* TipCutOff = TEXT("TIP: back off or zoom out until all of it fits.");
	static const TCHAR* TipMoment[] = {
		TEXT("TIP: wait until the manta is over the wreck."),
		TEXT("TIP: swim into the shoal fast, then film it opening."),
		TEXT("TIP: fetch the headlamp from the wreck, then switch it on with F."),
		TEXT("TIP: film them while they rise in the light."),
		TEXT("TIP: get under the giant and tilt the camera up."),
		TEXT("TIP: come close, in front of them, until they wave.") };
	static const TCHAR* TipShaky = TEXT("TIP: move the camera slowly while it records.");
	static const TCHAR* TipCentre = TEXT("TIP: keep the subject in the middle, or on a third line.");
	static const TCHAR* TipLost = TEXT("TIP: keep it in the frame for the whole three seconds.");
	static const TCHAR* TipNothing = TEXT("TIP: the brackets show what the lens has found.");
	static const TCHAR* TipFew = TEXT("TIP: get more of them into the picture.");

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
	static const TCHAR* Few = TEXT("Too few of them in frame");
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
