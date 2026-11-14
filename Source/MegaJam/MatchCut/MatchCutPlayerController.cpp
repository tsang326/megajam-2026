// Deep Cut slice: keyboard/gamepad polling. The robot itself is moved by the game mode.

#include "MatchCutPlayerController.h"
#include "MatchCutGameMode.h"
#include "DC3D.h"
#include "DCDoc.h"
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
		// in the dive ESC is "back" (menus, the board); only a second ESC, with nothing open, quits
		FDC3D* W3 = GM->World3D();
		FDCDoc* D3 = W3 ? W3->Doc : nullptr;
		static double LastEsc = -10.0;
		const double NowS = FPlatformTime::Seconds();
		const bool bFree = !D3 || (D3->State == EDCDocState::Shooting && !D3->bBoardOpen);
		if (!D3 || (bFree && NowS - LastEsc < 1.5))
		{
			UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
			return;
		}
		if (bFree)
		{
			LastEsc = NowS;
			D3->Alert = TEXT("ESC again to quit");
			D3->AlertTime = 1.5f;
		}
	}
	if (GM->IsScriptActive())
	{
		return;
	}
	if (FDC3D* W = GM->World3D())
	{
		// brief 06: swim where the camera looks. No key here is a macOS shortcut (no CTRL).
		if (!bMouseTaken)
		{
			bMouseTaken = true;
			bShowMouseCursor = false;
			SetInputMode(FInputModeGameOnly());
		}
		auto Down = [this](const FKey& Key) { return IsInputKeyDown(Key) ? 1.f : 0.f; };
		FVector Move(Down(EKeys::W) - Down(EKeys::S), Down(EKeys::D) - Down(EKeys::A),
			Down(EKeys::SpaceBar) - FMath::Max(Down(EKeys::X), Down(EKeys::Q)));
		const float PadX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
		const float PadY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
		if (FMath::Abs(PadX) > 0.2f) { Move.Y += PadX; }
		if (FMath::Abs(PadY) > 0.2f) { Move.X += PadY; }
		Move.Z += GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis) - GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis);
		W->Move = Move.GetClampedToMaxSize(1.0);
		W->bBoost = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift) || IsInputKeyDown(EKeys::Gamepad_LeftThumbstick);

		// look: mouse, arrow keys, right stick
		if (WasInputKeyJustPressed(EKeys::LeftBracket)) { W->Sens = FMath::Max(0.03f, W->Sens * 0.8f); }
		if (WasInputKeyJustPressed(EKeys::RightBracket)) { W->Sens = FMath::Min(0.5f, W->Sens * 1.25f); }
		if (WasInputKeyJustPressed(EKeys::I)) { W->bInvertY = !W->bInvertY; }
		float MouseX = 0.f, MouseY = 0.f;
		GetInputMouseDelta(MouseX, MouseY);
		if (W->Time > 0.5f) // the first counts after the window takes the mouse can be one big jump
		{
			W->LookDelta += FVector2D(MouseX, MouseY * (W->bInvertY ? -1.f : 1.f)) * W->Sens;
		}
		FVector2D Rate(Down(EKeys::Right) - Down(EKeys::Left), Down(EKeys::Up) - Down(EKeys::Down));
		const float LookX = GetInputAnalogKeyState(EKeys::Gamepad_RightX);
		const float LookY = GetInputAnalogKeyState(EKeys::Gamepad_RightY);
		if (FMath::Abs(LookX) > 0.2f) { Rate.X += LookX; }
		if (FMath::Abs(LookY) > 0.2f) { Rate.Y += LookY * (W->bInvertY ? -1.f : 1.f); }
		W->LookRate = FVector2D(Rate.X * 120.0, Rate.Y * 85.0);
		if (FDCDoc* D = W->Doc)
		{
			// brief 07: the hand camera. Mouse first; every action also has a key, for a trackpad.
			D->bAimHeld = IsInputKeyDown(EKeys::RightMouseButton) || IsInputKeyDown(EKeys::Gamepad_LeftShoulder);
			if (WasInputKeyJustPressed(EKeys::C)) { D->bAimToggle = !D->bAimToggle; }
			D->bRecordPressed |= WasInputKeyJustPressed(EKeys::LeftMouseButton) || WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Gamepad_RightShoulder);
			D->bLampPressed |= WasInputKeyJustPressed(EKeys::F) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top);
			D->bEnterPressed |= WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Right);
			D->bUsePressed |= WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom);
			if (D->State != EDCDocState::Shooting && WasInputKeyJustPressed(EKeys::E)) { D->bEnterPressed = true; }
			D->bBackPressed |= WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::BackSpace) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right);
			if (WasInputKeyJustPressed(EKeys::MouseScrollUp)) { D->ZoomIn += 1.f; }
			if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) { D->ZoomIn -= 1.f; }
			D->ZoomIn += (Down(EKeys::Two) + Down(EKeys::Gamepad_DPad_Up) - Down(EKeys::One) - Down(EKeys::Gamepad_DPad_Down)) * 7.f * DeltaTime;
			if (D->State != EDCDocState::Shooting)
			{
				if (WasInputKeyJustPressed(EKeys::A) || WasInputKeyJustPressed(EKeys::Left) || WasInputKeyJustPressed(EKeys::Gamepad_DPad_Left)) { D->NavX = -1; }
				if (WasInputKeyJustPressed(EKeys::D) || WasInputKeyJustPressed(EKeys::Right) || WasInputKeyJustPressed(EKeys::Gamepad_DPad_Right)) { D->NavX = 1; }
				if (WasInputKeyJustPressed(EKeys::W) || WasInputKeyJustPressed(EKeys::Up)) { D->NavY = -1; }
				if (WasInputKeyJustPressed(EKeys::S) || WasInputKeyJustPressed(EKeys::Down)) { D->NavY = 1; }
			}
		}
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
