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

	// x = screen right, y = screen up, z = away from the camera
	FVector Move(0.0, 0.0, 0.0);
	Move.X += (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left) ? 1.f : 0.f);
	Move.Y += (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down) ? 1.f : 0.f);
	// depth: Q / wheel up = farther from the camera, Z / wheel down = closer (brief 05; SPACE and SHIFT read as up and down)
	Move.Z += (IsInputKeyDown(EKeys::Q) || IsInputKeyDown(EKeys::Gamepad_RightShoulder) ? 1.f : 0.f)
		- (IsInputKeyDown(EKeys::Z) || IsInputKeyDown(EKeys::Gamepad_LeftShoulder) ? 1.f : 0.f);
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp)) { GM->AddDepthStep(1.f); }
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) { GM->AddDepthStep(-1.f); }
	const float PadX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float PadY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	if (FMath::Abs(PadX) > 0.2f) { Move.X += PadX; }
	if (FMath::Abs(PadY) > 0.2f) { Move.Y += PadY; }
	GM->SetMoveInput(Move);

	const bool bCut = WasInputKeyJustPressed(EKeys::C) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left);
	const bool bLamp = WasInputKeyJustPressed(EKeys::F) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom);
	const bool bUse = WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Gamepad_RightTrigger);
	const bool bRetake = WasInputKeyJustPressed(EKeys::R) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top);
	const bool bOther = WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::LeftMouseButton)
		|| WasInputKeyJustPressed(EKeys::W) || WasInputKeyJustPressed(EKeys::A) || WasInputKeyJustPressed(EKeys::S) || WasInputKeyJustPressed(EKeys::D)
		|| WasInputKeyJustPressed(EKeys::Up) || WasInputKeyJustPressed(EKeys::Down) || WasInputKeyJustPressed(EKeys::Left) || WasInputKeyJustPressed(EKeys::Right)
		|| WasInputKeyJustPressed(EKeys::Gamepad_Special_Right);

	if (bCut || bLamp || bRetake || bUse || bOther)
	{
		GM->PressAnyKey(); // skips the opening film once it has been seen
	}
	if (bCut) { GM->PressCut(); }
	if (bLamp) { GM->PressLamp(); }
	if (bUse) { GM->PressUse(); }
	if (bRetake) { GM->PressRetake(); }
}
