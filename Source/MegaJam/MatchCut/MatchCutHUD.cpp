// Match-cut movement prototype (brief 01).

#include "MatchCutHUD.h"
#include "MatchCutGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CanvasItem.h"

void AMatchCutHUD::Text(const FString& Str, float X, float Y, float Scale, const FLinearColor& Color, bool bCentre)
{
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!Font || !Canvas)
	{
		return;
	}
	// everything is laid out in a 1920x1080 space and scaled to the (letterboxed) view
	const float K = Canvas->ClipY / 1080.f;
	FCanvasTextItem Item(FVector2D(X * K, Y * K), FText::FromString(Str), Font, Color);
	Item.Scale = FVector2D(Scale * K, Scale * K);
	Item.bCentreX = bCentre;
	Item.bCentreY = bCentre;
	Item.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(Item);
}

void AMatchCutHUD::DrawHUD()
{
	Super::DrawHUD();

	AMatchCutGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMatchCutGameMode>() : nullptr;
	if (!GM || !Canvas)
	{
		return;
	}

	// ghost of the next shot: additive, so the black void of the next shot adds nothing
	if (UTextureRenderTarget2D* Ghost = GM->GetGhostTarget())
	{
		if (GM->GetState() != EMatchCutState::Wrap && Ghost->GetResource())
		{
			const float A = GM->GetGhostOpacity();
			FCanvasTileItem Tile(FVector2D::ZeroVector, Ghost->GetResource(), FVector2D(Canvas->ClipX, Canvas->ClipY), FLinearColor(A, A, A, 1.f));
			Tile.BlendMode = SE_BLEND_Additive;
			Canvas->DrawItem(Tile);
		}
	}

	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	Text(FString::Printf(TEXT("SHOT %d/%d   %s   TAKE %d"), GM->GetShotIndex() + 1, GM->GetNumShots(), *GM->GetShotName(), GM->GetTake()), 40.f, 30.f, 1.6f, White);
	Text(GM->GetShotHint(), 40.f, 75.f, 1.3f, FLinearColor(0.8f, 0.8f, 0.8f, 1.f));
	Text(TEXT("WASD move   SPACE jump   C cut   R retake   ESC quit"), 40.f, 1030.f, 1.2f, FLinearColor(0.7f, 0.7f, 0.7f, 1.f));

	if (GM->GetState() == EMatchCutState::NG)
	{
		Text(TEXT("NG"), 960.f, 480.f, 9.f, FLinearColor(1.f, 0.15f, 0.1f, 1.f), true);
		Text(GM->GetNGReason(), 960.f, 620.f, 2.f, FLinearColor(1.f, 0.5f, 0.4f, 1.f), true);
	}
	else if (GM->GetState() == EMatchCutState::Wrap)
	{
		Text(TEXT("THAT'S A WRAP"), 960.f, 480.f, 6.f, FLinearColor(1.f, 0.9f, 0.3f, 1.f), true);
		Text(TEXT("R to go again"), 960.f, 600.f, 2.f, White, true);
	}
	else if (GM->GetFlash() > 0.f)
	{
		Text(TEXT("CUT"), 960.f, 200.f, 4.f, FLinearColor(1.f, 1.f, 1.f, FMath::Clamp(GM->GetFlash() * 2.f, 0.f, 1.f)), true);
	}
}
