// Deep Cut: HUD. Layout space is 1920x1080; the picture is y 135..945, the two bars belong to the HUD.

#include "MatchCutHUD.h"
#include "MatchCutGameMode.h"
#include "DeepCutText.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CanvasItem.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Rendering/SlateRenderer.h"
#include "TextureResource.h"

namespace DCHud
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

using namespace DCHud;

void AMatchCutHUD::LoadFonts()
{
	// two OFL fonts shipped as loose files in Content/Data/Fonts (see ASSETS.md); the engine font is the fallback
	bFontsTried = true;
	RuntimeFont = NewObject<UFont>(this);
	RuntimeFont->FontCacheType = EFontCacheType::Runtime;
	const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Data/Fonts"));
	const FString Mono = Dir / TEXT("IBMPlexMono-Regular.ttf");
	const FString Title = Dir / TEXT("BigShoulders-Bold.ttf");
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	if (FPaths::FileExists(Mono))
	{
		MonoFont = MakeShared<FStandaloneCompositeFont>(NAME_None, Mono, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
	}
	if (FPaths::FileExists(Title))
	{
		TitleFont = MakeShared<FStandaloneCompositeFont>(NAME_None, Title, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
	}
	UE_LOG(LogMatchCut, Display, TEXT("HUD fonts: mono=%d title=%d (%s)"), MonoFont.IsValid() ? 1 : 0, TitleFont.IsValid() ? 1 : 0, *Dir);
}

void AMatchCutHUD::Text(const FString& Str, float X, float Y, float Scale, const FLinearColor& Color, int32 Align)
{
	if (!Canvas || Str.IsEmpty())
	{
		return;
	}
	if (!bFontsTried)
	{
		LoadFonts();
	}
	const bool bTitle = Scale >= 2.5f && TitleFont.IsValid();
	const TSharedPtr<const FCompositeFont>& Face = bTitle ? TitleFont : MonoFont;
	if (Face.IsValid() && RuntimeFont && FSlateApplication::IsInitialized())
	{
		// rasterised at the size it is drawn at, so it stays sharp at any resolution
		const float Size = FMath::Max(6.f, Scale * K * (bTitle ? 12.5f : 10.0f));
		const FSlateFontInfo Info(Face, Size);
		const FVector2D Extent = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str, Info);
		float Px = X * K;
		if (Align == 1) { Px -= Extent.X * 0.5f; }
		if (Align == 2) { Px -= Extent.X; }
		FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(Px), FMath::RoundToFloat(Y * K)), FText::FromString(Str), RuntimeFont, Color);
		Item.SlateFontInfo = Info;
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Color.A));
		Canvas->DrawItem(Item);
		return;
	}
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!Font)
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

void AMatchCutHUD::Quad(const FVector2D& C, float AngleDeg, float HalfLen, float HalfWidth, const FLinearColor& Color)
{
	const float A = FMath::DegreesToRadians(AngleDeg);
	const FVector2D D(FMath::Cos(A), FMath::Sin(A)), N(-D.Y, D.X);
	const FVector2D P0 = (C - D * HalfLen - N * HalfWidth) * K, P1 = (C + D * HalfLen - N * HalfWidth) * K;
	const FVector2D P2 = (C + D * HalfLen + N * HalfWidth) * K, P3 = (C - D * HalfLen + N * HalfWidth) * K;
	TArray<FCanvasUVTri> Tris;
	FCanvasUVTri T;
	T.V0_Color = T.V1_Color = T.V2_Color = Color;
	T.V0_UV = T.V1_UV = T.V2_UV = FVector2D(0.5, 0.5);
	T.V0_Pos = P0; T.V1_Pos = P1; T.V2_Pos = P2;
	Tris.Add(T);
	T.V0_Pos = P0; T.V1_Pos = P2; T.V2_Pos = P3;
	Tris.Add(T);
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AMatchCutHUD::Tape(const FVector2D& C, int32 Shape, float Size, const FLinearColor& Color, float Tilt)
{
	// a film-set floor mark: two strips of tape. Shape 0 = X, 1 = T.
	const float W = Size * 0.17f;
	const FLinearColor Shadow(0.f, 0.f, 0.f, 0.45f * Color.A);
	const FLinearColor Hi(FMath::Min(Color.R * 1.25f + 0.1f, 1.f), FMath::Min(Color.G * 1.25f + 0.1f, 1.f), FMath::Min(Color.B * 1.25f + 0.1f, 1.f), Color.A * 0.55f);
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		const FVector2D O = C + (Pass == 0 ? FVector2D(3.0, 4.0) : FVector2D::ZeroVector);
		const FLinearColor Col = Pass == 0 ? Shadow : (Pass == 1 ? Color : Hi);
		const float Wd = Pass == 2 ? W * 0.25f : W;
		const FVector2D Lift = Pass == 2 ? FVector2D(0.0, -W * 0.45) : FVector2D::ZeroVector;
		if (Shape == 1)
		{
			Quad(O + Lift + FVector2D(0.0, -Size * 0.55), Tilt, Size, Wd, Col);
			Quad(O + (Pass == 2 ? FVector2D(-W * 0.45, 0.0) : FVector2D::ZeroVector) + FVector2D(0.0, Size * 0.2), 90.f + Tilt, Size * 0.75f, Wd, Col);
		}
		else
		{
			Quad(O + Lift, 45.f + Tilt, Size, Wd, Col);
			Quad(O + Lift, -45.f + Tilt * 0.4f, Size, Wd, Col);
		}
	}
}

void AMatchCutHUD::Brackets(float X, float Y, float R, const FLinearColor& Color, float Thick)
{
	const float L = R * 0.45f;
	for (int32 i = 0; i < 4; ++i)
	{
		const float Sx = (i & 1) ? 1.f : -1.f, Sy = (i & 2) ? 1.f : -1.f;
		Line(X + Sx * R, Y + Sy * R, X + Sx * (R - L), Y + Sy * R, Color, Thick);
		Line(X + Sx * R, Y + Sy * R, X + Sx * R, Y + Sy * (R - L), Color, Thick);
	}
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
	const FDCShot& Shot = GM->Shots[GM->Cur];
	if (!bRestore && !Shot.Exits.IsValidIndex(GM->GhostExit))
	{
		return;
	}
	const float Time = GM->PlayClock;

	// a hint first (strong, breathing), a quiet guide once the first cut is done
	float A = GM->GhostOpacity;
	if (!GM->bFirstCutDone || bRestore)
	{
		A *= 1.25f + 0.45f * FMath::Sin(Time * 4.f);
	}
	else if (!Shot.bGateOpen)
	{
		A *= 0.6f; // the next shot stays faint until this one is solved
	}
	if (Shot.bDark)
	{
		A *= 0.18f; // on a dark picture the same ghost would glare
	}
	FLinearColor Tint = GM->GhostTint * A;
	Tint.A = 1.f;

	if (!bRestore && GM->Shots[Shot.Exits[GM->GhostExit].To].bDark)
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

void AMatchCutHUD::DrawMarks(AMatchCutGameMode* GM)
{
	const bool bRestore = GM->InRestore();
	const FDCShot& Shot = GM->Shots[GM->Cur];
	const float Time = GM->PlayClock;
	const FVector2D HeroPx = NdcToPx(GM->HeroNdc);
	const FDCCutEval& E = GM->CurEval;
	const bool bPlaying = GM->State == EDCState::Playing;
	auto RingPx = [GM](double Depth) { return (float)(GM->HeroRadius / (GM->TanH * FMath::Max(Depth, 100.0)) * 960.0 * 1.32); };

	if (bRestore)
	{
		// restore-by-cut keeps its own look: Unit 7's place in the removed frame, no tape
		const FVector2D T = NdcToPx(GM->RestoreNdc);
		const float R = GM->RestoreTol * 540.f + 44.f;
		const float Pulse = 1.f + 0.08f * FMath::Sin(Time * 5.f);
		Ring(T.X, T.Y, R * Pulse, Amber, 4.f);
		Line(T.X - R - 26.f, T.Y, T.X - R + 6.f, T.Y, Amber, 3.f);
		Line(T.X + R - 6.f, T.Y, T.X + R + 26.f, T.Y, Amber, 3.f);
		Line(T.X, T.Y - R - 26.f, T.X, T.Y - R + 6.f, Amber, 3.f);
		Line(T.X, T.Y + R - 6.f, T.X, T.Y + R + 26.f, Amber, 3.f);
		Text(DCText::RestoreMark, T.X, T.Y - R - 62.f, 1.4f, Amber, 1);
		if (bPlaying)
		{
			const FLinearColor Col = E.bOk ? Green : Red;
			const float HR = RingPx(GM->HeroS.Z);
			Disc(HeroPx.X, HeroPx.Y, HR, FLinearColor(Col.R, Col.G, Col.B, 0.16f));
			Ring(HeroPx.X, HeroPx.Y, HR, Col, 4.f);
			Text(E.bOk ? DCText::RingOk : E.Reason, HeroPx.X, HeroPx.Y + HR + 10.f, 1.25f, Col, 1);
		}
		return;
	}

	// sockets of this shot that still want something
	for (const FDCSocket& S : GM->Sockets)
	{
		if (S.Shot == GM->Cur && !S.bFilled)
		{
			const FVector2D P = NdcToPx(GM->ViewNdc(GM->Cur, S.S, false));
			const float R = 60.f * 900.f / (float)FMath::Max(S.S.Z, 100.0);
			Brackets(P.X, P.Y, R * (1.f + 0.06f * FMath::Sin(Time * 4.f)), Amber, 3.f);
			Text(S.Label, P.X, P.Y - R - 34.f, 1.1f, Amber, 1);
		}
	}

	const int32 ExitIndex = Shot.Exits.IsValidIndex(E.Exit) ? E.Exit : (Shot.Exits.Num() > 0 ? 0 : INDEX_NONE);
	if (ExitIndex != INDEX_NONE && GM->GhostMode != 0)
	{
		const FDCExit& Active = Shot.Exits[ExitIndex];
		const bool bTargetSeen = !GM->Shots[Active.To].bDark || GM->bLampOn;

		// where things of the next shot are: its sockets, and the crew signal
		for (const FDCSocket& S : GM->Sockets)
		{
			const FVector2D N = GM->ViewNdc(Active.To, S.S, true);
			if (S.Shot == Active.To && !S.bFilled && GM->MarkVisible(Active, N))
			{
				// green once a loose thing of this shot sits on the frame's place
				const int32 SocketIndex = (int32)(&S - GM->Sockets.GetData());
				const bool bOn = GM->Props.ContainsByPredicate([GM, SocketIndex](const FDCProp& Pr) { return GM->PropFitsSocket(Pr, SocketIndex); });
				const FLinearColor Col = bOn ? Green : FLinearColor(Amber.R, Amber.G, Amber.B, 0.75f);
				const FVector2D P = NdcToPx(N);
				const float R = 60.f * 900.f / (float)FMath::Max(S.S.Z, 100.0);
				Brackets(P.X, P.Y, R, Col, bOn ? 3.5f : 2.f);
				Text(S.Label, P.X, P.Y - R - 34.f, 1.0f, Col, 1);
			}
		}
		for (const FDCExit& Exit : Shot.Exits)
		{
			if (Exit.To == GM->CrewShot && !GM->bCrewRescued && Shot.bGateOpen)
			{
				// distress beacon of the trapped crew robot: seen even in the dark, so the player knows where to point the lamp
				const FVector2D B = NdcToPx(GM->ViewNdc(Exit.To, GM->Shots[Exit.To].CrewS, true));
				const float Blink = FMath::Fmod(Time, 1.2f);
				if (Blink < 0.75f)
				{
					// in the tape colour of the camera that leads to it, so label, colour and hint agree
					Disc(B.X, B.Y, 7.f, Exit.Color);
					Ring(B.X, B.Y, 12.f + 40.f * Blink, FLinearColor(Exit.Color.R, Exit.Color.G, Exit.Color.B, 0.8f - Blink), 2.5f);
				}
				Text(DCText::CrewSignal, B.X, B.Y + 30.f, 1.0f, Exit.Color, 1);
			}
		}

		// tape marks: only once this shot is solved
		if (Shot.bGateOpen)
		{
			const float Reveal = FMath::Clamp(GM->RevealTime / 1.2f, 0.f, 1.f); // 1 just revealed .. 0 settled
			for (int32 e = 0; e < Shot.Exits.Num(); ++e)
			{
				const FDCExit& Exit = Shot.Exits[e];
				for (int32 m = 0; m < Exit.Marks.Num(); ++m)
				{
					const FVector2D N = GM->ViewNdc(Exit.To, Exit.Marks[m], true);
					if (!GM->MarkVisible(Exit, N))
					{
						continue;
					}
					const FVector2D P = NdcToPx(N);
					const bool bMine = e == E.Exit && m == E.Mark;
					const float Size = FMath::Clamp(62.f * 900.f / (float)FMath::Max(Exit.Marks[m].Z, 100.0), 26.f, 120.f) * (1.f + 0.9f * Reveal * Reveal);
					FLinearColor Col = Exit.Color;
					Col.A = (1.f - 0.5f * Reveal) * (bMine || Shot.Exits.Num() == 1 ? 1.f : 0.8f);
					if (Exit.Period > 0.f)
					{
						// a mark that comes and goes: faint while it is closed, with an arc that runs down to the next opening
						const float Ph = FMath::Fmod(Time, Exit.Period);
						if (Ph >= Exit.OpenFor)
						{
							Col.A *= 0.22f;
							const float Left = (Exit.Period - Ph) / FMath::Max(Exit.Period - Exit.OpenFor, 0.1f);
							const int32 Segs = FMath::CeilToInt(24.f * Left);
							for (int32 s = 0; s < Segs; ++s)
							{
								const float A0 = -HALF_PI + s * (2.f * PI / 24.f), A1 = A0 + 2.f * PI / 24.f * 0.7f;
								const float Rr = GM->MarkRadius * 1080.f + 14.f;
								Line(P.X + Rr * FMath::Cos(A0), P.Y + Rr * FMath::Sin(A0), P.X + Rr * FMath::Cos(A1), P.Y + Rr * FMath::Sin(A1), FLinearColor(1.f, 1.f, 1.f, 0.6f), 3.f);
							}
						}
					}
					Tape(P, Exit.Shape, Size, Col, ((m * 37 + e * 11) % 13) - 6.f);
					// capture radius, and the size the robot's ring must have at this mark's depth
					const float Cap = GM->MarkRadius * 1080.f;
					Ring(P.X, P.Y, Cap, FLinearColor(Exit.Color.R, Exit.Color.G, Exit.Color.B, bMine && E.DistPx <= Cap ? 0.75f : 0.22f), 2.f);
					if (!Exit.bAnyDepth)
					{
						Ring(P.X, P.Y, RingPx(Exit.Marks[m].Z), FLinearColor(1.f, 1.f, 1.f, 0.55f), 2.f);
					}
					if (Shot.Exits.Num() > 1)
					{
						Text(FString::Printf(DCTEXT_MARK_CAM, Exit.To + 1, *GM->Shots[Exit.To].Name), P.X, P.Y - GM->MarkRadius * 1080.f - 30.f, 1.1f, Exit.Color, 1);
					}
					if (Reveal > 0.f)
					{
						// focus pull: brackets close in on the new mark
						Brackets(P.X, P.Y, Size * 1.2f + 260.f * Reveal * Reveal, FLinearColor(1.f, 1.f, 1.f, 0.9f * (1.f - Reveal * 0.5f)), 3.f);
					}
				}
			}
			if (Reveal > 0.f)
			{
				Text(DCText::MarksUp, 960.f, PicTop + 120.f, 2.0f, FLinearColor(1.f, 1.f, 1.f, FMath::Min(1.f, Reveal * 2.f)), 1);
			}
		}

		// loose things: what a cut would do with each of them
		if (bPlaying)
		{
			bool bAnyLoose = false;
			for (const FDCProp& Prop : GM->Props)
			{
				if (Prop.Shot != GM->Cur || Prop.bLocked)
				{
					continue;
				}
				bAnyLoose = true;
				if (!Shot.bGateOpen)
				{
					continue;
				}
				const int32 R = GM->PropAfterCut(Prop, Active);
				const FVector2D P = NdcToPx(GM->ViewNdc(GM->Cur, Prop.S, false));
				const float Rad = 62.f * 900.f / (float)FMath::Max(Prop.S.Z, 100.0);
				const FLinearColor Col = R == 2 ? Green : (R == 1 ? Active.Color : FLinearColor(Dim.R, Dim.G, Dim.B, 0.7f));
				Ring(P.X, P.Y, Rad, Col, R == 0 ? 2.f : 3.5f);
				Text(R == 2 ? DCText::PropLands : (R == 1 ? DCText::PropComes : DCText::PropStays), P.X, P.Y + Rad + 6.f, 1.0f, Col, 1);
			}
			if (bAnyLoose && Shot.bGateOpen)
			{
				// the carry radius: what is inside this circle comes through the cut
				const float CR = (float)(GM->CarryRadius / (GM->TanH * FMath::Max(GM->HeroS.Z, 100.0)) * 960.0);
				const int32 N = 48;
				for (int32 i = 0; i < N; i += 2)
				{
					const float A0 = 2.f * UE_PI * i / N + Time * 0.2f, A1 = 2.f * UE_PI * (i + 1) / N + Time * 0.2f;
					Line(HeroPx.X + CR * FMath::Cos(A0), HeroPx.Y + CR * FMath::Sin(A0), HeroPx.X + CR * FMath::Cos(A1), HeroPx.Y + CR * FMath::Sin(A1), FLinearColor(1.f, 1.f, 1.f, 0.28f), 2.f);
				}
			}
		}
		(void)bTargetSeen;
	}

	if (!bPlaying)
	{
		return;
	}

	// what E would do, at the thing itself
	const bool bCarrying = GM->Props.ContainsByPredicate([GM](const FDCProp& P) { return P.bCarried && P.Shot == GM->Cur; });
	FVector UseS;
	const FString Use = GM->UsePrompt(UseS);
	if (!Use.IsEmpty())
	{
		// above the thing; while something is held, above the robot (the held thing hangs under it)
		const FVector2D P = bCarrying ? FVector2D(HeroPx.X, HeroPx.Y - RingPx(GM->HeroS.Z) + 26.f) : NdcToPx(GM->ViewNdc(GM->Cur, UseS, false)); // held: the box ends 48 px over the ring, above the cut label
		Rect(P.X - 74.f, P.Y - 108.f, 148.f, 34.f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Text(Use, P.X, P.Y - 104.f, 1.35f, White, 1);
	}
	else if (Shot.bHasSwitch && !Shot.Switch.bOn)
	{
		// the valve asks to be looked at
		const FVector2D P = NdcToPx(GM->ViewNdc(GM->Cur, Shot.Switch.S, false));
		const float Ph = FMath::Fmod(Time, 1.4f) / 1.4f;
		Ring(P.X, P.Y, 46.f + 50.f * Ph, FLinearColor(1.f, 1.f, 1.f, 0.6f * (1.f - Ph)), 2.5f);
	}

	// the landing: where the robot would be after the cut. Same place, same size; green = C works, red = why not.
	if (Shot.Exits.Num() == 0)
	{
		return;
	}
	const float HR = RingPx(GM->HeroS.Z);
	if (!Shot.bGateOpen)
	{
		Ring(HeroPx.X, HeroPx.Y, HR, FLinearColor(Dim.R, Dim.G, Dim.B, 0.45f), 2.f);
		return;
	}
	const FLinearColor Col = E.bOk ? Green : Red;
	Disc(HeroPx.X, HeroPx.Y, HR, FLinearColor(Col.R, Col.G, Col.B, 0.16f));
	Ring(HeroPx.X, HeroPx.Y, HR, Col, 4.f);
	// under the ring; over it when something hangs under the robot or the ring is at the bottom of the picture
	const bool bUnder = !bCarrying && HeroPx.Y + HR + 10.f < PicBottom - 40.f;
	const float LabelY = bUnder ? HeroPx.Y + HR + 10.f : HeroPx.Y - HR - 38.f;
	const FString Label = E.bOk ? DCText::RingOk : E.Reason;
	const float LabelW = Label.Len() * 13.f + 20.f;
	Rect(HeroPx.X - LabelW * 0.5f, LabelY - 3.f, LabelW, 32.f, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Text(Label, HeroPx.X, LabelY, 1.25f, Col, 1);
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
		else if (const FDCReelSeg* First = GM->Reel.FindByPredicate([](const FDCReelSeg& S) { return S.Gap == 0; }))
		{
			// during play the head waits at the first hole in the film
			const FDCReelSeg& G = *First;
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

void AMatchCutHUD::DrawFeatures(AMatchCutGameMode* GM, float X, float Y, float Scale, bool bCard)
{
	// the design file's feature list: struck out = cut from the prototype, plain = built again
	float Cx = X;
	for (int32 i = 0; i < 3; ++i)
	{
		const bool bBuilt = GM->bFeature[i] && GM->State != EDCState::Film;
		const FString Name = DCText::Features[i];
		const float W = Name.Len() * 8.f * Scale;
		if (bCard)
		{
			const float Ry = Y + i * 62.f;
			Text(Name, X, Ry, Scale, bBuilt ? Amber : Dim);
			Line(X - 10.f, Ry + 9.f * Scale, X + W + 10.f, Ry + 9.f * Scale, Red, 3.f);
			Text(DCText::CardCut, X + 520.f, Ry, Scale, Red);
		}
		else
		{
			Text(Name, Cx, Y, Scale, bBuilt ? Amber : FLinearColor(Dim.R, Dim.G, Dim.B, 0.7f));
			if (!bBuilt)
			{
				Line(Cx - 3.f, Y + 9.f * Scale, Cx + W + 3.f, Y + 9.f * Scale, Red, 2.f);
			}
			Cx += W + 22.f;
		}
	}
}

void AMatchCutHUD::DrawFilm(AMatchCutGameMode* GM)
{
	if (GM->StateTime < 3.2f)
	{
		// design-file card: the prototype that was shelved, and what was cut from it
		Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(0.02f, 0.025f, 0.03f, 1.f));
		Frame(360.f, 190.f, 1200.f, 700.f, Dim, 3.f);
		Text(DCText::CardFile, 400.f, 215.f, 1.5f, Dim);
		Text(DCText::CardTitle, 400.f, 270.f, 6.0f, White);
		Text(DCText::CardShelved, 400.f, 430.f, 3.0f, Red);
		Text(DCText::CardReason, 400.f, 500.f, 1.8f, Dim);
		Text(DCText::CardFeatures, 400.f, 590.f, 1.5f, Dim);
		DrawFeatures(GM, 400.f, 640.f, 2.4f, true);
		if (GM->bFilmSkippable)
		{
			Text(DCText::FilmSkip, 1860.f, 56.f, 1.3f, Dim, 2);
		}
		return;
	}
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
	for (int32 Row = 0; Row < 34; ++Row) // play triangle
	{
		Rect(300.f, 995.f + Row, 26.f * (1.f - FMath::Abs(Row - 16.5f) / 17.f), 1.f, White);
	}
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
		DrawMarks(GM);
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
	Text(DCText::HudUnit, 60.f, 96.f, 1.0f, Dim);
	DrawFeatures(GM, 136.f, 96.f, 1.0f, false);

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

		const FDCShot& Shot = GM->Shots[GM->Cur];
		FString Hint = Shot.bGateOpen && !Shot.HintSolved.IsEmpty() ? Shot.HintSolved : Shot.Hint;
		FLinearColor HintCol = White;
		float HintScale = 1.7f;
		if (GM->State == EDCState::TakeCard || GM->State == EDCState::EndCard || GM->State == EDCState::Drop)
		{
			Hint.Empty();
		}
		else if (GM->SoftFailTime > 0.f)
		{
			Hint = Shot.bGateOpen ? DCText::SoftFail : DCText::SoftFailNoMark;
			HintCol = Amber;
		}
		else if (GM->BannerTime > 0.f && !GM->Banner2.IsEmpty())
		{
			Hint = GM->Banner2;
			HintCol = Amber;
		}
		else if (GM->InRestore())
		{
			Hint = DCText::RestoreHint;
			HintCol = Amber;
		}
		else if (!GM->bFirstCutDone)
		{
			HintCol = FLinearColor(1.f, 1.f, 1.f, 0.8f + 0.2f * FMath::Sin(Time * 4.f));
		}
		Text(Hint, 960.f, 962.f, HintScale, HintCol, 1);
		Text(FString(DCText::ControlsMove) + (GM->bFeature[1] ? DCText::ControlsGrab : TEXT("")) + (GM->bFeature[0] ? DCText::ControlsLampKey : TEXT("")) + DCText::ControlsEnd, 960.f, 1040.f, 1.0f, Dim, 1);
	}
}
