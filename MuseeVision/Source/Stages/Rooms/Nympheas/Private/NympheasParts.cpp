#include "NympheasParts.h"

#include "NympheasPlan.h"

namespace Musee::Nympheas
{
	FMuseeKey Key(const TCHAR* Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	const FOvalCurve& Oval()
	{
		static const FOvalCurve Curve(Plan::OvalA, Plan::OvalB);
		return Curve;
	}

	const FOvalCurve& Pond()
	{
		static const FOvalCurve Curve(Plan::PondA, Plan::PondB);
		return Curve;
	}

	FFormExtrusion Block(const FVector& Min, const FVector& Max)
	{
		FFormExtrusion Shape;
		Shape.Outline = FFormProfile::Rectangle(FVector2D(Min.X, Min.Y), FVector2D(Max.X, Max.Y));
		Shape.DepthMeters = Max.Z - Min.Z;
		return Shape;
	}

	FFormExtrusion Slab(FFormProfile Outline, TArray<FFormProfile> Holes, double Bottom, double Top)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Top - Bottom;
		return Shape;
	}

	double JambT()
	{
		return FMath::Asin(0.5 * Plan::DoorWidth / Plan::OvalB);
	}

	TArray<FVector2D> FootprintOutline()
	{
		const double Half = Plan::BlockHalfWidth;
		const double From = Oval().TWhereY(Half, Plan::WallMeters, 0.0, UE_DOUBLE_HALF_PI);
		const double To = 2.0 * UE_DOUBLE_PI - From;
		// The oval's stations are even steps of its parameter; the stretch outside the block takes its share of them.
		const int32 Stations = FMath::CeilToInt32(Plan::OvalStations * (To - From) / (2.0 * UE_DOUBLE_PI));
		TArray<FVector2D> Outline = {FVector2D(Plan::PortFaceX, Half)};
		Outline.Append(Oval().Stations(Plan::WallMeters, From, To, Stations));
		Outline.Add(FVector2D(Plan::PortFaceX, -Half));
		return Outline;
	}

	FFormProfile Circle(const FVector2D& Centre, double Radius)
	{
		return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
	}

	TArray<FVector2D> MirroredInY(TConstArrayView<FVector2D> Points)
	{
		TArray<FVector2D> Out;
		for (int32 Index = Points.Num() - 1; Index >= 0; --Index)
		{
			Out.Add(FVector2D(Points[Index].X, -Points[Index].Y));
		}
		return Out;
	}
}
