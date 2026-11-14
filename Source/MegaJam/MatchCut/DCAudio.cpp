// Deep Cut: mixer. See DCAudio.h.

#include "DCAudio.h"
#include "MatchCutGameMode.h"
#include "Components/AudioComponent.h"
#include "Dom/JsonObject.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"

void UDCAudio::Init(UWorld* InWorld)
{
	World = InWorld;
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data/audio.json");
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogMatchCut, Error, TEXT("AUDIO cannot read %s"), *Path);
		return;
	}
	double D = 0.0;
	if (Root->TryGetNumberField(TEXT("duck"), D))
	{
		DuckLevel = (float)D;
	}
	const TSharedPtr<FJsonObject>* BusObj = nullptr;
	if (Root->TryGetObjectField(TEXT("buses"), BusObj) && BusObj)
	{
		for (const auto& Pair : (*BusObj)->Values)
		{
			Buses.Add(FName(*FString(Pair.Key)), (float)Pair.Value->AsNumber());
		}
	}
	const TSharedPtr<FJsonObject>* SoundObj = nullptr;
	int32 Loaded = 0;
	if (Root->TryGetObjectField(TEXT("sounds"), SoundObj) && SoundObj)
	{
		for (const auto& Pair : (*SoundObj)->Values)
		{
			const TSharedPtr<FJsonObject> S = Pair.Value->AsObject();
			if (!S.IsValid())
			{
				continue;
			}
			const FString Name(Pair.Key);
			FDCSoundDef& Def = Sounds.Add(FName(*Name));
			FString Bus = TEXT("sfx");
			S->TryGetStringField(TEXT("bus"), Bus);
			Def.Bus = FName(*Bus);
			if (S->TryGetNumberField(TEXT("vol"), D))
			{
				Def.Vol = (float)D;
			}
			S->TryGetBoolField(TEXT("loop"), Def.bLoop);
			Def.Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/DeepCut/Audio/%s.%s"), *Name, *Name));
			if (Def.Sound)
			{
				++Loaded;
				if (USoundWave* Wave = Cast<USoundWave>(Def.Sound))
				{
					Wave->bLooping = Def.bLoop;
				}
			}
			else
			{
				UE_LOG(LogMatchCut, Warning, TEXT("AUDIO missing asset for %s"), *Name);
			}
		}
	}
	UE_LOG(LogMatchCut, Display, TEXT("AUDIO init: %d of %d sounds loaded, duck=%.2f"), Loaded, Sounds.Num(), DuckLevel);
}

float UDCAudio::BusGain(FName Bus) const
{
	const float* G = Buses.Find(Bus);
	float Out = (G ? *G : 1.f) * MasterVol;
	if (Bus == FName(TEXT("music")))
	{
		Out *= DuckNow * MusicVol;
	}
	return Out;
}

void UDCAudio::Play(FName Name, float Volume, float Pitch, float Jitter)
{
	FDCSoundDef* Def = Sounds.Find(Name);
	if (!Def)
	{
		UE_LOG(LogMatchCut, Warning, TEXT("AUDIO unknown sound %s"), *Name.ToString());
		return;
	}
	const float P = Pitch * (1.f + FMath::FRandRange(-Jitter, Jitter));
	const float V = Def->Vol * Volume * BusGain(Def->Bus);
	UAudioComponent* AC = Def->Sound && World ? UGameplayStatics::SpawnSound2D(World, Def->Sound, V, P) : nullptr;
	UE_LOG(LogMatchCut, Display, TEXT("AUDIO play %s bus=%s gain=%.2f pitch=%.3f device=%d"), *Name.ToString(), *Def->Bus.ToString(), V, P, AC ? 1 : 0);
}

void UDCAudio::SetLoop(FName Name, float Level, float FadeSeconds)
{
	FDCSoundDef* Def = Sounds.Find(Name);
	if (!Def)
	{
		return;
	}
	if (FMath::IsNearlyEqual(Def->Want, Level))
	{
		return;
	}
	Def->Want = Level;
	Def->FadeRate = 1.f / FMath::Max(FadeSeconds, 0.01f);
	if (Level > 0.f && !Def->Loop && Def->Sound && World)
	{
		Def->Loop = UGameplayStatics::SpawnSound2D(World, Def->Sound, 0.001f, 1.f, 0.f, nullptr, false, false);
	}
	UE_LOG(LogMatchCut, Display, TEXT("AUDIO loop %s bus=%s -> level %.2f (gain %.2f) fade=%.1fs device=%d"), *Name.ToString(), *Def->Bus.ToString(), Level, Def->Vol * Level, FadeSeconds, Def->Loop ? 1 : 0);
}

void UDCAudio::Tick(float Dt)
{
	DuckNow = FMath::FInterpConstantTo(DuckNow, bDucked ? DuckLevel : 1.f, Dt, 1.6f);
	for (auto& Pair : Sounds)
	{
		FDCSoundDef& Def = Pair.Value;
		if (!Def.bLoop)
		{
			continue;
		}
		Def.Level = FMath::FInterpConstantTo(Def.Level, Def.Want, Dt, Def.FadeRate);
		if (UAudioComponent* AC = Def.Loop)
		{
			if (Def.Level <= 0.f && Def.Want <= 0.f)
			{
				AC->Stop();
				Def.Loop = nullptr;
			}
			else
			{
				AC->SetVolumeMultiplier(FMath::Max(Def.Vol * Def.Level * BusGain(Def.Bus), 0.001f));
			}
		}
	}
}
