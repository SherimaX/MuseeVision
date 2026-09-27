#include "SphereParts.h"

#include "SpherePlan.h"

namespace Musee::Sphere
{
	FMuseeKey Key(const TCHAR* Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FVector2D AtBearing(double Radius, double BearingDeg)
	{
		const double B = FMath::DegreesToRadians(BearingDeg);
		return FVector2D(Radius * FMath::Sin(B), -Radius * FMath::Cos(B));
	}

	FVector Radial(double BearingDeg)
	{
		const FVector2D Unit = AtBearing(1.0, BearingDeg);
		return FVector(Unit.X, Unit.Y, 0.0);
	}

	FVector Tangent(double BearingDeg)
	{
		const double B = FMath::DegreesToRadians(BearingDeg);
		return FVector(FMath::Cos(B), FMath::Sin(B), 0.0);
	}

	double PlanDeg(double BearingDeg)
	{
		return BearingDeg - 90.0;
	}

	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(Origin, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	FMuseeFrame Raised(double Z)
	{
		return FMuseeFrame(FVector(0.0, 0.0, Z));
	}

	FMuseeFrame OnBearing(double BearingDeg, double Radius, double Z)
	{
		const FVector2D At = AtBearing(Radius, BearingDeg);
		return FrameOfAxes(FVector(At.X, At.Y, Z), Radial(BearingDeg), Tangent(BearingDeg));
	}

	double SphereZ(double SphereRadius, double Radius, bool bAbove)
	{
		const double Half = FMath::Sqrt(SphereRadius * SphereRadius - Radius * Radius);
		return Plan::CentreZ + (bAbove ? Half : -Half);
	}

	double SphereDeg(double SphereRadius, double Radius, bool bAbove)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(SphereZ(SphereRadius, Radius, bAbove) - Plan::CentreZ, Radius));
	}

	FFormLathe Ring(double Inner, double Outer, double Bottom, double Top, double FromBearing, double ToBearing)
	{
		FFormLathe Shape = Turned(FFormProfile::Rectangle(FVector2D(Inner, Bottom), FVector2D(Outer, Top)));
		if (ToBearing - FromBearing < 360.0)
		{
			Shape.FromDegrees = PlanDeg(FromBearing);
			Shape.ToDegrees = PlanDeg(ToBearing);
		}
		return Shape;
	}

	FFormLathe Turned(FFormProfile Section)
	{
		FFormLathe Shape;
		Shape.Section = MoveTemp(Section);
		return Shape;
	}

	FFormExtrusion Slab(FFormProfile Outline, TArray<FFormProfile> Holes, double Depth)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Depth;
		return Shape;
	}

	FFormProfile Circle(double Radius, const FVector2D& Centre)
	{
		return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
	}

	FFormProfile Rectangle(const FVector2D& Min, const FVector2D& Max)
	{
		return FFormProfile::Rectangle(Min, Max);
	}

	TArray<FVector2D> SeatPolygonPoints()
	{
		// The sides are centred on bearings 3.75°k, so the corners stand half a side's angle round from them.
		const double Side = 360.0 / Plan::SeatSides;
		TArray<FVector2D> Points;
		for (int32 Corner = 0; Corner < Plan::SeatSides; ++Corner)
		{
			Points.Add(AtBearing(Plan::SeatCircumradius, 0.5 * Side + Side * Corner));
		}
		return Points;
	}

	FFormProfile SeatPolygon()
	{
		return FFormProfile::Polyline(SeatPolygonPoints(), true);
	}

	double SeatInscribed()
	{
		return Plan::SeatCircumradius * FMath::Cos(FMath::DegreesToRadians(180.0 / Plan::SeatSides));
	}

	double AngleOf(const FVector2D& Point, double AtLeastDeg)
	{
		double Deg = FMath::RadiansToDegrees(FMath::Atan2(Point.Y, Point.X));
		while (Deg < AtLeastDeg)
		{
			Deg += 360.0;
		}
		return Deg;
	}

	FFormProfile Sector(double Inner, double Outer, double FromBearing, double ToBearing, double FromOffset, double ToOffset)
	{
		// A point on a circle of a radius, on the line parallel to a bearing's radial line and set off it sideways.
		auto OnLine = [](double Radius, double BearingDeg, double Offset)
		{
			const FVector Point = Radial(BearingDeg) * FMath::Sqrt(Radius * Radius - Offset * Offset) + Tangent(BearingDeg) * Offset;
			return FVector2D(Point.X, Point.Y);
		};
		const FVector2D FromInner = OnLine(Inner, FromBearing, FromOffset);
		const FVector2D FromOuter = OnLine(Outer, FromBearing, FromOffset);
		const FVector2D ToOuter = OnLine(Outer, ToBearing, -ToOffset);
		const FVector2D ToInner = OnLine(Inner, ToBearing, -ToOffset);
		// Plan angles rise with the bearing; the far end's angle is carried past the near one's where the sector crosses +x.
		const double FromOuterDeg = AngleOf(FromOuter);
		const double FromInnerDeg = AngleOf(FromInner);

		FFormProfile Outline = FFormProfile::From(FromInner);
		Outline.LineTo(FromOuter, EFormJoint::Crisp);
		Outline.ArcAbout(FVector2D::ZeroVector, Outer, Outer, FromOuterDeg, AngleOf(ToOuter, FromOuterDeg), EFormJoint::Crisp);
		Outline.LineTo(ToInner, EFormJoint::Crisp);
		Outline.ArcAbout(FVector2D::ZeroVector, Inner, Inner, AngleOf(ToInner, FromInnerDeg), FromInnerDeg, EFormJoint::Crisp);
		return Outline.Close();
	}

	FFormProfile Sector(double Inner, double Outer, double FromBearing, double ToBearing, double Offset)
	{
		return Sector(Inner, Outer, FromBearing, ToBearing, Offset, Offset);
	}

	TArray<FVector2D> SeatPolygonBetween(double FromBearing, double ToBearing)
	{
		// A bearing's radial line crosses the side centred on the nearest multiple of 3.75°, which lies at the inscribed
		// radius square to that multiple.
		const double Side = 360.0 / Plan::SeatSides;
		auto Crossing = [Side](double BearingDeg)
		{
			const double Centre = Side * FMath::RoundToDouble(BearingDeg / Side);
			return AtBearing(SeatInscribed() / FMath::Cos(FMath::DegreesToRadians(BearingDeg - Centre)), BearingDeg);
		};
		TArray<FVector2D> Points = {Crossing(FromBearing)};
		for (double Corner = Side * (FMath::FloorToDouble((FromBearing - 0.5 * Side) / Side) + 0.5); Corner < ToBearing; Corner += Side)
		{
			if (Corner > FromBearing)
			{
				Points.Add(AtBearing(Plan::SeatCircumradius, Corner));
			}
		}
		Points.Add(Crossing(ToBearing));
		return Points;
	}

	FFormLathe Rod(double Radius, double Bottom, double Top)
	{
		return Turned(FFormProfile::Polyline({FVector2D(0.0, Bottom), FVector2D(Radius, Bottom), FVector2D(Radius, Top), FVector2D(0.0, Top)}, false));
	}

	FFormProfile IrisBladeOpen()
	{
		const FVector2D Axis(-Plan::IrisPin, 0.0);
		const double Half = 0.5 * Plan::IrisWidth;
		const double Inner = Plan::IrisMiddle - Half;
		const double Outer = Plan::IrisMiddle + Half;
		const double Arc = Plan::IrisArcDeg;
		const double ArcRad = FMath::DegreesToRadians(Arc);
		const FVector2D NearEnd = Axis + FVector2D(Plan::IrisMiddle, 0.0);
		const FVector2D FarEnd = Axis + Plan::IrisMiddle * FVector2D(FMath::Cos(ArcRad), FMath::Sin(ArcRad));
		// The inner edge out, round the far end through its forward tangent, the outer edge back, round the pin's end.
		FFormProfile Blade = FFormProfile::From(Axis + FVector2D(Inner, 0.0));
		Blade.ArcAbout(Axis, Inner, Inner, 0.0, Arc)
			.ArcAbout(FarEnd, Half, Half, Arc + 180.0, Arc)
			.ArcAbout(Axis, Outer, Outer, Arc, 0.0)
			.ArcAbout(NearEnd, Half, Half, 0.0, -180.0);
		return Blade.Close();
	}

	FMuseeFrame IrisBladeShut(int32 BladeIndex)
	{
		const double Pin = 360.0 * BladeIndex / Plan::IrisBlades;
		return FMuseeFrame(FVector(0.0, 0.0, -Plan::IrisBlade), FQuat(FVector::UpVector, FMath::DegreesToRadians(Pin + Plan::IrisSwingDeg)));
	}

	FMuseeFrame IrisPinFrame(int32 BladeIndex, double TopZ)
	{
		const double Pin = FMath::DegreesToRadians(360.0 * BladeIndex / Plan::IrisBlades);
		return FMuseeFrame(FVector(Plan::IrisPin * FMath::Cos(Pin), Plan::IrisPin * FMath::Sin(Pin), TopZ - Plan::IrisPitch * BladeIndex));
	}

	FMuseeKey IrisBladeBody(const TCHAR* Iris, int32 BladeIndex)
	{
		return Key(*FString::Printf(TEXT("%s.iris.blade.%d"), Iris, BladeIndex + 1));
	}

	FMuseeKey DogBody(int32 DogIndex)
	{
		return Key(*FString::Printf(TEXT("mast.dogs.%d"), DogIndex + 1));
	}

	FMuseeFrame DogFrame(int32 DogIndex)
	{
		const double Bearing = Plan::DogBearings[DogIndex];
		const FVector2D Tip = AtBearing(Plan::DogRetracted, Bearing);
		const FVector Inward = -Radial(Bearing);
		return FrameOfAxes(FVector(Tip.X, Tip.Y, Plan::Mouth), Inward, FVector::CrossProduct(FVector::UpVector, Inward));
	}

	namespace Pieces
	{
		FString ThroatBand(int32 Index) { return FString::Printf(TEXT("neck.throat.band.%d"), Index + 1); }
		FString HousingChannel(int32 DogIndex) { return FString::Printf(TEXT("mast.dogs.housing.%d.channel"), DogIndex + 1); }
		FString HousingCap(int32 DogIndex) { return FString::Printf(TEXT("mast.dogs.housing.%d.cap"), DogIndex + 1); }
		FString NeckBlade(int32 BladeIndex) { return FString::Printf(TEXT("neck.iris.blade.%d"), BladeIndex + 1); }

		TArray<FName> NeckWall()
		{
			TArray<FName> Names = {Throat, FlangeBase, FlangeWall, FlangePlate};
			for (int32 Dog = 0; Dog < int32(UE_ARRAY_COUNT(Plan::DogBearings)); ++Dog)
			{
				Names.Add(FName(*ThroatBand(Dog)));
				Names.Add(FName(*HousingChannel(Dog)));
				Names.Add(FName(*HousingCap(Dog)));
			}
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				Names.Add(FName(*NeckBlade(Blade)));
			}
			return Names;
		}
	}
}
