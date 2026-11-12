// Deep Cut slice: HUD. Layout space is 1920x1080; the picture is y 135..945, the two bars belong to the HUD.

#include "MatchCutHUD.h"
#include "MatchCutGameMode.h"
#include "DeepCutText.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CanvasItem.h"

namespace
{
	const float BarH = 135.f;
	const float PicTop = 135.f;
	const float PicBottom = 945.f;
	const FLinearColor Green(0.25f, 1.f, 0.45f, 1.f);
	const FLinearColor Red(1.f, 0.22f, 0.15f, 1.f);
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor Dim(0.62f, 0.66f, 0.68f, 1.f);
	const FLinearColor Amber(1.f, 0.78f, 0.25f, 1.f);

	FVector2D NdcToPx(const FVector2D& N)
	{
		return FVector2D((N.X + 1.0) * 960.0, (1.0 - N.Y) * 540.0);
	}

	FString Timecode(float Seconds, int32 Hours)
	{
		const int32 Total = FMath::FloorToInt(Seconds * 24.f);
		return FString::Printf(TEXT("%02d:%02d:%02d:%02d"), Hours, (Total / (24 * 60)) % 60, (Total / 24) % 60, Total % 24);
	}
}

void AMatchCutHUD::Text(const FString& Str, float X, float Y, float Scale, const FLinearColor& Color, int32 Align)
{
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!Font || !Canvas || Str.IsEmpty())
	{
		return;
	}
	float XL = 0.f, YL = 0.f;
	Canvas->StrLen(Font, Str, XL, YL);
	float Px = X * K;
	if (Align == 1) { Px -= XL * Scale * K * 0.5f; }
	if (Align == 2) { Px -= XL * Scale * K; }
	FCanvasTextItem Item(FVector2D(Px, Y * K), FText::FromString(Str), Font, Color);
	Item.Scale = FVector2D(Scale * K, Scale * K);
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Color.A));
	Canvas->DrawItem(Item);
}

void AMatchCutHUD::Rect(float X, float Y, float W, float H, const FLinearColor& Color)
{
	FCanvasTileItem Tile(FVector2D(X * K, Y * K), FVector2D(W * K, H * K), Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

void AMatchCutHUD::Line(float X0, float Y0, float X1, float Y1, const FLinearColor& Color, float Thick)
{
	FCanvasLineItem Item(FVector2D(X0 * K, Y0 * K), FVector2D(X1 * K, Y1 * K));
	Item.SetColor(Color);
	Item.LineThickness = FMath::Max(1.f, Thick * K);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AMatchCutHUD::Ring(float X, float Y, float R, const FLinearColor& Color, float Thick)
{
	const int32 N = 40;
	for (int32 i = 0; i < N; ++i)
	{
		const float A0 = 2.f * UE_PI * i / N;
		const float A1 = 2.f * UE_PI * (i + 1) / N;
		Line(X + R * FMath::Cos(A0), Y + R * FMath::Sin(A0), X + R * FMath::Cos(A1), Y + R * FMath::Sin(A1), Color, Thick);
	}
}

void AMatchCutHUD::Disc(float X, float Y, float R, const FLinearColor& Color)
{
	FCanvasNGonItem Item(FVector2D(X * K, Y * K), FVector2D(R * K, R * K), 28, Color);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AMatchCutHUD::Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thick)
{
	Rect(X, Y, W, Thick, Color);
	Rect(X, Y + H - Thick, W, Thick, Color);
	Rect(X, Y, Thick, H, Color);
	Rect(X + W - Thick, Y, Thick, H, Color);
}

// ---------------------------------------------------------------------------------------------

void AMatchCutHUD::DrawGhost(AMatchCutGameMode* GM)
{
	UTextureRenderTarget2D* Ghost = GM->GhostTarget;
	if (GM->GhostMode == 0 || !Ghost || !Ghost->GetResource())
	{
		return;
	}
	const bool bRestore = GM->InRestore();
	if (!bRestore && !GM->Shots.IsValidIndex(GM->Cur + 1))
	{
		return;
	}
	const float Time = GM->PlayClock;

	if (GM->GhostMode == 2)
	{
		// brief-01 ghost: the lit next frame added at 35 %
		FCanvasTileItem Tile(FVector2D::ZeroVector, Ghost->GetResource(), FVector2D(Canvas->ClipX, Canvas->ClipY), FLinearColor(0.35f, 0.35f, 0.35f, 1.f));
		Tile.BlendMode = SE_BLEND_Additive;
		Canvas->DrawItem(Tile);
		return;
	}

	// a hint first (strong, breathing), a quiet guide once the first cut is done
	float A = GM->GhostOpacity;
	if (!GM->bFirstCutDone || bRestore)
	{
		A *= 1.25f + 0.45f * FMath::Sin(Time * 4.f);
	}
	FLinearColor Tint = GM->GhostTint * A;
	Tint.A = 1.f;

	if (!bRestore && GM->NextIsDark())
	{
		// a dark shot shows nothing; the lamp shows it inside its beam only
		if (!GM->bLampOn)
		{
			return;
		}
		const FVector2D C = NdcToPx(GM->HeroNdc);
		const float R = GM->LampRadius * 1080.f;
		const int32 N = 36;
		TArray<FCanvasUVTri> Tris;
		Tris.Reserve(N);
		const FLinearColor Edge(0.f, 0.f, 0.f, 1.f);
		for (int32 i = 0; i < N; ++i)
		{
			const float A0 = 2.f * UE_PI * i / N;
			const float A1 = 2.f * UE_PI * (i + 1) / N;
			const FVector2D P0 = C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R;
			const FVector2D P1 = C + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R;
			FCanvasUVTri T;
			T.V0_Pos = C * K;  T.V0_UV = FVector2D(C.X / 1920.0, C.Y / 1080.0);   T.V0_Color = Tint * 1.5f;
			T.V1_Pos = P0 * K; T.V1_UV = FVector2D(P0.X / 1920.0, P0.Y / 1080.0); T.V1_Color = Edge;
			T.V2_Pos = P1 * K; T.V2_UV = FVector2D(P1.X / 1920.0, P1.Y / 1080.0); T.V2_Color = Edge;
			Tris.Add(T);
		}
		FCanvasTriangleItem Item(Tris, Ghost->GetResource());
		Item.BlendMode = SE_BLEND_Additive;
		Canvas->DrawItem(Item);
		return;
	}

	FCanvasTileItem Tile(FVector2D::ZeroVector, Ghost->GetResource(), FVector2D(Canvas->ClipX, Canvas->ClipY), Tint);
	Tile.BlendMode = SE_BLEND_Additive;
	Canvas->DrawItem(Tile);
}

void AMatchCutHUD::DrawPreview(AMatchCutGameMode* GM)
{
	const bool bRestore = GM->InRestore();
	const bool bHasTarget = bRestore || GM->Shots.IsValidIndex(GM->Cur + 1);
	if (!bHasTarget || GM->GhostMode != 1)
	{
		return;
	}
	const float Time = GM->PlayClock;
	const bool bDarkNext = !bRestore && GM->NextIsDark();
	const FVector2D HeroPx = NdcToPx(GM->HeroNdc);
	const float LampPx = GM->LampRadius * 1080.f * 0.82f;

	// where a cut would work: tinted, with a bright outline
	if (GM->bMaskValid && !(bDarkNext && !GM->bLampOn))
	{
		const int32 W = AMatchCutGameMode::MaskW, H = AMatchCutGameMode::MaskH;
		const float CW = 1920.f / W, CH = (PicBottom - PicTop) / H;
		auto Valid = [&](int32 x, int32 y) -> bool
		{
			if (x < 0 || y < 0 || x >= W || y >= H || !GM->Mask[y * W + x])
			{
				return false;
			}
			if (bDarkNext)
			{
				const FVector2D P((x + 0.5f) * CW, PicTop + (y + 0.5f) * CH);
				return FVector2D::Distance(P, HeroPx) < LampPx;
			}
			return true;
		};
		float FillA = GM->bFirstCutDone ? 0.07f : 0.11f + 0.05f * FMath::Sin(Time * 4.f);
		const FLinearColor Fill(Green.R, Green.G, Green.B, FillA);
		const FLinearColor Edge(Green.R, Green.G, Green.B, 0.75f);
		for (int32 y = 0; y < H; ++y)
		{
			int32 Run = -1;
			for (int32 x = 0; x <= W; ++x)
			{
				const bool bV = Valid(x, y);
				if (bV && Run < 0) { Run = x; }
				if (!bV && Run >= 0)
				{
					Rect(Run * CW, PicTop + y * CH, (x - Run) * CW, CH, Fill);
					Run = -1;
				}
				if (bV)
				{
					const float X0 = x * CW, Y0 = PicTop + y * CH;
					if (!Valid(x - 1, y) && x > 0) { Rect(X0, Y0, 3.f, CH, Edge); }
					if (!Valid(x + 1, y) && x < W - 1) { Rect(X0 + CW - 3.f, Y0, 3.f, CH, Edge); }
					if (!Valid(x, y - 1) && y > 0) { Rect(X0, Y0, CW, 3.f, Edge); }
					if (!Valid(x, y + 1) && y < H - 1) { Rect(X0, Y0 + CH - 3.f, CW, 3.f, Edge); }
				}
			}
		}
	}

	if (bRestore)
	{
		// the mark to hit: Unit 7's place in the removed frame
		const FVector2D T = NdcToPx(GM->RestoreNdc);
		const float R = GM->RestoreTol * 540.f + 44.f;
		const float Pulse = 1.f + 0.08f * FMath::Sin(Time * 5.f);
		Ring(T.X, T.Y, R * Pulse, Amber, 4.f);
		Line(T.X - R - 26.f, T.Y, T.X - R + 6.f, T.Y, Amber, 3.f);
		Line(T.X + R - 6.f, T.Y, T.X + R + 26.f, T.Y, Amber, 3.f);
		Line(T.X, T.Y - R - 26.f, T.X, T.Y - R + 6.f, Amber, 3.f);
		Line(T.X, T.Y + R - 6.f, T.X, T.Y + R + 26.f, Amber, 3.f);
		Text(DCText::RestoreMark, T.X, T.Y - R - 62.f, 1.4f, Amber, 1);
	}
	else if (GM->Cur + 1 == GM->CrewShot && !GM->bCrewRescued && GM->bHasPart)
	{
		// distress beacon of the trapped crew robot: visible in the dark, so the player knows where to point the lamp
		const FVector2D B = NdcToPx(GM->Shots[GM->CrewShot].CrewNdc);
		const float Blink = FMath::Fmod(Time, 1.2f);
		if (Blink < 0.75f)
		{
			Disc(B.X, B.Y, 7.f, Amber);
			Ring(B.X, B.Y, 12.f + 40.f * Blink, FLinearColor(Amber.R, Amber.G, Amber.B, 0.8f - Blink), 2.5f);
		}
		Text(DCText::CrewSignal, B.X, B.Y + 30.f, 1.0f, Amber, 1);
	}

	if (GM->State != EDCState::Playing)
	{
		return;
	}

	// the landing: where the robot would be after the cut. Same place, same size; green = C works, red = why not.
	const FDCCutEval& E = GM->CurEval;
	const FLinearColor Col = E.bOk ? Green : Red;
	const float R = GM->HeroRadius / (GM->TanH * FMath::Max(E.Depth, 100.0)) * 960.f * 1.32f;
	Disc(HeroPx.X, HeroPx.Y, R, FLinearColor(Col.R, Col.G, Col.B, 0.16f));
	Ring(HeroPx.X, HeroPx.Y, R, Col, 4.f);
	const float LabelY = HeroPx.Y + R + 10.f < PicBottom - 40.f ? HeroPx.Y + R + 10.f : HeroPx.Y - R - 36.f;
	Text(E.bOk ? DCText::RingOk : E.Reason, HeroPx.X, LabelY, 1.25f, Col, 1);
}

void AMatchCutHUD::DrawReel(AMatchCutGameMode* GM, float Y, float PlayheadT)
{
	const float X0 = 460.f, W = 1000.f, H = 54.f;
	const float Len = FMath::Max(GM->FilmLength, 1.f);
	// film base with sprocket holes
	Rect(X0 - 8.f, Y - 12.f, W + 16.f, H + 24.f, FLinearColor(0.045f, 0.045f, 0.05f, 1.f));
	for (float X = X0; X < X0 + W - 6.f; X += 20.f)
	{
		Rect(X + 5.f, Y - 9.f, 9.f, 5.f, FLinearColor(0.f, 0.f, 0.f, 1.f));
		Rect(X + 5.f, Y + H + 4.f, 9.f, 5.f, FLinearColor(0.f, 0.f, 0.f, 1.f));
	}
	const float Time = GM->PlayClock;
	for (const FDCReelSeg& S : GM->Reel)
	{
		const float SX = X0 + S.T0 / Len * W;
		const float SW = (S.T1 - S.T0) / Len * W;
		if (S.Gap == INDEX_NONE)
		{
			Rect(SX + 1.f, Y, SW - 2.f, H, FLinearColor(0.10f, 0.19f, 0.22f, 1.f));
			for (float FX = SX + 1.f; FX < SX + SW - 30.f; FX += 38.f)
			{
				Rect(FX + 36.f, Y, 2.f, H, FLinearColor(0.045f, 0.045f, 0.05f, 1.f));
			}
		}
		else if (GM->bGapFilled[S.Gap])
		{
			if (GM->ThumbTarget && GM->ThumbTarget->GetResource())
			{
				FCanvasTileItem Tile(FVector2D((SX + 2.f) * K, Y * K), GM->ThumbTarget->GetResource(), FVector2D((SW - 4.f) * K, H * K), White);
				Tile.BlendMode = SE_BLEND_Opaque;
				Canvas->DrawItem(Tile);
			}
			Frame(SX + 1.f, Y, SW - 2.f, H, Green, 3.f);
		}
		else
		{
			Rect(SX + 1.f, Y, SW - 2.f, H, FLinearColor(0.f, 0.f, 0.f, 1.f));
			const bool bActive = S.Gap == 0 && GM->InRestore();
			const float A = bActive ? 0.55f + 0.45f * FMath::Sin(Time * 6.f) : 0.8f;
			Frame(SX + 1.f, Y, SW - 2.f, H, FLinearColor(Red.R, Red.G, Red.B, A), bActive ? 4.f : 2.f);
			Text(DCText::ReelGapLabel, SX + SW * 0.5f, Y + 18.f, 0.85f, FLinearColor(Red.R, Red.G, Red.B, 0.9f), 1);
		}
	}
	if (PlayheadT >= 0.f)
	{
		const float PX = X0 + FMath::Clamp(PlayheadT / Len, 0.f, 1.f) * W;
		Rect(PX - 1.5f, Y - 14.f, 3.f, H + 28.f, White);
		Rect(PX - 7.f, Y - 20.f, 14.f, 8.f, White);
	}
}

void AMatchCutHUD::DrawBars(AMatchCutGameMode* GM, float ReelY)
{
	const FLinearColor Black(0.f, 0.f, 0.f, 1.f);
	Rect(0.f, 0.f, 1920.f, BarH, Black);
	Rect(0.f, PicBottom, 1920.f, BarH, Black);
	// viewfinder corner marks, in the margin outside the swim area
	const float M = 26.f, L = 46.f;
	const FLinearColor Mark(1.f, 1.f, 1.f, 0.55f);
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = (i & 1) ? 1920.f - M : M;
		const float Y = (i & 2) ? PicBottom - M : PicTop + M;
		Rect((i & 1) ? X - L : X, Y - 1.f, L, 2.f, Mark);
		Rect(X - 1.f, (i & 2) ? Y - L : Y, 2.f, L, Mark);
	}
	if (ReelY >= 0.f)
	{
		float Playhead = -1.f;
		if (GM->State == EDCState::Film)
		{
			Playhead = GM->FilmTime;
		}
		else if (GM->Reel.Num() > 2)
		{
			// during play the head waits at the first hole in the film
			const FDCReelSeg& G = GM->Reel[2];
			Playhead = GM->State == EDCState::Flashback ? FMath::Lerp(G.T0, G.T1, FMath::Clamp(GM->StateTime / GM->FlashLength, 0.f, 1.f)) : (GM->bGapFilled[0] ? G.T1 : G.T0);
		}
		DrawReel(GM, ReelY, Playhead);
	}
}

void AMatchCutHUD::DrawClapper(AMatchCutGameMode* GM)
{
	// a failed take: clapper board, then straight back to the start mark
	const float X = 610.f, Y = 330.f, W = 700.f, H = 400.f;
	Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Rect(X, Y, W, H, FLinearColor(0.03f, 0.03f, 0.035f, 1.f));
	Frame(X, Y, W, H, White, 5.f);
	for (int32 i = 0; i < 10; ++i)
	{
		Rect(X + i * 70.f, Y, 70.f, 64.f, (i % 2) ? FLinearColor(0.03f, 0.03f, 0.035f, 1.f) : White);
	}
	Rect(X, Y + 64.f, W, 5.f, White);
	Text(FString::Printf(DCTEXT_SCENE, GM->Cur + 1), X + 40.f, Y + 96.f, 2.0f, Dim);
	Text(FString::Printf(DCTEXT_CAM_TAG, GM->Cur + 1), X + W - 40.f, Y + 96.f, 2.0f, Dim, 2);
	Text(FString::Printf(DCTEXT_TAKE, GM->Take), X + W * 0.5f, Y + 165.f, 7.0f, White, 1);
	Text(GM->TakeReason, X + W * 0.5f, Y + 322.f, 2.2f, Red, 1);
}

void AMatchCutHUD::DrawFilm(AMatchCutGameMode* GM)
{
	const float T = GM->FilmTime;
	const FDCReelSeg* Seg = nullptr;
	for (const FDCReelSeg& S : GM->Reel)
	{
		if (T >= S.T0 && T < S.T1)
		{
			Seg = &S;
		}
	}
	const FLinearColor Black(0.f, 0.f, 0.f, 1.f);
	if (!Seg)
	{
		Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, Black);
		Text(DCText::FilmEnd, 960.f, 500.f, 3.2f, White, 1);
	}
	else if (Seg->Gap != INDEX_NONE)
	{
		// a hole in the film
		Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, Black);
		const float Flick = FMath::Fmod(T * 9.f, 1.f) < 0.12f ? 0.75f : 1.f;
		Frame(520.f, 400.f, 880.f, 150.f, FLinearColor(Red.R, Red.G, Red.B, Flick), 5.f);
		Text(DCText::GapStamp, 960.f, 425.f, 5.2f, FLinearColor(Red.R, Red.G, Red.B, Flick), 1);
		Text(DCText::GapBy, 960.f, 590.f, 2.4f, FLinearColor(0.9f, 0.9f, 0.9f, 1.f), 1);
	}
	else
	{
		// caption plate, like a company film
		const float In = FMath::Clamp((T - Seg->T0 - 0.4f) * 3.f, 0.f, 1.f);
		Rect(0.f, 838.f, 1920.f, 74.f, FLinearColor(0.f, 0.f, 0.f, 0.55f * In));
		Text(Seg->Caption, 960.f, 852.f, 2.6f, FLinearColor(1.f, 1.f, 1.f, In), 1);
	}

	DrawBars(GM, 985.f);
	Text(DCText::FilmTitle, 60.f, 52.f, 1.7f, Dim);
	if (GM->bFilmSkippable)
	{
		Text(DCText::FilmSkip, 1860.f, 56.f, 1.3f, Dim, 2);
	}
	// play symbol and timecode, like a film player
	Rect(300.f, 995.f, 8.f, 34.f, White);
	Rect(316.f, 995.f, 8.f, 34.f, White);
	Text(Timecode(T, 0), 60.f, 1000.f, 1.6f, White);
	Text(Timecode(GM->FilmLength, 0), 1860.f, 1000.f, 1.6f, Dim, 2);
}

void AMatchCutHUD::DrawFlashback(AMatchCutGameMode* GM)
{
	const float T = GM->StateTime;
	// old film: flicker, scratches, dust
	FRandomStream Rand(FMath::FloorToInt(T * 14.f) * 31 + 7);
	Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, FLinearColor(0.f, 0.f, 0.f, Rand.FRandRange(0.f, 0.14f)));
	Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, FLinearColor(0.75f, 0.55f, 0.25f, 0.07f));
	for (int32 i = 0; i < 2; ++i)
	{
		if (Rand.FRand() < 0.55f)
		{
			const float X = Rand.FRandRange(80.f, 1840.f);
			Rect(X, PicTop, 2.f, PicBottom - PicTop, FLinearColor(1.f, 1.f, 0.9f, Rand.FRandRange(0.10f, 0.32f)));
		}
	}
	for (int32 i = 0; i < 7; ++i)
	{
		const float S = Rand.FRandRange(2.f, 7.f);
		Rect(Rand.FRandRange(0.f, 1920.f), Rand.FRandRange(PicTop, PicBottom), S, S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
	}
	// rounded gate: dark edges left and right
	Rect(0.f, PicTop, 26.f, PicBottom - PicTop, FLinearColor(0.f, 0.f, 0.f, 0.85f));
	Rect(1894.f, PicTop, 26.f, PicBottom - PicTop, FLinearColor(0.f, 0.f, 0.f, 0.85f));

	Text(DCText::Recovered, 70.f, PicTop + 30.f, 1.6f, FLinearColor(1.f, 0.9f, 0.7f, 0.9f));
	Text(FString::Printf(TEXT("TC %s"), *Timecode(T + 1403.f, 3)), 1850.f, PicBottom - 62.f, 1.6f, FLinearColor(1.f, 0.9f, 0.7f, 0.9f), 2);

	// burn in at the start, fade out at the end
	if (T < 0.35f)
	{
		Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, FLinearColor(1.f, 0.95f, 0.85f, 1.f - T / 0.35f));
	}
	const float Left = GM->FlashLength - T;
	if (Left < 0.7f)
	{
		Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, FLinearColor(0.f, 0.f, 0.f, 1.f - FMath::Max(Left, 0.f) / 0.7f));
	}

	DrawBars(GM, 40.f);
	Text(FString::Printf(TEXT("SCENE 1/%d"), AMatchCutGameMode::NumGaps), 960.f, 985.f, 1.8f, Amber, 1);
}

void AMatchCutHUD::DrawHUD()
{
	Super::DrawHUD();

	AMatchCutGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMatchCutGameMode>() : nullptr;
	if (!GM || !Canvas || GM->Shots.Num() == 0)
	{
		return;
	}
	K = Canvas->ClipY / 1080.f; // the Canvas is the 16:9 view rect, whatever the window is

	if (GM->State == EDCState::Film)
	{
		DrawFilm(GM);
		return;
	}
	if (GM->State == EDCState::Flashback)
	{
		DrawFlashback(GM);
		return;
	}

	const float Time = GM->PlayClock;
	const bool bPlayLike = GM->State == EDCState::Playing || GM->State == EDCState::TakeCard || GM->State == EDCState::Drop;
	if (bPlayLike)
	{
		DrawGhost(GM);
		DrawPreview(GM);
	}
	if (GM->CutFlash > 0.f)
	{
		Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, FLinearColor(1.f, 1.f, 1.f, FMath::Clamp(GM->CutFlash * 1.2f - 0.25f, 0.f, 0.5f)));
	}
	if (GM->State == EDCState::TakeCard)
	{
		DrawClapper(GM);
	}
	if (GM->BannerTime > 0.f && !GM->Banner1.IsEmpty())
	{
		const float A = FMath::Clamp(GM->BannerTime, 0.f, 1.f);
		Rect(560.f, PicTop + 34.f, 800.f, 64.f, FLinearColor(0.f, 0.f, 0.f, 0.6f * A));
		Text(GM->Banner1, 960.f, PicTop + 44.f, 2.6f, FLinearColor(Amber.R, Amber.G, Amber.B, A), 1);
	}
	if (GM->State == EDCState::EndCard)
	{
		Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(GM->StateTime, 0.f, 0.72f)));
		Text(DCText::EndTitle, 960.f, 400.f, 6.f, White, 1);
		Text(FString::Printf(DCTEXT_END_LINE, GM->bGapFilled[0] ? 1 : 0, AMatchCutGameMode::NumGaps, GM->bCrewRescued ? 1 : 0, GM->CrewTotal), 960.f, 560.f, 2.2f, Amber, 1);
		Text(DCText::EndKeys, 960.f, 640.f, 1.8f, Dim, 1);
	}

	// the film's timeline slides from the player bar at the bottom to the top bar when the film lets go
	float ReelY = 40.f;
	if (GM->State == EDCState::Drop)
	{
		const float A = FMath::Clamp(GM->StateTime / 1.2f, 0.f, 1.f);
		ReelY = FMath::Lerp(985.f, 40.f, A * A * (3.f - 2.f * A));
	}
	DrawBars(GM, ReelY);

	// top bar: scene/take on the left, crew on the right
	Text(FString::Printf(TEXT("%s   %s"), *FString::Printf(DCTEXT_SCENE, GM->Cur + 1), *FString::Printf(DCTEXT_TAKE, GM->Take)), 60.f, 52.f, 1.5f, Dim);
	Text(FString::Printf(DCTEXT_CREW, GM->bCrewRescued ? 1 : 0, GM->CrewTotal), 1860.f, 52.f, 1.7f, GM->bCrewRescued ? Amber : Dim, 2);

	// bottom bar: survey-camera tag, one line of guidance, timecode
	if (GM->State != EDCState::Drop || GM->StateTime > 1.0f)
	{
		if (FMath::Fmod(Time, 1.4f) < 0.9f)
		{
			Disc(74.f, 978.f, 9.f, Red);
		}
		Text(DCText::Rec, 94.f, 965.f, 1.5f, Red);
		Text(FString::Printf(DCTEXT_CAM_TAG, GM->Cur + 1), 60.f, 1005.f, 1.5f, White);
		Text(GM->Shots[GM->Cur].Name, 60.f, 1040.f, 1.1f, Dim);
		Text(Timecode(Time, 1), 1860.f, 985.f, 1.6f, White, 2);

		FString Hint = GM->Shots[GM->Cur].Hint;
		FLinearColor HintCol = White;
		float HintScale = 1.9f;
		if (GM->State == EDCState::TakeCard || GM->State == EDCState::EndCard || GM->State == EDCState::Drop)
		{
			Hint.Empty();
		}
		else if (GM->SoftFailTime > 0.f)
		{
			Hint = DCText::SoftFail;
			HintCol = Amber;
			HintScale = 2.3f;
		}
		else if (GM->BannerTime > 0.f && !GM->Banner2.IsEmpty())
		{
			Hint = GM->Banner2;
			HintCol = Amber;
		}
		else if (!GM->bFirstCutDone)
		{
			Hint = DCText::Tutorial;
			HintScale = 2.3f;
			HintCol = FLinearColor(1.f, 1.f, 1.f, 0.8f + 0.2f * FMath::Sin(Time * 4.f));
		}
		else if (GM->InRestore())
		{
			Hint = DCText::RestoreHint;
			HintCol = Amber;
		}
		else if (GM->Cur == GM->PartShot && GM->bHasPart)
		{
			Hint = DCText::DarkHint;
		}
		Text(Hint, 960.f, 962.f, HintScale, HintCol, 1);
		Text(GM->bHasPart ? DCText::ControlsLamp : DCText::Controls, 960.f, 1036.f, 1.15f, Dim, 1);
	}
}
