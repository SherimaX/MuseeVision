#include "AtriumParts.h"

namespace Musee::Atrium
{
	FMuseeKey Key(const TCHAR* Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	double PlanAngle(double Bearing)
	{
		return Bearing - 90.0;
	}

	FVector2D Polar(double Radius, double Bearing)
	{
		return Radius * Outward(Bearing);
	}

	FVector2D Outward(double Bearing)
	{
		const double B = FMath::DegreesToRadians(Bearing);
		return FVector2D(FMath::Sin(B), -FMath::Cos(B));
	}

	FVector2D Along(double Bearing)
	{
		const double B = FMath::DegreesToRadians(Bearing);
		return FVector2D(FMath::Cos(B), FMath::Sin(B));
	}

	double BearingOf(const FVector2D& Point)
	{
		const double Bearing = FMath::RadiansToDegrees(FMath::Atan2(Point.X, -Point.Y));
		return Bearing < 0.0 ? Bearing + 360.0 : Bearing;
	}

	double RibBearing(int32 Rib)
	{
		return Plan::RibFirstBearing + Plan::BayStep * Rib;
	}

	FFormProfile Section(TConstArrayView<Plan::FStep> Steps, bool bClosed, double Rise)
	{
		check(!Steps.IsEmpty() && !Steps[0].bArc);
		FFormProfile Profile = FFormProfile::From(FVector2D(Steps[0].A, Steps[0].B + Rise));
		for (int32 Index = 1; Index < Steps.Num(); ++Index)
		{
			const Plan::FStep& Step = Steps[Index];
			if (Step.bArc)
			{
				Profile.ArcAbout(FVector2D(Step.A, Step.B + Rise), Step.Radius, Step.Radius, Step.FromDeg, Step.ToDeg);
			}
			else
			{
				Profile.LineTo(FVector2D(Step.A, Step.B + Rise));
			}
		}
		return bClosed ? Profile.Close() : Profile;
	}

	FFormProfile Box(double A0, double B0, double A1, double B1)
	{
		return FFormProfile::Rectangle(FVector2D(FMath::Min(A0, A1), FMath::Min(B0, B1)), FVector2D(FMath::Max(A0, A1), FMath::Max(B0, B1)));
	}

	FFormLathe Turned(FFormProfile Section, double FromBearing, double ToBearing)
	{
		FFormLathe Shape;
		Shape.Section = MoveTemp(Section);
		Shape.FromDegrees = PlanAngle(FromBearing);
		Shape.ToDegrees = PlanAngle(ToBearing <= FromBearing ? ToBearing + 360.0 : ToBearing);
		return Shape;
	}

	FFormLathe TurnedAround(FFormProfile Section)
	{
		FFormLathe Shape;
		Shape.Section = MoveTemp(Section);
		return Shape;
	}

	FFormLathe Band(double R0, double R1, double Z0, double Z1, double FromBearing, double ToBearing)
	{
		return Turned(Box(R0, Z0, R1, Z1), FromBearing, ToBearing);
	}

	FFormExtrusion Prism(FFormProfile Outline, double Bottom, double Top, TArray<FFormProfile> Holes)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Top - Bottom;
		return Shape;
	}

	FMuseeFrame RadialPlane(double Bearing, double Half)
	{
		const FVector2D Out = Outward(Bearing);
		const FVector2D Back = -Along(Bearing);
		const FVector Z(Back.X, Back.Y, 0.0);
		return FrameOfAxes(-Half * Z, FVector(Out.X, Out.Y, 0.0), FVector::UpVector);
	}

	FMuseeFrame OnBearing(double Bearing, double Height)
	{
		return FMuseeFrame(FVector(0.0, 0.0, Height), FQuat(FVector::UpVector, FMath::DegreesToRadians(PlanAngle(Bearing))));
	}

	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(Origin, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	FFormProfile Sector(double R0, double R1, double FromBearing, double ToBearing, double HalfJoint)
	{
		const double To = ToBearing <= FromBearing ? ToBearing + 360.0 : ToBearing;
		// A joint of constant width: each side runs parallel to its radial line, HalfJoint into the slab.
		auto Inset = [HalfJoint](double Radius) { return FMath::RadiansToDegrees(FMath::Asin(HalfJoint / Radius)); };
		FFormProfile Outline = FFormProfile::From(Polar(R0, FromBearing + Inset(R0)));
		Outline.ArcAbout(FVector2D::ZeroVector, R0, R0, PlanAngle(FromBearing + Inset(R0)), PlanAngle(To - Inset(R0)));
		Outline.LineTo(Polar(R1, To - Inset(R1)), EFormJoint::Crisp);
		Outline.ArcAbout(FVector2D::ZeroVector, R1, R1, PlanAngle(To - Inset(R1)), PlanAngle(FromBearing + Inset(R1)), EFormJoint::Crisp);
		return Outline.Close();
	}

	FFormProfile AtBearing(double Bearing, double U0, double U1, double T0, double T1)
	{
		auto At = [Bearing](double U, double T) { return U * Outward(Bearing) + T * Along(Bearing); };
		return FFormProfile::Polyline({At(U0, T0), At(U1, T0), At(U1, T1), At(U0, T1)}, true);
	}

	double FromDoor(double Degrees)
	{
		return Plan::DoorBearing + Degrees;
	}

	double HalfAngle(double HalfWidth, double Radius)
	{
		return FMath::RadiansToDegrees(FMath::Asin(HalfWidth / Radius));
	}

	FFormDrawing& AsGlass(FFormDrawing& Drawing)
	{
		Drawing.bNanite = false;
		return Drawing;
	}
}
