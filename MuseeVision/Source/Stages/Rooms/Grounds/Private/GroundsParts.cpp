#include "GroundsParts.h"

namespace Musee::Grounds
{
	FMuseeKey Key(const FString& Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, *Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FMuseeFrame FrameOfAxes(const FVector& OriginMeters, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(OriginMeters, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	FFormProfile Polygon(TConstArrayView<FVector2D> Points)
	{
		return FFormProfile::Polyline(Points, true);
	}

	FFormProfile Section(TConstArrayView<Plan::FPlanPoint> Points)
	{
		TArray<FVector2D> Corners;
		for (const Plan::FPlanPoint& Point : Points)
		{
			Corners.Add(At(Point));
		}
		return Polygon(Corners);
	}

	FFormProfile Rectangle(const FVector2D& Min, const FVector2D& Max)
	{
		return FFormProfile::Rectangle(Min, Max);
	}

	FFormProfile Circle(const FVector2D& Centre, double Radius)
	{
		return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
	}

	FVector2D OnCircle(const FVector2D& Centre, double Radius, double Degrees)
	{
		const double Radians = FMath::DegreesToRadians(Degrees);
		return Centre + Radius * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
	}

	double AngleOf(const FVector2D& Centre, const FVector2D& Point)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(Point.Y - Centre.Y, Point.X - Centre.X));
	}

	FFormDrawing Laid(const FString& Name, const TCHAR* LocalSurface, FFormProfile Outline, TArray<FFormProfile> Holes, double Bottom,
		double Top, const Musee::Collision::FClass* Collision)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Top - Bottom;
		return Drawn(Name, LocalSurface, MoveTemp(Shape), Collision, FMuseeFrame(FVector(0.0, 0.0, Bottom)));
	}

	TArray<FFormProfile> LampBasesWithin(const FBox2D& Region)
	{
		TArray<FFormProfile> Bases;
		for (const Plan::FPlanPoint& Lamp : Plan::Lamps)
		{
			if (Region.IsInside(At(Lamp)))
			{
				Bases.Add(Circle(At(Lamp), Plan::PostBaseRadius));
			}
		}
		return Bases;
	}
}
