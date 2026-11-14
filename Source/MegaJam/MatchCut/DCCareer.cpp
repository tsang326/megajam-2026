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
	static const float TankAir[3] = { 150.f, 240.f, 360.f };
	static const float HullDepth[4] = { 150.f, 250.f, 350.f, 520.f };
	static const int32 TapeClips[3] = { 6, 10, 16 };
	static const float Speed[3] = { 1.f, 1.2f, 1.4f };
	static constexpr double Stand = 118.0; // body centre over the deck when standing: the eyes at about 1.4 m
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
	bBoardOpen = false;
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
		const float Rate = (W->bBoost ? 2.f : 1.f) * (bOverDepth ? 5.f : 1.f);
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
	if (bBoardOpen)
	{
		// the board closes on any of E, ENTER, ESC, or on walking away from it
		const bool bAway = W->BaseSpots.IsValidIndex(3) && FVector::Dist(W->Pos, W->BaseSpots[3]) > 650.0;
		if (bUsePressed || bBackPressed || bAway)
		{
			bBoardOpen = false;
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
	const FLinearColor Steel(0.10f, 0.11f, 0.12f), Deck(0.05f, 0.055f, 0.06f), Paint(0.42f, 0.30f, 0.08f);
	const double T = 30.0;
	const FVector C = BaseC, H = BaseHalf;
	// the hatch in the floor, toward the -x end: the way out and the way home
	const FVector Hatch = C + FVector(H.X - 260.0, 0.0, -H.Z);
	const double HW = 150.0;
	const double FloorZ = C.Z - H.Z - T * 0.5;
	// floor: four slabs round the hatch
	Block(FVector((C.X - H.X + Hatch.X - HW) * 0.5, C.Y, FloorZ), FRotator::ZeroRotator, FVector(Hatch.X - HW - (C.X - H.X), 2.0 * H.Y, T), Deck);
	Block(FVector((Hatch.X + HW + C.X + H.X) * 0.5, C.Y, FloorZ), FRotator::ZeroRotator, FVector(C.X + H.X - Hatch.X - HW, 2.0 * H.Y, T), Deck);
	Block(FVector(Hatch.X, (C.Y - H.Y + Hatch.Y - HW) * 0.5, FloorZ), FRotator::ZeroRotator, FVector(2.0 * HW, Hatch.Y - HW - (C.Y - H.Y), T), Deck);
	Block(FVector(Hatch.X, (Hatch.Y + HW + C.Y + H.Y) * 0.5, FloorZ), FRotator::ZeroRotator, FVector(2.0 * HW, C.Y + H.Y - Hatch.Y - HW, T), Deck);
	// ceiling and walls
	Block(C + FVector(0.0, 0.0, H.Z + T * 0.5), FRotator::ZeroRotator, FVector(2.0 * H.X, 2.0 * H.Y, T), Steel);
	Block(C + FVector(H.X + T * 0.5, 0.0, 0.0), FRotator::ZeroRotator, FVector(T, 2.0 * H.Y, 2.0 * H.Z), Steel);
	Block(C - FVector(H.X + T * 0.5, 0.0, 0.0), FRotator::ZeroRotator, FVector(T, 2.0 * H.Y, 2.0 * H.Z), Steel);
	Block(C + FVector(0.0, H.Y + T * 0.5, 0.0), FRotator::ZeroRotator, FVector(2.0 * H.X, T, 2.0 * H.Z), Steel);
	Block(C - FVector(0.0, H.Y + T * 0.5, 0.0), FRotator::ZeroRotator, FVector(2.0 * H.X, T, 2.0 * H.Z), Steel);
	// a yellow rim round the hatch, the legs down to the sand
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector Off = i < 2 ? FVector(0.0, (i ? 1.0 : -1.0) * (HW + 8.0), 0.0) : FVector((i == 3 ? 1.0 : -1.0) * (HW + 8.0), 0.0, 0.0);
		Block(Hatch + Off + FVector(0.0, 0.0, 6.0), FRotator::ZeroRotator, i < 2 ? FVector(2.0 * HW + 32.0, 16.0, 10.0) : FVector(16.0, 2.0 * HW + 32.0, 10.0), Paint);
	}
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector Leg(C.X + (i % 2 ? 1.0 : -1.0) * (H.X - 60.0), C.Y + (i / 2 ? 1.0 : -1.0) * (H.Y - 60.0), 0.0);
		const double Top = FloorZ - T * 0.5, Bot = Terrain(Leg.X, Leg.Y) - 40.0;
		Block(FVector(Leg.X, Leg.Y, (Top + Bot) * 0.5), FRotator::ZeroRotator, FVector(50.0, 50.0, Top - Bot), Steel);
	}
	// warm work lights inside, an amber beacon under the hatch so home can be found
	Glow(C + FVector(-H.X * 0.5, 0.0, H.Z - 40.0), FLinearColor(1.f, 0.78f, 0.5f), 3200.f, 900.f);
	Glow(C + FVector(H.X * 0.4, 0.0, H.Z - 40.0), FLinearColor(1.f, 0.78f, 0.5f), 2600.f, 900.f);
	Glow(Hatch - FVector(0.0, 0.0, 160.0), FLinearColor(1.f, 0.55f, 0.15f), 6000.f, 1400.f);
	// the controls: desk, locker, bunk, board along the walls (models from the art drop when there, plain blocks if not)
	const double FloorTop = C.Z - H.Z;
	BaseSpots.Reset();
	struct FSpotDef { const TCHAR* Id; FVector At; float Yaw; float Scale; FVector Box; FLinearColor Color; };
	const FSpotDef Defs[] = {
		{ TEXT("scene:editdesk"), FVector(C.X - H.X + 70.0, C.Y - 180.0, FloorTop), 90.f, 1.4f, FVector(110.0, 200.0, 90.0), FLinearColor(0.16f, 0.15f, 0.13f) },
		{ TEXT("scene:locker"), FVector(C.X - 120.0, C.Y - H.Y + 50.0, FloorTop), 0.f, 1.6f, FVector(160.0, 70.0, 200.0), FLinearColor(0.20f, 0.24f, 0.22f) },
		{ TEXT("scene:bunk"), FVector(C.X - 150.0, C.Y + H.Y - 60.0, FloorTop), 180.f, 1.6f, FVector(220.0, 90.0, 60.0), FLinearColor(0.18f, 0.17f, 0.16f) },
		{ TEXT("scene:corkboard"), FVector(C.X - H.X + 12.0, C.Y + 170.0, FloorTop + 110.0), 90.f, 1.2f, FVector(10.0, 160.0, 110.0), FLinearColor(0.40f, 0.30f, 0.18f) } };
	for (const FSpotDef& D : Defs)
	{
		AActor* Made = GM->Art.Contains(D.Id) ? Place(D.Id, 0, D.At, FRotator(0.f, D.Yaw, 0.f), D.Scale) : nullptr;
		if (!Made)
		{
			Block(D.At + FVector(0.0, 0.0, D.Box.Z * 0.5), FRotator::ZeroRotator, D.Box, D.Color);
		}
		BaseSpots.Add(D.At + FVector(0.0, 0.0, FMath::Max(50.0, D.Box.Z * 0.6)));
	}
	if (GM->Art.Contains(TEXT("scene:moonpool")))
	{
		Place(TEXT("scene:moonpool"), 0, Hatch - FVector(0.0, 0.0, 10.0), FRotator::ZeroRotator, 1.2f, false);
	}
	if (GM->Art.Contains(TEXT("scene:habitat")))
	{
		Place(TEXT("scene:habitat"), 0, FVector(C.X - 2200.0, C.Y + 600.0, Terrain(C.X - 2200.0, C.Y + 600.0)), FRotator(0.f, 30.f, 0.f), 6.f);
	}
	BaseWake = C + FVector(-H.X * 0.35, 60.0, -H.Z + DCCar::Stand);
	HatchAt = FVector(Hatch.X, Hatch.Y, FloorZ + T * 0.5 - 12.0);
	HatchHalf = HW;
	// the sea's surface in the hatch: still, dark, a little light in it
	if (AActor* Water = Block(HatchAt, FRotator::ZeroRotator, FVector(2.0 * HW, 2.0 * HW, 2.0), FLinearColor(0.015f, 0.07f, 0.08f)))
	{
		Water->SetActorEnableCollision(false);
	}
	UE_LOG(LogMatchCut, Display, TEXT("3D base at (%.0f,%.0f,%.0f), hatch (%.0f,%.0f,%.0f)"), C.X, C.Y, C.Z, Hatch.X, Hatch.Y, Hatch.Z);
}
