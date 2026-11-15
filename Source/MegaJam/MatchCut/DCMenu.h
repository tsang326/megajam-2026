// Deep Cut: the start menu, the pause menu, the settings (kept in Saved/DeepCutSettings.txt) and the Vietnamese text.
// The HUD draws the pages, the player controller drives them, the 3D game stands still while one is open.

#pragma once

#include "CoreMinimal.h"

class AMatchCutGameMode;

struct FDCSettings
{
	int32 Lang = 0;          // 0 English, 1 Vietnamese
	float Sens = 0.13f;      // degrees per mouse count
	float TurnSpeed = 1.f;   // arrow keys and the right stick
	bool bInvertY = false;
	bool bHeadBob = true;
	float Fov = 78.f;
	float Master = 0.8f;
	float Music = 0.7f;
	bool bFullscreen = true;
	int32 Quality = 2;       // 0 low .. 3 epic
	bool bTutorial = true;

	void Load();
	void Save() const;
	/** Pushes the settings into the game; bDisplay also sets the window mode and the quality (never in a test run). */
	void Apply(AMatchCutGameMode* GM, bool bDisplay) const;
};

enum class EDCMenuPage : uint8
{
	None,
	Title,
	Pause,
	Settings,
	HowTo
};

struct FDCMenu
{
	EDCMenuPage Page = EDCMenuPage::Title;
	EDCMenuPage From = EDCMenuPage::Title; // where Settings / How to play go back to
	int32 Sel = 0;
	float Time = 0.f;
	bool bStarted = false;   // the first Start has been pressed
	bool bQuit = false;      // the controller quits the game
	// rows on screen this frame (canvas pixels), written by the HUD, read by the controller for the mouse
	TArray<FBox2D> Rows;

	bool IsOpen() const { return Page != EDCMenuPage::None; }
	int32 NumRows() const;
	void Open(EDCMenuPage P);
	void Move(int32 D, AMatchCutGameMode* GM);
	/** Left / right on a settings row, or Enter on any row. */
	void Change(int32 D, AMatchCutGameMode* GM);
	void Activate(AMatchCutGameMode* GM);
	void Back(AMatchCutGameMode* GM);
	/** The label and the value of a row (English: the HUD translates). */
	void Row(int32 i, FString& Label, FString& Value) const;
};

/** The first dive's tutorial: one card at a time, each step done by doing it (in any order). */
struct FDCTutorial
{
	static constexpr int32 NumSteps = 10;
	bool Done[NumSteps] = {};
	int32 Step = 0;          // the first step not done that can be done right now; NumSteps when all are
	bool bWaiting = false;   // steps are left but none can be done where the player is: no card
	float Flash = 0.f;       // a step was just done: the tick shows
	int32 FlashStep = -1;
	float Looked = 0.f;      // degrees turned
	float Walked = 0.f;      // cm walked in the base
	float Swum = 0.f;        // seconds swimming
	float LastYaw = 0.f;
	bool bHadYaw = false;
	bool bOut = false;       // has been in the sea
	float Shown = 0.f;       // how long this card has been up
	bool bRunning = false;   // ticked: a real game, not a test script
	bool bForced = false;    // a test script turned it on

	bool IsActive() const;
	void Tick(struct FDC3D& W, float Dt);
	/** The heading and the line of a step (English: the HUD translates). */
	static void Card(int32 S, FString& Head, FString& Body);
};

FDCSettings& DCSettings();
FDCMenu& DCMenu();
FDCTutorial& DCTutorial();
/** Test script lines: menu title|pause|settings|howto|none, menusel N, lang en|vi, tut on|off. 0 = not one of these. */
int32 DCMenuRunLine(AMatchCutGameMode* GM, const TArray<FString>& T);

namespace DCLoc
{
	/** Reads Content/Data/loc_vi.txt: "English<TAB>Vietnamese" per line; {} marks a value that is carried over. */
	void Load();
	bool IsVi();
	/** The text in the chosen language. Unknown text comes back as it is. */
	FString Tr(const FString& En);
}
