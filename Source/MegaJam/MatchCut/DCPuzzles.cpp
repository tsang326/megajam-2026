// Deep Cut: what has to be solved before a shot shows its tape marks.
//   gate "switch"       swim to the valve and press E
//   gate "grab"         pick up the loose thing of this shot
//   gate "socket:<id>"  a prop sits in that socket
//   gate "restore"      the removed scene of this shot is back in the reel
//   gate "crew"         the crew robot of this shot is free
// Props are loose things (a plate, a power cell). E grabs and drops them; close to the robot they come through a cut.

#include "MatchCutGameMode.h"
#include "DCAudio.h"
#include "DeepCutText.h"

void AMatchCutGameMode::UpdateGate(int32 ShotIndex, bool bQuiet)
{
	FDCShot& Shot = Shots[ShotIndex];
	if (Shot.bGateOpen || Shot.Exits.Num() == 0)
	{
		return;
	}
	bool bOpen = false;
	if (Shot.Gate.IsEmpty())
	{
		bOpen = true;
	}
	else if (Shot.Gate == TEXT("switch"))
	{
		bOpen = Shot.Switch.bOn;
	}
	else if (Shot.Gate == TEXT("grab"))
	{
		for (const FDCProp& P : Props)
		{
			bOpen |= P.HomeShot == ShotIndex && P.bEverGrabbed;
		}
	}
	else if (Shot.Gate.StartsWith(TEXT("fits:")))
	{
		// a loose thing of this shot has been moved onto the place of that socket in the next shot's frame
		const FString Id = Shot.Gate.RightChop(5);
		for (int32 s = 0; s < Sockets.Num(); ++s)
		{
			for (const FDCProp& P : Props)
			{
				bOpen |= ShotIndex == Cur && Sockets[s].Id == Id && P.Shot == ShotIndex && (Sockets[s].bFilled || PropFitsSocket(P, s));
			}
		}
	}
	else if (Shot.Gate.StartsWith(TEXT("socket:")))
	{
		const FString Id = Shot.Gate.RightChop(7);
		for (const FDCSocket& S : Sockets)
		{
			bOpen |= S.Id == Id && S.bFilled;
		}
	}
	else if (Shot.Gate == TEXT("restore"))
	{
		bOpen = bGapFilled[Shot.bHasPart ? Shot.PartGap : 0];
	}
	else if (Shot.Gate == TEXT("crew"))
	{
		bOpen = bCrewRescued;
	}
	if (bOpen)
	{
		Shot.bGateOpen = true;
		if (!bQuiet)
		{
			RevealMarks(ShotIndex);
		}
	}
}

void AMatchCutGameMode::RevealMarks(int32 ShotIndex)
{
	RevealTime = 1.2f;
	int32 Marks = 0;
	for (const FDCExit& E : Shots[ShotIndex].Exits)
	{
		Marks += E.Marks.Num();
	}
	UE_LOG(LogMatchCut, Display, TEXT("MARKS revealed in shot %d \"%s\" (gate \"%s\"): %d exits, %d marks"), ShotIndex, *Shots[ShotIndex].Name, *Shots[ShotIndex].Gate, Shots[ShotIndex].Exits.Num(), Marks);
	if (Audio)
	{
		Audio->Play(TEXT("cam_rec"));
	}
}

void AMatchCutGameMode::FillSocket(int32 SocketIndex, int32 PropIndex, bool bQuiet)
{
	FDCSocket& Socket = Sockets[SocketIndex];
	FDCProp& Prop = Props[PropIndex];
	Socket.bFilled = true;
	Prop.Shot = Socket.Shot;
	Prop.S = Socket.S + (Prop.Kind == TEXT("plate") ? FVector(0.0, 12.0, 0.0) : FVector::ZeroVector);
	Prop.Socket = SocketIndex;
	Prop.bLocked = true;
	Prop.bCarried = false;
	if (Socket.Accepts == TEXT("cell") || Socket.Accepts == TEXT("crew"))
	{
		SetPaintEmissive(Socket.Paint, FLinearColor(0.4f, 5.f, 1.2f));
	}
	else
	{
		SetPaintEmissive(Socket.Paint, FLinearColor::Black);
	}
	if (bQuiet)
	{
		return;
	}
	UE_LOG(LogMatchCut, Display, TEXT("SOCKET %s in shot %d filled by %s"), *Socket.Id, Socket.Shot, *Prop.Id);
	if (Audio)
	{
		if (Socket.Accepts == TEXT("cell"))
		{
			Audio->Play(TEXT("part_pickup"));
		}
		else
		{
			Audio->Play(TEXT("plate_lift"), 0.9f, 0.7f);
		}
	}
	if (Socket.Accepts == TEXT("crew"))
	{
		ShowBanner(DCText::CrewSafe, FString::Printf(DCTEXT_CREW, CrewCount(), CrewTotal), 4.f);
		UE_LOG(LogMatchCut, Display, TEXT("RESCUE crew robot %s is in the lift in shot %d: CREW %d/%d"), *Prop.Id, Socket.Shot, CrewCount(), CrewTotal);
		if (Socket.bEnds)
		{
			EndTimer = 3.0f;
		}
	}
	else if (Socket.bEnds)
	{
		ShowBanner(DCText::PowerOn, TEXT(""), 3.f);
		EndTimer = 3.0f;
	}
	else
	{
		ShowBanner(DCText::CurrentStopped, TEXT(""), 3.f);
	}
}

void AMatchCutGameMode::CheckSockets(int32 ShotIndex)
{
	// a prop let go close to a socket drops into it
	for (int32 s = 0; s < Sockets.Num(); ++s)
	{
		if (Sockets[s].Shot != ShotIndex || Sockets[s].bFilled)
		{
			continue;
		}
		for (int32 p = 0; p < Props.Num(); ++p)
		{
			const FDCProp& Prop = Props[p];
			if (Prop.Shot == ShotIndex && ShotIndex == Cur && !Prop.bCarried && !Prop.bLocked && PropFitsSocket(Prop, s))
			{
				FillSocket(s, p, false);
				break;
			}
		}
	}
}

bool AMatchCutGameMode::PropFitsSocket(const FDCProp& Prop, int32 SocketIndex) const
{
	const FDCSocket& Socket = Sockets[SocketIndex];
	if (Socket.bFilled || Socket.Accepts != Prop.Kind || Prop.bLocked || Prop.Shot != Cur)
	{
		return false;
	}
	// the frame rule: same place on screen (the socket is seen from its own shot's entry camera), about the same depth
	const FVector2D N = ViewNdc(Cur, Prop.S, false);
	const FVector2D SN = ViewNdc(Socket.Shot, Socket.S, Socket.Shot != Cur);
	return FVector2D((N.X - SN.X) * 960.0, (N.Y - SN.Y) * 540.0).Size() <= SocketRadius * 1080.0 && FMath::Abs(Prop.S.Z - Socket.S.Z) <= 300.0;
}

void AMatchCutGameMode::NudgeProps()
{
	// The gripper is a cut feature until it is built again: before that, loose things are pushed by contact.
	// The thing is a box; it moves along the face the robot touches, so a push is straight and does not slip off.
	if (State != EDCState::Playing)
	{
		return;
	}
	const FDCShot& Shot = Shots[Cur];
	for (FDCProp& P : Props)
	{
		if (P.Shot != Cur || P.bLocked || P.bCarried || P.bHeavy)
		{
			continue;
		}
		const bool bFlat = P.Kind == TEXT("plate");
		const FVector H = bFlat ? FVector(65.0, 12.0, 75.0) : FVector(34.0, 46.0, 34.0);
		const FVector D = P.S - HeroS;
		const FVector O(H.X + HeroRadius - FMath::Abs(D.X), H.Y + HeroRadius - FMath::Abs(D.Y), H.Z + HeroRadius - FMath::Abs(D.Z));
		if (O.X <= 0.0 || O.Y <= 0.0 || O.Z <= 0.0)
		{
			continue;
		}
		const FVector Before = P.S;
		if (!bFlat && O.Y < O.X && O.Y < O.Z)
		{
			P.S.Y += (D.Y >= 0.0 ? O.Y : -O.Y);
		}
		else if (O.X <= O.Z)
		{
			P.S.X += (D.X >= 0.0 ? O.X : -O.X);
		}
		else
		{
			P.S.Z += (D.Z >= 0.0 ? O.Z : -O.Z);
		}
		PushOut(Cur, P.S, bFlat ? 14.0 : 32.0, nullptr);
		P.S.X = FMath::Clamp(P.S.X, Shot.VolMin.X, Shot.VolMax.X);
		P.S.Y = FMath::Clamp(P.S.Y, Shot.VolMin.Y - 60.0, Shot.VolMax.Y);
		P.S.Z = FMath::Clamp(P.S.Z, Shot.VolMin.Z, Shot.VolMax.Z);
		if (bFlat)
		{
			P.S.Y = Before.Y; // a plate stays on the floor it lies on
		}
		if (!P.bEverGrabbed && FVector::Dist(Before, P.S) > 0.5)
		{
			P.bEverGrabbed = true;
			UE_LOG(LogMatchCut, Display, TEXT("PROP %s nudged in shot %d"), *P.Id, Cur);
			if (Audio)
			{
				Audio->Play(TEXT("plate_lift"), 0.5f, 1.1f);
			}
		}
	}
	CheckSockets(Cur);
}

int32 AMatchCutGameMode::PropInReach() const
{
	if (!bFeature[1])
	{
		return INDEX_NONE; // no gripper yet
	}
	int32 Best = INDEX_NONE;
	double BestD = ReachRadius;
	for (int32 i = 0; i < Props.Num(); ++i)
	{
		const double D = FVector::Dist(Props[i].S, HeroS);
		if (Props[i].Shot == Cur && !Props[i].bLocked && D < BestD)
		{
			BestD = D;
			Best = i;
		}
	}
	return Best;
}

bool AMatchCutGameMode::SwitchInReach() const
{
	const FDCShot& Shot = Shots[Cur];
	return Shot.bHasSwitch && !Shot.Switch.bOn && FVector::Dist(Shot.Switch.S, HeroS) < ReachRadius + 20.0;
}

FString AMatchCutGameMode::UsePrompt(FVector& OutS) const
{
	for (const FDCProp& P : Props)
	{
		if (P.Shot == Cur && P.bCarried)
		{
			OutS = P.S;
			return DCText::UseDrop;
		}
	}
	if (SwitchInReach())
	{
		OutS = Shots[Cur].Switch.S;
		return DCText::UseTurn;
	}
	const int32 P = PropInReach();
	if (P != INDEX_NONE)
	{
		OutS = Props[P].S;
		return DCText::UseGrab;
	}
	return FString();
}

void AMatchCutGameMode::PressUse()
{
	if (State != EDCState::Playing)
	{
		return;
	}
	for (FDCProp& P : Props)
	{
		if (P.Shot == Cur && P.bCarried)
		{
			P.bCarried = false;
			UE_LOG(LogMatchCut, Display, TEXT("PROP %s dropped in shot %d at (%.0f,%.0f,%.0f)"), *P.Id, Cur, P.S.X, P.S.Y, P.S.Z);
			if (Audio)
			{
				Audio->Play(TEXT("plate_lift"), 0.6f, 0.85f);
			}
			CheckSockets(Cur);
			return;
		}
	}
	FDCShot& Shot = Shots[Cur];
	if (SwitchInReach())
	{
		Shot.Switch.bOn = true;
		UE_LOG(LogMatchCut, Display, TEXT("SWITCH turned in shot %d \"%s\""), Cur, *Shot.Name);
		if (Audio)
		{
			Audio->Play(TEXT("plate_lift"), 0.7f, 1.3f);
		}
		UpdateGate(Cur, false);
		return;
	}
	const int32 P = PropInReach();
	if (P != INDEX_NONE)
	{
		Props[P].bCarried = true;
		if (Props[P].Kind == TEXT("crew") && !Props[P].bEverGrabbed)
		{
			ShowBanner(DCText::CrewFound, DCText::CrewCarry, 5.f);
		}
		Props[P].bEverGrabbed = true;
		UE_LOG(LogMatchCut, Display, TEXT("PROP %s grabbed in shot %d"), *Props[P].Id, Cur);
		if (Audio)
		{
			Audio->Play(TEXT("plate_lift"));
		}
		UpdateGate(Cur, false);
	}
}

void AMatchCutGameMode::UpdateProps(float Dt)
{
	for (FDCProp& P : Props)
	{
		if (P.bCarried && P.Shot == Cur)
		{
			// held just under the robot
			const FVector Want = HeroS + FVector(0.0, -70.0, 0.0);
			P.S = FMath::VInterpTo(P.S, Want, Dt, 9.f);
		}
		if (const FDCRobot* Robot = PropRobots.Find((int32)(&P - Props.GetData())))
		{
			// a crew robot that is carried: lying on its side until it is picked up
			const FDCShot& In = Shots[P.Shot];
			PlaceRobot(*Robot, ShotToWorld(P.Shot, P.S), FVector2D(1.0, (P.bCarried || P.bLocked) ? 0.0 : -0.3), In.CamRot, (P.bCarried || P.bLocked) ? 0.f : 35.f);
			continue;
		}
		if (AActor* A = P.Actor.Get())
		{
			const FDCShot& Shot = Shots[P.Shot];
			FRotator Rot = Shot.CamRot;
			FVector Bob = FVector::ZeroVector;
			if (P.Kind == TEXT("cell"))
			{
				Rot = (Shot.CamRot.Quaternion() * FRotator(0.0, PlayClock * 40.0, 12.0).Quaternion()).Rotator();
			}
			if (!P.bLocked && !P.bCarried)
			{
				Bob = Shot.AxU * (5.0 * FMath::Sin(PlayClock * 1.7 + P.HomeS.X));
			}
			A->SetActorLocationAndRotation(ShotToWorld(P.Shot, P.S) + Bob, Rot);
		}
	}
}

void AMatchCutGameMode::UpdateCurrents(float Dt)
{
	for (int32 i = 0; i < Shots.Num(); ++i)
	{
		FDCShot& Shot = Shots[i];
		for (FDCCurrent& C : Shot.Currents)
		{
			bool bOn = true;
			for (const FDCSocket& S : Sockets)
			{
				if (S.Id == C.OffBy && S.bFilled)
				{
					bOn = false;
				}
			}
			C.bOn = bOn;
			const FVector Dir = C.Force.GetSafeNormal();
			// how far a streak travels: the box's size along the flow
			const double Len = 2.0 * (FMath::Abs(Dir.X) * C.Half.X + FMath::Abs(Dir.Y) * C.Half.Y + FMath::Abs(Dir.Z) * C.Half.Z);
			for (int32 k = 0; k < C.Streaks.Num(); ++k)
			{
				AActor* A = C.Streaks[k].Get();
				if (!A)
				{
					continue;
				}
				A->SetActorHiddenInGame(!bOn);
				if (!bOn || (i != Cur && !Shots[Cur].Exits.ContainsByPredicate([i](const FDCExit& E) { return E.To == i; })))
				{
					continue;
				}
				FRandomStream Rand(k * 131 + 7);
				const double Phase = FMath::Frac(PlayClock * Rand.FRandRange(0.7f, 1.3f) + Rand.FRand());
				const FVector Side(Rand.FRandRange(-1.f, 1.f) * C.Half.X, Rand.FRandRange(-1.f, 1.f) * C.Half.Y, Rand.FRandRange(-1.f, 1.f) * C.Half.Z);
				// keep the sideways scatter, replace the part along the flow by the phase
				const FVector S = C.C + Side - Dir * FVector::DotProduct(Side, Dir) + Dir * ((Phase - 0.5) * Len);
				A->SetActorLocation(ShotToWorld(i, S));
			}
		}
	}
}

void AMatchCutGameMode::TickPuzzles(float Dt)
{
	FDCShot& Shot = Shots[Cur];
	RevealTime = FMath::Max(0.f, RevealTime - Dt);

	// the valve turns, its hatch slides up out of the way
	if (Shot.bHasSwitch && Shot.Switch.bOn)
	{
		Shot.Switch.Turn = FMath::Min(Shot.Switch.Turn + Dt, 1.2f);
		if (AActor* A = Shot.Switch.Actor.Get())
		{
			A->SetActorRotation((Shot.CamRot.Quaternion() * FRotator(0.0, 0.0, Shot.Switch.Turn * 300.0).Quaternion()).Rotator());
		}
		for (FDCSolid& Solid : Shot.Solids)
		{
			if (Solid.Hatch.IsEmpty() || Solid.Open >= 1.f)
			{
				continue;
			}
			Solid.Open = FMath::Min(Solid.Open + Dt / 1.5f, 1.f);
			const FVector Loc = Solid.BaseLoc + Shot.AxU * ((Solid.Size.Y + 30.0) * Solid.Open);
			Solid.World.SetLocation(Loc);
			if (AActor* A = Solid.Actor.Get())
			{
				A->SetActorLocation(Loc);
			}
		}
	}

	// the part: a feature that was cut from Unit 0. Touching it builds the feature again.
	if (Shot.bHasPart && !bFeature[Shot.PartFeature] && Shot.PartActor.IsValid())
	{
		PartPulse += Dt;
		Shot.PartActor->SetActorLocationAndRotation(ShotToWorld(Cur, Shot.PartS) + FVector(0, 0, 8.0 * FMath::Sin(PartPulse * 2.5f)), FRotator(0.0, PartPulse * 60.0, 0.0));
		if (FVector::Dist(Shot.PartS, HeroS) < HeroRadius + 50.f)
		{
			bFeature[Shot.PartFeature] = true;
			Shot.PartActor->SetActorHiddenInGame(true);
			ShowBanner(FString::Printf(DCTEXT_PART_FOUND, DCText::Features[Shot.PartFeature]), Shot.PartFeature == 1 ? DCText::PartHowGrip : DCText::PartHow, 5.f);
			UE_LOG(LogMatchCut, Display, TEXT("PART picked up in shot %d: %s built, restore-by-cut armed (clip \"%s\", frame (%.3f,%.3f))"), Cur, DCText::Features[Shot.PartFeature], *Shot.PartClip, Shot.RestoreNdc.X, Shot.RestoreNdc.Y);
			if (Audio)
			{
				Audio->Play(TEXT("part_pickup"));
			}
			PointGhost();
		}
	}

	// the rescue
	if (Cur == CrewShot && !bCrewRescued && FVector::Dist(CrewPos, HeroPos) < HeroRadius * 2.f + 45.f)
	{
		bCrewRescued = true;
		ShowBanner(DCText::CrewFound, FString::Printf(DCTEXT_CREW, 1, CrewTotal), 4.f);
		UE_LOG(LogMatchCut, Display, TEXT("RESCUE crew robot freed in shot %d (lamp=%d): CREW 1/%d"), Cur, bLampOn ? 1 : 0, CrewTotal);
		if (Audio)
		{
			Audio->Play(TEXT("reel_restore"), 0.8f, 1.2f);
		}
	}

	UpdateGate(Cur, false);
}

void AMatchCutGameMode::TakeSnap(FDCSnap& Out) const
{
	Out.Shot = Cur;
	Out.HeroS = HeroS;
	Out.Props = Props;
	Out.Sockets.Reset();
	for (const FDCSocket& S : Sockets)
	{
		Out.Sockets.Add(S.bFilled);
	}
}

void AMatchCutGameMode::RestoreSnap(const FDCSnap& Snap)
{
	// loose things and sockets go back to how they were; valves, the part, the scene and the crew stay done
	for (int32 i = 0; i < Props.Num() && i < Snap.Props.Num(); ++i)
	{
		const TWeakObjectPtr<AActor> Actor = Props[i].Actor;
		const bool bEver = Props[i].bEverGrabbed;
		Props[i] = Snap.Props[i];
		Props[i].Actor = Actor;
		Props[i].bEverGrabbed = bEver;
		Props[i].bCarried = false;
	}
	for (int32 i = 0; i < Sockets.Num() && i < Snap.Sockets.Num(); ++i)
	{
		if (Sockets[i].bFilled && !Snap.Sockets[i])
		{
			Sockets[i].bFilled = false;
			SetPaintEmissive(Sockets[i].Paint, Sockets[i].Accepts == TEXT("cell") ? FLinearColor(5.f, 0.6f, 0.3f) : FLinearColor(0.5f, 1.4f, 1.4f));
		}
	}
}
