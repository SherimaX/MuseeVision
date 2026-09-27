#include "HallOfLightEnds.h"

#include "HallOfLightPlan.h"

namespace Musee::HallOfLight
{
	namespace
	{
		/** Two x closer than a micrometre are one: a square end that already lies on its face needs no wedge. */
		constexpr double SameMeters = 1.0e-6;

		/**
		 * The wedge between a face and the square end that just clears it, over [Y0, Y1] on one side of the axis: from the
		 * point where the end touches the face, along the face to the far edge, and back square to the end.
		 */
		FFormProfile Wedge(const FEndFace& Face, double EndX, double Y0, double Y1)
		{
			const bool bNearIsY0 = FMath::Abs(Y0) < FMath::Abs(Y1);
			const double Near = bNearIsY0 ? Y0 : Y1;
			const double Far = bNearIsY0 ? Y1 : Y0;
			FFormProfile Outline = FFormProfile::From(FVector2D(EndX, Near));
			AddFaceArc(Outline, Face, Near, Far);
			Outline.LineTo(FVector2D(EndX, Far), EFormJoint::Crisp);
			return Outline.Close();
		}
	}

	void AddFaceArc(FFormProfile& Profile, const FEndFace& Face, double FromY, double ToY)
	{
		// The arc is divided where the port's rough opening meets it: the joint's threshold and lining meet the face across
		// the rough opening, so there the two are divided alike and meet vertex for vertex (Form's stations are a function
		// of the arc and its ends).
		const double Rough = 0.5 * (Face.End == EHallEnd::West ? Plan::WestClearWidth : Plan::EastClearWidth) + Plan::RoughAllowance;
		TArray<double> Ys = {FromY};
		for (const double Edge : FromY < ToY ? TArray<double>{-Rough, Rough} : TArray<double>{Rough, -Rough})
		{
			if (Edge > FMath::Min(FromY, ToY) && Edge < FMath::Max(FromY, ToY))
			{
				Ys.Add(Edge);
			}
		}
		Ys.Add(ToY);
		for (int32 Index = 0; Index + 1 < Ys.Num(); ++Index)
		{
			Profile.ArcAbout(Face.Centre(), Face.Radius, Face.Radius, Face.AngleDegrees(Ys[Index]), Face.AngleDegrees(Ys[Index + 1]),
				Index == 0 ? EFormJoint::Crisp : EFormJoint::Smooth);
		}
	}

	FVector2D FEndFace::Centre() const
	{
		const double Axis = End == EHallEnd::West ? Plan::RotundaAxisX : Plan::AtriumAxisX;
		return FVector2D(Axis + Inward() * Shift, 0.0);
	}

	double FEndFace::X(double Y) const
	{
		return Centre().X + Inward() * FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Y * Y));
	}

	double FEndFace::AngleDegrees(double Y) const
	{
		const double Off = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Y / Radius, -1.0, 1.0)));
		return End == EHallEnd::West ? Off : 180.0 - Off;
	}

	FEndFace InnerFace(EHallEnd End, double Shift)
	{
		const FEndFace Port = PortFace(End);
		return FEndFace{End, Port.Radius + Plan::LiningMeters, Shift};
	}

	FEndFace PortFace(EHallEnd End)
	{
		return FEndFace{End, End == EHallEnd::West ? Plan::WestFaceRadius : Plan::EastFaceRadius, 0.0};
	}

	FRunLimit FRunLimit::OnFace(const FEndFace& InFace)
	{
		FRunLimit Limit;
		Limit.Face = InFace;
		return Limit;
	}

	FRunLimit FRunLimit::OnPlane(double X)
	{
		FRunLimit Limit;
		Limit.PlaneX = X;
		return Limit;
	}

	double FRunLimit::Innermost(double Y0, double Y1, double Towards) const
	{
		if (!Face)
		{
			return PlaneX;
		}
		// A face's x is extreme at the ends of the span, or on the axis where the span crosses it.
		double Best = Towards * FMath::Max(Towards * X(Y0), Towards * X(Y1));
		if (Y0 < 0.0 && Y1 > 0.0)
		{
			Best = Towards * FMath::Max(Towards * Best, Towards * X(0.0));
		}
		return Best;
	}

	FFormProfile PlanOutline(const FRunLimit& West, const FRunLimit& East, double Y0, double Y1)
	{
		FFormProfile Outline = FFormProfile::From(FVector2D(West.X(Y1), Y1));
		if (West.Face)
		{
			AddFaceArc(Outline, *West.Face, Y1, Y0);
		}
		else
		{
			Outline.LineTo(FVector2D(West.PlaneX, Y0), EFormJoint::Crisp);
		}
		Outline.LineTo(FVector2D(East.X(Y0), Y0), EFormJoint::Crisp);
		if (East.Face)
		{
			AddFaceArc(Outline, *East.Face, Y0, Y1);
		}
		else
		{
			Outline.LineTo(FVector2D(East.PlaneX, Y1), EFormJoint::Crisp);
		}
		return Outline.Close();
	}

	FFormDrawing PlanRun(const FString& Name, FName InSurface, const FRunLimit& West, const FRunLimit& East, double Y0, double Y1,
		double Bottom, double Top, const Musee::Collision::FClass* Collision)
	{
		FFormExtrusion Shape;
		Shape.Outline = PlanOutline(West, East, FMath::Min(Y0, Y1), FMath::Max(Y0, Y1));
		Shape.DepthMeters = Top - Bottom;
		return Drawn(Name, InSurface, MoveTemp(Shape), Collision, FMuseeFrame(FVector(0.0, 0.0, Bottom)));
	}

	FMuseeFrame AlongHall(double X)
	{
		return FrameOfAxes(FVector(X, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector::UpVector);
	}

	FFormExtrusion SectionPushed(TConstArrayView<FVector2D> Section, double X0, double X1)
	{
		FFormExtrusion Shape;
		Shape.Outline = FFormProfile::Polygon(Section);
		Shape.DepthMeters = X1 - X0;
		return Shape;
	}

	void SectionRun(TArray<FFormDrawing>& Out, const FString& Name, FName InSurface, TConstArrayView<FVector2D> Section, const FRunLimit& West,
		const FRunLimit& East, const Musee::Collision::FClass* Collision)
	{
		FBox2D Bounds(ForceInit);
		for (const FVector2D& Point : Section)
		{
			Bounds += Point;
		}
		const double Y0 = Bounds.Min.X;
		const double Y1 = Bounds.Max.X;
		// A member runs beside a glass wall, clear of the axis, so each face is nearest the hall's middle at one edge.
		check(Y0 * Y1 > 0.0);
		const double WestEnd = West.Innermost(Y0, Y1, 1.0);
		const double EastEnd = East.Innermost(Y0, Y1, -1.0);
		Out.Add(Drawn(Name, InSurface, SectionPushed(Section, WestEnd, EastEnd), Collision, AlongHall(WestEnd)));
		const FMuseeFrame Bed(FVector(0.0, 0.0, Bounds.Min.Y));
		const double Height = Bounds.Max.Y - Bounds.Min.Y;
		for (const TPair<const FRunLimit*, double>& End : {TPair<const FRunLimit*, double>(&West, WestEnd), TPair<const FRunLimit*, double>(&East, EastEnd)})
		{
			const FRunLimit& Limit = *End.Key;
			if (!Limit.Face || FMath::Abs(Limit.X(Y0) - Limit.X(Y1)) < SameMeters)
			{
				continue;
			}
			FFormExtrusion Shape;
			Shape.Outline = Wedge(*Limit.Face, End.Value, Y0, Y1);
			Shape.DepthMeters = Height;
			const TCHAR* Which = &Limit == &West ? TEXT("west") : TEXT("east");
			Out.Add(Drawn(FString::Printf(TEXT("%s.%s"), *Name, Which), InSurface, MoveTemp(Shape), Collision, Bed));
		}
	}
}
