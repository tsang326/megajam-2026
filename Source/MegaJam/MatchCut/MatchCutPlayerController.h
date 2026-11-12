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
};
