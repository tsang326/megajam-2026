// Match-cut movement prototype (brief 01).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MatchCutHUD.generated.h"

/** Ghost overlay of the next shot plus the few lines of text the test needs. */
UCLASS()
class AMatchCutHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void Text(const FString& Str, float X, float Y, float Scale, const FLinearColor& Color, bool bCentre = false);
};
