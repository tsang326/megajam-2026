// Deep Cut slice: keyboard/gamepad polling. The robot itself is moved by the game mode.

#include "MatchCutPlayerController.h"
#include "MatchCutGameMode.h"
#include "DC3D.h"
#include "DCDoc.h"
#include "DCMenu.h"
#include "DCAudio.h"
#include "MatchCutHUD.h"
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

	FDCMenu& Menu = DCMenu();
	if (Menu.bQuit)
	{
		Menu.bQuit = false;
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		return;
	}
	if (GM->IsScriptActive())
	{
		return;
	}
	if (Menu.IsOpen() && GM->World3D())
	{
		TickMenu(GM, DeltaTime);
		return;
	}
	if (bMenuMouse)
	{
		// back from a menu: the mouse is the look again, and its first jump is thrown away
		bMenuMouse = false;
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		float Mx = 0.f, My = 0.f;
		GetInputMouseDelta(Mx, My);
		IgnoreMouse = 0.15f;
	}
	IgnoreMouse = FMath::Max(0.f, IgnoreMouse - DeltaTime);
	if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Left))
	{
		// in the dive ESC is "back" (the board, the base's pages); with nothing open it pauses
		FDC3D* W3 = GM->World3D();
		FDCDoc* D3 = W3 ? W3->Doc : nullptr;
		if (!D3)
		{
			UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
			return;
		}
		if (D3->State == EDCDocState::Shooting && !D3->bBoardOpen && D3->Spot == INDEX_NONE)
		{
			Menu.Open(EDCMenuPage::Pause);
			if (GM->GetAudio())
			{
				GM->GetAudio()->Play(TEXT("ui_select"), 0.8f, 0.9f);
			}
			return;
		}
	}
	if (WasInputKeyJustPressed(EKeys::H) && DCTutorial().IsActive())
	{
		DCSettings().bTutorial = false; // H hides the tutorial (Settings brings it back)
		DCSettings().Save();
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
		float MouseX = 0.f, MouseY = 0.f;
		GetInputMouseDelta(MouseX, MouseY);
		if (W->Time > 0.5f && IgnoreMouse <= 0.f) // the first counts after the window takes the mouse can be one big jump
		{
			W->LookDelta += FVector2D(MouseX, MouseY * (W->bInvertY ? -1.f : 1.f)) * W->Sens;
		}
		FVector2D Rate(Down(EKeys::Right) - Down(EKeys::Left), Down(EKeys::Up) - Down(EKeys::Down));
		const float LookX = GetInputAnalogKeyState(EKeys::Gamepad_RightX);
		const float LookY = GetInputAnalogKeyState(EKeys::Gamepad_RightY);
		if (FMath::Abs(LookX) > 0.2f) { Rate.X += LookX; }
		if (FMath::Abs(LookY) > 0.2f) { Rate.Y += LookY * (W->bInvertY ? -1.f : 1.f); }
		// keys and sticks: the turn starts gentle (fine aim) and reaches full speed after a moment (a quick look round)
		LookHold = Rate.IsNearlyZero() ? 0.f : LookHold + DeltaTime;
		const float Ramp = FMath::Lerp(0.45f, 1.f, FMath::Clamp(LookHold / 0.35f, 0.f, 1.f));
		W->LookRate = FVector2D(Rate.X * 150.0, Rate.Y * 100.0) * DCSettings().TurnSpeed * Ramp;
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

void AMatchCutPlayerController::TickMenu(AMatchCutGameMode* GM, float DeltaTime)
{
	FDCMenu& Menu = DCMenu();
	if (!bMenuMouse)
	{
		bMenuMouse = true;
		bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
	}
	if (FDC3D* W = GM->World3D())
	{
		W->Move = FVector::ZeroVector;
		W->LookRate = FVector2D::ZeroVector;
		W->LookDelta = FVector2D::ZeroVector;
	}
	auto Hit = [this](std::initializer_list<FKey> Keys)
	{
		for (const FKey& Key : Keys)
		{
			if (WasInputKeyJustPressed(Key))
			{
				return true;
			}
		}
		return false;
	};
	if (Hit({ EKeys::W, EKeys::Up, EKeys::Gamepad_DPad_Up })) { Menu.Move(-1, GM); }
	if (Hit({ EKeys::S, EKeys::Down, EKeys::Gamepad_DPad_Down })) { Menu.Move(1, GM); }
	if (Hit({ EKeys::A, EKeys::Left, EKeys::Gamepad_DPad_Left })) { Menu.Change(-1, GM); }
	if (Hit({ EKeys::D, EKeys::Right, EKeys::Gamepad_DPad_Right })) { Menu.Change(1, GM); }
	if (Hit({ EKeys::Enter, EKeys::SpaceBar, EKeys::E, EKeys::Gamepad_FaceButton_Bottom })) { Menu.Activate(GM); }
	if (Hit({ EKeys::Escape, EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_Special_Left }) && Menu.Page != EDCMenuPage::Title)
	{
		Menu.Back(GM);
	}
	// the mouse: hover picks a row, a click acts on it (right click turns a setting the other way), the wheel turns it
	float Mx = 0.f, My = 0.f;
	if (GetMousePosition(Mx, My))
	{
		const FVector2D P(Mx, My); // the HUD stores the rows in viewport pixels
		int32 Over = INDEX_NONE;
		for (int32 i = 0; i < Menu.Rows.Num(); ++i)
		{
			if (Menu.Rows[i].IsInsideOrOn(P))
			{
				Over = i;
			}
		}
		const bool bMoved = !P.Equals(LastMouse, 0.5);
		LastMouse = P;
		if (Over != INDEX_NONE && (bMoved || WasInputKeyJustPressed(EKeys::LeftMouseButton)) && Over != Menu.Sel)
		{
			Menu.Sel = Over;
			if (GM->GetAudio())
			{
				GM->GetAudio()->Play(TEXT("ui_move"), 0.6f, 1.f, 0.04f);
			}
		}
		if (Over != INDEX_NONE && WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			Menu.Activate(GM);
		}
		if (Over != INDEX_NONE && WasInputKeyJustPressed(EKeys::RightMouseButton))
		{
			Menu.Change(-1, GM);
		}
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp)) { Menu.Change(1, GM); }
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) { Menu.Change(-1, GM); }
}
