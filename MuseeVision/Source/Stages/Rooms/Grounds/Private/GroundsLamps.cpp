#include "GroundsParts.h"

/**
 * The ten lamp standards (§3.7; as built MuseeLandscape.cpp:43-80): a cast CuSn10 post turned from the sheet's section (a
 * moulded base Ø 0.54, a shaft tapering Ø 0.17 to 0.124, a collar and a cup with a flat top at 3.41), a cased-opal globe
 * r 0.24 about z 3.62 truncated where it sits in the cup, of 3 mm glass, and a bronze finial seated on the globe's crown,
 * its underside following the glass; 4.01 overall, the foot at z 0 bedded in the gravel or paving. Light makes the lamp
 * inside each globe from its Mount anchor. The as-built post was lathed with 20 sides and its finial with 16; here each
 * is a true turn, divided by Form at its one chord tolerance.
 */
namespace Musee::Grounds
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** A turned section through the sheet's points from the one it starts at, smooth after each smooth vertex. */
		void Through(FFormProfile& Profile, TConstArrayView<Plan::FTurnPoint> Points, bool bFromSmooth)
		{
			bool bSmooth = bFromSmooth;
			for (const Plan::FTurnPoint& Point : Points)
			{
				Profile.LineTo(FVector2D(Point.R, Point.Z), bSmooth ? EFormJoint::Smooth : EFormJoint::Crisp);
				bSmooth = Point.bSmooth;
			}
		}

		FFormLathe Post()
		{
			const TConstArrayView<Plan::FTurnPoint> Points(Plan::PostSection);
			FFormLathe Shape;
			Shape.Section = FFormProfile::From(FVector2D(Points[0].R, Points[0].Z));
			Through(Shape.Section, Points.RightChop(1), Points[0].bSmooth);
			return Shape;
		}

		/** The angle (degrees above the equator, in the section's (r, z)) at which a sphere of a radius about the globe's centre reaches a height. */
		double LatitudeAt(double SphereRadius, double Z)
		{
			return FMath::RadiansToDegrees(FMath::Asin((Z - Plan::GlobeCentreZ) / SphereRadius));
		}

		/** The globe's glass, from the crown down its inner face to the cup's top and up its outer face to the crown. */
		FFormLathe Globe()
		{
			const FVector2D Centre(0.0, Plan::GlobeCentreZ);
			const double Outer = Plan::GlobeRadius;
			const double Inner = Plan::GlobeRadius - Plan::GlobeGlass;
			FFormLathe Shape;
			Shape.Section = FFormProfile::From(FVector2D(0.0, Plan::GlobeCentreZ + Inner));
			Shape.Section.ArcAbout(Centre, Inner, Inner, 90.0, LatitudeAt(Inner, Plan::CupTop), EFormJoint::Smooth);
			const double Seat = FMath::Sqrt(FMath::Square(Outer) - FMath::Square(Plan::GlobeCentreZ - Plan::CupTop));
			Shape.Section.LineTo(FVector2D(Seat, Plan::CupTop), EFormJoint::Crisp);
			Shape.Section.ArcAbout(Centre, Outer, Outer, LatitudeAt(Outer, Plan::CupTop), 90.0, EFormJoint::Crisp);
			return Shape;
		}

		/** The finial, its underside on the globe's outer face from the crown out to its foot, then the sheet's profile. */
		FFormLathe Finial()
		{
			const FVector2D Centre(0.0, Plan::GlobeCentreZ);
			const double Outer = Plan::GlobeRadius;
			const TConstArrayView<Plan::FTurnPoint> Points(Plan::FinialSection);
			const double Foot = FMath::RadiansToDegrees(FMath::Acos(Points[0].R / Outer));
			FFormLathe Shape;
			Shape.Section = FFormProfile::From(FVector2D(0.0, Plan::GlobeCentreZ + Outer));
			Shape.Section.ArcAbout(Centre, Outer, Outer, 90.0, Foot, EFormJoint::Smooth);
			Through(Shape.Section, Points.RightChop(1), false);
			return Shape;
		}
	}

	void DrawLamps(TArray<FFormDrawing>& Out)
	{
		for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::Lamps)); ++Index)
		{
			const FMuseeFrame Foot(FVector(Plan::Lamps[Index].X, Plan::Lamps[Index].Y, 0.0));
			const FString Name = FString::Printf(TEXT("lamp.%02d"), Index + 1);
			Out.Add(Drawn(Name + TEXT(".post"), TEXT("lamp.post"), Post(), Solid, Foot));
			FFormDrawing Glass = Drawn(Name + TEXT(".globe"), TEXT("lamp.globe"), Globe(), Solid, Foot);
			Glass.bNanite = false;
			Out.Add(MoveTemp(Glass));
			Out.Add(Drawn(Name + TEXT(".finial"), TEXT("lamp.post"), Finial(), Solid, Foot));
		}
	}
}
