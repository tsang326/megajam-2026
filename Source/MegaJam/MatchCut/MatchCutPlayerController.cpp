// Match-cut movement prototype (brief 01).

#include "MatchCutPlayerController.h"
#include "MatchCutGameMode.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"

AMatchCutPlayerController::AMatchCutPlayerController()
{
	// The shot cameras are the view target; never fall back to the pawn's follow camera.
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

	ACharacter* Char = Cast<ACharacter>(GetPawn());

	// movement, relative to the current shot camera (control rotation is set to the camera yaw on every shot change)
	FVector2D Move = GM->GetScriptMove();
	if (GM->AcceptsPlayerInput())
	{
		Move.X += (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left) ? 1.f : 0.f);
		Move.Y += (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down) ? 1.f : 0.f);

		const float PadX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
		const float PadY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
		if (FMath::Abs(PadX) > 0.2f) { Move.X += PadX; }
		if (FMath::Abs(PadY) > 0.2f) { Move.Y += PadY; }

		if (Char)
		{
			if (WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
			{
				Char->Jump();
			}
			if (WasInputKeyJustReleased(EKeys::SpaceBar) || WasInputKeyJustReleased(EKeys::Gamepad_FaceButton_Bottom))
			{
				Char->StopJumping();
			}
		}

		if (WasInputKeyJustPressed(EKeys::C) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left))
		{
			GM->TryCut();
		}
		if (WasInputKeyJustPressed(EKeys::R) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
		{
			GM->Retake();
		}
	}
	else if (GM->IsWrapped())
	{
		if (WasInputKeyJustPressed(EKeys::R) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
		{
			GM->RestartAll();
		}
	}

	if (Char && !Move.IsNearlyZero())
	{
		if (Move.SizeSquared() > 1.f)
		{
			Move.Normalize();
		}
		const FRotator YawRot(0.0, GetControlRotation().Yaw, 0.0);
		const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		Char->AddMovementInput(Fwd, Move.Y);
		Char->AddMovementInput(Right, Move.X);
	}
}
