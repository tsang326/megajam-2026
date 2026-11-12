// Deep Cut slice: everything on screen that is not the 3D scene. Drawn on the Canvas in a 1920x1080 space.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MatchCutHUD.generated.h"

class AMatchCutGameMode;

/** Letterbox bars, reel strip, survey-camera tag, ghost of the next shot, cut preview, clapper card, film player. */
UCLASS()
class AMatchCutHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void Text(const FString& Str, float X, float Y, float Scale, const FLinearColor& Color, int32 Align = 0); // 0 left, 1 centre, 2 right
	void Rect(float X, float Y, float W, float H, const FLinearColor& Color);
	void Line(float X0, float Y0, float X1, float Y1, const FLinearColor& Color, float Thick);
	void Ring(float X, float Y, float R, const FLinearColor& Color, float Thick);
	void Disc(float X, float Y, float R, const FLinearColor& Color);
	void Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thick);

	void DrawGhost(AMatchCutGameMode* GM);
	void DrawPreview(AMatchCutGameMode* GM);
	void DrawReel(AMatchCutGameMode* GM, float Y, float PlayheadT);
	void DrawBars(AMatchCutGameMode* GM, float ReelY);
	void DrawFilm(AMatchCutGameMode* GM);
	void DrawFlashback(AMatchCutGameMode* GM);
	void DrawClapper(AMatchCutGameMode* GM);

	float K = 1.f;
};
