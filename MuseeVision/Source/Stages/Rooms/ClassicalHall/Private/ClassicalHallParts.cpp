#include "ClassicalHallParts.h"

#include "ClassicalHallPlan.h"

namespace Musee::ClassicalHall
{
	FMuseeKey Key(const FString& Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, *Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(Origin, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	FVector Direction(double Deg)
	{
		const double Radians = FMath::DegreesToRadians(Deg);
		return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0);
	}

	FMuseeFrame Turned(const FVector& Origin, double Deg)
	{
		return FrameOfAxes(Origin, Direction(Deg), Direction(Deg + 90.0));
	}

	FVector2D Polar(double Radius, double Deg)
	{
		const double Radians = FMath::DegreesToRadians(Deg);
		return FVector2D(Radius * FMath::Cos(Radians), Radius * FMath::Sin(Radians));
	}

	FFormProfile Polygon(TConstArrayView<FVector2D> Points)
	{
		return FFormProfile::Polyline(Points, true);
	}

	FFormProfile Circle(const FVector2D& Centre, double Radius)
	{
		return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
	}

	FFormProfile Rect(double MinA, double MinB, double MaxA, double MaxB)
	{
		return FFormProfile::Rectangle(FVector2D(MinA, MinB), FVector2D(MaxA, MaxB));
	}

	FFormProfile RectOutward(double MinA, double MinB, double MaxA, double MaxB)
	{
		const FVector2D Corners[4] = {FVector2D(MinA, MinB), FVector2D(MinA, MaxB), FVector2D(MaxA, MaxB), FVector2D(MaxA, MinB)};
		return FFormProfile::Polyline(Corners, true);
	}

	FFormProfile AnnularSector(const FVector2D& Centre, double Inner, double Outer, double FromDeg, double ToDeg)
	{
		FPen Pen(Centre + Polar(Inner, FromDeg));
		Pen.Arc(Centre, Outer, FromDeg, ToDeg).Arc(Centre, Inner, ToDeg, FromDeg);
		return Pen.Closed();
	}

	FPen::FPen(const FVector2D& Start)
		: Profile(FFormProfile::From(Start))
		, Here(Start)
	{
	}

	FPen& FPen::To(double A, double B)
	{
		Here = FVector2D(A, B);
		Profile.LineTo(Here, EFormJoint::Auto);
		return *this;
	}

	FPen& FPen::Round(double A, double B, bool bConvex)
	{
		// A quarter circle on the chord: its centre half a chord off the chord's midpoint, on the side it doesn't bulge to.
		const FVector2D End(A, B);
		const FVector2D Chord = End - Here;
		const FVector2D Right = FVector2D(Chord.Y, -Chord.X).GetSafeNormal();
		const FVector2D Centre = 0.5 * (Here + End) - Right * (bConvex ? 0.5 : -0.5) * Chord.Size();
		const double From = FMath::RadiansToDegrees(FMath::Atan2(Here.Y - Centre.Y, Here.X - Centre.X));
		double To = FMath::RadiansToDegrees(FMath::Atan2(End.Y - Centre.Y, End.X - Centre.X));
		To += To - From > 180.0 ? -360.0 : (To - From < -180.0 ? 360.0 : 0.0);
		const double Radius = FVector2D::Distance(Here, Centre);
		Profile.ArcAbout(Centre, Radius, Radius, From, To, EFormJoint::Auto);
		Here = End;
		return *this;
	}

	FPen& FPen::Cyma(double A, double B, bool bConvexFirst)
	{
		const FVector2D Middle = 0.5 * (Here + FVector2D(A, B));
		Round(Middle.X, Middle.Y, bConvexFirst);
		return Round(A, B, !bConvexFirst);
	}

	FPen& FPen::Arc(const FVector2D& Centre, double Radius, double FromDeg, double ToDeg)
	{
		Profile.ArcAbout(Centre, Radius, Radius, FromDeg, ToDeg, EFormJoint::Auto);
		Here = Centre + Polar(Radius, ToDeg);
		return *this;
	}

	FPen& FPen::Strokes(TConstArrayView<Plan::FStroke> Written, const FVector2D& Offset)
	{
		for (const Plan::FStroke& Stroke : Written)
		{
			const FVector2D To = FVector2D(Stroke.A, Stroke.B) + Offset;
			switch (Stroke.Kind)
			{
			case Plan::EStroke::Line:
				if (!To.Equals(Here, UE_DOUBLE_KINDA_SMALL_NUMBER))
				{
					this->To(To.X, To.Y);
				}
				break;
			case Plan::EStroke::Convex:
			case Plan::EStroke::Concave:
				Round(To.X, To.Y, Stroke.Kind == Plan::EStroke::Convex);
				break;
			case Plan::EStroke::CymaReversa:
			case Plan::EStroke::CymaRecta:
				Cyma(To.X, To.Y, Stroke.Kind == Plan::EStroke::CymaReversa);
				break;
			}
		}
		return *this;
	}

	FFormProfile Written(TConstArrayView<Plan::FStroke> Strokes, const FVector2D& Offset)
	{
		FPen Pen(FVector2D(Strokes[0].A, Strokes[0].B) + Offset);
		return Pen.Strokes(Strokes.RightChop(1), Offset).Closed();
	}

	FPen& FPen::Ellipse(const FVector2D& Centre, double RadiusA, double RadiusB, double FromDeg, double ToDeg)
	{
		Profile.ArcAbout(Centre, RadiusA, RadiusB, FromDeg, ToDeg, EFormJoint::Auto);
		const double To = FMath::DegreesToRadians(ToDeg);
		Here = Centre + FVector2D(RadiusA * FMath::Cos(To), RadiusB * FMath::Sin(To));
		return *this;
	}

	FFormProfile FPen::Closed() const
	{
		FFormProfile Out = Profile;
		return Out.Close();
	}

	FFormProfile FPen::Open() const
	{
		return Profile;
	}

	FFormExtrusion Prism(FFormProfile Outline, double Depth, TArray<FFormProfile> Holes)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Depth;
		return Shape;
	}

	FFormDrawing PlanPrism(const FString& Name, const TCHAR* LocalSurface, FFormProfile Outline, TArray<FFormProfile> Holes, double Bottom,
		double Top, const Musee::Collision::FClass* Collision)
	{
		return Drawn(Name, LocalSurface, Prism(MoveTemp(Outline), Top - Bottom, MoveTemp(Holes)), Collision, FMuseeFrame(FVector(0.0, 0.0, Bottom)));
	}

	FFormDrawing PrismIn(const FString& Name, const TCHAR* LocalSurface, const FMuseeFrame& Frame, FFormProfile Outline, double Bottom,
		double Top, const Musee::Collision::FClass* Collision)
	{
		const FMuseeFrame Raised(Frame.PointToParent(FVector(0.0, 0.0, Bottom)), Frame.Rotation);
		return Drawn(Name, LocalSurface, Prism(MoveTemp(Outline), Top - Bottom), Collision, Raised);
	}

	FVector2D Barrel(double S, double Rho)
	{
		const double Angle = S / Plan::VaultRadius;
		return FVector2D(Rho * FMath::Sin(Angle), Plan::VaultAxisZ + Rho * FMath::Cos(Angle));
	}

	double BarrelDeg(double S)
	{
		return 90.0 - FMath::RadiansToDegrees(S / Plan::VaultRadius);
	}

	FMuseeFrame BarrelFrame(double Y)
	{
		return FrameOfAxes(FVector(0.0, Y, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, 0.0, 1.0));
	}

	double RosetteHeight(double Distance, double Around, double Radius)
	{
		const double Rise = Radius / Plan::RosetteDrawnRadius;
		const double Swell = Plan::RosettePetalBase + Plan::RosettePetalSwell * FMath::Abs(FMath::Cos(0.5 * Plan::RosettePetals * Around));
		const int32 Rows = int32(UE_ARRAY_COUNT(Plan::RosetteSection));
		double Outer = Radius * Plan::RosetteSection[0][0] * Swell;
		if (Distance > Outer)
		{
			return 0.0;
		}
		for (int32 Row = 1; Row < Rows; ++Row)
		{
			const double Inner = Radius * Plan::RosetteSection[Row][0] * (Row < Plan::RosettePetalRows ? Swell : 1.0);
			if (Distance >= Inner)
			{
				const double T = Outer > Inner ? (Outer - Distance) / (Outer - Inner) : 1.0;
				return Rise * FMath::Lerp(Plan::RosetteSection[Row - 1][1], Plan::RosetteSection[Row][1], T);
			}
			Outer = Inner;
		}
		return Rise * Plan::RosetteSection[Rows - 1][1];
	}

	FFormDrawing Rosette(const FString& Name, const TCHAR* LocalSurface, const FVector& Back, const FVector& Facing, const FVector& Across,
		double Radius)
	{
		const double Side = 2.0 * (Radius + Plan::RosetteGrid);
		const FVector Z = Facing.GetSafeNormal();
		const FVector X = (Across - Z * FVector::DotProduct(Across, Z)).GetSafeNormal();
		const FVector Y = FVector::CrossProduct(Z, X);
		const FVector Corner = Back - Z * Plan::RosetteBed - (X + Y) * (0.5 * Side);
		FFormRelief Relief;
		Relief.SizeMeters = FVector2D(Side, Side);
		Relief.GridMeters = Plan::RosetteGrid;
		Relief.BackMeters = Plan::RosetteBed;
		Relief.HeightMeters = [Side, Radius](double U, double V)
		{
			const FVector2D At(U - 0.5 * Side, V - 0.5 * Side);
			const double Height = RosetteHeight(At.Size(), FMath::Atan2(At.Y, At.X), Radius);
			return Height > 0.0 ? Height + Plan::RosetteBed : 0.0;
		};
		return Drawn(Name, LocalSurface, MoveTemp(Relief), nullptr, FrameOfAxes(Corner, X, Y));
	}

	FVector FNiche::Lateral() const
	{
		return FVector::CrossProduct(FVector::UpVector, In);
	}

	FMuseeFrame FNiche::PlanFrame() const
	{
		return FrameOfAxes(Origin, In, Lateral());
	}

	FMuseeFrame FNiche::WallFrame(double Depth) const
	{
		return FrameOfAxes(Origin + In * Depth, Lateral(), FVector::UpVector);
	}

	FString FNiche::AnchorLocal() const
	{
		return FString::Printf(TEXT("%s.niche.%s"), bTribune ? TEXT("tribune") : TEXT("gallery"), *Name);
	}

	double FNiche::BayHalf() const
	{
		return Radius + Plan::SurroundBand;
	}

	const TArray<FNiche>& Niches()
	{
		static const TArray<FNiche> All = []
		{
			TArray<FNiche> Out;
			for (const bool bEast : {true, false})
			{
				for (int32 Bay = 0; Bay < Plan::Bays; ++Bay)
				{
					FNiche& Niche = Out.AddDefaulted_GetRef();
					Niche.Name = FString::Printf(TEXT("%s%d"), bEast ? TEXT("e") : TEXT("w"), Bay + 1);
					const double Side = bEast ? 1.0 : -1.0;
					Niche.Origin = FVector(Side * Plan::GalleryHalf, Plan::BayCentres[Bay], 0.0);
					Niche.In = FVector(Side, 0.0, 0.0);
					Niche.Radius = Plan::GalleryNicheRadius;
					Niche.Springing = Plan::GalleryNicheSpringing;
				}
			}
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::TribuneNicheDeg)); ++Index)
			{
				FNiche& Niche = Out.AddDefaulted_GetRef();
				Niche.Name = Plan::TribuneNicheNames[Index];
				Niche.In = Direction(Plan::TribuneNicheDeg[Index]);
				Niche.Origin = Niche.In * Plan::TribuneRadius;
				Niche.Radius = Plan::TribuneNicheRadius;
				Niche.Springing = Plan::TribuneNicheSpringing;
				Niche.bTribune = true;
			}
			return Out;
		}();
		return All;
	}

	const FNiche* NicheOf(const TCHAR* AnchorLocal)
	{
		return Niches().FindByPredicate([AnchorLocal](const FNiche& Niche) { return Niche.AnchorLocal() == AnchorLocal; });
	}
}
