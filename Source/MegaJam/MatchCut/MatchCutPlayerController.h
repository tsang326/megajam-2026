// Match-cut movement prototype (brief 01).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MatchCutPlayerController.generated.h"

/** Keyboard/gamepad polling for the prototype: camera-relative move, jump, CUT, retake. */
UCLASS()
class AMatchCutPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMatchCutPlayerController();
	virtual void PlayerTick(float DeltaTime) override;

private:
	void TickMenu(class AMatchCutGameMode* GM, float DeltaTime);
	bool bMouseTaken = false;
	bool bMenuMouse = false;   // the menu has the mouse (cursor shown)
	float IgnoreMouse = 0.f;   // seconds the look ignores the mouse after a menu closes
	float LookHold = 0.f;      // seconds a look key has been held
	FVector2D LastMouse = FVector2D::ZeroVector;
};
