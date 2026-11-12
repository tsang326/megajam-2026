// Deep Cut slice: keyboard/gamepad polling. The robot itself is moved by the game mode.

#include "MatchCutPlayerController.h"
#include "MatchCutGameMode.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"

AMatchCutPlayerController::AMatchCutPlayerController()
{
	// The shot cameras are the view target; never fall back to a pawn.
	bAutoManageActiveCameraTarget = false;
}

void AMatchCutPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	AMatchCutGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMatchCutGameMode>() : nullptr;
	if (!GM)
	{
		return;
	}

	if (WasInputKeyJustPressed(EKeys::Escape))
	{
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		return;
	}
	if (GM->IsScriptActive())
	{
		return;
	}

	// x = screen right, y = screen up
	FVector2D Move(0.0, 0.0);
	Move.X += (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left) ? 1.f : 0.f);
	Move.Y += (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down) ? 1.f : 0.f);
	const float PadX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float PadY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	if (FMath::Abs(PadX) > 0.2f) { Move.X += PadX; }
	if (FMath::Abs(PadY) > 0.2f) { Move.Y += PadY; }
	GM->SetMoveInput(Move);

	const bool bCut = WasInputKeyJustPressed(EKeys::C) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left);
	const bool bLamp = WasInputKeyJustPressed(EKeys::F) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom);
	const bool bRetake = WasInputKeyJustPressed(EKeys::R) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top);
	const bool bOther = WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::LeftMouseButton)
		|| WasInputKeyJustPressed(EKeys::W) || WasInputKeyJustPressed(EKeys::A) || WasInputKeyJustPressed(EKeys::S) || WasInputKeyJustPressed(EKeys::D)
		|| WasInputKeyJustPressed(EKeys::Up) || WasInputKeyJustPressed(EKeys::Down) || WasInputKeyJustPressed(EKeys::Left) || WasInputKeyJustPressed(EKeys::Right)
		|| WasInputKeyJustPressed(EKeys::Gamepad_Special_Right);

	if (bCut || bLamp || bRetake || bOther)
	{
		GM->PressAnyKey(); // skips the opening film once it has been seen
	}
	if (bCut) { GM->PressCut(); }
	if (bLamp) { GM->PressLamp(); }
	if (bRetake) { GM->PressRetake(); }
}
