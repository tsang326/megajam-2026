// Deep Cut: everything on screen that is not the 3D scene. Drawn on the Canvas in a 1920x1080 space.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MatchCutHUD.generated.h"

class AMatchCutGameMode;
struct FCompositeFont;

/** Letterbox bars, reel strip, survey-camera tag, ghost of the next shot, tape marks, cut preview, clapper card, film player. */
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
	void DrawMarks(AMatchCutGameMode* GM);
	void Quad(const FVector2D& C, float AngleDeg, float HalfLen, float HalfWidth, const FLinearColor& Color);
	void Tape(const FVector2D& C, int32 Shape, float Size, const FLinearColor& Color, float Tilt);
	void Brackets(float X, float Y, float R, const FLinearColor& Color, float Thick);
	void LoadFonts();
	void DrawReel(AMatchCutGameMode* GM, float Y, float PlayheadT);
	void DrawBars(AMatchCutGameMode* GM, float ReelY);
	void DrawFeatures(class AMatchCutGameMode* GM, float X, float Y, float Scale, bool bCard);
	void DrawFilm(AMatchCutGameMode* GM);
	void DrawFlashback(AMatchCutGameMode* GM);
	void DrawClapper(AMatchCutGameMode* GM);
	void DrawHeadCam(float ClipT);
	void Noise(float T, float Alpha);

	float K = 1.f;
	/** Empty runtime-cache font: the canvas needs one to take the Slate text path. */
	UPROPERTY()
	TObjectPtr<UFont> RuntimeFont;

	bool bFontsTried = false;
	TSharedPtr<const FCompositeFont> MonoFont;
	TSharedPtr<const FCompositeFont> TitleFont;
};
