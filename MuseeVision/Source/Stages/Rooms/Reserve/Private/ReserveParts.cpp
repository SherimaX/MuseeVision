#include "ReserveParts.h"

#include "ReservePlan.h"

namespace Musee::Reserve
{
	FMuseeKey Key(const TCHAR* Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FFormExtrusion Block(const FVector& Min, const FVector& Max)
	{
		FFormExtrusion Shape;
		Shape.Outline = FFormProfile::Rectangle(FVector2D(Min.X, Min.Y), FVector2D(Max.X, Max.Y));
		Shape.DepthMeters = Max.Z - Min.Z;
		return Shape;
	}

	FFormProfile Round(double Diameter)
	{
		return FFormProfile::Ellipse(FVector2D::ZeroVector, 0.5 * Diameter, 0.5 * Diameter, 0.0, true);
	}

	FFormLathe Turned(TConstArrayView<FVector2D> Points, int32 Sides)
	{
		FFormLathe Shape;
		Shape.Section = FFormProfile::Polyline(Points, true);
		Shape.Sides = Sides;
		return Shape;
	}

	FFormLathe Cylinder(double Diameter, double From, double To)
	{
		const double R = 0.5 * Diameter;
		return Turned({FVector2D(0.0, From), FVector2D(R, From), FVector2D(R, To), FVector2D(0.0, To)});
	}

	FMuseeFrame AlongAxis(const FVector& Origin, const FVector& Direction)
	{
		return FMuseeFrame(Origin, FRotationMatrix::MakeFromZ(Direction.GetSafeNormal()).ToQuat());
	}

	double PierX(int32 Line)
	{
		return Plan::FirstPierX + Plan::PierPitch * Line;
	}

	double BayFrom(int32 Bay)
	{
		return Bay == 0 ? -Plan::InnerX : PierX(Bay - 1) + Plan::BandHalf;
	}

	double BayTo(int32 Bay)
	{
		return Bay == Plan::Bays - 1 ? Plan::InnerX : PierX(Bay) - Plan::BandHalf;
	}

	double BayCentre(int32 Bay)
	{
		return 0.5 * (BayFrom(Bay) + BayTo(Bay));
	}

	double BayHalf(int32 Bay)
	{
		return 0.5 * (BayTo(Bay) - BayFrom(Bay));
	}

	TConstArrayView<FRow> Rows()
	{
		static const FRow Three[] = {{TEXT("north"), -Plan::AisleMiddle, Plan::AisleHalf, Plan::AisleRise}, {TEXT("nave"), 0.0, Plan::NaveHalf, Plan::NaveRise},
			{TEXT("south"), Plan::AisleMiddle, Plan::AisleHalf, Plan::AisleRise}};
		return Three;
	}

	namespace
	{
		/** The height of a semi-elliptical barrel of a rise over a span, at a fraction of its half-span from its middle. */
		double Barrel(double Rise, double Fraction)
		{
			return Rise * FMath::Sqrt(FMath::Max(0.0, 1.0 - Fraction * Fraction));
		}
	}

	double Soffit(double X, double Y)
	{
		const FRow* Row = nullptr;
		for (const FRow& Each : Rows())
		{
			Row = FMath::Abs(Y - Each.Middle) <= Each.Half ? &Each : Row;
		}
		int32 Bay = INDEX_NONE;
		for (int32 Each = 0; Each < Plan::Bays; ++Each)
		{
			Bay = X >= BayFrom(Each) && X <= BayTo(Each) ? Each : Bay;
		}
		const double V = Row ? (Y - Row->Middle) / Row->Half : 0.0;
		const double U = Bay != INDEX_NONE ? (X - BayCentre(Bay)) / BayHalf(Bay) : 0.0;
		if (Row && Bay != INDEX_NONE)
		{
			return Plan::Springing + Plan::WebLift + FMath::Max(Barrel(Row->Rise, V), Barrel(Row->Rise, U));
		}
		if (Row)
		{
			return Plan::Springing + Barrel(Row->Rise, V);
		}
		if (Bay != INDEX_NONE)
		{
			return Plan::Springing + Barrel(Plan::ArcadeRise, U);
		}
		return Plan::Springing;
	}

	double SeatOver(double X, double Y, double Radius)
	{
		constexpr int32 RimStations = 16;
		double Highest = Soffit(X, Y);
		for (int32 Station = 0; Station < RimStations; ++Station)
		{
			const double Angle = 2.0 * UE_DOUBLE_PI * Station / RimStations;
			Highest = FMath::Max(Highest, Soffit(X + Radius * FMath::Cos(Angle), Y + Radius * FMath::Sin(Angle)));
		}
		return Highest;
	}

	FMuseeKey FRack::Body() const
	{
		return Key(*FString::Printf(TEXT("screen-%s"), *Name));
	}

	FString FRack::Piece(const TCHAR* Part) const
	{
		return FString::Printf(TEXT("screen-%s.%s"), *Name, Part);
	}

	double FRack::HomeY() const
	{
		return Side * Plan::RackHomeY;
	}

	TArray<FRack> Racks()
	{
		// North: A–F in bays 1 and 2, then N07–N18; south: S01–S18, three a bay, west to east.
		static const TCHAR* const Lettered[] = {TEXT("a"), TEXT("b"), TEXT("c"), TEXT("d"), TEXT("e"), TEXT("f")};
		TArray<FRack> Out;
		for (const int32 Side : {-1, 1})
		{
			for (int32 Bay = 0; Bay < Plan::RackBays; ++Bay)
			{
				for (int32 Place = 0; Place < 3; ++Place)
				{
					const int32 Index = 3 * Bay + Place;
					FRack& Rack = Out.AddDefaulted_GetRef();
					Rack.Name = Side < 0 ? (Index < 6 ? FString(Lettered[Index]) : FString::Printf(TEXT("n%02d"), Index + 1))
						: FString::Printf(TEXT("s%02d"), Index + 1);
					Rack.X = Plan::FirstPitchCentre + Plan::PierPitch * Bay + Plan::RackSpacing * (Place - 1);
					Rack.Side = Side;
				}
			}
		}
		return Out;
	}
}
