#include "AlbionParts.h"

#include "Algo/Reverse.h"

namespace Musee::Albion
{
	namespace
	{
		/** Two heights or distances closer than a micrometre are one (the kernel's weld tolerance). */
		constexpr double Same = 1.0e-6;

		double Degrees(double Radians) { return FMath::RadiansToDegrees(Radians); }
	}

	FMuseeKey Key(const FString& Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, *Local));
	}

	FName Surface(const FString& Local)
	{
		return FName(*FString::Printf(TEXT("%s.%s"), Plan::Stage, *Local));
	}

	FFormDrawing Glass(FFormDrawing Drawing)
	{
		Drawing.bNanite = false;
		return Drawing;
	}

	FMuseeFrame AcrossX(double X)
	{
		return FrameOfAxes(FVector(X, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector::UpVector);
	}

	FMuseeFrame AcrossY(double Y)
	{
		return FrameOfAxes(FVector(0.0, Y, 0.0), FVector(1.0, 0.0, 0.0), FVector::UpVector);
	}

	FMuseeFrame Along(const FVector& From, const FVector& To, const FVector& Up)
	{
		const FVector Direction = (To - From).GetSafeNormal();
		FVector Y = Up - Direction * FVector::DotProduct(Up, Direction);
		if (Y.IsNearlyZero())
		{
			// A member along the up hint itself: its section's y is laid east.
			Y = FVector::ForwardVector - Direction * FVector::DotProduct(FVector::ForwardVector, Direction);
		}
		Y.Normalize();
		return FrameOfAxes(From, FVector::CrossProduct(Y, Direction), Y);
	}

	FMuseeFrame OnFace(const FVector& Origin, const FVector& Normal)
	{
		return FrameOfAxes(Origin, FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal(), FVector::UpVector);
	}

	FFormProfile Section(TConstArrayView<FVector2D> Points)
	{
		double Twice = 0.0;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			Twice += FVector2D::CrossProduct(Points[Index], Points[(Index + 1) % Points.Num()]);
		}
		if (Twice >= 0.0)
		{
			return FFormProfile::Polygon(Points);
		}
		TArray<FVector2D> Reversed(Points.GetData(), Points.Num());
		Algo::Reverse(Reversed);
		return FFormProfile::Polygon(Reversed);
	}

	FFormProfile Circle(const FVector2D& Centre, double Radius)
	{
		return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
	}

	FFormProfile Turned(TConstArrayView<Plan::FTurnPoint> Points, double Lift)
	{
		FFormProfile Profile = FFormProfile::From(FVector2D(Points[0].R, Points[0].Z + Lift));
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Profile.LineTo(FVector2D(Points[Index].R, Points[Index].Z + Lift), Points[Index - 1].bSmooth ? EFormJoint::Smooth : EFormJoint::Crisp);
		}
		return Profile;
	}

	FFormDrawing Box(const FString& Name, const FString& LocalSurface, const FVector& Min, const FVector& Max,
		const Musee::Collision::FClass* Collision)
	{
		FFormExtrusion Shape;
		Shape.Outline = FFormProfile::Rectangle(FVector2D(Min.X, Min.Y), FVector2D(Max.X, Max.Y));
		Shape.DepthMeters = Max.Z - Min.Z;
		return Drawn(Name, LocalSurface, MoveTemp(Shape), Collision, FMuseeFrame(FVector(0.0, 0.0, Min.Z)));
	}

	FFormDrawing Bar(const FString& Name, const FString& LocalSurface, const FVector& From, const FVector& To, FFormProfile Profile,
		const Musee::Collision::FClass* Collision, const FVector& Up)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Profile);
		Shape.DepthMeters = FVector::Distance(From, To);
		return Drawn(Name, LocalSurface, MoveTemp(Shape), Collision, Along(From, To, Up));
	}

	FFormDrawing Beam(const FString& Name, const FString& LocalSurface, const FVector& From, const FVector& To, double Wide, double High,
		const Musee::Collision::FClass* Collision, const FVector& Up)
	{
		return Bar(Name, LocalSurface, From, To, FFormProfile::Rectangle(FVector2D(-0.5 * Wide, -0.5 * High), FVector2D(0.5 * Wide, 0.5 * High)),
			Collision, Up);
	}

	FFormDrawing Rod(const FString& Name, const FString& LocalSurface, const FVector& From, const FVector& To, double Radius,
		const Musee::Collision::FClass* Collision)
	{
		return Bar(Name, LocalSurface, From, To, Circle(FVector2D::ZeroVector, Radius), Collision);
	}

	FFormDrawing Lathe(const FString& Name, const FString& LocalSurface, const FVector& Foot, FFormProfile Profile,
		const Musee::Collision::FClass* Collision, double FromDegrees, double ToDegrees)
	{
		FFormLathe Shape;
		Shape.Section = MoveTemp(Profile);
		Shape.FromDegrees = FromDegrees;
		Shape.ToDegrees = ToDegrees;
		return Drawn(Name, LocalSurface, MoveTemp(Shape), Collision, FMuseeFrame(Foot));
	}

	FFormDrawing Ball(const FString& Name, const FString& LocalSurface, const FVector& Centre, double Radius,
		const Musee::Collision::FClass* Collision)
	{
		FFormProfile Profile = FFormProfile::From(FVector2D(0.0, -Radius));
		Profile.ArcAbout(FVector2D::ZeroVector, Radius, Radius, -90.0, 90.0, EFormJoint::Crisp);
		return Lathe(Name, LocalSurface, Centre, MoveTemp(Profile), Collision);
	}

	FFormDrawing Ring(const FString& Name, const FString& LocalSurface, const FMuseeFrame& Frame, double RingRadius, double Tube,
		const Musee::Collision::FClass* Collision)
	{
		FFormSweep Shape;
		Shape.Section = Circle(FVector2D::ZeroVector, 0.5 * Tube);
		Shape.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, RingRadius, RingRadius, 0.0, true);
		return Drawn(Name, LocalSurface, MoveTemp(Shape), Collision, Frame);
	}

	// ---- the arch ---------------------------------------------------------------------------------------------------------------
	FArch FArch::Pointed(double InCentre, double InHalf, double InSill, double InSpringing, double InCrown)
	{
		const double Rise = InCrown - InSpringing;
		FArch Arch;
		Arch.Centre = InCentre;
		Arch.Half = InHalf;
		Arch.Sill = InSill;
		Arch.Springing = InSpringing;
		Arch.Radius = (Rise * Rise + InHalf * InHalf) / (2.0 * InHalf);
		Arch.Offset = Arch.Radius - InHalf;
		return Arch;
	}

	FArch FArch::PointedToHead(double InCentre, double InHalf, double InSill, double InHead, double InRadius)
	{
		FArch Arch;
		Arch.Centre = InCentre;
		Arch.Half = InHalf;
		Arch.Sill = InSill;
		Arch.Radius = InRadius;
		Arch.Offset = InRadius - InHalf;
		Arch.Springing = InHead - FMath::Sqrt(InRadius * InRadius - Arch.Offset * Arch.Offset);
		return Arch;
	}

	FArch FArch::Round(double InCentre, double InHalf, double InSill, double InSpringing)
	{
		FArch Arch;
		Arch.Centre = InCentre;
		Arch.Half = InHalf;
		Arch.Sill = InSill;
		Arch.Springing = InSpringing;
		Arch.Radius = InHalf;
		return Arch;
	}

	double FArch::Crown() const
	{
		return Radius > 0.0 ? Springing + FMath::Sqrt(Radius * Radius - Offset * Offset) : Springing;
	}

	double FArch::HalfAt(double Z) const
	{
		if (Z <= Springing || Radius <= 0.0)
		{
			return Half;
		}
		const double Up = Z - Springing;
		return FMath::Max(0.0, FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Up * Up)) - Offset);
	}

	FArch FArch::Grown(double By) const
	{
		FArch Arch = *this;
		Arch.Half += By;
		if (Radius > 0.0)
		{
			Arch.Radius += By;
		}
		return Arch;
	}

	namespace
	{
		/** The angle, degrees, of the left arc at a height (180 at the springing, falling towards the crown). */
		double LeftDeg(const FArch& A, double Z)
		{
			return 180.0 - Degrees(FMath::Asin(FMath::Clamp((Z - A.Springing) / A.Radius, -1.0, 1.0)));
		}

		/** The angle, degrees, of the right arc at a height (0 at the springing, rising towards the crown). */
		double RightDeg(const FArch& A, double Z)
		{
			return Degrees(FMath::Asin(FMath::Clamp((Z - A.Springing) / A.Radius, -1.0, 1.0)));
		}

		FVector2D LeftCentre(const FArch& A) { return FVector2D(A.Centre + A.Offset, A.Springing); }
		FVector2D RightCentre(const FArch& A) { return FVector2D(A.Centre - A.Offset, A.Springing); }

		/** The two arcs meet at the crown in a point, or run through it smoothly when they share a centre. */
		EFormJoint AtCrown(const FArch& A) { return A.Offset > Same ? EFormJoint::Crisp : EFormJoint::Smooth; }
	}

	void FArch::UpLeft(FFormProfile& P, double Z0, double Z1) const
	{
		const bool bJamb = Z0 < Springing - Same;
		if (bJamb)
		{
			P.LineTo(FVector2D(Centre - Half, FMath::Min(Z1, Springing)), EFormJoint::Crisp);
		}
		if (Radius > 0.0 && Z1 > Springing + Same)
		{
			P.ArcAbout(LeftCentre(*this), Radius, Radius, LeftDeg(*this, FMath::Max(Z0, Springing)), LeftDeg(*this, Z1),
				bJamb ? EFormJoint::Smooth : EFormJoint::Crisp);
		}
	}

	void FArch::DownLeft(FFormProfile& P, double Z1, double Z0) const
	{
		const bool bArc = Radius > 0.0 && Z1 > Springing + Same;
		if (bArc)
		{
			P.ArcAbout(LeftCentre(*this), Radius, Radius, LeftDeg(*this, Z1), LeftDeg(*this, FMath::Max(Z0, Springing)), EFormJoint::Crisp);
		}
		if (Z0 < Springing - Same)
		{
			P.LineTo(FVector2D(Centre - Half, Z0), bArc ? EFormJoint::Smooth : EFormJoint::Crisp);
		}
	}

	void FArch::UpRight(FFormProfile& P, double Z0, double Z1) const
	{
		const bool bJamb = Z0 < Springing - Same;
		if (bJamb)
		{
			P.LineTo(FVector2D(Centre + Half, FMath::Min(Z1, Springing)), EFormJoint::Crisp);
		}
		if (Radius > 0.0 && Z1 > Springing + Same)
		{
			P.ArcAbout(RightCentre(*this), Radius, Radius, RightDeg(*this, FMath::Max(Z0, Springing)), RightDeg(*this, Z1),
				bJamb ? EFormJoint::Smooth : EFormJoint::Crisp);
		}
	}

	void FArch::DownRight(FFormProfile& P, double Z1, double Z0) const
	{
		const bool bArc = Radius > 0.0 && Z1 > Springing + Same;
		if (bArc)
		{
			P.ArcAbout(RightCentre(*this), Radius, Radius, RightDeg(*this, Z1), RightDeg(*this, FMath::Max(Z0, Springing)), EFormJoint::Crisp);
		}
		if (Z0 < Springing - Same)
		{
			P.LineTo(FVector2D(Centre + Half, Z0), bArc ? EFormJoint::Smooth : EFormJoint::Crisp);
		}
	}

	void FArch::OverHead(FFormProfile& P, double ZLeft, double ZRight) const
	{
		if (Radius <= 0.0)
		{
			UpLeft(P, ZLeft, Springing);
			P.LineTo(FVector2D(Centre + Half, Springing), EFormJoint::Crisp);
			if (ZRight < Springing - Same)
			{
				P.LineTo(FVector2D(Centre + Half, ZRight), EFormJoint::Crisp);
			}
			return;
		}
		const double Top = Crown();
		UpLeft(P, ZLeft, Top);
		P.ArcAbout(RightCentre(*this), Radius, Radius, RightDeg(*this, Top), RightDeg(*this, FMath::Max(ZRight, Springing)), AtCrown(*this));
		if (ZRight < Springing - Same)
		{
			P.LineTo(FVector2D(Centre + Half, ZRight), EFormJoint::Smooth);
		}
	}

	FFormProfile FArch::Outline() const
	{
		FFormProfile P = FFormProfile::From(FVector2D(Centre - Half, Sill));
		P.LineTo(FVector2D(Centre + Half, Sill), EFormJoint::Crisp);
		if (Radius <= 0.0)
		{
			P.LineTo(FVector2D(Centre + Half, Springing), EFormJoint::Crisp).LineTo(FVector2D(Centre - Half, Springing), EFormJoint::Crisp);
			return P.Close();
		}
		const double Top = Crown();
		UpRight(P, Sill, Top);
		P.ArcAbout(LeftCentre(*this), Radius, Radius, LeftDeg(*this, Top), 180.0, AtCrown(*this));
		return P.Close();
	}

	FFormProfile FArch::Head(double Z) const
	{
		FFormProfile P = FFormProfile::From(FVector2D(Centre - HalfAt(Z), Z));
		OverHead(P, Z, Z);
		return P;
	}

	FArch Lancet(int32 Bay)
	{
		return FArch::PointedToHead(Plan::BayY(Bay), Plan::LancetHalf, Plan::LancetSill, Plan::LancetHead, Plan::LancetRadius);
	}

	FArch CourtDoor()
	{
		return FArch::Pointed(0.0, Plan::CourtDoorHalf, 0.0, Plan::CourtDoorSpringing, Plan::CourtDoorCrown);
	}

	FArch RoughOpening()
	{
		return FArch::Round(0.0, Plan::PortClearHalf + Plan::RoughAllowance, Plan::RoughSill, Plan::PortSpringing);
	}

	// ---- the vaults ---------------------------------------------------------------------------------------------------------------
	double FVaultHalf::Radius() const
	{
		const double Span = FMath::Abs(CrownX - SpringX);
		const double Rise = CrownZ - Plan::Springing;
		return (Rise * Rise + Span * Span) / (2.0 * Span);
	}

	double FVaultHalf::Sign() const
	{
		return CrownX > SpringX ? 1.0 : -1.0;
	}

	double FVaultHalf::CentreX() const
	{
		return SpringX + Sign() * Radius();
	}

	double FVaultHalf::SpringDeg() const
	{
		return Sign() > 0.0 ? 180.0 : 0.0;
	}

	double FVaultHalf::CrownDeg() const
	{
		return Degrees(FMath::Atan2(CrownZ - Plan::Springing, CrownX - CentreX()));
	}

	double FVaultHalf::Length() const
	{
		return Radius() * FMath::DegreesToRadians(FMath::Abs(CrownDeg() - SpringDeg()));
	}

	double FVaultHalf::DegAt(double S) const
	{
		return SpringDeg() - Sign() * Degrees(S / Radius());
	}

	double FVaultHalf::SAtHeight(double Z) const
	{
		return Radius() * FMath::Asin(FMath::Clamp((Z - Plan::Springing) / Radius(), -1.0, 1.0));
	}

	FVector2D FVaultHalf::At(double S, double Depth) const
	{
		const double T = FMath::DegreesToRadians(DegAt(S));
		const double R = Radius() - Depth;
		return FVector2D(CentreX() + R * FMath::Cos(T), Plan::Springing + R * FMath::Sin(T));
	}

	FVector2D FVaultHalf::Normal(double S) const
	{
		const double T = FMath::DegreesToRadians(DegAt(S));
		return FVector2D(FMath::Cos(T), FMath::Sin(T));
	}

	TArray<double> FVaultHalf::PaneStations() const
	{
		const double From = SAtHeight(Plan::PanesFrom);
		const int32 Panes = FMath::Max(1, FMath::RoundToInt32((Length() - From) / Plan::PaneArc));
		TArray<double> Stations;
		for (int32 Index = 0; Index <= Panes; ++Index)
		{
			Stations.Add(FMath::Lerp(From, Length(), double(Index) / Panes));
		}
		return Stations;
	}

	FFormProfile FVaultHalf::Arc(double Depth, double S0, double S1) const
	{
		// Clockwise about the centre, so the centre (the inside) lies to the path's right: the west halves run up from the
		// springing, the east halves down from the crown.
		const double From = Sign() > 0.0 ? S0 : S1;
		const double To = Sign() > 0.0 ? S1 : S0;
		const double R = Radius() - Depth;
		FFormProfile P = FFormProfile::From(At(From, Depth));
		P.ArcAbout(FVector2D(CentreX(), Plan::Springing), R, R, DegAt(From), DegAt(To), EFormJoint::Crisp);
		return P;
	}

	FFormProfile FVaultHalf::Path(TArray<FVector2D> Points, bool bSmooth) const
	{
		if (Sign() < 0.0)
		{
			Algo::Reverse(Points);
		}
		FFormProfile P = FFormProfile::From(Points[0]);
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			P.LineTo(Points[Index], bSmooth && Index > 1 ? EFormJoint::Smooth : EFormJoint::Crisp);
		}
		return P;
	}

	TConstArrayView<FVaultHalf> VaultHalves()
	{
		static const FVaultHalf Halves[] = {
			{TEXT("nave-w"), true, -Plan::ArcadeX, 0.0, Plan::NaveCrown},
			{TEXT("nave-e"), true, Plan::ArcadeX, 0.0, Plan::NaveCrown},
			{TEXT("west-w"), false, -Plan::SideInnerX, -Plan::AisleCrownX, Plan::AisleCrown},
			{TEXT("west-e"), false, -Plan::ArcadeX, -Plan::AisleCrownX, Plan::AisleCrown},
			{TEXT("east-w"), false, Plan::ArcadeX, Plan::AisleCrownX, Plan::AisleCrown},
			{TEXT("east-e"), false, Plan::SideInnerX, Plan::AisleCrownX, Plan::AisleCrown}};
		return Halves;
	}

	double GlassZ(double X)
	{
		const double Across = FMath::Abs(X);
		const TConstArrayView<FVaultHalf> Halves = VaultHalves();
		const FVaultHalf& Half = Across <= Plan::ArcadeX ? Halves[1] : Halves[5];
		const double FromCrown = Across <= Plan::ArcadeX ? Across : FMath::Abs(Across - Plan::AisleCrownX);
		const double HalfSpan = Across <= Plan::ArcadeX ? Plan::NaveHalfSpan : Plan::AisleHalfSpan;
		const double R = Half.Radius();
		const double Offset = R - HalfSpan;
		return Plan::Springing + FMath::Sqrt(FMath::Max(0.0, R * R - FMath::Square(FromCrown + Offset)));
	}
}
