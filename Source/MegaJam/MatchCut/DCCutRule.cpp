// Deep Cut: shot maths, collision, movement and the cut.
//
// The cut rule (brief 03): on CUT the robot keeps its place on screen and its distance to the camera. The take
// only works when the current shot's puzzle is solved (its tape marks are showing), the robot is on a mark of the
// camera it cuts to (inside the capture radius on screen, and at the mark's depth unless the mark takes any depth),
// and the landing point is not inside rock. Loose things close to the robot come through by the same rule.

#include "MatchCutGameMode.h"
#include "DCAudio.h"
#include "DeepCutText.h"
#include "Camera/CameraActor.h"

namespace DCMove
{
	const float Accel = 1500.f;
	const float AccelDepth = 1200.f;
	const float MaxSpeed = 430.f;
	const float Drag = 2.4f;
}

FVector AMatchCutGameMode::ShotToWorld(int32 Shot, const FVector& S) const
{
	const FDCShot& Sh = Shots[Shot];
	return Sh.CamPos + Sh.AxF * S.Z + Sh.AxR * S.X + Sh.AxU * S.Y;
}

FVector AMatchCutGameMode::WorldToShot(int32 Shot, const FVector& W) const
{
	const FDCShot& Sh = Shots[Shot];
	const FVector D = W - Sh.CamPos;
	return FVector(FVector::DotProduct(D, Sh.AxR), FVector::DotProduct(D, Sh.AxU), FVector::DotProduct(D, Sh.AxF));
}

FVector2D AMatchCutGameMode::ViewNdc(int32 Shot, const FVector& S, bool bEntry) const
{
	const FDCShot& Sh = Shots[Shot];
	const double T = bEntry ? Sh.Rail.Z : Sh.Truck;
	const double Z = FMath::Max(S.Z, 1.0);
	return FVector2D((S.X - T) / (Z * TanH), S.Y / (Z * TanV));
}

FVector AMatchCutGameMode::NdcToShot(int32 Shot, const FVector2D& Ndc, double Depth, bool bEntry) const
{
	const FDCShot& Sh = Shots[Shot];
	const double T = bEntry ? Sh.Rail.Z : Sh.Truck;
	return FVector(T + Ndc.X * Depth * TanH, Ndc.Y * Depth * TanV, Depth);
}

// ---------------------------------------------------------------------------------------------
// collision: a sphere against the boxes of the shot. Each box is the transform of the mesh that draws it.
// ---------------------------------------------------------------------------------------------

bool AMatchCutGameMode::Blocked(int32 Shot, const FVector& S, double Radius) const
{
	const FVector W = ShotToWorld(Shot, S);
	for (const FDCSolid& Solid : Shots[Shot].Solids)
	{
		if (!Solid.bActive)
		{
			continue;
		}
		const FVector L = Solid.World.InverseTransformPositionNoScale(W);
		const FVector C(FMath::Clamp(L.X, -Solid.Half.X, Solid.Half.X), FMath::Clamp(L.Y, -Solid.Half.Y, Solid.Half.Y), FMath::Clamp(L.Z, -Solid.Half.Z, Solid.Half.Z));
		if (FVector::DistSquared(L, C) < FMath::Square(Radius - 1.0))
		{
			return true;
		}
	}
	return false;
}

bool AMatchCutGameMode::PushOut(int32 Shot, FVector& S, double Radius, FVector* Vel) const
{
	const FDCShot& Sh = Shots[Shot];
	bool bAny = false;
	for (int32 Iter = 0; Iter < 3; ++Iter)
	{
		bool bHit = false;
		for (const FDCSolid& Solid : Sh.Solids)
		{
			if (!Solid.bActive)
			{
				continue;
			}
			FVector L = Solid.World.InverseTransformPositionNoScale(ShotToWorld(Shot, S));
			const FVector C(FMath::Clamp(L.X, -Solid.Half.X, Solid.Half.X), FMath::Clamp(L.Y, -Solid.Half.Y, Solid.Half.Y), FMath::Clamp(L.Z, -Solid.Half.Z, Solid.Half.Z));
			const FVector D = L - C;
			const double Dist = D.Size();
			if (Dist >= Radius)
			{
				continue;
			}
			FVector N;
			if (Dist > 0.01)
			{
				N = D / Dist;
				L = C + N * Radius;
			}
			else
			{
				// the centre is inside: leave through the nearest face
				const FVector Room(Solid.Half.X - FMath::Abs(L.X), Solid.Half.Y - FMath::Abs(L.Y), Solid.Half.Z - FMath::Abs(L.Z));
				if (Room.X <= Room.Y && Room.X <= Room.Z) { N = FVector(L.X >= 0 ? 1 : -1, 0, 0); L.X = N.X * (Solid.Half.X + Radius); }
				else if (Room.Y <= Room.Z) { N = FVector(0, L.Y >= 0 ? 1 : -1, 0); L.Y = N.Y * (Solid.Half.Y + Radius); }
				else { N = FVector(0, 0, L.Z >= 0 ? 1 : -1); L.Z = N.Z * (Solid.Half.Z + Radius); }
			}
			S = WorldToShot(Shot, Solid.World.TransformPositionNoScale(L));
			if (Vel)
			{
				const FVector NW = Solid.World.TransformVectorNoScale(N);
				const FVector NS(FVector::DotProduct(NW, Sh.AxR), FVector::DotProduct(NW, Sh.AxU), FVector::DotProduct(NW, Sh.AxF));
				const double Into = FVector::DotProduct(*Vel, NS);
				if (Into < 0.0)
				{
					*Vel -= NS * Into; // slide
				}
			}
			bHit = bAny = true;
		}
		if (!bHit)
		{
			break;
		}
	}
	return bAny;
}

void AMatchCutGameMode::MoveHero(const FVector& Delta)
{
	const FDCShot& Shot = Shots[Cur];
	const double R = HeroRadius;
	const int32 Steps = FMath::Clamp(FMath::CeilToInt32(Delta.Size() / (R * 0.4)), 1, 12);
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		HeroS += Delta / Steps;
		PushOut(Cur, HeroS, R, &VelS);
	}

	// the water volume of the shot, and the edge of the frame
	const double Z = FMath::Clamp(HeroS.Z, Shot.VolMin.Z, Shot.VolMax.Z);
	if (Z != HeroS.Z) { VelS.Z = 0.0; HeroS.Z = Z; }
	const double X0 = FMath::Max(Shot.VolMin.X, Shot.Truck - DC::FrameX * Z * TanH), X1 = FMath::Min(Shot.VolMax.X, Shot.Truck + DC::FrameX * Z * TanH);
	const double Y0 = FMath::Max(Shot.VolMin.Y, -DC::FrameY * Z * TanV), Y1 = FMath::Min(Shot.VolMax.Y, DC::FrameY * Z * TanV);
	const double X = FMath::Clamp(HeroS.X, X0, FMath::Max(X0, X1)), Y = FMath::Clamp(HeroS.Y, Y0, FMath::Max(Y0, Y1));
	if (X != HeroS.X) { VelS.X = 0.0; HeroS.X = X; }
	if (Y != HeroS.Y) { VelS.Y = 0.0; HeroS.Y = Y; }
}

void AMatchCutGameMode::UpdateRail(float Dt, bool bSnap)
{
	// A wide shot: the survey camera trucks sideways on its rail, slowly, and only when the robot nears the edge.
	FDCShot& Shot = Shots[Cur];
	if (Shot.Rail.Y > Shot.Rail.X)
	{
		const double Dead = 0.30 * HeroS.Z * TanH;
		const double Goal = FMath::Clamp(HeroS.X - FMath::Clamp(HeroS.X - Shot.Truck, -Dead, Dead), Shot.Rail.X, Shot.Rail.Y);
		Shot.Truck = bSnap ? Shot.Truck : FMath::FInterpConstantTo(FMath::FInterpTo(Shot.Truck, Goal, Dt, 2.5f), Goal, Dt, 40.f);
	}
	if (ACameraActor* Cam = Shot.Camera.Get())
	{
		Cam->SetActorLocation(Shot.CamPos + Shot.AxR * Shot.Truck);
	}
}

void AMatchCutGameMode::TickMovement(float Dt)
{
	FVector In = MoveInput;
	if (bScriptActive)
	{
		In = ScriptMoveLeft > 0.f ? ScriptMove : FVector::ZeroVector;
		if (bScriptSwimming || bScriptHold)
		{
			const FVector To = ScriptSwimTarget - HeroS;
			In = To.Size() < 12.0 ? FVector::ZeroVector : To.GetSafeNormal() * FMath::Clamp(To.Size() / 160.0, 0.12, 1.0);
		}
	}
	if (In.SizeSquared() > 1.0)
	{
		In.Normalize();
	}
	if (FMath::Abs(In.Z) > 0.3)
	{
		bDepthUsed = true;
	}

	VelS += FVector(In.X * DCMove::Accel, In.Y * DCMove::Accel, In.Z * DCMove::AccelDepth) * Dt;
	for (const FDCCurrent& C : Shots[Cur].Currents)
	{
		const FVector D = HeroS - C.C;
		if (C.bOn && FMath::Abs(D.X) < C.Half.X && FMath::Abs(D.Y) < C.Half.Y && FMath::Abs(D.Z) < C.Half.Z)
		{
			VelS += C.Force * Dt;
		}
	}
	// with no input the robot brakes harder, so it settles within about one body length
	VelS -= VelS * FMath::Min((In.IsNearlyZero() ? DCMove::Drag * 2.5f : DCMove::Drag) * Dt, 1.f);
	if (VelS.Size() > DCMove::MaxSpeed * 1.4f)
	{
		VelS = VelS.GetSafeNormal() * DCMove::MaxSpeed * 1.4f;
	}
	MoveHero(VelS * Dt);

	if (!In.IsNearlyZero(0.05))
	{
		Heading = FMath::Vector2DInterpTo(Heading, FVector2D(FMath::Abs(In.X) > 0.1 ? In.X : Heading.X, In.Y), Dt, 8.f);
	}
	else
	{
		Heading.Y = FMath::FInterpTo(Heading.Y, 0.0, Dt, 2.f);
	}

	// thruster: one burst when the robot starts to swim, then one about every second while it keeps going
	const bool bMoving = In.Size() > 0.3;
	ThrustTimer -= Dt;
	if (bMoving && ThrustTimer <= 0.f && Audio)
	{
		Audio->Play(TEXT("thruster"), bWasMoving ? 0.7f : 1.f, 1.f, 0.05f);
		ThrustTimer = 1.1f;
	}
	bWasMoving = bMoving;

	UpdateRail(Dt, false);
	HeroPos = ShotToWorld(Cur, HeroS);
	HeroNdc = ViewNdc(Cur, HeroS, false);
}

// ---------------------------------------------------------------------------------------------
// the cut
// ---------------------------------------------------------------------------------------------

bool AMatchCutGameMode::MarkVisible(const FDCExit& Exit, const FVector2D& MarkNdc) const
{
	if (!Shots[Exit.To].bDark)
	{
		return true;
	}
	// a mark painted in the dark only shows inside the lamp's circle
	const FVector2D D((MarkNdc.X - HeroNdc.X) * 960.0, (MarkNdc.Y - HeroNdc.Y) * 540.0);
	return bLampOn && D.Size() < LampRadius * 1080.0 * 0.82;
}

FDCCutEval AMatchCutGameMode::EvalCut() const
{
	FDCCutEval E;
	const FDCShot& A = Shots[Cur];

	if (InRestore())
	{
		// restore-by-cut: line the robot up with Unit 7 in the removed frame
		const double Dx = (HeroNdc.X - RestoreNdc.X) * DC::Aspect;
		const double Dy = HeroNdc.Y - RestoreNdc.Y;
		E.bOk = FMath::Sqrt(Dx * Dx + Dy * Dy) <= RestoreTol;
		E.Reason = E.bOk ? TEXT("") : DCText::ReasonLineUp;
		return E;
	}
	if (A.Exits.Num() == 0)
	{
		E.Reason = DCText::ReasonLast;
		return E;
	}
	E.Exit = FMath::Clamp(GhostExit, 0, A.Exits.Num() - 1);
	if (!A.bGateOpen)
	{
		E.Reason = DCText::ReasonNoMark;
		return E;
	}

	float HiddenPx = 1.e6f; // nearest mark that the dark is hiding
	for (int32 e = 0; e < A.Exits.Num(); ++e)
	{
		const FDCExit& Exit = A.Exits[e];
		for (int32 m = 0; m < Exit.Marks.Num(); ++m)
		{
			const FVector2D N = ViewNdc(Exit.To, Exit.Marks[m], true);
			const float D = (float)FVector2D((N.X - HeroNdc.X) * 960.0, (N.Y - HeroNdc.Y) * 540.0).Size();
			if (!MarkVisible(Exit, N))
			{
				HiddenPx = FMath::Min(HiddenPx, D);
				continue;
			}
			if (D < E.DistPx)
			{
				E.DistPx = D;
				E.Exit = e;
				E.Mark = m;
			}
		}
	}
	if (HiddenPx < E.DistPx && HiddenPx <= MarkRadius * 1080.f * 1.5f && bHasPart)
	{
		E.Reason = DCText::ReasonDarkLamp; // standing on a mark that only the lamp can show
		return E;
	}
	if (E.Mark == INDEX_NONE)
	{
		E.Reason = DCText::ReasonOffMark;
		return E;
	}
	const FDCExit& Exit = A.Exits[E.Exit];
	const FDCShot& B = Shots[Exit.To];
	E.TargetS = NdcToShot(Exit.To, HeroNdc, FMath::Clamp(HeroS.Z, B.VolMin.Z, B.VolMax.Z), true);
	const double Dz = HeroS.Z - Exit.Marks[E.Mark].Z;
	if (E.DistPx > MarkRadius * 1080.f)
	{
		E.Reason = DCText::ReasonOffMark;
	}
	else if (!Exit.bAnyDepth && Dz < -MarkDepthTol)
	{
		E.Reason = DCText::ReasonTooClose;
	}
	else if (!Exit.bAnyDepth && Dz > MarkDepthTol)
	{
		E.Reason = DCText::ReasonTooFar;
	}
	else if (Blocked(Exit.To, E.TargetS, HeroRadius))
	{
		E.Reason = DCText::ReasonRock;
	}
	else
	{
		E.bOk = true;
	}
	return E;
}

int32 AMatchCutGameMode::PropAfterCut(const FDCProp& Prop, const FDCExit& Exit, FVector* OutS, int32* OutSocket) const
{
	if (Prop.Shot != Cur || Prop.bLocked)
	{
		return 0;
	}
	if (!Prop.bCarried && FVector::Dist(Prop.S, HeroS) > CarryRadius)
	{
		return 0;
	}
	const FDCShot& B = Shots[Exit.To];
	const FVector S = NdcToShot(Exit.To, ViewNdc(Cur, Prop.S, false), FMath::Clamp(Prop.S.Z, B.VolMin.Z - 200.0, B.VolMax.Z + 200.0), true);
	if (OutS)
	{
		*OutS = S;
	}
	const FVector2D N = ViewNdc(Exit.To, S, true);
	for (int32 i = 0; i < Sockets.Num(); ++i)
	{
		const FDCSocket& Socket = Sockets[i];
		if (Socket.Shot != Exit.To || Socket.bFilled || Socket.Accepts != Prop.Kind)
		{
			continue;
		}
		const FVector2D SN = ViewNdc(Exit.To, Socket.S, true);
		if (FVector2D((N.X - SN.X) * 960.0, (N.Y - SN.Y) * 540.0).Size() <= SocketRadius * 1080.0 && FMath::Abs(S.Z - Socket.S.Z) <= 300.0)
		{
			if (OutSocket)
			{
				*OutSocket = i;
			}
			return 2;
		}
	}
	return 1;
}

bool AMatchCutGameMode::TryCut()
{
	if (State != EDCState::Playing)
	{
		return false;
	}
	HeroNdc = ViewNdc(Cur, HeroS, false);
	const FDCCutEval E = EvalCut();
	CurEval = E;

	if (InRestore())
	{
		const double ErrPx = FVector2D((HeroNdc.X - RestoreNdc.X) * 960.0, (HeroNdc.Y - RestoreNdc.Y) * 540.0).Size();
		if (!E.bOk)
		{
			UE_LOG(LogMatchCut, Display, TEXT("RESTORE NG reason=\"%s\" ndc=(%.4f,%.4f) target=(%.4f,%.4f) off=%.0fpx@1080p take=%d"), E.Reason, HeroNdc.X, HeroNdc.Y, RestoreNdc.X, RestoreNdc.Y, ErrPx, Take);
			FailTake(E.Reason);
			return false;
		}
		UE_LOG(LogMatchCut, Display, TEXT("RESTORE OK ndc=(%.4f,%.4f) target=(%.4f,%.4f) off=%.0fpx@1080p"), HeroNdc.X, HeroNdc.Y, RestoreNdc.X, RestoreNdc.Y, ErrPx);
		Take = 1;
		StartFlashback();
		return true;
	}

	const FDCShot& A = Shots[Cur];
	const int32 To = A.Exits.IsValidIndex(E.Exit) ? A.Exits[E.Exit].To : INDEX_NONE;
	if (!E.bOk)
	{
		UE_LOG(LogMatchCut, Display, TEXT("CUT %d->%d NG reason=\"%s\" ndc=(%.4f,%.4f) depth=%.0f markDist=%.0fpx gate=%d lamp=%d first=%d take=%d"),
			Cur, To, E.Reason, HeroNdc.X, HeroNdc.Y, HeroS.Z, E.DistPx > 1.e5f ? -1.f : E.DistPx, A.bGateOpen ? 1 : 0, bLampOn ? 1 : 0, bFirstCutDone ? 0 : 1, Take);
		if (A.Exits.Num() == 0)
		{
			return false;
		}
		if (!bFirstCutDone || !A.bGateOpen)
		{
			// the first cut cannot be failed, and a press before any mark shows only gets the hint
			SoftFailTime = 2.5f;
			return false;
		}
		FailTake(E.Reason);
		return false;
	}

	const FDCExit Exit = A.Exits[E.Exit];
	const FVector2D Before = HeroNdc;
	const int32 From = Cur;

	// what comes through: everything loose inside the carry radius, by the same rule as the robot
	TArray<TPair<int32, int32>> Landed;
	FString Carried;
	for (int32 i = 0; i < Props.Num(); ++i)
	{
		FVector S;
		int32 Socket = INDEX_NONE;
		const int32 R = PropAfterCut(Props[i], Exit, &S, &Socket);
		if (R == 0)
		{
			continue;
		}
		Props[i].Shot = Exit.To;
		Props[i].S = S;
		Carried += FString::Printf(TEXT(" %s%s"), *Props[i].Id, R == 2 ? TEXT("(socket)") : TEXT(""));
		if (R == 2)
		{
			Landed.Add(TPair<int32, int32>(Socket, i));
		}
	}

	bFirstCutDone = true;
	SoftFailTime = 0.f;
	CutFlash = 0.5f;
	Take = 1;
	if (Audio)
	{
		Audio->Play(TEXT("cut_splice"));
	}
	EnterShot(Exit.To, E.TargetS, true);
	const FVector2D After = ViewNdc(Cur, HeroS, false);
	const double ErrPx = FVector2D((After.X - Before.X) * 960.0, (After.Y - Before.Y) * 540.0).Size();
	UE_LOG(LogMatchCut, Display, TEXT("CUT %d->%d OK tape=%s mark=%d markDist=%.0fpx ndc=(%.4f,%.4f) depth=%.0f ndcAfter=(%.4f,%.4f) err=%.2fpx@1080p carried=[%s ] lamp=%d"),
		From, Cur, *Exit.ColorName, E.Mark, E.DistPx, Before.X, Before.Y, HeroS.Z, After.X, After.Y, ErrPx, *Carried, bLampOn ? 1 : 0);

	for (const TPair<int32, int32>& L : Landed)
	{
		FillSocket(L.Key, L.Value, false);
	}
	if (bCrewRescued)
	{
		CrewPos = HeroPos; // the rescued robot comes through the cut with us
	}
	UpdateGate(Cur, false);
	// the snapshot R goes back to is taken after everything has landed
	if (History.Num() > 0)
	{
		TakeSnap(History.Last());
	}
	return true;
}

void AMatchCutGameMode::FailTake(const TCHAR* Reason)
{
	State = EDCState::TakeCard;
	StateTime = 0.f;
	TakeReason = Reason;
	++Take;
	VelS = FVector::ZeroVector;
	if (Audio)
	{
		Audio->Play(TEXT("take_clapper"));
	}
}
