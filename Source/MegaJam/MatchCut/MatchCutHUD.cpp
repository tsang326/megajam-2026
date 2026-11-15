// Deep Cut: HUD. Layout space is 1920x1080; the picture is y 135..945, the two bars belong to the HUD.

#include "MatchCutHUD.h"
#include "MatchCutGameMode.h"
#include "DC3D.h"
#include "DCDoc.h"
#include "DCDocText.h"
#include "DCMenu.h"
#include "DeepCutText.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/StrongObjectPtr.h"
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

void AMatchCutHUD::Text(const FString& InStr, float X, float Y, float Scale, const FLinearColor& Color, int32 Align)
{
	if (!Canvas || InStr.IsEmpty())
	{
		return;
	}
	const FString Str = bNoTr ? InStr : DCLoc::Tr(InStr);
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
				if (S.Accepts == TEXT("long") || S.Accepts == TEXT("round"))
				{
					// graphic match: the shape the next shot has in this place, as an outline
					FDCProp Shape;
					Shape.Kind = S.Accepts;
					const FVector H = GM->PropHalf(Shape);
					const float W = (float)(H.X / (GM->TanH * FMath::Max(S.S.Z, 100.0)) * 960.0);
					const float Hh = FMath::Max(W * (float)(H.Z / H.X) * 0.38f, 10.f);
					if (S.Accepts == TEXT("round"))
					{
						for (int32 i = 0; i < 32; ++i)
						{
							const float A0 = 2.f * UE_PI * i / 32, A1 = 2.f * UE_PI * (i + 1) / 32;
							Line(P.X + W * FMath::Cos(A0), P.Y + Hh * FMath::Sin(A0), P.X + W * FMath::Cos(A1), P.Y + Hh * FMath::Sin(A1), Col, bOn ? 4.f : 3.f);
						}
					}
					else
					{
						Frame(P.X - W, P.Y - Hh, 2.f * W, 2.f * Hh, Col, bOn ? 4.f : 3.f);
					}
					Text(S.Label, P.X, P.Y - Hh - 34.f, 1.0f, Col, 1);
					// a loose thing on the place with another shape: say so
					for (const FDCProp& Pr : GM->Props)
					{
						const FVector2D Q = NdcToPx(GM->ViewNdc(GM->Cur, Pr.S, false));
						if (Pr.Shot == GM->Cur && !Pr.bLocked && Pr.Kind != S.Accepts && FVector2D::Distance(Q, P) <= GM->SocketRadius * 1080.f)
						{
							Rect(P.X - 80.f, P.Y + Hh + 8.f, 160.f, 30.f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
							Text(DCText::WrongShape, P.X, P.Y + Hh + 10.f, 1.2f, Red, 1);
						}
					}
				}
				else
				{
					Brackets(P.X, P.Y, R, Col, bOn ? 3.5f : 2.f);
					Text(S.Label, P.X, P.Y - R - 34.f, 1.0f, Col, 1);
				}
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
			for (const FDCExit& Exit : Shot.Exits)
			{
				if (Exit.When == TEXT("clear") && GM->GentleShot == Exit.To && Exit.Period > 0.f)
				{
					// what is in the next shot's frame right now: the big creature, drawn as a warm shape over the landing place
					const FVector2D C = NdcToPx(GM->ViewNdc(Exit.To, GM->GentleS, true));
					if (!Exit.OpenAt(Time))
					{
						Text(DCText::NextShotBody, C.X, C.Y - 150.f, 1.1f, FLinearColor(1.f, 0.72f, 0.35f, 0.95f), 1);
					}
				}
			}
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
					if (Exit.bBack)
					{
						Text(FString::Printf(TEXT("%s %s"), DCText::BackMark, *GM->Shots[Exit.To].Name), P.X, P.Y - GM->MarkRadius * 1080.f - 30.f, 1.1f, Exit.Color, 1);
					}
					else if (Shot.Exits.Num() > 1)
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
				Text(R == 2 ? DCText::PropLands : (R == 1 ? DCText::PropComes : (Prop.bHeavy ? DCText::PropHeavy : DCText::PropStays)), P.X, P.Y + Rad + 6.f, 1.0f, Col, 1);
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
			if (GM->ThumbTargets.IsValidIndex(S.Gap) && GM->ThumbTargets[S.Gap] && GM->ThumbTargets[S.Gap]->GetResource())
			{
				FCanvasTileItem Tile(FVector2D((SX + 2.f) * K, Y * K), GM->ThumbTargets[S.Gap]->GetResource(), FVector2D((SW - 4.f) * K, H * K), White);
				Tile.BlendMode = SE_BLEND_Opaque;
				Canvas->DrawItem(Tile);
			}
			Frame(SX + 1.f, Y, SW - 2.f, H, Green, 3.f);
		}
		else
		{
			Rect(SX + 1.f, Y, SW - 2.f, H, FLinearColor(0.f, 0.f, 0.f, 1.f));
			const bool bActive = GM->InRestore() && S.Gap == GM->Shots[GM->Cur].PartGap;
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
		else
		{
			// during play the head waits at the first hole in the film that is still open
			int32 Want = 0;
			while (Want < AMatchCutGameMode::NumGaps - 1 && GM->bGapFilled[Want])
			{
				++Want;
			}
			const bool bRestoring = GM->State == EDCState::Flashback && !GM->bFlashOfficial;
			if (bRestoring)
			{
				Want = GM->FlashGap;
			}
			if (const FDCReelSeg* G = GM->Reel.FindByPredicate([Want](const FDCReelSeg& S) { return S.Gap == Want; }))
			{
				Playhead = bRestoring ? FMath::Lerp(G->T0, G->T1, FMath::Clamp(GM->StateTime / GM->FlashLength, 0.f, 1.f)) : (GM->bGapFilled[Want] ? G->T1 : G->T0);
			}
		}
		DrawReel(GM, ReelY, Playhead);
	}
}

void AMatchCutHUD::DrawClapper(AMatchCutGameMode* GM)
{
	// a failed take: clapper board, then straight back to the start mark
	if (GM->bCaught)
	{
		// caught by the predator: the signal is lost. No blood, no fight: static, then the clapper.
		Noise(GM->StateTime, FMath::Clamp(1.3f - GM->StateTime * 0.5f, 0.4f, 1.f));
		if (GM->StateTime < 0.8f)
		{
			Text(DCText::SignalLost, 960.f, 480.f, 4.5f, White, 1);
			return;
		}
	}
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
	// what to do next (brief 05)
	const FString How = GM->TakeHow;
	if (!How.IsEmpty())
	{
		Rect(960.f - How.Len() * 9.5f - 24.f, Y + H + 22.f, How.Len() * 19.f + 48.f, 50.f, FLinearColor(0.f, 0.f, 0.f, 0.8f));
		Text(How, 960.f, Y + H + 30.f, 1.7f, White, 1);
	}
}

void AMatchCutHUD::Arrow(const FVector2D& From, const FVector2D& To, const FLinearColor& Color, float Thick)
{
	const FVector2D D = (To - From).GetSafeNormal();
	const FVector2D N(-D.Y, D.X);
	Line(From.X, From.Y, To.X, To.Y, Color, Thick);
	const FVector2D B = To - D * 22.0;
	Line(To.X, To.Y, B.X + N.X * 13.0, B.Y + N.Y * 13.0, Color, Thick);
	Line(To.X, To.Y, B.X - N.X * 13.0, B.Y - N.Y * 13.0, Color, Thick);
}

void AMatchCutHUD::DrawTeach(AMatchCutGameMode* GM)
{
	const FDCShot& Shot = GM->Shots[GM->Cur];
	const float Time = GM->PlayClock;
	const FVector2D HeroPx = NdcToPx(GM->HeroNdc);

	// depth can be read: a soft shadow on the floor under Unit 0, with a thin line down to it
	{
		const FVector F(GM->HeroS.X, GM->HeroFloorY, GM->HeroS.Z);
		const FVector2D P = NdcToPx(GM->ViewNdc(GM->Cur, F, false));
		const float W = (float)(GM->HeroRadius / (GM->TanH * FMath::Max(GM->HeroS.Z, 100.0)) * 960.0) * 1.25f;
		if (P.Y < PicBottom - 4.f && P.Y > HeroPx.Y)
		{
			const int32 N = 28;
			Rect(P.X - W * 0.7f, P.Y - W * 0.12f, W * 1.4f, W * 0.24f, FLinearColor(0.f, 0.f, 0.f, 0.35f));
			for (int32 i = 0; i < N; ++i)
			{
				const float A0 = 2.f * UE_PI * i / N, A1 = 2.f * UE_PI * (i + 1) / N;
				Line(P.X + W * FMath::Cos(A0), P.Y + W * 0.28f * FMath::Sin(A0), P.X + W * FMath::Cos(A1), P.Y + W * 0.28f * FMath::Sin(A1), FLinearColor(0.55f, 0.95f, 1.f, 0.55f), 2.f);
			}
			for (float Y = HeroPx.Y + W; Y < P.Y - 6.f; Y += 14.f)
			{
				Rect(P.X - 1.f, Y, 2.f, 6.f, FLinearColor(0.55f, 0.95f, 1.f, 0.25f));
			}
		}
	}

	// loose things read at a glance: a thin ring until the marks show (then the STAYS / COMES / FITS ring takes over)
	if (!GM->InRestore())
	{
		for (const FDCProp& Prop : GM->Props)
		{
			if (Prop.Shot == GM->Cur && !Prop.bLocked && !Prop.bCarried && !Shot.bGateOpen)
			{
				const FVector2D P = NdcToPx(GM->ViewNdc(GM->Cur, Prop.S, false));
				const float Rad = 62.f * 900.f / (float)FMath::Max(Prop.S.Z, 100.0);
				Ring(P.X, P.Y, Rad * (1.f + 0.04f * FMath::Sin(Time * 3.f)), FLinearColor(1.f, 1.f, 1.f, 0.55f), 2.f);
			}
		}
	}

	// PUSH cue on contact
	if (GM->PushCue > 0.f && GM->Props.IsValidIndex(GM->PushProp))
	{
		const FDCProp& Prop = GM->Props[GM->PushProp];
		const FVector2D P = NdcToPx(GM->ViewNdc(GM->Cur, Prop.S, false));
		const float A = FMath::Clamp(GM->PushCue * 3.f, 0.f, 1.f);
		Rect(P.X - 44.f, P.Y - 96.f, 88.f, 30.f, FLinearColor(0.f, 0.f, 0.f, 0.6f * A));
		Text(DCText::PushCue, P.X, P.Y - 94.f, 1.25f, FLinearColor(1.f, 1.f, 1.f, A), 1);
		Arrow(FVector2D(P.X + GM->PushDir * 60.f, P.Y - 46.f), FVector2D(P.X + GM->PushDir * 110.f, P.Y - 46.f), FLinearColor(1.f, 1.f, 1.f, A), 3.f);
	}

	// the one-time prompt for a control
	if (GM->PromptTime > 0.f && GM->PromptId != INDEX_NONE)
	{
		const FString S = DCText::Prompts[GM->PromptId];
		const float A = FMath::Clamp(GM->PromptTime * 2.f, 0.f, 1.f);
		const float W = S.Len() * 21.f + 60.f;
		const float Y = PicTop + 150.f;
		Rect(960.f - W * 0.5f, Y, W, 58.f, FLinearColor(0.f, 0.f, 0.f, 0.72f * A));
		Frame(960.f - W * 0.5f, Y, W, 58.f, FLinearColor(1.f, 1.f, 1.f, 0.8f * A), 2.f);
		Text(S, 960.f, Y + 10.f, 1.9f, FLinearColor(1.f, 1.f, 1.f, A), 1);
		if (GM->PromptId == DCTeach::Depth)
		{
			Text(DCText::DepthFirst, 960.f, Y + 70.f, 1.3f, FLinearColor(Amber.R, Amber.G, Amber.B, A), 1);
		}
	}

	// stuck for 20 s: point at the thing to use, and where it goes
	if (GM->bStuckShown && GM->PredPhase == 0)
	{
		FVector2D A, B;
		bool bTo = false;
		const float Ph = FMath::Fmod(Time, 1.1f) / 1.1f;
		if (GM->StuckTarget(A, B, bTo))
		{
			const FVector2D P = NdcToPx(A);
			const float Bob = 10.f * FMath::Sin(Time * 6.f);
			Ring(P.X, P.Y, 50.f + 60.f * Ph, FLinearColor(1.f, 1.f, 1.f, 0.9f * (1.f - Ph)), 4.f);
			Ring(P.X, P.Y, 50.f, White, 3.f);
			Arrow(FVector2D(P.X, P.Y - 190.f + Bob), FVector2D(P.X, P.Y - 70.f + Bob), White, 6.f);
			if (bTo)
			{
				const FVector2D Q = NdcToPx(B);
				const FVector2D D = (Q - P).GetSafeNormal();
				if (FVector2D::Distance(P, Q) > 150.0)
				{
					Arrow(P + D * 70.0, Q - D * 70.0, FLinearColor(Amber.R, Amber.G, Amber.B, 0.6f + 0.4f * FMath::Sin(Time * 6.f)), 4.f);
				}
			}
		}
		else
		{
			Rect(660.f, PicTop + 220.f, 600.f, 50.f, FLinearColor(0.f, 0.f, 0.f, 0.7f));
			Text(DCText::StuckBack, 960.f, PicTop + 228.f, 1.7f, White, 1);
		}
	}
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

void AMatchCutHUD::Noise(float T, float Alpha)
{
	// no signal: grey bars that change twenty times a second
	FRandomStream Rand(FMath::FloorToInt(T * 20.f) * 53 + 11);
	Rect(0.f, PicTop, 1920.f, PicBottom - PicTop, FLinearColor(0.05f, 0.05f, 0.06f, Alpha));
	for (int32 i = 0; i < 200; ++i)
	{
		const float W = Rand.FRandRange(30.f, 420.f), H = Rand.FRandRange(3.f, 16.f);
		const float G = Rand.FRandRange(0.1f, 0.95f);
		Rect(Rand.FRandRange(-100.f, 1900.f), Rand.FRandRange(PicTop, PicBottom - H), W, H, FLinearColor(G, G, G * 1.05f, Alpha * Rand.FRandRange(0.25f, 0.9f)));
	}
}

void AMatchCutHUD::DrawHeadCam(float ClipT)
{
	// a diver's mask in front of the lens: dark rounded corners, the nose piece, breath going up past the glass
	const FLinearColor Rubber(0.f, 0.f, 0.f, 0.95f);
	const float H = PicBottom - PicTop;
	Rect(0.f, PicTop, 70.f, H, Rubber);
	Rect(1850.f, PicTop, 70.f, H, Rubber);
	Rect(70.f, PicTop, 1780.f, 26.f, Rubber);
	Rect(70.f, PicBottom - 30.f, 1780.f, 30.f, Rubber);
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bRight = (i & 1) != 0, bLow = (i & 2) != 0;
		for (int32 s = 0; s < 10; ++s)
		{
			// a stepped corner: ten bars, each shorter than the one before
			const float W = 260.f * FMath::Square(1.f - s / 10.f);
			const float Y = bLow ? PicBottom - 30.f - (s + 1) * 20.f : PicTop + 26.f + s * 20.f;
			Rect(bRight ? 1850.f - W : 70.f, Y, W, 20.f, Rubber);
		}
	}
	Disc(960.f, PicBottom + 90.f, 200.f, Rubber);
	for (int32 i = 0; i < 14; ++i)
	{
		// one breath every 3.4 s
		FRandomStream Rand(i * 97 + 5);
		const float Age = FMath::Fmod(ClipT + 0.3f, 3.4f) - Rand.FRandRange(0.f, 0.9f);
		const float Side = Rand.FRandRange(-260.f, 260.f);
		const float Rise = Rand.FRandRange(260.f, 420.f);
		const float Size = Rand.FRandRange(6.f, 22.f);
		const float Y = PicBottom - 40.f - Age * Rise;
		if (Age < 0.f || Age > 2.2f || Y < PicTop + 30.f)
		{
			continue;
		}
		const float X = 960.f + Side + 30.f * FMath::Sin(Age * 5.f + i);
		const float R = Size * (1.f + 0.2f * Age);
		const float A = 0.55f * (1.f - Age / 2.2f);
		Ring(X, Y, R, FLinearColor(0.85f, 0.95f, 1.f, A), 2.5f);
		Disc(X - R * 0.3f, Y - R * 0.3f, R * 0.2f, FLinearColor(1.f, 1.f, 1.f, A));
	}
	if (FMath::Fmod(ClipT, 1.f) < 0.6f)
	{
		Disc(112.f, PicTop + 78.f, 8.f, Red);
	}
	Text(DCText::DiverCam, 130.f, PicTop + 66.f, 1.4f, White);
}

void AMatchCutHUD::DrawFlashback(AMatchCutGameMode* GM)
{
	const float T = GM->StateTime;
	const FDCClip* Clip = GM->Clips.Find(GM->FlashClip);
	const FDCFilmSet* Set = GM->ClipSet(GM->FlashClip);
	const bool bHead = Set && Set->bHeadCam;
	const float CT = GM->FlashClipT;
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

	if (bHead)
	{
		DrawHeadCam(CT);
	}
	Text(GM->bFlashOfficial ? DCText::OfficialRecord : DCText::Recovered, bHead ? 130.f : 70.f, PicTop + 30.f, 1.6f, FLinearColor(1.f, 0.9f, 0.7f, 0.9f));
	// the timecode is the clip's own: in the official cut it jumps where seconds were taken out
	Text(FString::Printf(TEXT("TC %s"), *Timecode(CT + (bHead ? 2710.f : 1403.f), 3)), bHead ? 1800.f : 1850.f, PicBottom - 62.f, 1.6f, FLinearColor(1.f, 0.9f, 0.7f, 0.9f), 2);
	if (Clip)
	{
		for (const FDCCaption& C : Clip->Captions)
		{
			if (CT >= C.T0 && CT < C.T1 && (C.Who == 0 || (C.Who == 1) == GM->bFlashOfficial))
			{
				Rect(0.f, 838.f, 1920.f, 74.f, FLinearColor(0.f, 0.f, 0.f, 0.55f));
				Text(C.Text, 960.f, 852.f, 2.6f, White, 1);
			}
		}
		if (!GM->bFlashOfficial && Clip->InCut(CT))
		{
			// these seconds were not in the official version
			const float Blink = FMath::Fmod(T, 0.9f) < 0.6f ? 1.f : 0.45f;
			Frame(8.f, PicTop + 4.f, 1904.f, PicBottom - PicTop - 8.f, FLinearColor(Amber.R, Amber.G, Amber.B, 0.85f * Blink), 5.f);
			Rect(760.f, PicTop + 24.f, 400.f, 44.f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			Text(DCText::CutByFathom, 960.f, PicTop + 30.f, 1.8f, FLinearColor(Amber.R, Amber.G, Amber.B, Blink), 1);
		}
		for (const FVector2D& S : Clip->Static)
		{
			if (CT >= S.X && CT < S.Y)
			{
				Noise(T, 1.f);
				Text(DCText::SignalLost, 960.f, 480.f, 4.5f, White, 1);
			}
		}
	}

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
	Text(GM->bFlashOfficial ? FString(DCText::OfficialTag) : FString::Printf(TEXT("SCENE %d/%d"), GM->FlashGap + 1, AMatchCutGameMode::NumGaps), 960.f, 985.f, 1.8f, GM->bFlashOfficial ? Dim : Amber, 1);
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
	if (GM->World3D())
	{
		const FDCMenu& Menu = DCMenu();
		if (!(Menu.IsOpen() && !Menu.bStarted))
		{
			Draw3D(GM);
		}
		if (DCTutorial().IsActive())
		{
			DrawTutorial(GM);
		}
		if (Menu.IsOpen())
		{
			DrawMenu(GM);
		}
		return;
	}

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
		if (GM->State == EDCState::Playing)
		{
			DrawTeach(GM);
		}
	}
	if (GM->PredPhase > 0 && GM->State == EDCState::Playing)
	{
		// the predator: which side it comes from, and where it is. Slow blink = warning, fast = it is coming.
		const bool bLeft = GM->PredS.X < GM->HeroS.X;
		const float A = 0.30f + 0.25f * FMath::Sin(Time * (GM->PredPhase == 2 ? 11.f : 4.5f));
		Rect(bLeft ? 0.f : 1896.f, PicTop, 24.f, PicBottom - PicTop, FLinearColor(Red.R, Red.G, Red.B, A));
		const FVector2D P = NdcToPx(GM->ViewNdc(GM->Cur, GM->PredS, false));
		Brackets(FMath::Clamp((float)P.X, 70.f, 1850.f), FMath::Clamp((float)P.Y, PicTop + 70.f, PicBottom - 70.f), 78.f, FLinearColor(Red.R, Red.G, Red.B, 0.45f + A), 3.f);
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
		Text(FString::Printf(DCTEXT_END_TITLE, GM->Shots[GM->Cur].Stage), 960.f, 400.f, 6.f, White, 1);
		Text(FString::Printf(DCTEXT_END_LINE, GM->ScenesCount(), AMatchCutGameMode::NumGaps, GM->CrewCount(), GM->CrewTotal), 960.f, 560.f, 2.2f, Amber, 1);
		Text(GM->HasNextStage() ? DCText::EndKeysNext : DCText::EndKeys, 960.f, 640.f, 1.8f, GM->HasNextStage() ? White : Dim, 1);
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
	Text(FString::Printf(DCTEXT_CREW, GM->CrewCount(), GM->CrewTotal), 1860.f, 52.f, 1.7f, GM->CrewCount() > 0 ? Amber : Dim, 2);
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
		if (Shot.bShoalGone && !Shot.HintAfter.IsEmpty())
		{
			Hint = Shot.HintAfter;
		}
		FLinearColor HintCol = White;
		float HintScale = 1.7f;
		if (GM->State == EDCState::TakeCard || GM->State == EDCState::EndCard || GM->State == EDCState::Drop)
		{
			Hint.Empty();
		}
		else if (GM->PredPhase > 0)
		{
			Hint = Shot.ShoalSolid != INDEX_NONE && !Shot.bShoalGone ? DCText::PredHintTool : DCText::PredHint;
			HintCol = Red;
		}
		else if (GM->SoftFailTime > 0.f)
		{
			// the reason and the next action: the shot's own hint says what to do (brief 05)
			Hint = Shot.bGateOpen ? FString(DCText::SoftFail) : FString(DCText::SoftFailNoMark) + TEXT(" ") + Shot.Hint;
			HintCol = Amber;
			HintScale = Hint.Len() > 70 ? 1.45f : 1.7f;
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
		Text(GM->ControlsLine(), 960.f, 1040.f, 1.0f, Dim, 1);
	}
}

// ---------------------------------------------------------------------------------------------
// the 3D game (brief 06): a survey camera's overlay, and as little text as possible
// ---------------------------------------------------------------------------------------------

void AMatchCutHUD::Draw3D(AMatchCutGameMode* GM)
{
	const FDC3D& W = *GM->World3D();
	const float Time = GM->PlayClock;
	if (!W.Doc)
	{
		return;
	}
	const FDCDoc& D = *W.Doc;
	auto Pips = [this](float X, float Y, int32 Stars, float R)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			if (i < Stars)
			{
				Disc(X + i * R * 2.8f, Y, R, Amber);
			}
			else
			{
				Ring(X + i * R * 2.8f, Y, R, FLinearColor(1.f, 1.f, 1.f, 0.35f), 1.5f);
			}
		}
	};
	auto Fade = [](const FLinearColor& C, float A) { return FLinearColor(C.R, C.G, C.B, C.A * A); };
	auto Money = [](int64 V) { return FString::Printf(TEXT("$%lld"), V); };
	auto Big = [](int64 V) { return V >= 10000 ? FString::Printf(TEXT("%.1fk"), V / 1000.0) : FString::Printf(TEXT("%lld"), V); };

	// ---------------- the Director's Cut, and how it ends
	if (D.State == EDCDocState::Film || D.State == EDCDocState::Rating)
	{
		Rect(0.f, 0.f, 1920.f, 104.f, FLinearColor::Black);
		Rect(0.f, 976.f, 1920.f, 104.f, FLinearColor::Black);
		if (D.State == EDCDocState::Rating)
		{
			Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(D.StateTime * 1.5f, 0.f, 0.94f)));
			static const TCHAR* Heads[4] = { TEXT(""), TEXT("THE TRUTH"), TEXT("DOUBT"), TEXT("THE OFFICIAL STORY") };
			static const TCHAR* Lines1[4] = { TEXT(""),
				TEXT("The hearing watched your film twice. Nobody spoke."),
				TEXT("Some of the panel believed it. The company called it a fake."),
				TEXT("Without proof, the company's film stands as the record.") };
			static const TCHAR* Lines2[4] = { TEXT(""),
				TEXT("The trench is closed to drilling. What lives there is left alone."),
				TEXT("The drilling goes on, slower, with cameras watching."),
				TEXT("The trench opens for drilling next spring. Something down there will notice.") };
			const int32 E = FMath::Clamp(D.Ending, 1, 3);
			Text(TEXT("DEEP CUT"), 960.f, 190.f, 3.f, Dim, 1);
			Text(Heads[E], 960.f, 280.f, 6.f, E == 1 ? Green : E == 2 ? Amber : Red, 1);
			Text(Lines1[E], 960.f, 450.f, 1.9f, White, 1);
			Text(Lines2[E], 960.f, 500.f, 1.9f, White, 1);
			Text(FString::Printf(TEXT("PROOFS  %d / %d"), D.EvidenceCount(), D.EvidenceTotal()), 960.f, 620.f, 2.2f, Amber, 1);
			Text(FString::Printf(TEXT("%s views   %s subscribers   %s earned"), *Big(D.TotalViews), *Big(D.Subscribers), *Money(D.Money)), 960.f, 680.f, 1.5f, Dim, 1);
			int32 Col = 0;
			for (int32 s = 0; s < D.Subjects.Num(); ++s)
			{
				if (!D.Subjects[s].bEvidence)
				{
					continue;
				}
				bool bHave = false;
				for (const FDCTake& Kept : D.Archive)
				{
					bHave |= Kept.Shot == s;
				}
				const float X = 960.f + (Col % 5 - 2) * 330.f, Y = 760.f + (Col / 5) * 44.f;
				Text(bHave ? D.Subjects[s].Name : FString(TEXT("? ? ?")), X, Y, 1.2f, bHave ? Green : Dim, 1);
				++Col;
			}
			Text(TEXT("ENTER  watch it again"), 960.f, 900.f, 1.4f, Dim, 1);
			return;
		}
		if (D.FilmPart == 0)
		{
			Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor::Black);
			const float A = FMath::Clamp(FMath::Min(D.FilmTime * 1.2f, (5.f - D.FilmTime) * 2.f), 0.f, 1.f);
			Text(TEXT("THE DIRECTOR'S CUT"), 960.f, 400.f, 7.f, FLinearColor(1.f, 1.f, 1.f, A), 1);
			Text(TEXT("what the official film left out"), 960.f, 580.f, 2.f, Fade(Amber, A), 1);
		}
		else if (D.FilmPart > D.Takes.Num())
		{
			Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor::Black);
			const float A = FMath::Clamp(FMath::Min(D.FilmTime * 1.2f, (7.f - D.FilmTime) * 2.f), 0.f, 1.f);
			Text(FString::Printf(TEXT("%d of %d proofs on film."), D.EvidenceCount(), D.EvidenceTotal()), 960.f, 450.f, 2.6f, FLinearColor(1.f, 1.f, 1.f, A), 1);
			Text(TEXT("The hearing is in the morning."), 960.f, 540.f, 1.8f, Fade(Dim, A), 1);
		}
		else if (D.Takes.IsValidIndex(D.PlayTake))
		{
			const float A = FMath::Clamp(FMath::Min(D.FilmTime * 3.f, (2.6f - D.FilmTime) * 2.f), 0.f, 1.f);
			Text(D.SubjectName(D.Takes[D.PlayTake].Shot).ToUpper(), 120.f, 880.f, 2.4f, FLinearColor(1.f, 1.f, 1.f, A));
			Text(FString::Printf(TEXT("EXHIBIT %d"), D.FilmPart), 120.f, 840.f, 1.2f, Fade(Amber, A));
			Rect(120.f, 936.f, 90.f * A, 3.f, Fade(Amber, A));
			if (D.FilmTime < 0.12f)
			{
				Rect(0.f, 104.f, 1920.f, 872.f, FLinearColor(1.f, 1.f, 1.f, 0.35f)); // the splice
			}
		}
		return;
	}

	// ---------------- the cutting room: today's clips, the thumbnail, what is cut
	if (D.State == EDCDocState::Edit)
	{
		Rect(0.f, 0.f, 1920.f, 96.f, FLinearColor::Black);
		Rect(0.f, 850.f, 1920.f, 230.f, FLinearColor::Black);
		Text(FString::Printf(TEXT("CUTTING ROOM   DAY %d"), D.Day), 60.f, 30.f, 2.2f, White);
		Text(FString::Printf(TEXT("%d CLIPS   %d IN THE VIDEO"), D.Takes.Num(), D.ClipsKept()), 1860.f, 34.f, 1.8f, Amber, 2);
		const int32 First = FMath::Clamp(D.EditShot - 2, 0, FMath::Max(0, D.Takes.Num() - 6));
		for (int32 k = 0; k < 6 && First + k < D.Takes.Num(); ++k)
		{
			const int32 i = First + k;
			const FDCTake& T = D.Takes[i];
			const float X = 60.f + k * 302.f;
			const bool bSel = i == D.EditShot;
			const FDCSubject* S = D.Subjects.IsValidIndex(T.Shot) ? &D.Subjects[T.Shot] : nullptr;
			Rect(X, 872.f, 290.f, 128.f, FLinearColor(1.f, 1.f, 1.f, bSel ? 0.10f : 0.04f));
			Frame(X, 872.f, 290.f, 128.f, bSel ? Amber : FLinearColor(1.f, 1.f, 1.f, 0.25f), bSel ? 3.f : 1.5f);
			for (int32 h = 0; h < 7; ++h)
			{
				Rect(X + 14.f + h * 40.f, 862.f, 18.f, 6.f, FLinearColor(1.f, 1.f, 1.f, 0.25f));
				Rect(X + 14.f + h * 40.f, 1004.f, 18.f, 6.f, FLinearColor(1.f, 1.f, 1.f, 0.25f));
			}
			Text(D.SubjectName(T.Shot), X + 16.f, 884.f, 1.25f, T.bKeep ? White : Dim);
			Pips(X + 28.f, 940.f, T.Stars, 9.f);
			FString Tags;
			Tags += i == D.Headline ? TEXT("THUMBNAIL  ") : TEXT("");
			Tags += S && S->bEvidence ? TEXT("PROOF  ") : TEXT("");
			Tags += T.bDanger ? TEXT("CLOSE CALL  ") : TEXT("");
			Tags += T.Shot == D.Trending ? TEXT("TRENDING") : TEXT("");
			Text(Tags, X + 16.f, 962.f, 0.95f, S && S->bEvidence ? Green : Amber);
			if (!T.bKeep)
			{
				Rect(X, 934.f, 290.f, 3.f, Red);
				Text(TEXT("CUT"), X + 274.f, 884.f, 1.2f, Red, 2);
			}
		}
		Text(TEXT("A / D  clip     W  thumbnail (x2.5 views)     S  keep / cut     ENTER  upload     ESC  back"), 960.f, 1030.f, 1.3f, Dim, 1);
		if (D.Takes.IsValidIndex(D.PlayTake))
		{
			const float P = D.PlayT / FMath::Max(0.1f, D.Takes[D.PlayTake].Len);
			Rect(660.f, 836.f, 600.f, 3.f, FLinearColor(1.f, 1.f, 1.f, 0.2f));
			Rect(660.f, 836.f, 600.f * P, 3.f, Amber);
		}
		if (D.AlertTime > 0.f)
		{
			Text(D.Alert, 960.f, 130.f, 1.6f, Red, 1);
		}
		return;
	}

	// ---------------- the upload: what each clip brought
	if (D.State == EDCDocState::Upload)
	{
		Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(0.02f, 0.025f, 0.03f, 0.96f));
		Text(TEXT("UPLOADED"), 120.f, 90.f, 1.4f, Dim);
		Text(D.VideoTitle, 120.f, 124.f, 2.6f, White);
		const int32 Shown = FMath::Min(D.UploadLines.Num(), (int32)(D.StateTime * 5.f));
		for (int32 i = 0; i < Shown && i < 14; ++i)
		{
			const FDCUploadLine& L = D.UploadLines[i];
			const float Y = 230.f + i * 46.f;
			const bool bProof = D.Subjects.IsValidIndex(L.Subject) && D.Subjects[L.Subject].bEvidence;
			Text(D.SubjectName(L.Subject), 120.f, Y, 1.4f, bProof ? Green : White);
			Pips(560.f, Y + 14.f, L.Stars, 7.f);
			Text(L.bDup ? TEXT("again in this video") : L.bNew ? TEXT("NEW") : TEXT("seen before"), 680.f, Y + 2.f, 1.1f, L.bNew && !L.bDup ? Amber : Dim);
			Text(FString::Printf(TEXT("%s views"), *Big(L.Views)), 1080.f, Y, 1.4f, White, 2);
		}
		if (D.StateTime * 5.f > D.UploadLines.Num() + 2)
		{
			const float Y = 260.f + FMath::Min(D.UploadLines.Num(), 14) * 46.f;
			Rect(120.f, Y - 10.f, 960.f, 2.f, FLinearColor(1.f, 1.f, 1.f, 0.3f));
			Text(FString::Printf(TEXT("%s views    +%s    +%d subscribers"), *Big(D.VideoViews), *Money(D.Earned), D.NewSubs), 120.f, Y + 6.f, 2.f, Amber);
			Text(FString::Printf(TEXT("BANK %s"), *Money(D.Money)), 1080.f, Y + 6.f, 2.f, White, 2);
			Text(TEXT("COMMENTS"), 1240.f, 230.f, 1.2f, Dim);
			for (int32 i = 0; i < D.Comments.Num(); ++i)
			{
				Text(D.Comments[i], 1240.f, 270.f + i * 56.f, 1.15f, White);
			}
			Text(FString::Printf(TEXT("PROOFS IN THE ARCHIVE  %d / %d"), D.EvidenceCount(), D.EvidenceTotal()), 1240.f, 270.f + D.Comments.Num() * 56.f + 30.f, 1.3f, Green);
			Text(TEXT("ENTER  back to the base"), 960.f, 1010.f, 1.4f, Dim, 1);
		}
		return;
	}

	// ---------------- the locker: gear for money
	if (D.State == EDCDocState::Shop)
	{
		Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(0.02f, 0.025f, 0.03f, 0.9f));
		Text(TEXT("THE LOCKER"), 160.f, 120.f, 3.f, White);
		Text(Money(D.Money), 1760.f, 128.f, 2.6f, Amber, 2);
		for (int32 g = 0; g < DCGear::Num; ++g)
		{
			const float Y = 260.f + g * 100.f;
			const bool bSel = g == D.ShopSel;
			const int32 Cost = D.GearCost(g);
			Rect(160.f, Y - 14.f, 1600.f, 84.f, FLinearColor(1.f, 1.f, 1.f, bSel ? 0.10f : 0.03f));
			if (bSel)
			{
				Frame(160.f, Y - 14.f, 1600.f, 84.f, Amber, 2.f);
			}
			Text(D.GearName(g), 200.f, Y, 1.8f, White);
			for (int32 l = 0; l < D.GearLevels(g); ++l)
			{
				if (l <= D.Gear[g]) { Disc(560.f + l * 26.f, Y + 20.f, 8.f, Amber); }
				else { Ring(560.f + l * 26.f, Y + 20.f, 8.f, FLinearColor(1.f, 1.f, 1.f, 0.4f), 1.5f); }
			}
			Text(D.GearLine(g), 700.f, Y + 4.f, 1.5f, Dim);
			Text(Cost == INDEX_NONE ? FString(TEXT("BEST")) : Money(Cost), 1720.f, Y, 1.8f, Cost == INDEX_NONE ? Dim : Cost <= D.Money ? Green : Red, 2);
		}
		if (D.ShopNoteTime > 0.f)
		{
			Text(D.ShopNote, 960.f, 900.f, 1.6f, Amber, 1);
		}
		Text(TEXT("W / S  choose     ENTER  buy     ESC  close"), 960.f, 1000.f, 1.4f, Dim, 1);
		return;
	}

	// ---------------- diving
	const float Aim = D.AimBlend;
	const FLinearColor Edge(1.f, 1.f, 1.f, 0.35f);
	const float M = 44.f, L = 54.f;
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = (i & 1) ? 1920.f - M : M;
		const float Y = (i & 2) ? 1080.f - M : M;
		Rect((i & 1) ? X - L : X, Y - 1.f, L, 3.f, Edge);
		Rect(X - 1.f, (i & 2) ? Y - L : Y, 3.f, L, Edge);
	}
	if (D.bRecording)
	{
		Disc(84.f, 92.f, 10.f, Red);
		Text(DCText::Rec, 104.f, 78.f, 1.5f, Red);
	}
	else
	{
		Text(TEXT("STBY"), 76.f, 78.f, 1.5f, Dim);
	}
	Text(FString::Printf(TEXT("DAY %d / %d"), D.Day, D.Days), 190.f, 78.f, 1.5f, White);
	Text(FString::Printf(TEXT("TAPE %d / %d"), D.Takes.Num(), D.TapeCap()), 360.f, 78.f, 1.5f, D.Takes.Num() >= D.TapeCap() ? Red : Dim);
	Text(Money(D.Money), 560.f, 78.f, 1.5f, Amber);
	const int32 Heading = ((int32)FMath::RoundToInt(W.Yaw) % 360 + 360) % 360;
	Text(FString::Printf(TEXT("DEPTH %.0f m / %.0f   HDG %03d"), W.ZoneDepth, D.HullLimit(), Heading), 1836.f, 78.f, 1.5f, D.bOverDepth ? Red : Dim, 2);
	// air
	{
		const float P = FMath::Clamp(D.O2 / FMath::Max(1.f, D.O2Max), 0.f, 1.f);
		const bool bLow = P < 0.25f;
		const float Blink = bLow && !D.bInBase ? 0.6f + 0.4f * FMath::Sin(Time * 9.f) : 1.f;
		Text(TEXT("AIR"), 1476.f, 118.f, 1.2f, Fade(bLow ? Red : Dim, Blink));
		Rect(1530.f, 126.f, 306.f, 10.f, FLinearColor(1.f, 1.f, 1.f, 0.15f));
		Rect(1530.f, 126.f, 306.f * P, 10.f, Fade(bLow ? Red : P < 0.5f ? Amber : Green, Blink));
		Text(FString::Printf(TEXT("%d s"), FMath::CeilToInt(D.O2 / ((W.bBoost ? 1.25f : 1.f) * (D.bOverDepth ? 5.f : 1.f)))), 1836.f, 146.f, 1.1f, Dim, 2);
	}
	if (W.ZoneTitleTime >= 0.f && !W.ZoneTitle.IsEmpty())
	{
		const float T = W.ZoneTitleTime;
		const float A = FMath::Clamp(T / 0.8f, 0.f, 1.f) * FMath::Clamp((6.f - T) / 1.2f, 0.f, 1.f);
		Text(W.ZoneTitle, 960.f, 640.f, 2.6f, FLinearColor(1.f, 0.86f, 0.55f, 0.9f * A), 1);
		Text(FString::Printf(TEXT("%.0f m"), W.ZoneDepth), 960.f, 692.f, 1.4f, FLinearColor(1.f, 1.f, 1.f, 0.6f * A), 1);
	}
	// something that bites is close; a bite
	if (D.DangerNear > 0.f || D.bHunting)
	{
		const float A = 0.10f + 0.08f * FMath::Sin(Time * (D.bHunting ? 10.f : 5.f));
		Rect(0.f, 0.f, 26.f, 1080.f, FLinearColor(Red.R, Red.G, Red.B, A));
		Rect(1894.f, 0.f, 26.f, 1080.f, FLinearColor(Red.R, Red.G, Red.B, A));
	}
	if (D.HurtTime > 0.f)
	{
		Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(0.6f, 0.f, 0.f, 0.45f * D.HurtTime));
	}

	if (Aim > 0.5f)
	{
		const float A = (Aim - 0.5f) * 2.f;
		const FLinearColor Thin(1.f, 1.f, 1.f, 0.13f * A);
		Rect(640.f, 120.f, 1.5f, 840.f, Thin);
		Rect(1280.f, 120.f, 1.5f, 840.f, Thin);
		Rect(120.f, 360.f, 1680.f, 1.5f, Thin);
		Rect(120.f, 720.f, 1680.f, 1.5f, Thin);
		Rect(948.f, 539.f, 24.f, 2.f, FLinearColor(1.f, 1.f, 1.f, 0.5f * A));
		Rect(959.f, 528.f, 2.f, 24.f, FLinearColor(1.f, 1.f, 1.f, 0.5f * A));
		if (D.Seen.IsValidIndex(D.Lock))
		{
			const FDCSeen& S = D.Seen[D.Lock];
			const FDCSubject* Sub = D.Subjects.IsValidIndex(D.Lock) ? &D.Subjects[D.Lock] : nullptr;
			const FLinearColor C = Sub && Sub->bEvidence ? Green : S.bAction && S.bWhole ? Green : S.bAction ? Amber : FLinearColor(1.f, 1.f, 1.f, 0.7f);
			const float X = 960.f + (float)S.At.X * 960.f, Y = 540.f - (float)S.At.Y * 540.f;
			const float R = FMath::Clamp(S.Size * 540.f * 1.15f, 34.f, 420.f);
			Brackets(X, Y, R, C, 2.5f);
			const bool bKnown = D.TimesUploaded.Contains(D.Lock);
			Text(FString::Printf(TEXT("%s%s"), *D.SubjectName(D.Lock), Sub && Sub->bEvidence ? TEXT("   PROOF") : bKnown ? TEXT("") : TEXT("   NEW")), X, Y + R + 10.f, 1.2f, C, 1);
		}
		const float Focal = 18.f / FMath::Tan(FMath::DegreesToRadians(W.ViewFov * 0.5f));
		Text(FString::Printf(TEXT("%.0f mm     FOCUS %.1f m"), Focal, D.Focus / 100.f), 960.f, 1000.f, 1.5f, FLinearColor(1.f, 1.f, 1.f, 0.8f * A), 1);
		if (D.bRecording)
		{
			const float P = FMath::Clamp(D.RecTime / 3.f, 0.f, 1.f);
			Rect(760.f, 84.f, 400.f, 6.f, FLinearColor(1.f, 1.f, 1.f, 0.2f));
			Rect(760.f, 84.f, 400.f * P, 6.f, Red);
			Frame(96.f, 110.f, 1728.f, 860.f, FLinearColor(Red.R, Red.G, Red.B, 0.55f), 2.f);
		}
	}

	// the way home: the hatch under the base
	if (!D.bInBase && D.bLeftBase && !D.bRecording)
	{
		const FVector Hatch = W.HatchAt - FVector(0.0, 0.0, 120.0);
		FVector2D At;
		const bool bFront = D.Bearing(Hatch, At);
		const float Metres = (float)FVector::Dist(Hatch, W.Pos) / 100.f;
		const bool bLowAir = D.O2 < D.O2Max * 0.35f;
		const FLinearColor C = Fade(bLowAir ? Red : FLinearColor(0.6f, 0.85f, 1.f, 1.f), bLowAir ? 0.95f : 0.6f);
		const FString Label = FString::Printf(TEXT("BASE  %.0f m"), Metres);
		if (bFront && FMath::Abs(At.X) < 0.9 && FMath::Abs(At.Y) < 0.8)
		{
			const float X = 960.f + (float)At.X * 960.f, Y = 540.f - (float)At.Y * 540.f;
			Ring(X, Y, 12.f, C, 2.f);
			Text(Label, X, Y + 20.f, 1.1f, C, 1);
		}
		else
		{
			const double Kx = FMath::Abs(At.X) / 0.86, Ky = FMath::Abs(At.Y) / 0.74;
			const FVector2D E = At / FMath::Max(1.0, FMath::Max(Kx, Ky));
			const FVector2D P(960.0 + E.X * 960.0, 540.0 - E.Y * 540.0);
			const FVector2D Dir = FVector2D(At.X, -At.Y).GetSafeNormal();
			Arrow(P - Dir * 22.f, P + Dir * 22.f, C, 3.f);
			Text(Label, FMath::Clamp((float)(P.X - Dir.X * 70.0), 150.f, 1770.f), FMath::Clamp((float)(P.Y - Dir.Y * 60.0) - 10.f, 200.f, 900.f), 1.1f, C, 1);
		}
	}

	DrawMission(W, D);
	// under the moon pool: how to get in
	if (W.bHatchAssist && D.Spot == INDEX_NONE)
	{
		Rect(660.f, 690.f, 600.f, 52.f, FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Text(TEXT("SPACE   MOON POOL: swim up into the base"), 960.f, 700.f, 1.4f, Amber, 1);
	}
	// in the base: what the control in front of the diver does
	if (D.Spot != INDEX_NONE)
	{
		static const TCHAR* What[5] = { TEXT("E   EDIT DESK: cut and upload today's tape"), TEXT("E   LOCKER: buy gear"), TEXT("E   BUNK: sleep to the next day"), TEXT("E   BOARD: the proofs"), TEXT("E   NEWSPAPER: read it") };
		FString Line = What[D.Spot];
		if (D.Spot == 2 && D.Day >= D.Days)
		{
			Line = TEXT("E   BUNK: last night. The Director's Cut goes out.");
		}
		Rect(660.f, 690.f, 600.f, 52.f, FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Text(Line, 960.f, 700.f, 1.4f, Amber, 1);
	}
	if (D.bBoardOpen || D.bBagOpen)
	{
		DrawTargets(W, D, D.bBagOpen);
	}
	if (D.bNewsOpen)
	{
		DrawNews();
	}

	// the verdict on the last clip
	if (D.ResultTime > 0.f && !D.PageOpen())
	{
		const float A = FMath::Clamp(D.ResultTime, 0.f, 1.f);
		const bool bTip = !D.Tip.IsEmpty();
		Rect(bTip ? 520.f : 660.f, 256.f, bTip ? 880.f : 600.f, bTip ? 190.f : 150.f, FLinearColor(0.f, 0.f, 0.f, 0.55f * A));
		if (bTip)
		{
			Text(D.Tip, 960.f, 400.f, 1.45f, FLinearColor(1.f, 1.f, 1.f, 0.9f * A), 1);
		}
		if (D.ResultStars > 0)
		{
			Pips(960.f - 50.f, 290.f, D.ResultStars, 18.f);
			Text(D.SubjectName(D.ResultShot), 960.f, 322.f, 1.5f, FLinearColor(1.f, 1.f, 1.f, A), 1);
		}
		else
		{
			Text(DCDocText::NoGood, 960.f, 268.f, 2.6f, FLinearColor(Red.R, Red.G, Red.B, A), 1);
		}
		Text(D.ResultReason, 960.f, 358.f, 1.5f, Fade(Amber, A), 1);
	}

	// one warning line
	if (D.AlertTime > 0.f && !D.Alert.IsEmpty())
	{
		const float A = FMath::Clamp(D.AlertTime * 2.f, 0.f, 1.f);
		Rect(460.f, 196.f, 1000.f, 46.f, FLinearColor(0.f, 0.f, 0.f, 0.5f * A));
		Text(D.Alert, 960.f, 204.f, 1.35f, Fade(D.bOverDepth || D.O2 < D.O2Max * 0.25f || D.HurtTime > 0.f ? Red : Amber, A), 1);
	}

	// one line of help at a time
	FString Line;
	FLinearColor LineColor(1.f, 1.f, 1.f, 0.85f);
	if (D.Day == 1 && !D.bLeftBase && D.Spot == INDEX_NONE && !D.PageOpen())
	{
		Line = TEXT("The hatch in the floor leads out. Film what you find. Come back before the air runs out.");
	}
	else if (D.bInBase && D.Takes.Num() > 0 && !D.bUploadedToday && D.Spot == INDEX_NONE)
	{
		Line = TEXT("Tape to upload: the edit desk.");
		LineColor = Amber;
	}
	else if (!D.bEverAimed && !D.bInBase)
	{
		Line = DCDocText::TutAim;
	}
	else if (!D.bEverRecorded && Aim > 0.5f)
	{
		Line = DCDocText::TutRecord;
	}
	else if (!D.bEverZoomed && Aim > 0.5f && !D.bRecording)
	{
		Line = DCDocText::TutZoom;
	}
	else if (W.IdleTime > 12.f && Aim < 0.5f)
	{
		Line = DCText::Controls3D;
		LineColor.A = FMath::Clamp((W.IdleTime - 12.f) * 0.8f, 0.f, 0.85f);
	}
	if (!Line.IsEmpty() && !DCTutorial().IsActive())
	{
		Text(Line, 960.f, Aim > 0.5f ? 920.f : 962.f, 1.4f, LineColor, 1);
	}

	// the air ran out
	if (D.bBlackout)
	{
		const float A = FMath::Clamp(D.BlackT / 1.5f, 0.f, 1.f);
		Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(0.f, 0.f, 0.f, A));
		Text(TEXT("OUT OF AIR"), 960.f, 480.f, 3.f, Fade(Red, A), 1);
	}
}

// ---------------------------------------------------------------------------------------------
// the menus (DCMenu.h) and the tutorial card
// ---------------------------------------------------------------------------------------------

float AMatchCutHUD::TextWidth(const FString& Str, float Scale)
{
	if (!bFontsTried)
	{
		LoadFonts();
	}
	const bool bTitle = Scale >= 2.5f && TitleFont.IsValid();
	const TSharedPtr<const FCompositeFont>& Face = bTitle ? TitleFont : MonoFont;
	if (Face.IsValid() && FSlateApplication::IsInitialized())
	{
		const float Size = FMath::Max(6.f, Scale * K * (bTitle ? 12.5f : 10.0f));
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str, FSlateFontInfo(Face, Size)).X / FMath::Max(K, 0.01f);
	}
	return Str.Len() * 9.f * Scale;
}

float AMatchCutHUD::Wrap(const FString& InStr, float X, float Y, float Scale, const FLinearColor& Color, float MaxW, int32 Align)
{
	const FString Str = DCLoc::Tr(InStr);
	TArray<FString> Words;
	Str.ParseIntoArray(Words, TEXT(" "), true);
	const float LineH = 15.f * Scale + 6.f;
	FString Cur;
	float Yy = Y;
	bNoTr = true;
	for (const FString& Word : Words)
	{
		const FString Try = Cur.IsEmpty() ? Word : Cur + TEXT(" ") + Word;
		if (!Cur.IsEmpty() && TextWidth(Try, Scale) > MaxW)
		{
			Text(Cur, X, Yy, Scale, Color, Align);
			Yy += LineH;
			Cur = Word;
		}
		else
		{
			Cur = Try;
		}
	}
	if (!Cur.IsEmpty())
	{
		Text(Cur, X, Yy, Scale, Color, Align);
		Yy += LineH;
	}
	bNoTr = false;
	return Yy - Y;
}

// the targets: the board in the base and the bag (TAB) show the same list, ticked once filmed, with the still of each
void AMatchCutHUD::DrawTargets(const FDC3D& W, const FDCDoc& D, bool bBag)
{
	TArray<int32> List = D.Missions;
	if (List.Num() == 0)
	{
		for (int32 s = 0; s < D.Subjects.Num(); ++s)
		{
			if (D.Subjects[s].bEvidence)
			{
				List.Add(s);
			}
		}
	}
	int32 Found = 0, Proofs = 0, ProofsFound = 0;
	for (int32 s : List)
	{
		Found += D.IsFound(s) ? 1 : 0;
		Proofs += D.Subjects[s].bEvidence ? 1 : 0;
		ProofsFound += D.Subjects[s].bEvidence && D.IsFound(s) ? 1 : 0;
	}
	const float X0 = 300.f, Y0 = 110.f, Wd = 1320.f, Ht = 880.f;
	Rect(X0, Y0, Wd, Ht, FLinearColor(0.05f, 0.04f, 0.03f, 0.93f));
	Frame(X0, Y0, Wd, Ht, FLinearColor(0.6f, 0.45f, 0.25f, 1.f), 3.f);
	Text(bBag ? FString::Printf(TEXT("BAG   %d / %d"), Found, List.Num()) : FString::Printf(TEXT("THE PROOFS   %d / %d"), ProofsFound, Proofs), 960.f, Y0 + 22.f, 2.2f, White, 1);
	Text(TEXT("Film each target: in the frame is enough. Upload at the edit desk to make it count."), 960.f, Y0 + 76.f, 1.05f, Dim, 1);
	const int32 Now = D.Mission();
	const float RowH = 82.f;
	for (int32 r = 0; r < List.Num() && r < 9; ++r)
	{
		const int32 s = List[r];
		const FDCSubject& S = D.Subjects[s];
		const bool bHave = D.IsFound(s), bUp = D.IsUploaded(s);
		const float Y = Y0 + 120.f + r * RowH;
		if (s == Now)
		{
			Rect(X0 + 20.f, Y - 6.f, Wd - 40.f, RowH - 8.f, FLinearColor(1.f, 0.78f, 0.3f, 0.10f));
		}
		// the tick box
		const float Bx = X0 + 40.f, By = Y + 14.f;
		Frame(Bx, By, 36.f, 36.f, bHave ? Green : Dim, 2.f);
		if (bHave)
		{
			Line(Bx + 7.f, By + 19.f, Bx + 15.f, By + 28.f, Green, 4.f);
			Line(Bx + 15.f, By + 28.f, Bx + 30.f, By + 8.f, Green, 4.f);
		}
		// the still
		const float Px = X0 + 100.f, Pw = 112.f, Ph = 63.f;
		Rect(Px, Y + 2.f, Pw, Ph, FLinearColor(0.12f, 0.12f, 0.12f, 1.f));
		if (const TStrongObjectPtr<UTextureRenderTarget2D>* Ph2 = D.Photos.Find(s))
		{
			if (Ph2->IsValid() && (*Ph2)->GetResource())
			{
				FCanvasTileItem Tile(FVector2D(Px * K, (Y + 2.f) * K), (*Ph2)->GetResource(), FVector2D(Pw * K, Ph * K), FLinearColor::White);
				Tile.BlendMode = SE_BLEND_Opaque;
				Canvas->DrawItem(Tile);
			}
		}
		const bool bNamed = bHave || !S.bEvidence;
		Text(bNamed ? S.Name : FString(TEXT("? ? ?")), X0 + 240.f, Y + 6.f, 1.5f, bHave ? Green : White);
		const FString Where = W.Zones.IsValidIndex(S.Zone) ? FString(W.Zones[S.Zone].Title) : FString(TEXT("near the station"));
		Text(FString::Printf(TEXT("%s%s"), *Where, S.bLensOnly ? TEXT("   (seen only through the camera)") : TEXT("")), X0 + 240.f, Y + 42.f, 1.0f, Dim);
		const TCHAR* State = bUp ? TEXT("uploaded") : bHave ? TEXT("on the tape: upload it at the edit desk") : s == Now ? TEXT("TARGET NOW") : TEXT("");
		Text(State, X0 + Wd - 40.f, Y + 20.f, 1.1f, bUp ? Green : bHave ? Amber : Amber, 2);
	}
	Text(FString::Printf(TEXT("Day %d of %d. The hearing is after day %d."), D.Day, D.Days, D.Days), 960.f, Y0 + Ht - 50.f, 1.2f, Amber, 1);
	Text(bBag ? TEXT("TAB / ESC  close") : TEXT("E / ESC  close"), X0 + Wd - 24.f, Y0 + 26.f, 1.1f, Dim, 2);
}

// the target now: its name at the top left, a marker on it in the sea; the saved still flying into the bag
void AMatchCutHUD::DrawMission(const FDC3D& W, const FDCDoc& D)
{
	const FLinearColor Gold(1.f, 0.82f, 0.25f, 1.f);
	const int32 Now = D.Mission();
	if (Now != INDEX_NONE && !D.PageOpen() && D.Subjects.IsValidIndex(Now))
	{
		const FDCSubject& S = D.Subjects[Now];
		const bool bNamed = !S.bEvidence;
		const FString Name = bNamed ? S.Name : FString(TEXT("? ? ?"));
		const FString Where = W.Zones.IsValidIndex(S.Zone) ? FString(W.Zones[S.Zone].Title) : FString(TEXT("near the station"));
		FVector At;
		const bool bAt = D.MissionWhere(Now, At);
		const float Metres = bAt ? (float)FVector::Dist(At, W.Pos) / 100.f : 0.f;
		Rect(180.f, 112.f, 560.f, 64.f, FLinearColor(0.f, 0.f, 0.f, 0.45f));
		Text(FString::Printf(TEXT("TARGET  %s"), *Name), 196.f, 118.f, 1.35f, Gold);
		Text(D.bInBase ? FString(TEXT("Dive through the moon pool to find it.")) : FString::Printf(TEXT("%s  ·  %.0f m"), *Where, Metres), 196.f, 150.f, 1.0f, Dim);
		if (bAt && !D.bInBase && Metres > 2.f)
		{
			FVector2D P;
			const bool bFront = D.Bearing(At, P);
			const FLinearColor C(Gold.R, Gold.G, Gold.B, 0.85f);
			const FString Label = FString::Printf(TEXT("%s  %.0f m"), *Name, Metres);
			if (bFront && FMath::Abs(P.X) < 0.9 && FMath::Abs(P.Y) < 0.8)
			{
				const float X = 960.f + (float)P.X * 960.f, Y = 540.f - (float)P.Y * 540.f;
				// a diamond
				Line(X, Y - 16.f, X + 16.f, Y, C, 3.f);
				Line(X + 16.f, Y, X, Y + 16.f, C, 3.f);
				Line(X, Y + 16.f, X - 16.f, Y, C, 3.f);
				Line(X - 16.f, Y, X, Y - 16.f, C, 3.f);
				Text(Label, X, Y + 24.f, 1.1f, C, 1);
			}
			else
			{
				const double Kx = FMath::Abs(P.X) / 0.86, Ky = FMath::Abs(P.Y) / 0.74;
				const FVector2D E = P / FMath::Max(1.0, FMath::Max(Kx, Ky));
				const FVector2D Q(960.0 + E.X * 960.0, 540.0 - E.Y * 540.0);
				const FVector2D Dir = FVector2D(P.X, -P.Y).GetSafeNormal();
				Arrow(Q - Dir * 22.f, Q + Dir * 22.f, C, 3.f);
				Text(Label, FMath::Clamp((float)(Q.X - Dir.X * 70.0), 150.f, 1770.f), FMath::Clamp((float)(Q.Y - Dir.Y * 60.0) - 10.f, 200.f, 900.f), 1.1f, C, 1);
			}
		}
	}
	// the bag, bottom right: TAB
	int32 Found = 0;
	for (int32 s : D.Missions)
	{
		Found += D.IsFound(s) ? 1 : 0;
	}
	const float Bx = 1770.f, By = 940.f;
	const float Pulse = D.SavedTime > 0.f && D.SavedTime < 0.6f ? FMath::Sin(D.SavedTime / 0.6f * UE_PI) : 0.f;
	const FLinearColor BagC(0.85f + 0.15f * Pulse, 0.7f + 0.3f * Pulse, 0.45f, 0.9f);
	Rect(Bx - 4.f * Pulse, By - 4.f * Pulse, 84.f + 8.f * Pulse, 62.f + 8.f * Pulse, FLinearColor(0.f, 0.f, 0.f, 0.5f));
	Frame(Bx - 4.f * Pulse, By - 4.f * Pulse, 84.f + 8.f * Pulse, 62.f + 8.f * Pulse, BagC, 2.f);
	Line(Bx + 26.f, By, Bx + 30.f, By - 12.f, BagC, 2.f);
	Line(Bx + 30.f, By - 12.f, Bx + 54.f, By - 12.f, BagC, 2.f);
	Line(Bx + 54.f, By - 12.f, Bx + 58.f, By, BagC, 2.f);
	Text(FString::Printf(TEXT("%d/%d"), Found, D.Missions.Num()), Bx + 42.f, By + 8.f, 1.3f, BagC, 1);
	Text(TEXT("TAB  bag"), Bx + 42.f, By + 70.f, 0.9f, Dim, 1);
	// the still, just saved: it comes up big, then drops into the bag
	if (D.SavedTime > 0.f && D.Subjects.IsValidIndex(D.SavedSub))
	{
		const float El = 2.4f - D.SavedTime;
		const float Grow = FMath::Clamp(El / 0.18f, 0.f, 1.f);
		const float Go = FMath::Clamp((El - 1.3f) / 0.6f, 0.f, 1.f);
		const float Ease = Go * Go * (3.f - 2.f * Go);
		const float Scale = FMath::Lerp(0.7f + 0.3f * Grow, 0.1f, Ease);
		const FVector2D From(960.0, 640.0), To(Bx + 42.0, By + 30.0); // below the verdict panel (y 256..446)
		const FVector2D C = From + (To - From) * Ease;
		const float Cw = 440.f * Scale, Ch = 300.f * Scale;
		const float A = 1.f - FMath::Clamp((El - 1.85f) / 0.15f, 0.f, 1.f);
		if (El < 0.12f)
		{
			Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(1.f, 1.f, 1.f, 0.55f * (1.f - El / 0.12f))); // the shutter's flash
		}
		Rect(C.X - Cw * 0.5f, C.Y - Ch * 0.5f, Cw, Ch, FLinearColor(0.95f, 0.94f, 0.9f, A));
		const float Iw = 400.f * Scale, Ih = 225.f * Scale;
		const float Ix = C.X - Iw * 0.5f, Iy = C.Y - Ch * 0.5f + 20.f * Scale;
		if (const TStrongObjectPtr<UTextureRenderTarget2D>* Ph = D.Photos.Find(D.SavedSub))
		{
			if (Ph->IsValid() && (*Ph)->GetResource())
			{
				FCanvasTileItem Tile(FVector2D(Ix * K, Iy * K), (*Ph)->GetResource(), FVector2D(Iw * K, Ih * K), FLinearColor(1.f, 1.f, 1.f, A));
				Tile.BlendMode = SE_BLEND_Translucent;
				Canvas->DrawItem(Tile);
			}
		}
		if (Scale > 0.5f)
		{
			Text(FString::Printf(TEXT("SAVED  %s"), *D.Subjects[D.SavedSub].Name), C.X, Iy + Ih + 12.f * Scale, 1.2f * Scale, FLinearColor(0.1f, 0.1f, 0.1f, A), 1);
		}
	}
}

// the newspaper on the base's wall, read: the front page set in the HUD's own type (so it is translated), the photo
// taken from the printed page's texture (scene:news, a page whose photo sits at NewsPhotoUV on it)
void AMatchCutHUD::DrawNews()
{
	static TStrongObjectPtr<UTexture2D> Page; // kept from the garbage collector
	static bool bTried = false;
	if (!bTried)
	{
		bTried = true;
		Page.Reset(LoadObject<UTexture2D>(nullptr, TEXT("/Game/DeepCut/Art/scene_news/scene_news/Textures/news_page.news_page")));
		UE_LOG(LogTemp, Display, TEXT("DEEPCUT news page texture %s"), Page.IsValid() ? TEXT("found") : TEXT("missing"));
	}
	const FLinearColor Paper(0.86f, 0.83f, 0.76f, 0.98f), Ink(0.08f, 0.07f, 0.06f, 1.f), Grey(0.30f, 0.28f, 0.25f, 1.f);
	const float X0 = 330.f, Y0 = 120.f, Wd = 1260.f, Ht = 860.f;
	Rect(X0 - 8.f, Y0 - 8.f, Wd + 16.f, Ht + 16.f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Rect(X0, Y0, Wd, Ht, Paper);
	Text(TEXT("THE HARBOUR LIGHT"), 960.f, Y0 + 18.f, 2.6f, Ink, 1);
	Rect(X0 + 30.f, Y0 + 70.f, Wd - 60.f, 3.f, Ink);
	Text(TEXT("Local news   ·   Saturday edition   ·   cut out and pinned up by the last crew"), 960.f, Y0 + 80.f, 0.95f, Grey, 1);
	Rect(X0 + 30.f, Y0 + 104.f, Wd - 60.f, 1.f, Ink);
	Text(TEXT("THIRD DIVER MISSING OFF THE TRENCH"), 960.f, Y0 + 120.f, 2.3f, Ink, 1);
	Text(TEXT("The company calls the water empty. The families ask what is in the photo."), 960.f, Y0 + 172.f, 1.15f, Grey, 1);
	// the photo, left
	const float PX = X0 + 40.f, PY = Y0 + 215.f, PW = 560.f, PH = 358.f;
	Rect(PX - 3.f, PY - 3.f, PW + 6.f, PH + 6.f, Ink);
	if (UTexture2D* T = Page.Get())
	{
		if (T->GetResource())
		{
			FCanvasTileItem Tile(FVector2D(PX * K, PY * K), T->GetResource(), FVector2D(PW * K, PH * K), FVector2D(NewsPhotoUV.X, NewsPhotoUV.Y), FVector2D(NewsPhotoUV.Z, NewsPhotoUV.W), FLinearColor::White);
			Tile.BlendMode = SE_BLEND_Opaque;
			Canvas->DrawItem(Tile);
		}
	}
	else
	{
		Rect(PX, PY, PW, PH, FLinearColor(0.12f, 0.13f, 0.13f, 1.f));
	}
	Wrap(TEXT("Taken from the dive boat minutes before the third diver's line went slack. The company says it is the shadow of a cloud."), PX, PY + PH + 14.f, 0.95f, Grey, PW);
	// the story, right
	const float TX = X0 + 640.f, TW = Wd - 680.f;
	float Y = Y0 + 215.f;
	static const TCHAR* Story[] = {
		TEXT("The search boats were called back on Thursday, after a third diver failed to come up near the abyss trench, a few kilometres from the research station."),
		TEXT("All three worked for contractors of the company that is surveying the trench for drilling. Each went down in daylight, in calm water, with an hour of air."),
		TEXT("The company's survey film, shown to the council last month, says the trench holds \"nothing larger than a shrimp\". Asked about the photo, a spokesperson called it \"a cloud on the water\" and took no more questions."),
		TEXT("At the harbour they tell it another way. \"Nets come up cut clean,\" said a fisherman who would not give his name. \"Nobody fishes past the drop any more. Not for years.\""),
		TEXT("The search is over. The survey goes on as planned.") };
	for (const TCHAR* Para : Story)
	{
		Y += Wrap(Para, TX, Y, 1.05f, Ink, TW) + 12.f;
	}
	Text(TEXT("E / ESC  close"), X0 + Wd - 24.f, Y0 + Ht - 36.f, 1.1f, Grey, 2);
}

void AMatchCutHUD::DrawTutorial(AMatchCutGameMode* GM)
{
	const FDC3D* W = GM->World3D();
	if (!W || !W->Doc || W->Doc->State != EDCDocState::Shooting || W->Doc->PageOpen() || W->Doc->bBlackout)
	{
		return;
	}
	const FDCTutorial& T = DCTutorial();
	const float Aim = W->Doc->AimBlend;
	const float A = FMath::Clamp(T.Shown * 3.f, 0.f, 1.f) * (1.f - 0.45f * Aim);
	auto Fade = [A](const FLinearColor& C) { return FLinearColor(C.R, C.G, C.B, C.A * A); };
	const float X = 56.f, Y = 250.f, Wd = 560.f;
	FString Head, Body;
	FDCTutorial::Card(T.Step, Head, Body);
	// measure the body to size the panel
	const FString BodyTr = DCLoc::Tr(Body);
	const float Lines = FMath::Max(1.f, FMath::CeilToFloat(TextWidth(BodyTr, 1.3f) / (Wd - 48.f)));
	const float H = 118.f + Lines * 25.5f;
	Rect(X, Y, Wd, H, FLinearColor(0.f, 0.02f, 0.03f, 0.62f * A));
	Rect(X, Y, 5.f, H, Fade(Amber));
	Text(FString::Printf(TEXT("TUTORIAL  %d / %d"), T.Step + 1, FDCTutorial::NumSteps), X + 24.f, Y + 16.f, 1.0f, Fade(Dim));
	Text(Head, X + 24.f, Y + 42.f, 1.75f, Fade(Amber));
	Wrap(Body, X + 24.f, Y + 82.f, 1.3f, Fade(White), Wd - 48.f);
	Text(TEXT("H  hide the tutorial"), X + Wd - 20.f, Y + 16.f, 0.95f, Fade(FLinearColor(1.f, 1.f, 1.f, 0.4f)), 2);
	if (T.Flash > 0.f && T.FlashStep >= 0)
	{
		FString PrevHead, PrevBody;
		FDCTutorial::Card(T.FlashStep, PrevHead, PrevBody);
		const float F = FMath::Clamp(T.Flash * 2.f, 0.f, 1.f);
		Rect(X, Y - 52.f, Wd, 42.f, FLinearColor(0.f, 0.08f, 0.03f, 0.6f * F));
		Text(FString::Printf(TEXT("DONE: %s"), *PrevHead), X + 24.f, Y - 44.f, 1.3f, FLinearColor(Green.R, Green.G, Green.B, F));
	}
}

void AMatchCutHUD::DrawMenu(AMatchCutGameMode* GM)
{
	FDCMenu& Menu = DCMenu();
	const float T = Menu.Time;
	Menu.Rows.Reset();
	// the canvas is the 16:9 view, centred in the window: the mouse is in window pixels
	FVector2D Off(0.0, 0.0);
	if (const APlayerController* PC = GetOwningPlayerController())
	{
		int32 Vx = 0, Vy = 0;
		PC->GetViewportSize(Vx, Vy);
		Off = FVector2D(FMath::Max(0.0, (Vx - Canvas->ClipX) * 0.5), FMath::Max(0.0, (Vy - Canvas->ClipY) * 0.5));
	}
	auto AddRow = [this, &Menu, Off](float X0, float Y0, float X1, float Y1)
	{
		Menu.Rows.Add(FBox2D(FVector2D(X0 * K, Y0 * K) + Off, FVector2D(X1 * K, Y1 * K) + Off));
	};
	const FLinearColor Sel(1.f, 0.78f, 0.25f, 1.f);
	const FLinearColor Panel(0.f, 0.015f, 0.025f, 0.78f);
	const FString Keys = TEXT("W S / mouse: choose     A D: change     ENTER / click: select     ESC: back");

	if (Menu.Page == EDCMenuPage::Title || Menu.Page == EDCMenuPage::Pause)
	{
		const bool bTitle = Menu.Page == EDCMenuPage::Title;
		if (bTitle)
		{
			// the left third darkened, the base turning slowly behind
			for (int32 i = 0; i < 24; ++i)
			{
				const float X0 = i * 40.f;
				Rect(X0, 0.f, 40.f, 1080.f, FLinearColor(0.f, 0.01f, 0.02f, 0.86f * (1.f - i / 24.f)));
			}
			Text(TEXT("DEEP CUT"), 130.f, 210.f, 7.f, White);
			Text(TEXT("A documentary from the bottom of the sea"), 136.f, 352.f, 1.45f, Dim);
			Rect(136.f, 394.f, 120.f, 3.f, Amber);
		}
		else
		{
			Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(0.f, 0.f, 0.f, 0.62f));
			Text(TEXT("PAUSED"), 130.f, 230.f, 4.5f, White);
		}
		for (int32 i = 0; i < Menu.NumRows(); ++i)
		{
			FString Label, Value;
			Menu.Row(i, Label, Value);
			const float Y = 450.f + i * 74.f;
			const bool bOn = Menu.Sel == i;
			if (bOn)
			{
				Rect(120.f, Y - 10.f, 560.f, 60.f, FLinearColor(1.f, 0.78f, 0.25f, 0.13f));
				Rect(120.f, Y - 10.f, 5.f, 60.f, Sel);
			}
			Text(Label, 150.f + (bOn ? 10.f : 0.f), Y, 2.1f, bOn ? Sel : FLinearColor(1.f, 1.f, 1.f, 0.82f));
			AddRow(120.f, Y - 10.f, 680.f, Y + 50.f);
		}
		Text(TEXT("ENTER / click: select     W S / mouse: choose"), 136.f, 1000.f, 1.1f, Dim);
		if (bTitle)
		{
			bNoTr = true; // both languages, so either reader finds it
			Text(TEXT("English / Tiếng Việt:  Settings > Language"), 1880.f, 1030.f, 1.05f, FLinearColor(1.f, 1.f, 1.f, 0.5f), 2);
			bNoTr = false;
			Text(TEXT("Epic MegaJam 2026  ·  theme: Director's Cut"), 1880.f, 1000.f, 1.0f, FLinearColor(1.f, 1.f, 1.f, 0.35f), 2);
		}
		return;
	}

	// Settings and How to play: a centred panel over the dimmed game
	Rect(0.f, 0.f, 1920.f, 1080.f, FLinearColor(0.f, 0.f, 0.f, Menu.bStarted ? 0.55f : 0.35f));
	Rect(400.f, 110.f, 1120.f, 860.f, Panel);
	Rect(400.f, 110.f, 1120.f, 4.f, Amber);
	if (Menu.Page == EDCMenuPage::Settings)
	{
		Text(TEXT("SETTINGS"), 460.f, 140.f, 3.2f, White);
		for (int32 i = 0; i < Menu.NumRows(); ++i)
		{
			FString Label, Value;
			Menu.Row(i, Label, Value);
			const float Y = 240.f + i * 50.f + (i >= Menu.NumRows() - 2 ? 14.f : 0.f);
			const bool bOn = Menu.Sel == i;
			if (bOn)
			{
				Rect(440.f, Y - 9.f, 1040.f, 46.f, FLinearColor(1.f, 0.78f, 0.25f, 0.12f));
				Rect(440.f, Y - 9.f, 4.f, 46.f, Sel);
			}
			Text(Label, 470.f, Y, 1.55f, bOn ? Sel : FLinearColor(1.f, 1.f, 1.f, 0.85f));
			if (!Value.IsEmpty())
			{
				Text(Value, 1300.f, Y, 1.55f, bOn ? White : Dim, 1);
				if (bOn)
				{
					const float Nudge = 3.f * FMath::Sin(T * 6.f);
					Text(TEXT("<"), 1150.f - Nudge, Y, 1.55f, Sel, 1);
					Text(TEXT(">"), 1450.f + Nudge, Y, 1.55f, Sel, 1);
				}
			}
			AddRow(440.f, Y - 9.f, 1480.f, Y + 37.f);
		}
		Text(Keys, 960.f, 920.f, 1.05f, Dim, 1);
		return;
	}

	// How to play
	Text(TEXT("HOW TO PLAY"), 460.f, 140.f, 3.2f, White);
	float Y = 240.f;
	Y += Wrap(TEXT("The company's film says the abyss trench is empty. You know it is not. Dive from your base, film what lives down there, and bring the tapes back."), 460.f, Y, 1.3f, White, 1000.f) + 8.f;
	Y += Wrap(TEXT("Upload your footage at the edit desk: views pay for gear (lamp, air, lenses). The proofs on the board decide the hearing after the last day."), 460.f, Y, 1.3f, White, 1000.f) + 22.f;
	static const TCHAR* Ctl[][2] = {
		{ TEXT("MOUSE / ARROW KEYS"), TEXT("look") },
		{ TEXT("W A S D"), TEXT("walk, swim") },
		{ TEXT("SPACE / Q"), TEXT("swim up / down") },
		{ TEXT("SHIFT"), TEXT("run, swim fast") },
		{ TEXT("RIGHT MOUSE / C"), TEXT("raise the camera") },
		{ TEXT("LEFT MOUSE / E"), TEXT("record") },
		{ TEXT("WHEEL / 1 2"), TEXT("zoom") },
		{ TEXT("F"), TEXT("headlamp") },
		{ TEXT("E"), TEXT("use: desk, locker, bunk, board") },
		{ TEXT("ESC"), TEXT("pause, settings") } };
	for (int32 i = 0; i < (int32)UE_ARRAY_COUNT(Ctl); ++i)
	{
		const float Yy = Y + (i % 5) * 40.f;
		const float X = i < 5 ? 460.f : 990.f;
		Text(Ctl[i][0], X, Yy, 1.2f, Amber);
		Text(Ctl[i][1], X + 250.f, Yy, 1.2f, White);
	}
	const float By = Y + 5 * 40.f + 50.f;
	Rect(440.f, By - 9.f, 300.f, 46.f, FLinearColor(1.f, 0.78f, 0.25f, 0.12f));
	Rect(440.f, By - 9.f, 4.f, 46.f, Sel);
	Text(TEXT("Back"), 470.f, By, 1.55f, Sel);
	AddRow(440.f, By - 9.f, 740.f, By + 37.f);
	(void)GM;
}
