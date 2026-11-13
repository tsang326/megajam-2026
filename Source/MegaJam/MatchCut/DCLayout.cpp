// Deep Cut: reads the stage (Content/Data/deepcut.json) and the film clips (Content/Data/film.json).
// Content/Data is staged as UFS (DefaultGame.ini), so these paths also work in a packaged build.

#include "MatchCutGameMode.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const double AspectRatio = 16.0 / 9.0;

	FVector ReadVec(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, const FVector& Default = FVector::ZeroVector)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Obj.IsValid() && Obj->TryGetArrayField(Field, Arr) && Arr && Arr->Num() >= 3)
		{
			return FVector((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
		}
		return Default;
	}

	bool HasField(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field)
	{
		return Obj.IsValid() && Obj->HasField(Field);
	}

	FLinearColor ReadColor(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, const FLinearColor& Default)
	{
		const FVector V = ReadVec(Obj, Field, FVector(Default.R, Default.G, Default.B));
		return FLinearColor(V.X, V.Y, V.Z);
	}

	void ReadFloat(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, float& Out)
	{
		double D = 0.0;
		if (Obj->TryGetNumberField(Field, D))
		{
			Out = (float)D;
		}
	}

	TSharedPtr<FJsonObject> LoadJson(const FString& Path)
	{
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			UE_LOG(LogMatchCut, Error, TEXT("Cannot read %s"), *Path);
			return nullptr;
		}
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			UE_LOG(LogMatchCut, Error, TEXT("%s is not valid JSON"), *Path);
			return nullptr;
		}
		return Root;
	}

	FLinearColor TapeColor(const FString& Name)
	{
		if (Name == TEXT("blue")) { return FLinearColor(0.20f, 0.72f, 1.f); }
		if (Name == TEXT("pink")) { return FLinearColor(1.f, 0.35f, 0.75f); }
		if (Name == TEXT("white")) { return FLinearColor(0.95f, 0.95f, 0.9f); }
		return FLinearColor(1.f, 0.86f, 0.10f); // yellow
	}
}

int32 AMatchCutGameMode::FindShot(const FString& Id) const
{
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		if (Shots[i].Id == Id)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void AMatchCutGameMode::ReadSolids(const TSharedPtr<FJsonObject>& Obj, TArray<FDCSolid>& Out) const
{
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (!Obj.IsValid() || !Obj->TryGetArrayField(TEXT("solids"), Arr) || !Arr)
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& V : *Arr)
	{
		const TSharedPtr<FJsonObject> R = V->AsObject();
		if (!R.IsValid())
		{
			continue;
		}
		FDCSolid& S = Out.AddDefaulted_GetRef();
		S.C = ReadVec(R, TEXT("p"));
		S.Size = ReadVec(R, TEXT("s"), FVector(100.0));
		const FVector Rot = ReadVec(R, TEXT("r")); // pitch, yaw, roll in the camera frame
		S.Rot = FRotator(Rot.X, Rot.Y, Rot.Z);
		FString Kind;
		if (R->TryGetStringField(TEXT("kind"), Kind))
		{
			S.Kind = FName(*Kind);
		}
		R->TryGetStringField(TEXT("hatch"), S.Hatch);
	}
}

bool AMatchCutGameMode::LoadLayout()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data/deepcut.json");
	const TSharedPtr<FJsonObject> Root = LoadJson(Path);
	if (!Root.IsValid())
	{
		return false;
	}

	ReadFloat(Root, TEXT("fov"), Fov);
	ReadFloat(Root, TEXT("ghost"), GhostOpacity);
	GhostTint = ReadColor(Root, TEXT("ghostTint"), GhostTint);
	ReadFloat(Root, TEXT("exposure"), ExposureEV);
	ReadFloat(Root, TEXT("heroRadius"), HeroRadius);
	ReadFloat(Root, TEXT("restoreTol"), RestoreTol);
	ReadFloat(Root, TEXT("lampRadius"), LampRadius);
	ReadFloat(Root, TEXT("markRadius"), MarkRadius);
	ReadFloat(Root, TEXT("markDepthTol"), MarkDepthTol);
	ReadFloat(Root, TEXT("carryRadius"), CarryRadius);
	ReadFloat(Root, TEXT("socketRadius"), SocketRadius);
	ReadFloat(Root, TEXT("reachRadius"), ReachRadius);
	Root->TryGetNumberField(TEXT("crewTotal"), CrewTotal);
	Root->TryGetStringField(TEXT("look"), LookName);
	double Spacing = 50000.0;
	Root->TryGetNumberField(TEXT("spacing"), Spacing);

	TanH = FMath::Tan(FMath::DegreesToRadians(Fov) * 0.5);
	TanV = TanH / AspectRatio;

	const TSharedPtr<FJsonObject>* LooksObj = nullptr;
	if (Root->TryGetObjectField(TEXT("looks"), LooksObj) && LooksObj)
	{
		for (const auto& Pair : (*LooksObj)->Values)
		{
			const TSharedPtr<FJsonObject> L = Pair.Value->AsObject();
			if (!L.IsValid())
			{
				continue;
			}
			FDCLook& Out = Looks.Add(FString(Pair.Key));
			ReadFloat(L, TEXT("fogDensity"), Out.FogDensity);
			Out.FogColor = ReadColor(L, TEXT("fogColor"), Out.FogColor);
			L->TryGetBoolField(TEXT("volumetric"), Out.bVolumetric);
			ReadFloat(L, TEXT("volScatter"), Out.VolScatter);
			ReadFloat(L, TEXT("volExtinction"), Out.VolExtinction);
			ReadFloat(L, TEXT("volEmissive"), Out.VolEmissive);
			ReadFloat(L, TEXT("keyIntensity"), Out.KeyIntensity);
			Out.KeyColor = ReadColor(L, TEXT("keyColor"), Out.KeyColor);
			ReadFloat(L, TEXT("fillIntensity"), Out.FillIntensity);
			Out.FillColor = ReadColor(L, TEXT("fillColor"), Out.FillColor);
			ReadFloat(L, TEXT("lampIntensity"), Out.LampIntensity);
			ReadFloat(L, TEXT("glowIntensity"), Out.GlowIntensity);
			L->TryGetBoolField(TEXT("toon"), Out.bToon);
			L->TryGetBoolField(TEXT("shafts"), Out.bShafts);
			ReadFloat(L, TEXT("vignette"), Out.Vignette);
			ReadFloat(L, TEXT("grain"), Out.Grain);
			ReadFloat(L, TEXT("darkFactor"), Out.DarkFactor);
			ReadFloat(L, TEXT("ambient"), Out.Ambient);
			Out.Backdrop = ReadColor(L, TEXT("backdrop"), Out.Backdrop);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ShotArr = nullptr;
	if (!Root->TryGetArrayField(TEXT("shots"), ShotArr) || !ShotArr)
	{
		return false;
	}

	for (int32 i = 0; i < ShotArr->Num(); ++i)
	{
		const TSharedPtr<FJsonObject> S = (*ShotArr)[i]->AsObject();
		if (!S.IsValid())
		{
			continue;
		}
		const int32 Index = Shots.Num();
		FDCShot& Shot = Shots.AddDefaulted_GetRef();
		const FVector Origin(0.0, Spacing * Index, 0.0);
		S->TryGetStringField(TEXT("id"), Shot.Id);
		S->TryGetStringField(TEXT("name"), Shot.Name);
		S->TryGetStringField(TEXT("hint"), Shot.Hint);
		S->TryGetStringField(TEXT("hintSolved"), Shot.HintSolved);
		S->TryGetStringField(TEXT("gate"), Shot.Gate);
		S->TryGetBoolField(TEXT("dark"), Shot.bDark);
		Shot.Start = ReadVec(S, TEXT("start"), Shot.Start);
		Shot.VolMin = ReadVec(S, TEXT("volMin"), Shot.VolMin);
		Shot.VolMax = ReadVec(S, TEXT("volMax"), Shot.VolMax);
		Shot.Rail = ReadVec(S, TEXT("rail"), FVector::ZeroVector);
		Shot.Truck = Shot.Rail.Z;
		if (HasField(S, TEXT("part")))
		{
			Shot.bHasPart = true;
			Shot.PartS = ReadVec(S, TEXT("part"));
			PartShot = Index;
		}
		if (HasField(S, TEXT("crew")))
		{
			Shot.bHasCrew = true;
			Shot.CrewS = ReadVec(S, TEXT("crew"));
			CrewShot = Index;
		}
		if (HasField(S, TEXT("switch")))
		{
			Shot.bHasSwitch = true;
			Shot.Switch.S = ReadVec(S, TEXT("switch"));
			Shot.Switch.Id = Shot.Id;
		}

		const TSharedPtr<FJsonObject>* Cam = nullptr;
		if (S->TryGetObjectField(TEXT("cam"), Cam) && Cam)
		{
			const FVector Pos = ReadVec(*Cam, TEXT("pos"));
			const FVector LookAt = ReadVec(*Cam, TEXT("look"));
			Shot.CamPos = Origin + Pos;
			Shot.CamRot = (LookAt - Pos).Rotation();
		}
		const FRotationMatrix M(Shot.CamRot);
		Shot.AxF = M.GetUnitAxis(EAxis::X);
		Shot.AxR = M.GetUnitAxis(EAxis::Y);
		Shot.AxU = M.GetUnitAxis(EAxis::Z);

		ReadSolids(S, Shot.Solids);

		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (S->TryGetArrayField(TEXT("exits"), Arr) && Arr)
		{
			for (const TSharedPtr<FJsonValue>& V : *Arr)
			{
				const TSharedPtr<FJsonObject> E = V->AsObject();
				if (!E.IsValid())
				{
					continue;
				}
				FDCExit& Exit = Shot.Exits.AddDefaulted_GetRef();
				E->TryGetStringField(TEXT("to"), Exit.ToId);
				E->TryGetStringField(TEXT("color"), Exit.ColorName);
				Exit.Color = TapeColor(Exit.ColorName);
				E->TryGetBoolField(TEXT("anyDepth"), Exit.bAnyDepth);
				Exit.Shape = Shot.Exits.Num() - 1;
				const TArray<TSharedPtr<FJsonValue>>* Marks = nullptr;
				if (E->TryGetArrayField(TEXT("marks"), Marks) && Marks)
				{
					for (const TSharedPtr<FJsonValue>& MV : *Marks)
					{
						const TArray<TSharedPtr<FJsonValue>>& P = MV->AsArray();
						if (P.Num() >= 3)
						{
							Exit.Marks.Add(FVector(P[0]->AsNumber(), P[1]->AsNumber(), P[2]->AsNumber()));
						}
					}
				}
			}
		}
		if (S->TryGetArrayField(TEXT("props"), Arr) && Arr)
		{
			for (const TSharedPtr<FJsonValue>& V : *Arr)
			{
				const TSharedPtr<FJsonObject> P = V->AsObject();
				if (!P.IsValid())
				{
					continue;
				}
				FDCProp& Prop = Props.AddDefaulted_GetRef();
				P->TryGetStringField(TEXT("id"), Prop.Id);
				FString Kind;
				if (P->TryGetStringField(TEXT("kind"), Kind))
				{
					Prop.Kind = FName(*Kind);
				}
				Prop.Shot = Prop.HomeShot = Index;
				Prop.S = Prop.HomeS = ReadVec(P, TEXT("p"));
			}
		}
		if (S->TryGetArrayField(TEXT("sockets"), Arr) && Arr)
		{
			for (const TSharedPtr<FJsonValue>& V : *Arr)
			{
				const TSharedPtr<FJsonObject> P = V->AsObject();
				if (!P.IsValid())
				{
					continue;
				}
				FDCSocket& Socket = Sockets.AddDefaulted_GetRef();
				P->TryGetStringField(TEXT("id"), Socket.Id);
				P->TryGetStringField(TEXT("label"), Socket.Label);
				P->TryGetBoolField(TEXT("ends"), Socket.bEnds);
				FString Kind;
				if (P->TryGetStringField(TEXT("accepts"), Kind))
				{
					Socket.Accepts = FName(*Kind);
				}
				Socket.Shot = Index;
				Socket.S = ReadVec(P, TEXT("p"));
			}
		}
		if (S->TryGetArrayField(TEXT("currents"), Arr) && Arr)
		{
			for (const TSharedPtr<FJsonValue>& V : *Arr)
			{
				const TSharedPtr<FJsonObject> P = V->AsObject();
				if (!P.IsValid())
				{
					continue;
				}
				FDCCurrent& C = Shot.Currents.AddDefaulted_GetRef();
				C.Shot = Index;
				C.C = ReadVec(P, TEXT("p"));
				C.Half = ReadVec(P, TEXT("s"), FVector(200.0)) * 0.5;
				C.Force = ReadVec(P, TEXT("force"));
				P->TryGetStringField(TEXT("offBy"), C.OffBy);
			}
		}
	}

	for (FDCShot& Shot : Shots)
	{
		for (FDCExit& Exit : Shot.Exits)
		{
			Exit.To = FindShot(Exit.ToId);
			if (Exit.To == INDEX_NONE)
			{
				UE_LOG(LogMatchCut, Error, TEXT("Shot %s: exit to unknown shot \"%s\""), *Shot.Id, *Exit.ToId);
			}
		}
		Shot.Exits.RemoveAll([](const FDCExit& E) { return E.To == INDEX_NONE; });
	}

	UE_LOG(LogMatchCut, Display, TEXT("Layout loaded from %s: %d shots, %d props, %d sockets, %d looks, fov %.1f"), *Path, Shots.Num(), Props.Num(), Sockets.Num(), Looks.Num(), Fov);
	return Shots.Num() > 1;
}

bool AMatchCutGameMode::LoadFilm()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data/film.json");
	const TSharedPtr<FJsonObject> Root = LoadJson(Path);
	if (!Root.IsValid())
	{
		return false;
	}
	Root->TryGetStringField(TEXT("flashback"), FlashClip);

	const TSharedPtr<FJsonObject>* SetsObj = nullptr;
	if (Root->TryGetObjectField(TEXT("sets"), SetsObj) && SetsObj)
	{
		for (const auto& Pair : (*SetsObj)->Values)
		{
			const TSharedPtr<FJsonObject> S = Pair.Value->AsObject();
			FDCFilmSet& Set = FilmSets.Add(FString(Pair.Key));
			if (S.IsValid())
			{
				S->TryGetBoolField(TEXT("oldFilm"), Set.bOldFilm);
				ReadSolids(S, FilmSetSolids.Add(FString(Pair.Key)));
			}
		}
	}

	const TSharedPtr<FJsonObject>* ClipsObj = nullptr;
	if (Root->TryGetObjectField(TEXT("clips"), ClipsObj) && ClipsObj)
	{
		for (const auto& Pair : (*ClipsObj)->Values)
		{
			const TSharedPtr<FJsonObject> C = Pair.Value->AsObject();
			if (!C.IsValid())
			{
				continue;
			}
			FDCClip& Clip = Clips.Add(FString(Pair.Key));
			C->TryGetStringField(TEXT("set"), Clip.Set);
			ReadFloat(C, TEXT("length"), Clip.Length);
			const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
			if (C->TryGetArrayField(TEXT("lamp"), Arr) && Arr)
			{
				for (const TSharedPtr<FJsonValue>& V : *Arr)
				{
					const TArray<TSharedPtr<FJsonValue>>& K = V->AsArray();
					if (K.Num() >= 2)
					{
						Clip.Lamp.Add(FVector2D(K[0]->AsNumber(), K[1]->AsNumber()));
					}
				}
			}
			const TSharedPtr<FJsonObject>* Tracks = nullptr;
			if (C->TryGetObjectField(TEXT("tracks"), Tracks) && Tracks)
			{
				for (const auto& TP : (*Tracks)->Values)
				{
					FDCTrack& Track = Clip.Tracks.AddDefaulted_GetRef();
					Track.Actor = FString(TP.Key);
					// key: [t, x, y, z, headX, headY, roll, eye]
					for (const TSharedPtr<FJsonValue>& KV : TP.Value->AsArray())
					{
						const TArray<TSharedPtr<FJsonValue>>& K = KV->AsArray();
						if (K.Num() < 4)
						{
							continue;
						}
						FDCKey& Key = Track.Keys.AddDefaulted_GetRef();
						Key.T = (float)K[0]->AsNumber();
						Key.P = FVector(K[1]->AsNumber(), K[2]->AsNumber(), K[3]->AsNumber());
						if (K.Num() >= 6) { Key.Head = FVector2D(K[4]->AsNumber(), K[5]->AsNumber()); }
						if (K.Num() >= 7) { Key.Roll = (float)K[6]->AsNumber(); }
						if (K.Num() >= 8) { Key.Eye = (float)K[7]->AsNumber(); }
					}
				}
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ReelArr = nullptr;
	FilmLength = 0.f;
	int32 Gap = 0;
	if (Root->TryGetArrayField(TEXT("reel"), ReelArr) && ReelArr)
	{
		for (const TSharedPtr<FJsonValue>& V : *ReelArr)
		{
			const TSharedPtr<FJsonObject> R = V->AsObject();
			if (!R.IsValid())
			{
				continue;
			}
			FDCReelSeg& Seg = Reel.AddDefaulted_GetRef();
			float Len = 2.5f;
			ReadFloat(R, TEXT("len"), Len);
			Seg.T0 = FilmLength;
			Seg.T1 = FilmLength + Len;
			FilmLength = Seg.T1;
			bool bGap = false;
			R->TryGetBoolField(TEXT("gap"), bGap);
			if (bGap)
			{
				Seg.Gap = Gap++;
			}
			R->TryGetStringField(TEXT("clip"), Seg.Clip);
			R->TryGetStringField(TEXT("caption"), Seg.Caption);
		}
	}
	if (const FDCClip* F = Clips.Find(FlashClip))
	{
		FlashLength = F->Length;
	}
	UE_LOG(LogMatchCut, Display, TEXT("Film loaded from %s: %d sets, %d clips, reel %d segments, %.1fs"), *Path, FilmSets.Num(), Clips.Num(), Reel.Num(), FilmLength);
	return Reel.Num() > 0 && Clips.Contains(FlashClip);
}
