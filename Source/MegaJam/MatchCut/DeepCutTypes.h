// Deep Cut - data types shared by the game mode files and the HUD (brief 03).
//
// Shot space: every shot is authored in the frame of its survey camera at rest.
//   x = right, y = up, z = distance from the camera along its view axis (all in cm).
// A point (x, y, z) is seen at screen position ((x - truck) / (z tanH), y / (z tanV)), so z is also the on-screen size.

#pragma once

#include "CoreMinimal.h"

class AActor;
class ACameraActor;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

namespace DC
{
	const double Aspect = 16.0 / 9.0;
	const double PictureHalf = 0.75; // the picture is the middle 75 % of the 16:9 frame; the HUD owns the bars
	const double FrameX = 0.93;      // the robot stays inside this part of the frame
	const double FrameY = 0.64;
}

/** Controls, in the order they are taught. */
namespace DCTeach
{
	enum { Move, Cut, Back, Push, Depth, Lamp, Grab, Num };
}

/** A box of rock or metal. The collision box is read from the same transform that draws the mesh. */
struct FDCSolid
{
	FVector C = FVector::ZeroVector;      // centre, shot space
	FVector Size = FVector(100.0);        // width (x), height (y), depth (z)
	FRotator Rot = FRotator::ZeroRotator; // extra rotation in the camera frame
	FName Kind = TEXT("rock");
	FString Hatch;                        // id of the switch that opens it
	bool bActive = true;
	float Open = 0.f;                     // 0 closed .. 1 slid away
	TWeakObjectPtr<AActor> Actor;
	FTransform World;                     // unscaled actor transform (centre + rotation)
	FVector Half = FVector(50.0);         // half extents in the actor's local axes (X forward, Y right, Z up)
	FVector BaseLoc = FVector::ZeroVector;
};

/** A way out of a shot: the camera it cuts to and the tape marks painted in that camera's shot. */
struct FDCExit
{
	FString ToId;
	int32 To = INDEX_NONE;
	FString ColorName = TEXT("yellow");
	FLinearColor Color = FLinearColor(1.f, 0.85f, 0.1f);
	TArray<FVector> Marks; // shot space of the target shot
	bool bAnyDepth = false;
	bool bBack = false; // a mark that cuts back to an earlier shot (brief 05: the shuttle)
	int32 Shape = 0; // 0 = X, 1 = T
	// "cut on the right moment": the mark is only good for OpenFor seconds out of every Period (0 = always)
	float Period = 0.f;
	float OpenFor = 0.f;
	// why the mark comes and goes: "glow" = the gentle creature's light drifts over it in this shot,
	// "clear" = the creature's body drifts over the landing spot in the next shot
	FString When;
	float Amp = 450.f;
	bool OpenAt(float Clock) const { return Period <= 0.f || FMath::Fmod(Clock, Period) < OpenFor; }
};

/** Something loose that can be grabbed and that comes through a cut when it is close to Unit 0. */
struct FDCProp
{
	FString Id;
	FName Kind = TEXT("plate");
	int32 Shot = 0;
	FVector S = FVector::ZeroVector; // shot space
	bool bCarried = false;
	bool bLocked = false; // sits in a socket
	bool bEverGrabbed = false; // grabbed or nudged at least once
	bool bHeavy = false;       // cannot be nudged, sinks when let go: needs the gripper
	int32 Socket = INDEX_NONE; // the socket it sits in
	int32 EyePaint = INDEX_NONE;
	int32 HomeShot = 0;
	FVector HomeS = FVector::ZeroVector;
	TWeakObjectPtr<AActor> Actor;
};

/** A place that wants a prop (a vent that needs a plate, a dock that needs a power cell). */
struct FDCSocket
{
	FString Id;
	int32 Shot = 0;
	FVector S = FVector::ZeroVector;
	FName Accepts = TEXT("plate");
	FString Label;
	bool bFilled = false;
	bool bEnds = false; // filling it ends the stage
	float Drop = 0.f;   // once filled it sinks this far (a net pulled down by a weight)
	float Anim = 0.f;
	int32 Paint = INDEX_NONE;
	TWeakObjectPtr<AActor> Actor;
};

/** A crew robot that is freed by touching it. */
struct FDCCrewBot
{
	int32 Shot = 0;
	FVector S = FVector::ZeroVector;
	bool bRescued = false;
	FVector Pos = FVector::ZeroVector; // world
	TWeakObjectPtr<AActor> Actor;
	int32 EyePaint = INDEX_NONE;
};

/** Moving water. Pushes the robot while it is inside the box. Stops when its socket is filled. */
struct FDCCurrent
{
	int32 Shot = 0;
	FVector C = FVector::ZeroVector;
	FVector Half = FVector(100.0);
	FVector Force = FVector::ZeroVector; // shot space, cm/s^2
	FString OffBy;
	bool bOn = true;
	TArray<TWeakObjectPtr<AActor>> Streaks;
};

/** A valve. Swim to it and press E. */
struct FDCSwitch
{
	FString Id;
	FVector S = FVector::ZeroVector;
	bool bOn = false;
	float Turn = 0.f;
	TWeakObjectPtr<AActor> Actor;
};

/** One shot: a survey camera and the 3D water volume in front of it. */
struct FDCShot
{
	FString Id;
	FString Name;
	FString Hint;       // before the puzzle is solved
	FString HintSolved; // after
	FVector CamPos = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	FVector AxF = FVector::ForwardVector, AxR = FVector::RightVector, AxU = FVector::UpVector;
	FVector VolMin = FVector(-900.0, -400.0, 650.0);
	FVector VolMax = FVector(900.0, 400.0, 1400.0);
	FVector Start = FVector(0.0, 0.0, 900.0);
	FVector Rail = FVector::ZeroVector; // x = min truck, y = max truck, z = truck when the shot starts
	double Truck = 0.0;
	bool bDark = false;
	FString Gate; // "", "switch", "grab", "fits:<socket>", "socket:<id>", "restore", "crew"
	bool bGateOpen = false;
	int32 Stage = 1;
	bool bDeadEnd = false;  // no way on and no end card: R is the only way out
	FString Intro;          // clip played (official cut) the first time the shot is entered
	bool bIntroPlayed = false;
	// a part: one cut feature of Unit 0 and the removed scene that comes back with it
	int32 ShoalSolid = INDEX_NONE; // a shoal of fish that blocks the way until the predator is left behind in this shot
	bool bShoalGone = false;
	float ShoalTime = 0.f;
	FString HintAfter;
	TWeakObjectPtr<AActor> ShoalActor;
	bool bHasPart = false;
	FVector PartS = FVector::ZeroVector;
	int32 PartFeature = 0;  // index into DCText::Features
	int32 PartGap = 0;      // which hole in the reel it fills
	FString PartClip;       // the removed scene
	FVector2D RestoreNdc = FVector2D(-0.2, -0.15);
	float RestoreT = 0.f;   // the frame of the clip that is shown as the frame to match
	TWeakObjectPtr<AActor> PartActor;
	// the predator: comes once the marks are showing
	bool bPredator = false;
	FVector PredFrom = FVector::ZeroVector;
	float PredDelay = 4.f;
	float PredWarn = 4.f;
	float PredSpeed = 110.f;
	float PredAccel = 18.f;
	// the gentle creature rests here
	bool bGentle = false;
	FVector GentleS = FVector::ZeroVector;
	bool bHasCrew = false;
	FVector CrewS = FVector::ZeroVector;
	bool bHasSwitch = false;
	FDCSwitch Switch;
	TWeakObjectPtr<ACameraActor> Camera;
	TArray<FDCSolid> Solids;
	TArray<FDCExit> Exits;
	TArray<FDCCurrent> Currents;
};

/** One of the three look-dev looks. All numbers come from the JSON so they can be tuned without a rebuild. */
struct FDCLook
{
	float FogDensity = 0.3f;
	FLinearColor FogColor = FLinearColor(0.02f, 0.15f, 0.14f);
	bool bVolumetric = false;
	float VolScatter = 1.f;
	float VolExtinction = 1.f;
	float VolEmissive = 1.f;
	float KeyIntensity = 6.f;
	FLinearColor KeyColor = FLinearColor(0.7f, 1.f, 0.95f);
	float FillIntensity = 2.f;
	FLinearColor FillColor = FLinearColor(0.4f, 0.7f, 0.8f);
	float LampIntensity = 30000.f;
	float GlowIntensity = 4000.f;
	bool bToon = false;
	bool bShafts = false;
	float Vignette = 0.4f;
	float Grain = 0.f;
	float DarkFactor = 0.04f;
	float Ambient = 0.f;
	FLinearColor Backdrop = FLinearColor(0.01f, 0.05f, 0.06f);
	// brief 05: colour grading and the water's flecks, per look
	float Saturation = 1.f;
	float Contrast = 1.f;
	FLinearColor Tint = FLinearColor::White;
	float Snow = 1.f;
	FLinearColor SnowColor = FLinearColor(0.30f, 0.42f, 0.38f);
};

struct FDCPaint
{
	TWeakObjectPtr<UStaticMeshComponent> Comp;
	FLinearColor Color = FLinearColor::White;
	FLinearColor Emissive = FLinearColor::Black;
	TWeakObjectPtr<UMaterialInstanceDynamic> MID;
};

/** A robot made of engine shapes. */
/** One imported model of the art drop: index into the game mode's mesh list, and a turn about the up axis. */
struct FDCArtRef
{
	int32 Mesh = 0;
	float Yaw = 0.f;
};

struct FDCRobot
{
	bool bArt = false; // an imported model stands in for the code-built body
	TWeakObjectPtr<AActor> Actor;
	int32 EyePaint = INDEX_NONE;
	bool bPlain = false; // not a robot (a cable, a crate): position only
	bool bCreature = false; // faces where its key says; (0,0) = faces the camera
	FLinearColor EyeColor = FLinearColor(4.0f, 2.6f, 0.6f);
};

struct FDCCutEval
{
	bool bOk = false;
	const TCHAR* Reason = TEXT("");
	int32 Exit = INDEX_NONE;   // exit the cut would use (or the nearest one)
	int32 Mark = INDEX_NONE;
	FVector TargetS = FVector::ZeroVector; // landing in the target shot
	float DistPx = 1.e6f;
};

// ---- film and flashbacks as data (Content/Data/film.json)

struct FDCKey
{
	float T = 0.f;
	FVector P = FVector::ZeroVector; // set camera space
	FVector2D Head = FVector2D(1.0, 0.0);
	float Roll = 0.f;
	float Eye = 1.f;
};

struct FDCTrack
{
	FString Actor;
	TArray<FDCKey> Keys;
};

struct FDCCaption
{
	float T0 = 0.f;
	float T1 = 0.f;
	FString Text;
	int32 Who = 0; // 0 both versions, 1 the official cut only, 2 the restored clip only
};

struct FDCClip
{
	FString Set;
	float Length = 1.f;
	TArray<FDCTrack> Tracks;
	TArray<FVector2D> Lamp; // (time, intensity) keys for the set's lamp, if any
	TArray<FVector2D> Cuts; // (from, to): the seconds the official cut leaves out
	TArray<FVector2D> Static; // (from, to): no signal
	TArray<struct FDCCaption> Captions;
	FString After; // one line shown when the restored clip ends
	bool bHasMatch = false;
	FVector2D MatchNdc = FVector2D::ZeroVector;
	float MatchT = 0.f;
	float ThumbT = 4.f;
	float OfficialLength() const
	{
		float L = Length;
		for (const FVector2D& C : Cuts) { L -= (float)(C.Y - C.X); }
		return FMath::Max(L, 0.5f);
	}
	/** Clip time for a play time of the official cut (the cut seconds are skipped). */
	float OfficialToClip(float T) const
	{
		for (const FVector2D& C : Cuts)
		{
			if (T >= C.X) { T += (float)(C.Y - C.X); }
		}
		return T;
	}
	bool InCut(float T) const
	{
		for (const FVector2D& C : Cuts) { if (T >= C.X && T < C.Y) { return true; } }
		return false;
	}
};

struct FDCFilmSet
{
	FVector Origin = FVector::ZeroVector;
	TWeakObjectPtr<ACameraActor> Camera;
	bool bOldFilm = false;
	bool bHeadCam = false; // first-person record: mask frame, bubbles
	float TorchPower = 0.f; // a torch on the camera
};

/** A stretch of the film timeline: a clip of the official film or a removed scene. */
struct FDCReelSeg
{
	float T0 = 0.f;
	float T1 = 0.f;
	int32 Gap = INDEX_NONE;
	FString Clip;
	FString Caption;
};

/** State saved when a shot is entered, so R can go back one shot with every loose thing where it was. */
struct FDCSnap
{
	int32 Shot = 0;
	FVector HeroS = FVector::ZeroVector;
	TArray<FDCProp> Props;
	TArray<bool> Sockets;
	TArray<bool> Gates;
	int32 Carried = INDEX_NONE;
};
