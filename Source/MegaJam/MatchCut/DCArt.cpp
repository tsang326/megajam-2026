// Deep Cut (brief 05): models from the art drop. Tools/import_art.sh imports them and writes Content/Data/art.json
// (id -> meshes). A thing uses its mesh when its id is listed and the code-built shapes when it is not.
// Gameplay sizes never change: a mesh is fitted inside the box the shapes used (uniform scale, centred, on the ground).

#include "MatchCutGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void AMatchCutGameMode::LoadArt()
{
	Art.Reset();
	ArtMeshes.Reset();
	FString Name = TEXT("art.json");
	FParse::Value(FCommandLine::Get(), TEXT("-MCArt="), Name); // -MCArt=art_test.json: the pipeline test set; -MCArt=none: shapes only
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data") / Name;
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogMatchCut, Display, TEXT("ART no list at %s: code-built shapes everywhere"), *Path);
		return;
	}
	for (const auto& Pair : Root->Values)
	{
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (!Pair.Value.IsValid() || !Pair.Value->TryGetArray(List))
		{
			continue;
		}
		for (const TSharedPtr<FJsonValue>& V : *List)
		{
			const TSharedPtr<FJsonObject>* O = nullptr;
			FString MeshPath;
			if (!V->TryGetObject(O) || !(*O)->TryGetStringField(TEXT("mesh"), MeshPath))
			{
				continue;
			}
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
			if (!Mesh)
			{
				UE_LOG(LogMatchCut, Warning, TEXT("ART %s: mesh not found: %s (shapes are used)"), *FString(Pair.Key), *MeshPath);
				continue;
			}
			FDCArtRef Ref;
			Ref.Mesh = ArtMeshes.Add(Mesh);
			double Yaw = 0.0;
			(*O)->TryGetNumberField(TEXT("yaw"), Yaw);
			Ref.Yaw = (float)Yaw;
			Art.FindOrAdd(FString(Pair.Key)).Add(Ref);
			const FBox B = Mesh->GetBoundingBox();
			UE_LOG(LogMatchCut, Display, TEXT("ART %s <- %s  size=(%.0f,%.0f,%.0f) yaw=%.0f"), *FString(Pair.Key), *MeshPath, B.GetSize().X, B.GetSize().Y, B.GetSize().Z, Ref.Yaw);
		}
	}
}

UStaticMeshComponent* AMatchCutGameMode::AddArt(AActor* Owner, const FString& Id, const FVector& Centre, const FVector& Size, uint32 Hash, bool bFill)
{
	const TArray<FDCArtRef>* Set = Art.Find(Id);
	if (!Set || Set->Num() == 0 || !Owner)
	{
		return nullptr;
	}
	// a box that is filled takes a model of about its own proportions (a pillar for a tall box, a slab for a flat one),
	// so nothing is stretched much; among those that suit, the hash of the place picks
	TArray<int32> Suit;
	if (bFill && Set->Num() > 1)
	{
		const double BoxTall = Size.Z / FMath::Max3(Size.X, Size.Y, 1.0);
		double Best = 1.e9;
		int32 BestIndex = 0;
		for (int32 i = 0; i < Set->Num(); ++i)
		{
			const FVector E = ArtMeshes[(*Set)[i].Mesh]->GetBoundingBox().GetSize();
			const double Off = FMath::Abs(FMath::Loge((E.Z / FMath::Max3(E.X, E.Y, 1.0)) / FMath::Max(BoxTall, 0.01)));
			if (Off < 0.75)
			{
				Suit.Add(i);
			}
			if (Off < Best)
			{
				Best = Off;
				BestIndex = i;
			}
		}
		if (Suit.Num() == 0)
		{
			Suit.Add(BestIndex);
		}
	}
	const FDCArtRef& Ref = Suit.Num() > 0 ? (*Set)[Suit[Hash % Suit.Num()]] : (*Set)[Hash % Set->Num()];
	UStaticMesh* Mesh = ArtMeshes[Ref.Mesh];
	const FBox B = Mesh->GetBoundingBox();
	// a filled box (rock) may be turned in quarter turns by the hash, so two boxes with one mesh do not look the same
	const float Yaw = Ref.Yaw + (bFill ? 90.f * ((Hash / 7) % 4) : 0.f);
	const FQuat Q(FRotator(0.f, Yaw, 0.f));
	const FBox RB = B.TransformBy(FTransform(Q));
	const FVector Ext = RB.GetSize().ComponentMax(FVector(0.1));
	FVector Scale;
	FVector Loc;
	if (bFill)
	{
		// per-axis, in the owner's axes: the picture has to cover the box Unit 0 bumps into
		const FVector S = Size / Ext;
		const bool bQuarter = FMath::Abs(FMath::Fmod(FMath::Abs(Yaw), 180.f) - 90.f) < 1.f;
		Scale = bQuarter ? FVector(S.Y, S.X, S.Z) : S;
		Loc = Centre - RB.GetCenter() * S;
	}
	else
	{
		const double S = FMath::Min3(Size.X / Ext.X, Size.Y / Ext.Y, Size.Z / Ext.Z);
		Scale = FVector(S);
		Loc = Centre - RB.GetCenter() * S;
		Loc.Z = Centre.Z - Size.Z * 0.5 - RB.Min.Z * S; // on the ground of its box
	}
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetupAttachment(Owner->GetRootComponent());
	C->RegisterComponent();
	C->SetRelativeLocationAndRotation(Loc, Q);
	C->SetRelativeScale3D(Scale);
	// The game has no reflections (cheap on integrated GPUs), so a metal surface would come out black: models are drawn
	// as non-metal, and what the model lights up (eyes, lamp, chest) is made bright enough to read under water.
	for (int32 m = 0; m < C->GetNumMaterials(); ++m)
	{
		UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(m);
		if (!MID)
		{
			continue;
		}
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		FString Names;
		MID->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& I : Infos)
		{
			const FString N = I.Name.ToString();
			Names += N + TEXT(" ");
			if (N.Contains(TEXT("Metallic")) && !N.Contains(TEXT("Tex")))
			{
				MID->SetScalarParameterValue(I.Name, 0.f);
			}
			else if (N.Contains(TEXT("EmissiveStrength")) && Id == TEXT("hero")) // the only model with an emissive map
			{
				MID->SetScalarParameterValue(I.Name, 6.f);
			}
		}
		Infos.Reset();
		Ids.Reset();
		MID->GetAllVectorParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& I : Infos)
		{
			const FString N = I.Name.ToString();
			Names += N + TEXT(" ");
			if (N == TEXT("EmissiveFactor") && Id == TEXT("hero"))
			{
				MID->SetVectorParameterValue(I.Name, D3 ? FLinearColor::Black : FLinearColor::White); // the 3D game sets it itself (DC3D.cpp)
			}
		}
		if (!bArtParamsLogged.Contains(Id))
		{
			bArtParamsLogged.Add(Id);
			UE_LOG(LogMatchCut, Display, TEXT("ART %s material %d params: %s"), *Id, m, *Names);
		}
	}
	UE_LOG(LogMatchCut, Display, TEXT("ART use %s (%s) in a box (%.0f,%.0f,%.0f): scale (%.3f,%.3f,%.3f)%s"), *Id, *Mesh->GetName(), Size.X, Size.Y, Size.Z, Scale.X, Scale.Y, Scale.Z, bFill ? TEXT(" fill") : TEXT(""));
	return C;
}
