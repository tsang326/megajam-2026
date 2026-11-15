// Deep Cut: the expedition. Days of dives from a flooded base over the station: air, depth, tape, gear. Footage goes up
// for views and money; money buys the gear that reaches deeper; the proofs found there make the Director's Cut, and the
// Director's Cut decides how it ends.

#include "DCDoc.h"
#include "DC3D.h"
#include "DCAudio.h"
#include "MatchCutGameMode.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"

namespace DCCar
{
	struct FSpecies { const TCHAR* Id; const TCHAR* Name; int32 Value; bool bDanger; bool bEvidence; bool bLensOnly; int32 Zone; };
	static const FSpecies Species[] = {
		{ TEXT("creature:squid"), TEXT("Squid"), 400, false, false, false, -1 },
		{ TEXT("creature:nautilus"), TEXT("Nautilus"), 500, false, false, false, -1 },
		{ TEXT("creature:crab"), TEXT("Spider crab"), 250, false, false, false, -1 },
		{ TEXT("creature:isopod"), TEXT("Giant isopod"), 450, false, false, false, -1 },
		{ TEXT("creature:lantern"), TEXT("Lanternfish"), 200, false, false, false, -1 },
		{ TEXT("creature:whale"), TEXT("Whale"), 1500, false, false, false, -1 },
		{ TEXT("creature:shark"), TEXT("Shark"), 900, true, false, false, -1 },
		{ TEXT("creature:eel"), TEXT("Moray"), 350, false, false, false, -1 },
		{ TEXT("creature:vampsquid"), TEXT("Vampire squid"), 1800, false, false, false, 5 },
		{ TEXT("creature:hatchet"), TEXT("Hatchetfish"), 600, false, false, false, 3 },
		{ TEXT("creature:frilled"), TEXT("Frilled shark"), 2200, false, false, false, 3 },
		{ TEXT("creature:seapig"), TEXT("Sea pig"), 1200, false, false, false, 3 },
		{ TEXT("creature:barreleye"), TEXT("Barreleye"), 2500, false, false, false, 5 },
		{ TEXT("creature:coelacanth"), TEXT("Coelacanth"), 3000, false, false, false, 2 },
		{ TEXT("creature:oarfish"), TEXT("Oarfish"), 2800, false, false, false, 2 },
		{ TEXT("creature:stingray"), TEXT("Stingray"), 400, false, false, false, -1 },
		{ TEXT("creature:cuttle"), TEXT("Cuttlefish"), 450, false, false, false, -1 },
		{ TEXT("creature:brittlestar"), TEXT("Brittle star"), 150, false, false, false, -1 },
		{ TEXT("creature:siphon"), TEXT("Siphonophore"), 1600, false, false, false, 5 },
		{ TEXT("creature:seaangel"), TEXT("Sea angel"), 700, false, false, false, -1 },
		{ TEXT("creature:octopus"), TEXT("Octopus"), 600, false, false, false, -1 },
		{ TEXT("creature:goblin"), TEXT("Goblin shark"), 4000, true, false, false, 5 },
		{ TEXT("creature:gulper"), TEXT("Gulper eel"), 2600, false, false, false, 5 },
		{ TEXT("creature:sunfish"), TEXT("Sunfish"), 1400, false, false, false, 2 },
		{ TEXT("creature:seadragon"), TEXT("Sea dragon"), 900, false, false, false, 0 },
		{ TEXT("creature:seahorse"), TEXT("Seahorse"), 500, false, false, false, 0 },
		{ TEXT("creature:grouper"), TEXT("Grouper"), 300, false, false, false, -1 },
		{ TEXT("creature:lobster"), TEXT("Lobster"), 250, false, false, false, -1 },
		{ TEXT("creature:lionsmane"), TEXT("Lion's mane"), 1100, false, false, false, -1 },
		{ TEXT("creature:nudibranch"), TEXT("Nudibranch"), 600, false, false, false, -1 },
		{ TEXT("creature:krill"), TEXT("Krill"), 100, false, false, false, -1 },
		{ TEXT("creature:turtle2"), TEXT("Sea turtle"), 600, false, false, false, 0 },
		{ TEXT("creature:dolphin"), TEXT("Dolphins"), 800, false, false, false, 0 },
		{ TEXT("creature:parrotfish"), TEXT("Parrotfish"), 250, false, false, false, 0 },
		{ TEXT("creature:barracuda"), TEXT("Barracuda"), 400, true, false, false, 0 },
		{ TEXT("creature:hammerhead"), TEXT("Hammerhead"), 1600, true, false, false, 1 },
		{ TEXT("creature:yeticrab"), TEXT("Yeti crab"), 2400, false, false, false, 4 },
		{ TEXT("creature:ventshrimp"), TEXT("Vent shrimp"), 1300, false, false, false, 4 },
		{ TEXT("creature:giantsquid"), TEXT("Giant squid"), 6000, false, false, false, 5 },
		{ TEXT("creature:dragonfish"), TEXT("Dragonfish"), 2200, false, false, false, 5 },
		{ TEXT("creature:viperfish"), TEXT("Viperfish"), 2000, false, false, false, 5 },
		{ TEXT("creature:atolla"), TEXT("Atolla jelly"), 1800, false, false, false, 5 },
		{ TEXT("creature:seatoad"), TEXT("Sea toad"), 1500, false, false, false, 5 },
		// the things the official film says do not exist: the proofs
		{ TEXT("creature:ribray"), TEXT("The Ribcage Ray"), 3000, true, true, false, 0 },
		{ TEXT("creature:hollowdiver"), TEXT("The Hollow Diver"), 4500, true, true, false, 1 },
		{ TEXT("creature:drowner"), TEXT("The Drowner"), 5000, true, true, false, 2 },
		{ TEXT("creature:choir"), TEXT("The Choir"), 5500, true, true, false, 3 },
		{ TEXT("creature:mawworm"), TEXT("Maw Worm"), 4000, true, false, false, 3 },
		{ TEXT("creature:stilt"), TEXT("The Stilt Walker"), 6000, true, true, false, 4 },
		{ TEXT("creature:widow"), TEXT("The Lamp Widow"), 8000, true, true, false, 5 },
		{ TEXT("creature:warden"), TEXT("The Warden"), 15000, true, true, true, 5 },
	};
	static const TCHAR* GearNames[DCGear::Num] = { TEXT("AIR TANK"), TEXT("HULL RATING"), TEXT("HEADLAMP"), TEXT("LONG LENS"), TEXT("THRUSTER"), TEXT("TAPE") };
	static const int32 Levels[DCGear::Num] = { 3, 4, 2, 2, 3, 3 };
	static const int32 Costs[DCGear::Num][4] = { { 0, 300, 800, 0 }, { 0, 500, 1200, 2500 }, { 0, 250, 0, 0 }, { 0, 350, 0, 0 }, { 0, 400, 900, 0 }, { 0, 200, 600, 0 } };
	static const float TankAir[3] = { 210.f, 330.f, 480.f };
	static const float HullDepth[4] = { 150.f, 250.f, 350.f, 520.f };
	static const int32 TapeClips[3] = { 6, 10, 16 };
	static const float Speed[3] = { 1.f, 1.2f, 1.4f };
	static constexpr double Stand = FDC3D::Stand;
}

// ---------------------------------------------------------------------------------------------
// what is worth filming
// ---------------------------------------------------------------------------------------------

void FDCDoc::SetupSubjects()
{
	Subjects.Reset();
	auto Add = [this](const TCHAR* Id, const TCHAR* Name, int32 Value, float Radius)
	{
		FDCSubject S;
		S.Id = Id;
		S.Name = Name;
		S.Value = Value;
		S.Radius = Radius;
		return Subjects.Add(S);
	};
	// the six moments of the station's site, in DCSub order
	Add(TEXT("moment:manta"), TEXT("Manta ray"), 1200, 250.f);
	Add(TEXT("moment:shoal"), TEXT("The shoal"), 700, 300.f);
	Add(TEXT("moment:angler"), TEXT("Anglerfish"), 2500, 90.f);
	Add(TEXT("moment:jelly"), TEXT("Jellyfish bloom"), 600, 300.f);
	Add(TEXT("moment:giant"), TEXT("The giant overhead"), 3500, 380.f);
	Add(TEXT("moment:crew"), TEXT("Crewmate"), 80, 90.f);
	for (const DCCar::FSpecies& Sp : DCCar::Species)
	{
		if (!GM->Art.Contains(Sp.Id))
		{
			continue;
		}
		const int32 i = Add(Sp.Id, Sp.Name, Sp.Value, 60.f);
		Subjects[i].bDanger = Sp.bDanger;
		Subjects[i].bEvidence = Sp.bEvidence;
		Subjects[i].bLensOnly = Sp.bLensOnly;
		Subjects[i].Zone = Sp.Zone;
	}
	Legend = INDEX_NONE;
	if (GM->Art.Contains(TEXT("creature:giantsquid")))
	{
		Legend = Add(TEXT("legend:leviathan"), TEXT("THE LEVIATHAN"), 20000, 700.f);
		Subjects[Legend].bDanger = Subjects[Legend].bEvidence = Subjects[Legend].bLensOnly = true;
		Subjects[Legend].Zone = 5;
	}
	UE_LOG(LogMatchCut, Display, TEXT("DOC subjects: %d, proofs %d, legend %d"), Subjects.Num(), EvidenceTotal(), Legend);
}

int32 FDCDoc::EvidenceTotal() const
{
	int32 N = 0;
	for (const FDCSubject& S : Subjects)
	{
		N += S.bEvidence ? 1 : 0;
	}
	return N;
}

int32 FDCDoc::EvidenceCount() const
{
	return Archive.Num();
}

bool FDCDoc::IsUploaded(int32 Sub) const
{
	if (Uploaded.Contains(Sub))
	{
		return true;
	}
	for (const FDCTake& K : Archive)
	{
		if (K.Shot == Sub)
		{
			return true;
		}
	}
	return false;
}

bool FDCDoc::IsFound(int32 Sub) const
{
	if (IsUploaded(Sub))
	{
		return true;
	}
	for (const FDCTake& T : Takes)
	{
		if (T.Shot == Sub)
		{
			return true;
		}
	}
	return false;
}

int32 FDCDoc::Mission() const
{
	for (int32 Sub : Missions)
	{
		if (!IsFound(Sub))
		{
			return Sub;
		}
	}
	return INDEX_NONE;
}

bool FDCDoc::MissionWhere(int32 Sub, FVector& Out) const
{
	double Near = 1e30;
	bool bAny = false;
	for (const FAmbient& One : Ambient)
	{
		if (One.Subject != Sub)
		{
			continue;
		}
		const AActor* A = One.Actor.Get();
		const FVector At = A && !One.bAway ? A->GetActorLocation() : One.Centre;
		const double D = FVector::DistSquared(At, W->Pos);
		if (D < Near)
		{
			Near = D;
			Out = At;
			bAny = true;
		}
	}
	if (!bAny && Sub == Legend)
	{
		if (const AActor* H = Hunter.Get())
		{
			Out = H->GetActorLocation();
			return true;
		}
	}
	if (!bAny && Subjects.IsValidIndex(Sub) && W->Zones.IsValidIndex(Subjects[Sub].Zone))
	{
		const FVector2D Z = W->Zones[Subjects[Sub].Zone].At;
		Out = FVector(Z.X, Z.Y, W->Terrain(Z.X, Z.Y) + 600.0);
		return true;
	}
	return bAny;
}

void FDCDoc::BuildMissions()
{
	Missions.Reset();
	FirstTarget = INDEX_NONE;
	const FVector From = W->HatchAt;
	// the first: a calm animal of the station's own site, big enough to see, the nearest to the moon pool
	double Near = 1e30;
	for (const FAmbient& One : Ambient)
	{
		if (!Subjects.IsValidIndex(One.Subject) || One.Subject < DCSub::Num || One.Radius < 45.f)
		{
			continue;
		}
		const FDCSubject& S = Subjects[One.Subject];
		if (S.bDanger || S.bEvidence || S.bLensOnly || S.Zone != -1)
		{
			continue;
		}
		const double D = FVector::DistSquared(One.Centre, From);
		if (D < Near)
		{
			Near = D;
			FirstTarget = One.Subject;
		}
	}
	if (FirstTarget != INDEX_NONE)
	{
		Missions.Add(FirstTarget);
	}
	TArray<TPair<double, int32>> Proofs;
	for (int32 s = 0; s < Subjects.Num(); ++s)
	{
		if (Subjects[s].bEvidence)
		{
			FVector At;
			Proofs.Add(TPair<double, int32>(MissionWhere(s, At) ? FVector::Dist(At, From) : 1e12, s));
		}
	}
	Proofs.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
	for (const TPair<double, int32>& P : Proofs)
	{
		Missions.Add(P.Value);
	}
	UE_LOG(LogMatchCut, Display, TEXT("DOC MISSIONS first %s (%.0f m from the pool), %d targets"), FirstTarget != INDEX_NONE ? *Subjects[FirstTarget].Name : TEXT("none"), FMath::Sqrt(Near) / 100.0, Missions.Num());
}

void FDCDoc::TakePhoto(int32 Sub)
{
	UWorld* World = GM->GetWorld();
	if (!World)
	{
		return;
	}
	if (!PhotoCam.IsValid())
	{
		ASceneCapture2D* C = World->SpawnActor<ASceneCapture2D>(CamP, CamR);
		if (!C)
		{
			return;
		}
		USceneCaptureComponent2D* Cap = C->GetCaptureComponent2D();
		Cap->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Cap->bCaptureEveryFrame = false;
		Cap->bCaptureOnMovement = false;
		Cap->PostProcessBlendWeight = 1.f;
		PhotoCam = C;
	}
	TStrongObjectPtr<UTextureRenderTarget2D>& RT = Photos.FindOrAdd(Sub);
	if (!RT.IsValid())
	{
		UTextureRenderTarget2D* T = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
		T->RenderTargetFormat = RTF_RGBA8_SRGB;
		T->ClearColor = FLinearColor::Black;
		T->InitAutoFormat(384, 216);
		T->UpdateResourceImmediate(true);
		RT.Reset(T);
	}
	ASceneCapture2D* C = PhotoCam.Get();
	USceneCaptureComponent2D* Cap = C->GetCaptureComponent2D();
	if (ACameraActor* V = W->Cam.Get())
	{
		Cap->PostProcessSettings = V->GetCameraComponent()->PostProcessSettings;
	}
	Cap->TextureTarget = RT.Get();
	Cap->FOVAngle = CamFov;
	C->SetActorLocationAndRotation(CamP, CamR);
	Cap->CaptureScene();
}

// ---------------------------------------------------------------------------------------------
// gear
// ---------------------------------------------------------------------------------------------

int32 FDCDoc::GearLevels(int32 G) const { return DCCar::Levels[G]; }
const TCHAR* FDCDoc::GearName(int32 G) const { return DCCar::GearNames[G]; }
int32 FDCDoc::GearCost(int32 G) const { return Gear[G] + 1 < DCCar::Levels[G] ? DCCar::Costs[G][Gear[G] + 1] : INDEX_NONE; }
int32 FDCDoc::TapeCap() const { return DCCar::TapeClips[Gear[DCGear::Tape]]; }
float FDCDoc::HullLimit() const { return DCCar::HullDepth[Gear[DCGear::Hull]]; }
float FDCDoc::SpeedMul() const { return DCCar::Speed[Gear[DCGear::Scooter]]; }

FString FDCDoc::GearLine(int32 G) const
{
	switch (G)
	{
	case DCGear::Tank: return FString::Printf(TEXT("%.0f s of air"), DCCar::TankAir[Gear[G]]);
	case DCGear::Hull: return FString::Printf(TEXT("safe to %.0f m"), DCCar::HullDepth[Gear[G]]);
	case DCGear::Lamp: return Gear[G] ? TEXT("strong beam") : TEXT("weak beam");
	case DCGear::Lens: return Gear[G] ? TEXT("zoom to 14 deg") : TEXT("zoom to 30 deg");
	case DCGear::Scooter: return FString::Printf(TEXT("%.0f%% speed"), DCCar::Speed[Gear[G]] * 100.f);
	case DCGear::Tape: return FString::Printf(TEXT("%d clips a dive"), TapeCap());
	}
	return FString();
}

void FDCDoc::Buy(int32 G)
{
	const int32 Cost = GearCost(G);
	if (Cost == INDEX_NONE)
	{
		ShopNote = TEXT("Already the best there is.");
	}
	else if (Money < Cost)
	{
		ShopNote = FString::Printf(TEXT("Not enough money: $%d more."), Cost - Money);
	}
	else
	{
		Money -= Cost;
		++Gear[G];
		ShopNote = FString::Printf(TEXT("%s upgraded: %s."), GearName(G), *GearLine(G));
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("part_pickup"), 0.8f, 1.f);
		}
		UE_LOG(LogMatchCut, Display, TEXT("DOC SHOP bought %s level %d, money %d"), GearName(G), Gear[G], Money);
	}
	ShopNoteTime = 4.f;
	O2Max = DCCar::TankAir[Gear[DCGear::Tank]];
	LampPower = Gear[DCGear::Lamp] ? 70.f : 26.f;
	MinFov = Gear[DCGear::Lens] ? 14.f : 30.f;
	bLens = Gear[DCGear::Lens] > 0;
}

int32 FDCDoc::ClipsKept() const
{
	int32 N = 0;
	for (const FDCTake& T : Takes)
	{
		N += T.bKeep ? 1 : 0;
	}
	return N;
}

// ---------------------------------------------------------------------------------------------
// the day
// ---------------------------------------------------------------------------------------------

void FDCDoc::StartDay()
{
	O2Max = DCCar::TankAir[Gear[DCGear::Tank]];
	O2 = O2Max;
	LampPower = Gear[DCGear::Lamp] ? 70.f : 26.f;
	bLampOwned = bLampOn = true;
	MinFov = Gear[DCGear::Lens] ? 14.f : 30.f;
	Takes.Reset();
	Headline = INDEX_NONE;
	bLeftBase = bUploadedToday = bBlackout = bLostTape = false;
	BlackT = -1.f;
	bBoardOpen = bNewsOpen = false;
	SleepAsk = 0.f;
	W->Pos = W->BaseWake;
	W->Vel = FVector::ZeroVector;
	W->Yaw = 180.f;
	W->Pitch = -10.f;
	W->Pivot = W->Pos;
	// what the viewers ask for today: something not yet uploaded, worth a lot, that today's gear can reach
	Trending = INDEX_NONE;
	FRandomStream R(Day * 977 + 13);
	TArray<int32> Pool;
	for (int32 s = DCSub::Num; s < Subjects.Num(); ++s)
	{
		const FDCSubject& S = Subjects[s];
		if (!S.bEvidence && !TimesUploaded.Contains(s) && S.Value >= 400 && S.Zone <= Gear[DCGear::Hull] + 1)
		{
			Pool.Add(s);
		}
	}
	if (Pool.Num())
	{
		Trending = Pool[R.RandRange(0, Pool.Num() - 1)];
	}
	Alert = FString::Printf(TEXT("DAY %d OF %d"), Day, Days);
	AlertTime = 4.f;
	UE_LOG(LogMatchCut, Display, TEXT("DOC DAY %d: money %d, air %.0f, hull %.0f m, tape %d, trending %s"), Day, Money, O2Max, HullLimit(), TapeCap(), *SubjectName(Trending));
}

void FDCDoc::Surface(bool bLost)
{
	// kept for the test script: straight back into the base
	W->Pos = W->BaseWake;
	W->Vel = FVector::ZeroVector;
	bLostTape = bLost;
}

void FDCDoc::Sleep()
{
	if (Takes.Num() > 0 && !bUploadedToday && SleepAsk <= 0.f)
	{
		Alert = TEXT("Today's footage is not uploaded. It will be lost. Use the bunk again to sleep anyway.");
		AlertTime = 4.f;
		SleepAsk = 4.f;
		return;
	}
	SleepAsk = 0.f;
	if (Day >= Days)
	{
		// the hearing is tomorrow: the Director's Cut goes out tonight
		const int32 E = EvidenceCount(), T = FMath::Max(1, EvidenceTotal());
		bool bLegend = false;
		for (const FDCTake& K : Archive)
		{
			bLegend |= Subjects.IsValidIndex(K.Shot) && Subjects[K.Shot].bLensOnly;
		}
		Ending = (E >= T - 2 && bLegend) ? 1 : E >= 3 ? 2 : 3;
		Takes = Archive;
		for (int32 i = 0; i < DCSub::Num; ++i)
		{
			Pick[i] = INDEX_NONE;
		}
		bFinalCut = true;
		UE_LOG(LogMatchCut, Display, TEXT("DOC FINAL CUT: %d/%d proofs, legend %d, ending %d, views %lld, money %d"), E, T, bLegend ? 1 : 0, Ending, TotalViews, Money);
		SetState(Takes.Num() ? EDCDocState::Film : EDCDocState::Rating);
		return;
	}
	++Day;
	if (GM->Audio)
	{
		GM->Audio->Play(TEXT("take_clapper"), 0.6f, 0.7f);
	}
	StartDay();
}

void FDCDoc::UseSpot(int32 S)
{
	switch (S)
	{
	case 0:
		if (bUploadedToday)
		{
			Alert = TEXT("Today's video is up. Rest at the bunk, or buy gear at the locker.");
		}
		else if (Takes.Num() == 0)
		{
			Alert = TEXT("No footage on the tape. Go down through the hatch and film something.");
		}
		else
		{
			EditShot = 0;
			Headline = Headline == INDEX_NONE ? 0 : Headline;
			SetState(EDCDocState::Edit);
			StartPlayTake(0);
			return;
		}
		AlertTime = 4.f;
		break;
	case 1:
		ShopSel = 0;
		SetState(EDCDocState::Shop);
		break;
	case 2:
		Sleep();
		break;
	case 3:
		bBoardOpen = !bBoardOpen;
		break;
	case 4:
		bNewsOpen = !bNewsOpen;
		break;
	}
}

void FDCDoc::TickDive(float Dt)
{
	AlertTime = FMath::Max(0.f, AlertTime - Dt);
	HurtTime = FMath::Max(0.f, HurtTime - Dt);
	HitCooldown = FMath::Max(0.f, HitCooldown - Dt);
	SleepAsk = FMath::Max(0.f, SleepAsk - Dt);
	if (bBlackout)
	{
		BlackT += Dt;
		W->Vel *= FMath::Exp(-2.f * Dt);
		if (BlackT > 4.f)
		{
			// you wake in the base: the crew pulled you in; the tape did not come back, and the day is gone
			bBlackout = false;
			Takes.Reset();
			if (Day >= Days)
			{
				Sleep();
			}
			else
			{
				++Day;
				StartDay();
				Alert = TEXT("The crew pulled you in. The tape is lost.");
				AlertTime = 6.f;
			}
		}
		return;
	}
	const bool bWasIn = bInBase;
	bInBase = W->InBase(W->Pos);
	if (!bInBase)
	{
		bLeftBase = true;
		bBoardOpen = false;
		bNewsOpen = false;
	}
	if (bInBase != bWasIn && GM->Audio)
	{
		GM->Audio->Play(TEXT("plate_lift"), 0.5f, bInBase ? 1.1f : 0.8f);
	}
	// air
	bOverDepth = W->ZoneDepth > HullLimit();
	if (bInBase)
	{
		O2 = FMath::Min(O2Max, O2 + 60.f * Dt);
	}
	else
	{
		const float Rate = (W->bBoost ? 1.25f : 1.f) * (bOverDepth ? 5.f : 1.f);
		const float Before = O2;
		O2 -= Rate * Dt;
		if (bOverDepth && AlertTime <= 0.f)
		{
			Alert = FString::Printf(TEXT("HULL OVER RATING (%.0f m): air going fast. Climb, or buy a better hull."), HullLimit());
			AlertTime = 2.f;
		}
		else if (Before > O2Max * 0.25f && O2 <= O2Max * 0.25f)
		{
			Alert = TEXT("AIR LOW. Head back to the base.");
			AlertTime = 5.f;
		}
		if (O2 <= 0.f)
		{
			O2 = 0.f;
			bBlackout = true;
			BlackT = 0.f;
			bRecording = false;
			bAimToggle = false;
			UE_LOG(LogMatchCut, Display, TEXT("DOC BLACKOUT day %d at depth %.0f"), Day, W->ZoneDepth);
			return;
		}
	}
	// what bites
	DangerNear = 0.f;
	auto Bite = [this](const FVector& From, float Hurt, const TCHAR* What)
	{
		O2 = FMath::Max(1.f, O2 - Hurt);
		HurtTime = 0.7f;
		HitCooldown = 2.5f;
		W->ShieldAmt = 1.f;
		W->Vel += (W->Pos - From).GetSafeNormal() * 700.0;
		Alert = FString::Printf(TEXT("%s bit you: -%.0f s of air."), What, Hurt);
		AlertTime = 3.f;
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("plate_lift"), 1.f, 0.5f);
		}
		UE_LOG(LogMatchCut, Display, TEXT("DOC BITE %s, air %.0f"), What, O2);
	};
	for (const FAmbient& One : Ambient)
	{
		if (One.bAway || !Subjects.IsValidIndex(One.Subject) || !Subjects[One.Subject].bDanger)
		{
			continue;
		}
		if (const AActor* A = One.Actor.Get())
		{
			const double D = FVector::Dist(A->GetActorLocation(), W->Pos);
			if (D < 900.0)
			{
				DangerNear = 1.f;
			}
			if (D < One.Radius + 120.0 && HitCooldown <= 0.f && !bInBase)
			{
				Bite(A->GetActorLocation(), 25.f, *Subjects[One.Subject].Name);
			}
		}
	}
	TickHunter(Dt, true);
	if (bHunting && FVector::Dist(HuntPos, W->Pos) < 1200.0)
	{
		DangerNear = 1.f;
	}
	if (bHunting && FVector::Dist(HuntPos, W->Pos) < 420.0 && HitCooldown <= 0.f)
	{
		Bite(HuntPos, 45.f, TEXT("Something huge"));
		HuntAlert = 0.f;
		bHunting = false;
	}
	// what only the lens sees
	const bool bThroughLens = AimBlend > 0.5f;
	for (const FAmbient& One : Ambient)
	{
		if (!One.bAway && Subjects.IsValidIndex(One.Subject) && Subjects[One.Subject].bLensOnly)
		{
			if (AActor* A = One.Actor.Get())
			{
				A->SetActorHiddenInGame(!bThroughLens);
			}
		}
	}
	if (AActor* H = Hunter.Get())
	{
		H->SetActorHiddenInGame(!bThroughLens);
	}
	// the base's controls: the one looked at, close by
	Spot = INDEX_NONE;
	if (bInBase && !bAim)
	{
		float BestDot = 0.92f;
		const FVector Eye = W->CamPos, Dir = FRotator(W->Pitch, W->Yaw, 0.f).Vector();
		for (int32 i = 0; i < W->BaseSpots.Num(); ++i)
		{
			const FVector To = W->BaseSpots[i] - Eye;
			const float Dot = FVector::DotProduct(To.GetSafeNormal(), Dir);
			if (To.Size() < 520.0 && Dot > BestDot)
			{
				BestDot = Dot;
				Spot = i;
			}
		}
	}
	W->ReachAmt = FMath::FInterpTo(W->ReachAmt, Spot != INDEX_NONE ? 0.35f : 0.f, Dt, 6.f);
	SavedTime = FMath::Max(0.f, SavedTime - Dt);
	if (Missions.Num() == 0 && Ambient.Num() > 0 && !W->HatchAt.IsZero())
	{
		BuildMissions();
	}
	if (bBagPressed)
	{
		// TAB: the bag, anywhere
		const bool bWas = bBagOpen;
		bBoardOpen = bNewsOpen = false;
		bBagOpen = !bWas;
		bEverBag |= bBagOpen;
		bUsePressed = false;
		if (GM->Audio)
		{
			GM->Audio->Play(TEXT("ui_select"), 0.6f, bBagOpen ? 1.1f : 0.9f);
		}
	}
	else if (PageOpen())
	{
		// the board (or the news, the bag) closes on any of E, ENTER, ESC, or on walking away from it
		const int32 At = bBoardOpen ? 3 : bNewsOpen ? 4 : INDEX_NONE;
		const bool bAway = W->BaseSpots.IsValidIndex(At) && FVector::Dist(W->Pos, W->BaseSpots[At]) > 650.0;
		if (bUsePressed || bBackPressed || bAway)
		{
			bBoardOpen = false;
			bNewsOpen = false;
			bBagOpen = false;
			bUsePressed = false;
		}
	}
	else if (bUsePressed && Spot != INDEX_NONE)
	{
		W->ReachAmt = 1.f;
		UseSpot(Spot);
	}
}

// ---------------------------------------------------------------------------------------------
// the upload
// ---------------------------------------------------------------------------------------------

void FDCDoc::MakeUpload()
{
	UploadLines.Reset();
	Comments.Reset();
	VideoViews = 0;
	TSet<int32> InVideo;
	float BestValue = -1.f;
	int32 BestSub = INDEX_NONE;
	bool bAnyProof = false, bAnyDanger = false, bAnyLegend = false;
	for (int32 i = 0; i < Takes.Num(); ++i)
	{
		const FDCTake& T = Takes[i];
		if (!T.bKeep || !Subjects.IsValidIndex(T.Shot))
		{
			continue;
		}
		const FDCSubject& S = Subjects[T.Shot];
		FDCUploadLine L;
		L.Subject = T.Shot;
		L.Stars = T.Stars;
		L.bNew = !TimesUploaded.Contains(T.Shot);
		L.bDup = InVideo.Contains(T.Shot);
		const float StarMul = T.Stars >= 3 ? 1.5f : T.Stars == 2 ? 0.8f : 0.4f;
		const int32 Times = TimesUploaded.FindRef(T.Shot);
		const float Novel = L.bDup ? 0.2f : Times == 0 ? 1.f : Times == 1 ? 0.5f : 0.25f;
		const float Mul = StarMul * Novel * (T.bDanger ? 1.5f : 1.f) * (T.Shot == Trending ? 2.f : 1.f) * (i == Headline ? 2.5f : 1.f);
		L.Views = FMath::RoundToInt(S.Value * Mul);
		VideoViews += L.Views;
		UploadLines.Add(L);
		InVideo.Add(T.Shot);
		bAnyProof |= S.bEvidence;
		bAnyDanger |= T.bDanger;
		bAnyLegend |= T.Shot == Legend;
		if (S.Value * StarMul > BestValue)
		{
			BestValue = S.Value * StarMul;
			BestSub = T.Shot;
		}
		Uploaded.Add(T.Shot);
		// a proof goes into the archive for the Director's Cut: the best clip of each
		if (S.bEvidence)
		{
			int32 Have = INDEX_NONE;
			for (int32 k = 0; k < Archive.Num(); ++k)
			{
				Have = Archive[k].Shot == T.Shot ? k : Have;
			}
			if (Have == INDEX_NONE)
			{
				Archive.Add(T);
			}
			else if (T.Stars > Archive[Have].Stars)
			{
				Archive[Have] = T;
			}
		}
	}
	for (int32 s : InVideo)
	{
		TimesUploaded.FindOrAdd(s)++;
	}
	const int32 HeadSub = Takes.IsValidIndex(Headline) ? Takes[Headline].Shot : BestSub;
	VideoTitle = FString::Printf(TEXT("DAY %d: %s%s"), Day, *SubjectName(HeadSub).ToUpper(), bAnyProof ? TEXT(" (NOT FAKE)") : TEXT(""));
	Earned = (int32)(VideoViews / 8);
	NewSubs = (int32)(VideoViews / 60);
	Money += Earned;
	TotalViews += VideoViews;
	Subscribers += NewSubs;
	bUploadedToday = true;
	// what the comments say
	if (bAnyLegend) { Comments.Add(TEXT("THAT IS NOT A SQUID. THAT IS NOT ANYTHING.")); }
	if (bAnyProof) { Comments.Add(TEXT("The company said that trench was empty. Explain THIS.")); Comments.Add(TEXT("frame 0:42. zoom in. i'm not sleeping tonight")); }
	if (bAnyDanger) { Comments.Add(TEXT("how is the camera still rolling when that thing is THAT close")); }
	if (VideoViews < 1500) { Comments.Add(TEXT("nice fish i guess")); }
	if (VideoViews >= 6000) { Comments.Add(TEXT("subscribed. go deeper.")); }
	Comments.Add(Trending != INDEX_NONE && !InVideo.Contains(Trending) ? FString::Printf(TEXT("when are you filming the %s??"), *SubjectName(Trending).ToLower()) : FString(TEXT("upload schedule is insane, respect")));
	UE_LOG(LogMatchCut, Display, TEXT("DOC UPLOAD day %d: %d clips, views %lld, earned %d, money %d, proofs %d/%d \"%s\""), Day, UploadLines.Num(), VideoViews, Earned, Money, EvidenceCount(), EvidenceTotal(), *VideoTitle);
}

// ---------------------------------------------------------------------------------------------
// the legend of the abyss: it drifts in the trench; light or speed in its water brings it
// ---------------------------------------------------------------------------------------------

void FDCDoc::TickHunter(float Dt, bool bLive)
{
	AActor* A = Hunter.Get();
	if (!A || !W->Zones.IsValidIndex(5))
	{
		return;
	}
	const FDC3D::FZone& Z = W->Zones[5];
	const FVector Home(Z.At.X, Z.At.Y, W->Terrain(Z.At.X, Z.At.Y) + 1500.0);
	if (bLive)
	{
		const bool bInTrench = FVector::Dist2D(W->Pos, Home) < Z.Radius * 1.1 && W->Pos.Z < Home.Z + 1500.0;
		const bool bLoud = bLampOn || W->bBoost || W->Vel.Size() > 700.0;
		HuntAlert = FMath::Clamp(HuntAlert + Dt * (bInTrench ? (bLoud ? 0.35f : 0.05f) : -0.3f), 0.f, 1.f);
		const bool bWas = bHunting;
		bHunting = HuntAlert > 0.6f || (bHunting && HuntAlert > 0.2f);
		if (bHunting && !bWas)
		{
			Alert = TEXT("Something moved in the dark. Lamp off. Keep still.");
			AlertTime = 4.f;
			UE_LOG(LogMatchCut, Display, TEXT("DOC LEGEND hunting"));
		}
		FVector Want;
		float Speed;
		if (bHunting)
		{
			Want = W->Pos;
			Speed = 520.f + 400.f * HuntAlert;
		}
		else
		{
			const float T = Clock * 0.05f;
			Want = Home + FVector(FMath::Cos(T) * 2600.0, FMath::Sin(T) * 2600.0, 300.0 * FMath::Sin(T * 3.f));
			Speed = 260.f;
		}
		if (HuntPos.IsZero())
		{
			HuntPos = Want;
		}
		const FVector V = (Want - HuntPos).GetClampedToMaxSize(Speed);
		HuntVel = FMath::Lerp(HuntVel, V, 1.0 - FMath::Exp(-1.5 * Dt));
		HuntPos += HuntVel * Dt;
		HuntPos.Z = FMath::Max(HuntPos.Z, (double)W->Terrain(HuntPos.X, HuntPos.Y) + 300.0);
		if (HuntVel.SizeSquared() > 100.0)
		{
			HuntYaw = HuntVel.Rotation().Yaw;
		}
	}
	A->SetActorLocationAndRotation(HuntPos, FRotator(0.f, HuntYaw, 0.f));
}

// ---------------------------------------------------------------------------------------------
// the base itself (FDC3D: the world builds it)
// ---------------------------------------------------------------------------------------------

bool FDC3D::InBase(const FVector& P) const
{
	const FVector D = (P - BaseC).GetAbs();
	return D.X < BaseHalf.X && D.Y < BaseHalf.Y && D.Z < BaseHalf.Z + 40.0;
}

void FDC3D::BuildBase()
{
	// a modern habitat: pale panels on a steel frame, light strips, viewports onto the sea, a moon pool at one end
	const FLinearColor PanelC(0.50f, 0.52f, 0.54f), FrameC(0.10f, 0.11f, 0.12f), DeckC(0.11f, 0.115f, 0.12f), CeilC(0.34f, 0.35f, 0.36f), Paint(0.55f, 0.40f, 0.06f);
	const double T = 30.0;
	// never into the sea floor: the whole box (and a little room under it) sits above the highest ground in its footprint
	// (the hull outside is about twice as long as it is wide, longer than the room: its footprint counts)
	double GroundTop = -1e9;
	const double FootX = BaseHalf.X + 140.0, FootY = BaseHalf.Y + 140.0;
	for (int32 ix = 0; ix <= 10; ++ix)
	{
		for (int32 iy = 0; iy <= 6; ++iy)
		{
			GroundTop = FMath::Max(GroundTop, (double)Terrain(BaseC.X - FootX + 2.0 * FootX * ix / 10.0, BaseC.Y - FootY + 2.0 * FootY * iy / 6.0));
		}
	}
	BaseC.Z = FMath::Max(BaseC.Z, GroundTop + 260.0 + BaseHalf.Z);
	const FVector C = BaseC, H = BaseHalf;
	const double FloorTop = C.Z - H.Z, CeilZ = C.Z + H.Z;
	// the architecture kit (scene:wallpanel ... scene:moonring): models stretched onto a grid. The boxes stay for
	// collision, hidden. Without the kit the code-built room is drawn (the old look).
	auto KitMesh = [this](const TCHAR* Id) -> UStaticMesh*
	{
		const TArray<FDCArtRef>* Set = GM->Art.Find(Id);
		return Set && Set->Num() > 0 && GM->ArtMeshes.IsValidIndex((*Set)[0].Mesh) ? GM->ArtMeshes[(*Set)[0].Mesh].Get() : nullptr;
	};
	auto Kit = [this, &KitMesh](const TCHAR* Id, const FVector& Centre, const FRotator& Rot, const FVector& Size) -> bool
	{
		UStaticMesh* M = KitMesh(Id);
		if (!M)
		{
			return false;
		}
		const FBox B = M->GetBoundingBox();
		const FVector S = Size / B.GetSize().ComponentMax(FVector(0.1));
		AActor* A = GM->SpawnGroup(Centre);
		A->SetActorRotation(Rot);
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(A);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetStaticMesh(M);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetupAttachment(A->GetRootComponent());
		Part->RegisterComponent();
		Part->SetRelativeScale3D(S);
		Part->SetRelativeLocation(-B.GetCenter() * S);
		return true;
	};
	// set A (scene:awall ...: the bright research-station look the user picked) wins over the first kit when it is there
	const bool bA = KitMesh(TEXT("scene:awall")) && KitMesh(TEXT("scene:afloor")) && !FParse::Param(FCommandLine::Get(), TEXT("MCOldKit"));
	const bool bKit = (bA || (KitMesh(TEXT("scene:wallpanel")) && KitMesh(TEXT("scene:floortile")))) && !FParse::Param(FCommandLine::Get(), TEXT("MCNoKit"));
	auto Pick = [&](const TCHAR* IdA, const TCHAR* IdOld) -> const TCHAR* { return bA && KitMesh(IdA) ? IdA : IdOld; };
	// warm white in set A, the old cold blue otherwise
	const FLinearColor StripC = bA ? FLinearColor(3.2f, 2.9f, 2.4f) : FLinearColor(1.6f, 2.6f, 3.2f);
	const FLinearColor CeilLightC = bA ? FLinearColor(3.6f, 3.3f, 2.8f) : FLinearColor(3.f, 3.2f, 3.4f);
	// which way the kit's panels face (their decorated side), checked on snapshots; flags to flip them for a test
	float KitFront = 1.f, CeilFront = 1.f;
	FParse::Value(FCommandLine::Get(), TEXT("-MCKitFront="), KitFront);
	FParse::Value(FCommandLine::Get(), TEXT("-MCCeilFront="), CeilFront);
	// the floor and the ceiling are a grid of NX x NY tiles (about 1.9 x 1.6 m); the hatch is a hole of 2 x 2 of them, one
	// tile in from the +x end. NY is even: the hatch sits on the middle line
	const int32 NX = FMath::Max(4, FMath::RoundToInt(2.0 * H.X / 191.0)), NY = FMath::Max(2, FMath::RoundToInt(H.Y / 162.0)) * 2;
	const double TileX = 2.0 * H.X / NX, TileY = 2.0 * H.Y / NY;
	const FVector Hatch = C + FVector(-H.X + (NX - 2) * TileX, 0.0, -H.Z);
	const double HW = 150.0;
	const double HoleX = bKit ? TileX : HW, HoleY = bKit ? TileY : HW;
	const double FloorZ = FloorTop - T * 0.5;
	// a light panel: no collision, it only shines
	auto LightPanel = [this](const FVector& At, const FVector& Size, const FLinearColor& Em)
	{
		AActor* A = GM->SpawnGroup(At);
		GM->AddPart(A, GM->CubeMesh, FVector::ZeroVector, FRotator::ZeroRotator, Size / 100.0, FLinearColor(0.9f, 0.95f, 1.f), Em);
	};
	// floor: four slabs round the hatch
	TArray<AActor*> Shell; // the collision boxes the kit covers: hidden when it is there
	Shell.Add(Block(FVector((C.X - H.X + Hatch.X - HoleX) * 0.5, C.Y, FloorZ), FRotator::ZeroRotator, FVector(Hatch.X - HoleX - (C.X - H.X), 2.0 * H.Y, T), DeckC));
	Shell.Add(Block(FVector((Hatch.X + HoleX + C.X + H.X) * 0.5, C.Y, FloorZ), FRotator::ZeroRotator, FVector(C.X + H.X - Hatch.X - HoleX, 2.0 * H.Y, T), DeckC));
	Shell.Add(Block(FVector(Hatch.X, (C.Y - H.Y + Hatch.Y - HoleY) * 0.5, FloorZ), FRotator::ZeroRotator, FVector(2.0 * HoleX, Hatch.Y - HoleY - (C.Y - H.Y), T), DeckC));
	Shell.Add(Block(FVector(Hatch.X, (Hatch.Y + HoleY + C.Y + H.Y) * 0.5, FloorZ), FRotator::ZeroRotator, FVector(2.0 * HoleX, C.Y + H.Y - Hatch.Y - HoleY, T), DeckC));
	Shell.Add(Block(C + FVector(0.0, 0.0, H.Z + T * 0.5), FRotator::ZeroRotator, FVector(2.0 * H.X, 2.0 * H.Y, T), CeilC));
	if (bKit)
	{
		// a dark deck under the tiles (the collision slabs are hidden): a seam between tiles shows steel, never the sea
		for (int32 i = 0; i < 4; ++i)
		{
			const FBox SB = Shell[i]->GetComponentsBoundingBox();
			AActor* Under = GM->SpawnGroup(FVector(SB.GetCenter().X, SB.GetCenter().Y, FloorTop - 12.0));
			GM->AddPart(Under, GM->CubeMesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(SB.GetSize().X, SB.GetSize().Y, 4.0) / 100.0, FLinearColor(0.05f, 0.05f, 0.055f), FLinearColor::Black);
		}
		for (int32 ix = 0; ix < NX; ++ix)
		{
			for (int32 iy = 0; iy < NY; ++iy)
			{
				const double X = C.X - H.X + (ix + 0.5) * TileX, Y = C.Y - H.Y + (iy + 0.5) * TileY;
				if (FMath::Abs(X - Hatch.X) < TileX && FMath::Abs(Y - Hatch.Y) < TileY)
				{
					continue; // the hatch
				}
				const uint32 Hash = (uint32)(ix * 7 + iy * 13);
				Kit(Pick(TEXT("scene:afloor"), TEXT("scene:floortile")), FVector(X, Y, FloorTop - 7.0), FRotator(0.f, (Hash % 2) * 180.f, 0.f), FVector(TileX * (bA ? 1.04 : 1.0) + 1.0, TileY * (bA ? 1.04 : 1.0) + 1.0, 14.0)); // set A's tile: a cable off one corner makes its plate ~3% under its box
				if (bA && KitMesh(TEXT("scene:aceiling")))
				{
					// set A's panel lies flat with its lit face up: turned over to face down
					Kit(TEXT("scene:aceiling"), FVector(X, Y, CeilZ + 7.0), FRotator(0.f, 0.f, CeilFront > 0.f ? 180.f : 0.f), FVector(TileX + 1.0, TileY + 1.0, 14.0));
					// each panel's diffuser glows (the model's own light is only painted on)
					if ((ix + iy) % 2 == 0)
					{
						LightPanel(FVector(X, Y, CeilZ - 1.5), FVector(TileX * 0.55, TileY * 0.55, 2.0), FLinearColor(2.4f, 2.2f, 1.85f));
					}
				}
				else
				{
					// the ceiling: the panel stands up in its file, its fitted side toward its +y; laid flat facing down
					Kit(TEXT("scene:ceilingpanel"), FVector(X, Y, CeilZ + 7.0), FRotator(0.f, 0.f, -90.f * CeilFront), FVector(TileX + 1.0, 14.0, TileY + 1.0));
				}
			}
		}
	}
	else
	{
		// a walkway of lighter plates down the middle
		Block(FVector((C.X - H.X + Hatch.X - HW) * 0.5, C.Y, FloorTop + 0.5), FRotator::ZeroRotator, FVector(Hatch.X - HW - (C.X - H.X) - 120.0, 220.0, 1.0), FLinearColor(0.20f, 0.21f, 0.22f));
	}

	// walls with viewports. Along: the axis the wall runs on; Fixed: where it stands on the other axis.
	const double SillZ = FloorTop + 95.0, LintelZ = FloorTop + 245.0;
	auto WallPiece = [&](bool bAlongX, double Fixed, double A0, double A1, double Z0, double Z1, const FLinearColor& Col, double Thick) -> AActor*
	{
		if (A1 - A0 < 1.0 || Z1 - Z0 < 1.0)
		{
			return nullptr;
		}
		const FVector At = bAlongX ? FVector((A0 + A1) * 0.5, Fixed, (Z0 + Z1) * 0.5) : FVector(Fixed, (A0 + A1) * 0.5, (Z0 + Z1) * 0.5);
		const FVector Size = bAlongX ? FVector(A1 - A0, Thick, Z1 - Z0) : FVector(Thick, A1 - A0, Z1 - Z0);
		return Block(At, FRotator::ZeroRotator, Size, Col);
	};
	auto Wall = [&](bool bAlongX, double Fixed, double InSide, double A0, double A1, const TArray<FVector2D>& Ports)
	{
		if (bKit)
		{
			// one hidden box holds the room in (the windows are glass), panels about 2.4 m wide cover it
			Shell.Add(WallPiece(bAlongX, Fixed, A0, A1, FloorTop, CeilZ, PanelC, T));
			const bool bDoor = !bAlongX && InSide > 0.0; // the far end: a door to the rest of the station (shut)
			int32 N = FMath::Max(1, FMath::RoundToInt((A1 - A0) / 240.0));
			N += bDoor && N % 2 == 0 ? 1 : 0; // the door in the middle panel
			const double W = (A1 - A0) / N;
			const float Yaw = bAlongX ? (InSide * KitFront > 0.0 ? 0.f : 180.f) : (float)(-90.0 * InSide * KitFront);
			for (int32 i = 0; i < N; ++i)
			{
				const double Mid = A0 + (i + 0.5) * W;
				bool bPort = false;
				for (const FVector2D& Port : Ports)
				{
					bPort |= FMath::Abs(Port.X - Mid) < W * 0.5;
				}
				const FVector At = bAlongX ? FVector(Mid, Fixed, C.Z) : FVector(Fixed, Mid, C.Z);
				const TCHAR* DoorId = Pick(TEXT("scene:adoor"), TEXT("scene:bulkhead"));
				const TCHAR* WinId = Pick(TEXT("scene:awindow"), TEXT("scene:wallwindow"));
				// set A's window model is only the round porthole (no panel round it): a whole wall panel goes there, the
				// porthole at its real size on its face, so nothing is open to the sea round the ring
				const bool bRingWin = bA && bPort && !(bDoor && i == N / 2) && KitMesh(TEXT("scene:awindow"));
				const TCHAR* Id = bDoor && i == N / 2 && KitMesh(DoorId) ? DoorId : bPort && KitMesh(WinId) && !bRingWin ? WinId : Pick(TEXT("scene:awall"), TEXT("scene:wallpanel"));
				Kit(Id, At, FRotator(0.f, Yaw, 0.f), FVector(W + 1.0, T + 6.0, 2.0 * H.Z + 2.0));
				if (bRingWin)
				{
					float PortD = 120.f;
					FParse::Value(FCommandLine::Get(), TEXT("-MCPortD="), PortD);
					const double Out = InSide * ((T + 6.0) * 0.5 + 6.0);
					const FVector RingAt = (bAlongX ? FVector(Mid, Fixed + Out, 0.0) : FVector(Fixed + Out, Mid, 0.0)) + FVector(0.0, 0.0, FloorTop + 160.0);
					Kit(TEXT("scene:awindow"), RingAt, FRotator(0.f, Yaw, 0.f), FVector(PortD, 12.0, PortD));
				}
			}
			const FVector Strip = bAlongX ? FVector((A0 + A1) * 0.5, Fixed + InSide * (T * 0.5 + 8.0), CeilZ - 14.0) : FVector(Fixed + InSide * (T * 0.5 + 8.0), (A0 + A1) * 0.5, CeilZ - 14.0);
			LightPanel(Strip, bAlongX ? FVector(A1 - A0 - 40.0, 5.0, 4.0) : FVector(5.0, A1 - A0 - 40.0, 4.0), StripC);
			return;
		}
		double From = A0;
		for (const FVector2D& Port : Ports)
		{
			const double P0 = Port.X - Port.Y, P1 = Port.X + Port.Y;
			WallPiece(bAlongX, Fixed, From, P0, FloorTop, CeilZ, PanelC, T);
			WallPiece(bAlongX, Fixed, P0, P1, FloorTop, SillZ, PanelC, T);
			WallPiece(bAlongX, Fixed, P0, P1, LintelZ, CeilZ, PanelC, T);
			// the glass: holds the sea out, invisible
			if (AActor* Glass = WallPiece(bAlongX, Fixed, P0, P1, SillZ, LintelZ, PanelC, T))
			{
				Glass->SetActorHiddenInGame(true);
			}
			// a dark frame round it, standing a little into the room
			const double In = Fixed + InSide * (T * 0.5 + 4.0);
			WallPiece(bAlongX, In, P0 - 14.0, P1 + 14.0, SillZ - 14.0, SillZ, FrameC, 10.0);
			WallPiece(bAlongX, In, P0 - 14.0, P1 + 14.0, LintelZ, LintelZ + 14.0, FrameC, 10.0);
			WallPiece(bAlongX, In, P0 - 14.0, P0, SillZ, LintelZ, FrameC, 10.0);
			WallPiece(bAlongX, In, P1, P1 + 14.0, SillZ, LintelZ, FrameC, 10.0);
			From = P1;
		}
		WallPiece(bAlongX, Fixed, From, A1, FloorTop, CeilZ, PanelC, T);
		// kick plate at the foot, a cove light strip under the ceiling
		const double In = Fixed + InSide * (T * 0.5 + 3.0);
		WallPiece(bAlongX, In, A0, A1, FloorTop, FloorTop + 22.0, FrameC, 6.0);
		const FVector Strip = bAlongX ? FVector((A0 + A1) * 0.5, Fixed + InSide * (T * 0.5 + 6.0), CeilZ - 14.0) : FVector(Fixed + InSide * (T * 0.5 + 6.0), (A0 + A1) * 0.5, CeilZ - 14.0);
		LightPanel(Strip, bAlongX ? FVector(A1 - A0 - 40.0, 6.0, 5.0) : FVector(6.0, A1 - A0 - 40.0, 5.0), StripC);
	};
	const double Bay = 2.0 * H.X / 6.0; // six bays between the frame ribs
	const TArray<FVector2D> SidePorts = { FVector2D(C.X + Bay * 0.5, 130.0), FVector2D(C.X + Bay * 1.5, 130.0) };
	Wall(true, C.Y + H.Y + T * 0.5, -1.0, C.X - H.X - T, C.X + H.X + T, SidePorts);
	Wall(true, C.Y - H.Y - T * 0.5, 1.0, C.X - H.X - T, C.X + H.X + T, SidePorts);
	Wall(false, C.X + H.X + T * 0.5, -1.0, C.Y - H.Y, C.Y + H.Y, { FVector2D(C.Y, 300.0) });
	Wall(false, C.X - H.X - T * 0.5, 1.0, C.Y - H.Y, C.Y + H.Y, {});

	// the frame: ribs up the long walls and beams across the ceiling, a light panel and a lamp in every bay
	for (int32 i = 1; i < 6; ++i)
	{
		const double X = C.X - H.X + Bay * i;
		if (bKit && bA)
		{
			continue; // set A's panels carry their own frames
		}
		if (bKit)
		{
			// pipe runs up the long walls between the panels
			for (int32 s = -1; s <= 1; s += 2)
			{
				Kit(TEXT("scene:pipes"), FVector(X, C.Y + s * (H.Y - 14.0), C.Z), FRotator(0.f, s > 0 ? 180.f * KitFront : 0.f, 0.f), FVector(46.0, 22.0, 2.0 * H.Z));
			}
			continue;
		}
		for (int32 s = -1; s <= 1; s += 2)
		{
			Block(FVector(X, C.Y + s * (H.Y - 10.0), C.Z), FRotator::ZeroRotator, FVector(26.0, 20.0, 2.0 * H.Z), FrameC);
		}
		Block(FVector(X, C.Y, CeilZ - 12.0), FRotator::ZeroRotator, FVector(26.0, 2.0 * H.Y, 24.0), FrameC);
	}
	const int32 NLights = FMath::Clamp(FMath::RoundToInt(2.0 * H.X / 450.0), 2, 6); // one room light every ~4.5 m
	for (int32 i = 0; i < NLights; ++i)
	{
		const double X = C.X - H.X + 2.0 * H.X / NLights * (i + 0.5);
		if (!bA)
		{
			LightPanel(FVector(X, C.Y - 160.0, CeilZ - 3.0), FVector(150.0, 50.0, 4.0), CeilLightC);
			LightPanel(FVector(X, C.Y + 160.0, CeilZ - 3.0), FVector(150.0, 50.0, 4.0), CeilLightC);
		}
		if (UPointLightComponent* Room = Glow(FVector(X, C.Y, CeilZ - 50.0), bA ? FLinearColor(1.f, 0.93f, 0.82f) : FLinearColor(0.86f, 0.93f, 1.f), bA ? 8.5f : 7.f, 1150.f))
		{
			// a soft room light that reaches the walls (the engine's true falloff leaves the corners black)
			Room->bUseInverseSquaredFalloff = false;
			Room->SetLightFalloffExponent(2.f);
			Room->SetVolumetricScatteringIntensity(0.f);
			Room->MarkRenderStateDirty();
		}
	}

	// the moon pool: an open square in the floor straight onto the sea below (the user, 2026-11-16: no trunk, no lid of
	// water, no rim model in the way), a thin steel lip flush with the deck and a rail on three sides (-x is the way in)
	{
		const FLinearColor Lip(0.58f, 0.60f, 0.62f), Rail(0.80f, 0.58f, 0.06f);
		for (int32 i = 0; i < 4; ++i)
		{
			const bool bYSide = i < 2;
			const double Sg = (i % 2) ? 1.0 : -1.0;
			const FVector Off = bYSide ? FVector(0.0, Sg * (HoleY + 7.0), 0.0) : FVector(Sg * (HoleX + 7.0), 0.0, 0.0);
			const FVector Size = bYSide ? FVector(2.0 * HoleX + 28.0, 14.0, 3.0) : FVector(14.0, 2.0 * HoleY + 28.0, 3.0);
			if (AActor* L = Block(FVector(Hatch.X + Off.X, Hatch.Y + Off.Y, FloorTop + 1.5), FRotator::ZeroRotator, Size, Lip))
			{
				L->SetActorEnableCollision(false);
			}
		}
		const double RX = HoleX + 30.0, RY = HoleY + 30.0;
		const FVector2D Posts[6] = { {RX, -RY}, {RX, 0.0}, {RX, RY}, {-RX, -RY}, {-RX, RY}, {0.0, -RY} };
		for (const FVector2D& Pt : Posts)
		{
			Block(FVector(Hatch.X + Pt.X, Hatch.Y + Pt.Y, FloorTop + 52.0), FRotator::ZeroRotator, FVector(7.0, 7.0, 104.0), Rail);
		}
		Block(FVector(Hatch.X, Hatch.Y + RY, FloorTop + 52.0), FRotator::ZeroRotator, FVector(7.0, 7.0, 104.0), Rail);
		Block(FVector(Hatch.X + RX, Hatch.Y, FloorTop + 104.0), FRotator::ZeroRotator, FVector(6.0, 2.0 * RY + 6.0, 6.0), Rail);
		Block(FVector(Hatch.X, Hatch.Y - RY, FloorTop + 104.0), FRotator::ZeroRotator, FVector(2.0 * RX + 6.0, 6.0, 6.0), Rail);
		Block(FVector(Hatch.X, Hatch.Y + RY, FloorTop + 104.0), FRotator::ZeroRotator, FVector(2.0 * RX + 6.0, 6.0, 6.0), Rail);
		Block(FVector(Hatch.X + RX, Hatch.Y, FloorTop + 55.0), FRotator::ZeroRotator, FVector(5.0, 2.0 * RY + 6.0, 5.0), Rail);
		Block(FVector(Hatch.X, Hatch.Y - RY, FloorTop + 55.0), FRotator::ZeroRotator, FVector(2.0 * RX + 6.0, 5.0, 5.0), Rail);
		Block(FVector(Hatch.X, Hatch.Y + RY, FloorTop + 55.0), FRotator::ZeroRotator, FVector(2.0 * RX + 6.0, 5.0, 5.0), Rail);
	}
	Glow(Hatch - FVector(0.0, 0.0, 160.0), FLinearColor(0.35f, 0.8f, 1.f), 5000.f, 1400.f);
	// set A outside: a habitat module (scene:ahull, a long rounded hull with portholes) wraps the room (scene:ahull, a long rounded hull with portholes),
	// standing on steel legs (scene:aleg). Only seen from the sea; the room's walls hide them from inside
	const bool bHull = bA && KitMesh(TEXT("scene:ahull")) && !FParse::Param(FCommandLine::Get(), TEXT("MCNoHull"));
	// the hull is the room's own size plus its skin (the user, 2026-11-16: outside and inside must match)
	const double HullBot = FloorTop - 70.0, HullTop = CeilZ + T + 70.0, HullW = H.Y + T + 60.0, HullHalfX = H.X + T + 60.0;
	if (bHull)
	{
		// one module round the whole room (two side by side would each put an inner wall down the middle of the room)
		Kit(TEXT("scene:ahull"), FVector(C.X, C.Y, (HullBot + HullTop) * 0.5), FRotator::ZeroRotator, FVector(2.0 * HullHalfX, 2.0 * HullW, HullTop - HullBot));
	}
	// the way in, seen from the sea: a short trunk under the hull round the moon pool, a lit yellow rim at its mouth and
	// warm light inside, so a diver outside can see where to go up (no collision: the swim-up stays as it is)
	{
		const double TBot = HullBot - 120.0, TTop = HullBot + 10.0, TW = 14.0;
		const FLinearColor TrunkC(0.42f, 0.43f, 0.44f), RimC(0.9f, 0.62f, 0.08f), RimE(4.0f, 2.6f, 0.4f);
		for (int32 i = 0; i < 4; ++i)
		{
			const bool bYSide = i < 2;
			const double Sg = (i % 2) ? 1.0 : -1.0;
			const FVector Off = bYSide ? FVector(0.0, Sg * (HoleY + TW * 0.5), 0.0) : FVector(Sg * (HoleX + TW * 0.5), 0.0, 0.0);
			const FVector Side = bYSide ? FVector(2.0 * HoleX + 2.0 * TW, TW, TTop - TBot) : FVector(TW, 2.0 * HoleY, TTop - TBot);
			if (AActor* S = Block(FVector(Hatch.X + Off.X, Hatch.Y + Off.Y, (TBot + TTop) * 0.5), FRotator::ZeroRotator, Side, TrunkC))
			{
				S->SetActorEnableCollision(false);
			}
			const FVector RimSize = bYSide ? FVector(2.0 * HoleX + 2.0 * TW + 8.0, TW + 8.0, 16.0) : FVector(TW + 8.0, 2.0 * HoleY + 8.0, 16.0);
			LightPanel(FVector(Hatch.X + Off.X, Hatch.Y + Off.Y, TBot + 8.0), RimSize, RimE);
		}
		Glow(FVector(Hatch.X, Hatch.Y, HullBot - 40.0), FLinearColor(1.f, 0.85f, 0.6f), 1200.f, 600.f);
	}
	// the legs down to the sand
	for (int32 i = 0; i < 6; ++i)
	{
		const FVector Leg(C.X + (i % 3 - 1) * ((bHull ? HullHalfX - 120.0 : H.X) - 80.0), C.Y + (i / 3 ? 1.0 : -1.0) * (H.Y - 80.0), 0.0);
		const double Top = bHull ? HullBot + 20.0 : FloorZ - T * 0.5, Bot = Terrain(Leg.X, Leg.Y) - 40.0;
		AActor* LegBox = Block(FVector(Leg.X, Leg.Y, (Top + Bot) * 0.5), FRotator::ZeroRotator, FVector(60.0, 60.0, Top - Bot), FrameC);
		if (bHull && KitMesh(TEXT("scene:aleg")) && Kit(TEXT("scene:aleg"), FVector(Leg.X, Leg.Y, (Top + Bot) * 0.5), FRotator::ZeroRotator, FVector(110.0, 110.0, Top - Bot)))
		{
			Shell.Add(LegBox);
		}
	}
	// running lights on the hull so home can be found from out in the dark
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector At = bHull ? FVector(C.X + (i % 2 ? 1.0 : -1.0) * (HullHalfX + 10.0), C.Y + (i / 2 ? 1.0 : -1.0) * (HullW * 0.6), HullTop - 60.0)
			: FVector(C.X + (i % 2 ? 1.0 : -1.0) * (H.X + T + 8.0), C.Y + (i / 2 ? 1.0 : -1.0) * (H.Y * 0.6), CeilZ - 20.0);
		LightPanel(At, FVector(14.0, 14.0, 14.0), FLinearColor(6.f, 3.5f, 0.6f));
	}

	// the controls: desk, locker, bunk, board, news along the walls of the dry end (-x), each fitted to its real size
	// (length, height). W0 is the -x wall: the door, the desk and the board are on it
	const double W0 = C.X - H.X;
	BaseSpots.Reset();
	float WorkYaw = 90.f; // the edit desk's screens toward the room
	FParse::Value(FCommandLine::Get(), TEXT("-MCWorkYaw="), WorkYaw);
	float NewsYaw = 180.f; // the page's printed face toward the room (-y)
	FParse::Value(FCommandLine::Get(), TEXT("-MCNewsYaw="), NewsYaw);
	struct FSpotDef { const TCHAR* Id; FVector At; float Yaw; double Len; double High; FVector Box; FLinearColor Color; };
	const FSpotDef Defs[] = {
		{ bA && KitMesh(TEXT("scene:adesk")) ? TEXT("scene:adesk") : bKit && KitMesh(TEXT("scene:workstation")) ? TEXT("scene:workstation") : TEXT("scene:editdesk"), FVector(C.X - H.X + 95.0, C.Y - 260.0, FloorTop), WorkYaw, 200.0, 140.0, FVector(90.0, 180.0, 78.0), FLinearColor(0.16f, 0.15f, 0.13f) },
		{ Pick(TEXT("scene:alocker"), TEXT("scene:locker")), FVector(W0 + 265.0, C.Y - H.Y + 55.0, FloorTop), 0.f, bA ? 170.0 : 120.0, bA ? 205.0 : 195.0, FVector(120.0, 60.0, 190.0), FLinearColor(0.20f, 0.24f, 0.22f) },
		{ Pick(TEXT("scene:abunk"), TEXT("scene:bunk")), FVector(W0 + 175.0, C.Y + H.Y - 55.0, FloorTop), 180.f, 205.0, bA ? 195.0 : 175.0, FVector(205.0, 90.0, 60.0), FLinearColor(0.18f, 0.17f, 0.16f) },
		{ TEXT("scene:corkboard"), FVector(C.X - H.X + 12.0, C.Y + 220.0, FloorTop + 95.0), 90.f, 150.0, 115.0, FVector(10.0, 150.0, 110.0), FLinearColor(0.40f, 0.30f, 0.18f) },
		// the news: a newspaper page framed on the north wall, the shadow in the water on its front page
		{ TEXT("scene:news"), FVector(W0 + 440.0, C.Y + H.Y - 9.0, FloorTop + 95.0), NewsYaw, 95.0, 125.0, FVector(80.0, 4.0, 110.0), FLinearColor(0.75f, 0.73f, 0.68f) } };
	for (const FSpotDef& D : Defs)
	{
		AActor* Made = nullptr;
		double Tall = D.Box.Z;
		if (const TArray<FDCArtRef>* Set = GM->Art.Find(D.Id))
		{
			if (Set->Num() > 0 && GM->ArtMeshes.IsValidIndex((*Set)[0].Mesh))
			{
				const FVector Raw = GM->ArtMeshes[(*Set)[0].Mesh]->GetBoundingBox().GetSize();
				const double Fit = FMath::Min(D.Len / FMath::Max(1.0, FMath::Max(Raw.X, Raw.Y)), D.High / FMath::Max(1.0, Raw.Z));
				Made = Place(D.Id, 0, D.At, FRotator(0.f, D.Yaw, 0.f), (float)Fit);
				Tall = Raw.Z * Fit;
			}
		}
		if (!Made)
		{
			Block(D.At + FVector(0.0, 0.0, D.Box.Z * 0.5), FRotator::ZeroRotator, D.Box, D.Color);
		}
		BaseSpots.Add(D.At + FVector(0.0, 0.0, FMath::Clamp(Tall * 0.6, 50.0, 120.0)));
	}
	// a second locker beside the first, a screen over the desk
	if (const TArray<FDCArtRef>* Set = bA && KitMesh(TEXT("scene:alocker")) ? nullptr : GM->Art.Find(TEXT("scene:locker")))
	{
		if (Set->Num() > 0 && GM->ArtMeshes.IsValidIndex((*Set)[0].Mesh))
		{
			const FVector Raw = GM->ArtMeshes[(*Set)[0].Mesh]->GetBoundingBox().GetSize();
			Place(TEXT("scene:locker"), 0, FVector(W0 + 150.0, C.Y - H.Y + 45.0, FloorTop), FRotator::ZeroRotator, (float)FMath::Min(120.0 / FMath::Max(Raw.X, Raw.Y), 195.0 / Raw.Z));
		}
	}
	LightPanel(FVector(C.X - H.X + 8.0, C.Y - 260.0, FloorTop + 215.0), FVector(4.0, 170.0, 70.0), FLinearColor(0.25f, 0.9f, 1.2f));
	if (bKit)
	{
		if (const UStaticMesh* TanksM = bA ? KitMesh(TEXT("scene:atanks")) : nullptr)
		{
			Place(TEXT("scene:atanks"), 0, FVector(W0 + 460.0, C.Y - H.Y + 50.0, FloorTop), FRotator(0.f, 0.f, 0.f), (float)(175.0 / FMath::Max(1.0, TanksM->GetBoundingBox().GetSize().Z)));
		}
		else if (const TArray<FDCArtRef>* Set = GM->Art.Find(TEXT("scene:gearcases")))
		{
			if (Set->Num() > 0)
			{
				Place(TEXT("scene:gearcases"), 0, FVector(W0 + 460.0, C.Y - H.Y + 70.0, FloorTop), FRotator(0.f, 0.f, 0.f), 0.75f);
			}
		}
		for (AActor* A : Shell)
		{
			if (A)
			{
				A->SetActorHiddenInGame(true);
			}
		}
	}
	if (GM->Art.Contains(TEXT("scene:habitat")))
	{
		Place(TEXT("scene:habitat"), 0, FVector(C.X - 2600.0, C.Y + 900.0, Terrain(C.X - 2600.0, C.Y + 900.0)), FRotator(0.f, 30.f, 0.f), 6.f);
	}
	BaseWake = C + FVector(-H.X * 0.35, 60.0, -H.Z + DCCar::Stand);
	HatchAt = FVector(Hatch.X, Hatch.Y, FloorZ + T * 0.5 - 12.0);
	HatchHalf = HW;
	// no lid of water in the hatch: the sea is seen straight through it
	UE_LOG(LogMatchCut, Display, TEXT("3D base at (%.0f,%.0f,%.0f) half (%.0f,%.0f,%.0f), ground top %.0f, hatch (%.0f,%.0f,%.0f)"), C.X, C.Y, C.Z, H.X, H.Y, H.Z, GroundTop, Hatch.X, Hatch.Y, Hatch.Z);
}
