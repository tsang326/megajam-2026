// Deep Cut: a small mixer. Three buses (music, ambience, SFX), loops that fade, and a duck on the music bus.
// Every sound event is logged (LogMatchCut "AUDIO ..."), also when the game runs with -nosound.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DCAudio.generated.h"

class USoundBase;
class UAudioComponent;

USTRUCT()
struct FDCSoundDef
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USoundBase> Sound;
	UPROPERTY()
	TObjectPtr<UAudioComponent> Loop;
	FName Bus;
	float Vol = 1.f;
	bool bLoop = false;
	float Want = 0.f;   // loop target level 0..1
	float Level = 0.f;  // loop level now
	float FadeRate = 1.f;
};

UCLASS()
class UDCAudio : public UObject
{
	GENERATED_BODY()

public:
	/** Reads Content/Data/audio.json and loads /Game/DeepCut/Audio/<name>. */
	void Init(UWorld* InWorld);
	/** One-shot. Pitch is multiplied by a random factor in [1-Jitter, 1+Jitter]. */
	void Play(FName Name, float Volume = 1.f, float Pitch = 1.f, float Jitter = 0.f);
	/** Starts, fades or stops a loop. Level 0 fades it out. */
	void SetLoop(FName Name, float Level, float FadeSeconds = 1.f);
	/** Ducks the music bus (1 = full, the "duck" value of the JSON = under the film). */
	void SetDuck(bool bDuck) { bDucked = bDuck; }
	/** The player's volume settings: everything, and the music on top. */
	void SetMix(float InMaster, float InMusic) { MasterVol = InMaster; MusicVol = InMusic; }
	void Tick(float Dt);

private:
	float BusGain(FName Bus) const;

	UPROPERTY()
	TMap<FName, FDCSoundDef> Sounds;
	UPROPERTY()
	TObjectPtr<UWorld> World;
	TMap<FName, float> Buses;
	float DuckLevel = 0.2f;
	float DuckNow = 1.f;
	bool bDucked = false;
	float MasterVol = 1.f;
	float MusicVol = 1.f;
};
