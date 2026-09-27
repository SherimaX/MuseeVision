#include "ChenghuaiParts.h"

#include "Algo/BinarySearch.h"
#include "Algo/Reverse.h"

namespace Musee::Chenghuai
{
	namespace
	{
		/** Two stations nearer than a tenth of a millimetre are one: far below any drawn detail, far above rounding. */
		constexpr double StationMergeMeters = 1.0e-4;

		/** Two corners nearer than a micrometre are one (Form's weld tolerance); a corner on a straight run is none. */
		constexpr double SameCorner = 1.0e-6;

		/**
		 * A loop as its corners: each point nearer than a micrometre to the last kept dropped, and each point on a straight run
		 * between its neighbours, so no side of the solid is drawn with no length or no turn.
		 */
		TArray<FVector2D> Corners(TConstArrayView<FVector2D> Loop)
		{
			TArray<FVector2D> Kept;
			for (const FVector2D& Point : Loop)
			{
				if (Kept.IsEmpty() || FVector2D::Distance(Point, Kept.Last()) > SameCorner)
				{
					Kept.Add(Point);
				}
			}
			while (Kept.Num() > 1 && FVector2D::Distance(Kept[0], Kept.Last()) <= SameCorner)
			{
				Kept.Pop();
			}
			TArray<FVector2D> Out;
			for (int32 Index = 0; Index < Kept.Num(); ++Index)
			{
				const FVector2D& Prev = Kept[(Index + Kept.Num() - 1) % Kept.Num()];
				const FVector2D& Next = Kept[(Index + 1) % Kept.Num()];
				const FVector2D In = (Kept[Index] - Prev).GetSafeNormal();
				const FVector2D Onward = (Next - Kept[Index]).GetSafeNormal();
				if (FMath::Abs(FVector2D::CrossProduct(In, Onward)) > SameCorner || FVector2D::DotProduct(In, Onward) < 0.0)
				{
					Out.Add(Kept[Index]);
				}
			}
			return Out;
		}

		/** The holes as profiles; a loop left with no area once its corners are found is no hole. */
		TArray<FFormProfile> Profiles(TConstArrayView<TArray<FVector2D>> Loops)
		{
			TArray<FFormProfile> Out;
			for (const TArray<FVector2D>& Loop : Loops)
			{
				const TArray<FVector2D> Kept = Corners(Loop);
				if (Kept.Num() >= 3 && FMath::Abs(SignedArea(Kept)) > SameCorner * SameCorner)
				{
					Out.Add(Polygon(Kept));
				}
			}
			return Out;
		}

		FFormExtrusion Extrusion(TConstArrayView<FVector2D> Outline, TConstArrayView<TArray<FVector2D>> Holes, double Depth)
		{
			FFormExtrusion Shape;
			Shape.Outline = Polygon(Corners(Outline));
			Shape.Holes = Profiles(Holes);
			Shape.DepthMeters = Depth;
			return Shape;
		}

		/** A unit direction square to Axis, for a frame whose z is Axis: the horizontal one where there is one. */
		FVector SquareTo(const FVector& Axis)
		{
			const FVector Across = FVector::CrossProduct(FVector::UpVector, Axis);
			return Across.SizeSquared() > 1.0e-12 ? Across.GetSafeNormal() : FVector::ForwardVector;
		}
	}

	FMuseeKey Key(const FString& Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, *Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FFormProfile Section(TConstArrayView<FSectionPoint> Points, bool bClosed, double ScaleA)
	{
		FFormProfile Profile = FFormProfile::From(Points.IsEmpty() ? FVector2D::ZeroVector : FVector2D(ScaleA * Points[0].A, Points[0].B));
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			// The joint at a point is how the element leaving it meets the one arriving: smooth where the sheet marks it.
			Profile.LineTo(FVector2D(ScaleA * Points[Index].A, Points[Index].B), Points[Index - 1].bSmooth ? EFormJoint::Smooth : EFormJoint::Crisp);
		}
		Profile.Closed = bClosed;
		return Profile;
	}

	FFormProfile Polygon(TConstArrayView<FVector2D> Points)
	{
		return FFormProfile::Polyline(Points, true);
	}

	FFormProfile SmoothPath(TConstArrayView<FVector2D> Points)
	{
		FFormProfile Path = FFormProfile::From(Points.IsEmpty() ? FVector2D::ZeroVector : Points[0]);
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Path.LineTo(Points[Index], EFormJoint::Smooth);
		}
		return Path;
	}

	FFormProfile Circle(const FVector2D& Centre, double Radius)
	{
		return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
	}

	TArray<FVector2D> Rectangle(double A0, double B0, double A1, double B1)
	{
		return {FVector2D(A0, B0), FVector2D(A1, B0), FVector2D(A1, B1), FVector2D(A0, B1)};
	}

	TArray<FVector2D> RegularPolygon(const FVector2D& Centre, double Radius, int32 Sides, double PhaseDegrees)
	{
		TArray<FVector2D> Points;
		for (int32 Corner = 0; Corner < Sides; ++Corner)
		{
			const double Angle = FMath::DegreesToRadians(PhaseDegrees + 360.0 * Corner / Sides);
			Points.Add(Centre + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
		return Points;
	}

	FName CollisionOf(EHit Hit)
	{
		switch (Hit)
		{
		case EHit::Solid: return Musee::Collision::Solid.Profile();
		case EHit::Walkable: return Musee::Collision::Walkable.Profile();
		default: return NAME_None;
		}
	}

	FFormDrawing& Block(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F, double U0,
		double U1, double V0, double V1, double Z0, double Z1, EHit Hit)
	{
		return Raised(Out, Name, LocalSurface, F, Rectangle(U0, V0, U1, V1), {}, Z0, Z1, Hit);
	}

	FFormDrawing& AlongU(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> SectionVZ, TConstArrayView<TArray<FVector2D>> Holes, double U0, double U1, EHit Hit)
	{
		// The drawing's z is the frame's u times its hand, so the run starts from the end that way round.
		const double Start = F.Hand() > 0.0 ? FMath::Min(U0, U1) : FMath::Max(U0, U1);
		return Draw(Out, Name, LocalSurface, Extrusion(SectionVZ, Holes, FMath::Abs(U1 - U0)), Hit, F.AlongU(Start));
	}

	FFormDrawing& AlongV(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> SectionUZ, TConstArrayView<TArray<FVector2D>> Holes, double V0, double V1, EHit Hit)
	{
		const double Start = F.Hand() > 0.0 ? FMath::Max(V0, V1) : FMath::Min(V0, V1);
		return Draw(Out, Name, LocalSurface, Extrusion(SectionUZ, Holes, FMath::Abs(V1 - V0)), Hit, F.AlongV(Start));
	}

	FFormDrawing& Raised(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> OutlineUV, TConstArrayView<TArray<FVector2D>> Holes, double Z0, double Z1, EHit Hit)
	{
		const double Hand = F.Hand();
		auto Flip = [Hand](TConstArrayView<FVector2D> Loop)
		{
			TArray<FVector2D> Flat;
			for (const FVector2D& Point : Loop)
			{
				Flat.Add(FVector2D(Point.X, Hand * Point.Y));
			}
			return Flat;
		};
		TArray<TArray<FVector2D>> FlatHoles;
		for (const TArray<FVector2D>& Hole : Holes)
		{
			FlatHoles.Add(Flip(Hole));
		}
		return Draw(Out, Name, LocalSurface, Extrusion(Flip(OutlineUV), FlatHoles, Z1 - Z0), Hit, F.Flat(Z0));
	}

	FFormDrawing& Turned(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& Foot,
		FFormProfile Section, EHit Hit)
	{
		FFormLathe Shape;
		Shape.Section = MoveTemp(Section);
		return Draw(Out, Name, LocalSurface, MoveTemp(Shape), Hit, FMuseeFrame(Foot));
	}

	FFormDrawing& TurnedAbout(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& Foot,
		const FVector& Axis, FFormProfile Section, EHit Hit)
	{
		const FVector Z = Axis.GetSafeNormal();
		const FVector X = SquareTo(Z);
		FFormLathe Shape;
		Shape.Section = MoveTemp(Section);
		return Draw(Out, Name, LocalSurface, MoveTemp(Shape), Hit, FrameOfAxes(Foot, X, FVector::CrossProduct(Z, X)));
	}

	FFormDrawing& Rod(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& A, const FVector& B,
		double Radius, EHit Hit)
	{
		const FVector Z = (B - A).GetSafeNormal();
		const FVector X = SquareTo(Z);
		FFormExtrusion Shape;
		Shape.Outline = Circle(FVector2D::ZeroVector, Radius);
		Shape.DepthMeters = FVector::Distance(A, B);
		return Draw(Out, Name, LocalSurface, MoveTemp(Shape), Hit, FrameOfAxes(A, X, FVector::CrossProduct(Z, X)));
	}

	FFormDrawing& Bar(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& A, const FVector& B,
		const FVector& Across, double Width, double Height, EHit Hit)
	{
		const FVector Z = (B - A).GetSafeNormal();
		const FVector X = (Across - Z * FVector::DotProduct(Across, Z)).GetSafeNormal();
		const TArray<FVector2D> Outline = Rectangle(-0.5 * Width, -0.5 * Height, 0.5 * Width, 0.5 * Height);
		return Draw(Out, Name, LocalSurface, Extrusion(Outline, {}, FVector::Distance(A, B)), Hit,
			FrameOfAxes(A, X, FVector::CrossProduct(Z, X)));
	}

	FFormDrawing& ReliefAlongU(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> CurveVZ, double UFrom, double UTo, double Grid, double Back, TFunction<double(double S, double U)> Height)
	{
		// The drawing's z is u times the hand: the relief's V is that, and its band runs between the two ends so taken.
		const double Hand = F.Hand();
		FFormRelief Shape;
		Shape.Path = SmoothPath(CurveVZ);
		Shape.BottomMeters = FMath::Min(Hand * UFrom, Hand * UTo);
		Shape.TopMeters = FMath::Max(Hand * UFrom, Hand * UTo);
		Shape.GridMeters = Grid;
		Shape.BackMeters = Back;
		Shape.HeightMeters = [Height = MoveTemp(Height), Hand](double S, double V) { return Height(S, Hand * V); };
		return Draw(Out, Name, LocalSurface, MoveTemp(Shape), EHit::None, F.AlongU(0.0));
	}

	FFormDrawing& ReliefAlongV(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> CurveUZ, double VFrom, double VTo, double Grid, double Back, TFunction<double(double S, double V)> Height)
	{
		const double Depth = -F.Hand();
		FFormRelief Shape;
		Shape.Path = SmoothPath(CurveUZ);
		Shape.BottomMeters = FMath::Min(Depth * VFrom, Depth * VTo);
		Shape.TopMeters = FMath::Max(Depth * VFrom, Depth * VTo);
		Shape.GridMeters = Grid;
		Shape.BackMeters = Back;
		Shape.HeightMeters = [Height = MoveTemp(Height), Depth](double S, double V) { return Height(S, Depth * V); };
		return Draw(Out, Name, LocalSurface, MoveTemp(Shape), EHit::None, F.AlongV(0.0));
	}

	FFormDrawing& ReliefOnPlan(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		double U0, double V0, const FVector2D& Size, double Z, double Grid, double Back, TFunction<double(double U, double V)> Height)
	{
		// The plane's y runs along v times the hand: a mirrored frame starts the relief at the far side of its v.
		const double Hand = F.Hand();
		const double VStart = Hand > 0.0 ? V0 : V0 + Size.Y;
		FFormRelief Shape;
		Shape.SizeMeters = Size;
		Shape.GridMeters = Grid;
		Shape.BackMeters = Back;
		Shape.HeightMeters = [Height = MoveTemp(Height), U0, VStart, Hand](double X, double Y) { return Height(U0 + X, VStart + Hand * Y); };
		const FMuseeFrame Plane = FrameOfAxes(F.At(U0, VStart, Z), F.U3(), F.V3() * Hand);
		return Draw(Out, Name, LocalSurface, MoveTemp(Shape), EHit::None, Plane);
	}

	double TopAt(TConstArrayView<FSectionPoint> Top, double A)
	{
		for (int32 Index = 0; Index + 1 < Top.Num(); ++Index)
		{
			const FSectionPoint& P = Top[Index];
			const FSectionPoint& Q = Top[Index + 1];
			if (A >= P.A && A <= Q.A)
			{
				return FMath::Lerp(P.B, Q.B, Q.A > P.A ? (A - P.A) / (Q.A - P.A) : 0.0);
			}
		}
		return 0.0;
	}

	FMeasuredLine::FMeasuredLine(TArray<FVector2D> InPoints)
		: Points(MoveTemp(InPoints))
	{
		double Sum = 0.0;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			Sum += Index > 0 ? FVector2D::Distance(Points[Index], Points[Index - 1]) : 0.0;
			Lengths.Add(Sum);
		}
	}

	FVector2D FMeasuredLine::At(double Distance) const
	{
		if (Points.Num() < 2)
		{
			return Points.IsEmpty() ? FVector2D::ZeroVector : Points[0];
		}
		const int32 After = FMath::Clamp(Algo::LowerBound(Lengths, Distance), 1, Points.Num() - 1);
		const double Span = Lengths[After] - Lengths[After - 1];
		const double Alpha = Span > 0.0 ? FMath::Clamp((Distance - Lengths[After - 1]) / Span, 0.0, 1.0) : 0.0;
		return FMath::Lerp(Points[After - 1], Points[After], Alpha);
	}

	double SignedArea(TConstArrayView<FVector2D> Loop)
	{
		double Twice = 0.0;
		for (int32 Index = 0; Index < Loop.Num(); ++Index)
		{
			Twice += FVector2D::CrossProduct(Loop[Index], Loop[(Index + 1) % Loop.Num()]);
		}
		return 0.5 * Twice;
	}

	bool Contains(TConstArrayView<FVector2D> Loop, const FVector2D& Point)
	{
		bool bInside = false;
		for (int32 I = 0, J = Loop.Num() - 1; I < Loop.Num(); J = I++)
		{
			const FVector2D& A = Loop[I];
			const FVector2D& B = Loop[J];
			if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X)
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}

	TArray<TArray<FVector2D>> OpenRegions(double A0, double A1, double B0, double B1, TConstArrayView<FBox2D> Covered)
	{
		auto Unique = [](TArray<double> Values)
		{
			Values.Sort();
			TArray<double> Out;
			for (const double Value : Values)
			{
				if (Out.IsEmpty() || Value - Out.Last() > StationMergeMeters * 0.01)
				{
					Out.Add(Value);
				}
			}
			return Out;
		};
		TArray<double> Xs = {A0, A1};
		TArray<double> Ys = {B0, B1};
		for (const FBox2D& Box : Covered)
		{
			Xs.Append({FMath::Clamp(Box.Min.X, A0, A1), FMath::Clamp(Box.Max.X, A0, A1)});
			Ys.Append({FMath::Clamp(Box.Min.Y, B0, B1), FMath::Clamp(Box.Max.Y, B0, B1)});
		}
		Xs = Unique(MoveTemp(Xs));
		Ys = Unique(MoveTemp(Ys));
		const int32 NX = Xs.Num() - 1;
		const int32 NY = Ys.Num() - 1;
		TArray<bool> Open;
		Open.Init(true, NX * NY);
		for (int32 J = 0; J < NY; ++J)
		{
			for (int32 I = 0; I < NX; ++I)
			{
				const FVector2D Mid(0.5 * (Xs[I] + Xs[I + 1]), 0.5 * (Ys[J] + Ys[J + 1]));
				Open[J * NX + I] = !Covered.ContainsByPredicate([&Mid](const FBox2D& Box)
				{
					return Mid.X > Box.Min.X && Mid.X < Box.Max.X && Mid.Y > Box.Min.Y && Mid.Y < Box.Max.Y;
				});
			}
		}
		auto IsOpen = [&](int32 I, int32 J) { return I >= 0 && J >= 0 && I < NX && J < NY && Open[J * NX + I]; };
		auto Vertex = [NX](int32 I, int32 J) { return J * (NX + 1) + I; };
		// Every boundary edge of the open cells, run with the open cell on its left.
		TArray<TPair<int32, int32>> Edges;
		for (int32 J = 0; J < NY; ++J)
		{
			for (int32 I = 0; I < NX; ++I)
			{
				if (!IsOpen(I, J))
				{
					continue;
				}
				if (!IsOpen(I, J - 1)) { Edges.Add({Vertex(I, J), Vertex(I + 1, J)}); }
				if (!IsOpen(I + 1, J)) { Edges.Add({Vertex(I + 1, J), Vertex(I + 1, J + 1)}); }
				if (!IsOpen(I, J + 1)) { Edges.Add({Vertex(I + 1, J + 1), Vertex(I, J + 1)}); }
				if (!IsOpen(I - 1, J)) { Edges.Add({Vertex(I, J + 1), Vertex(I, J)}); }
			}
		}
		TMultiMap<int32, int32> From;
		for (int32 Index = 0; Index < Edges.Num(); ++Index)
		{
			From.Add(Edges[Index].Key, Index);
		}
		TArray<bool> Used;
		Used.Init(false, Edges.Num());
		TArray<TArray<FVector2D>> Loops;
		for (int32 Start = 0; Start < Edges.Num(); ++Start)
		{
			TArray<FVector2D> Loop;
			for (int32 Edge = Start; Edge != INDEX_NONE && !Used[Edge];)
			{
				Used[Edge] = true;
				const int32 At = Edges[Edge].Key;
				Loop.Add(FVector2D(Xs[At % (NX + 1)], Ys[At / (NX + 1)]));
				TArray<int32> Next;
				From.MultiFind(Edges[Edge].Value, Next);
				Edge = INDEX_NONE;
				for (const int32 Candidate : Next)
				{
					Edge = Used[Candidate] ? Edge : Candidate;
				}
			}
			// Only the corners: drop the points on a straight run.
			TArray<FVector2D> Corners;
			for (int32 Index = 0; Index < Loop.Num(); ++Index)
			{
				const FVector2D& Prev = Loop[(Index + Loop.Num() - 1) % Loop.Num()];
				const FVector2D& Next = Loop[(Index + 1) % Loop.Num()];
				if (FMath::Abs(FVector2D::CrossProduct(Loop[Index] - Prev, Next - Loop[Index])) > 1.0e-12)
				{
					Corners.Add(Loop[Index]);
				}
			}
			if (Corners.Num() >= 3)
			{
				Loops.Add(MoveTemp(Corners));
			}
		}
		return Loops;
	}

	void RaisedRegions(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<TArray<FVector2D>> Loops, double Z0, double Z1, EHit Hit)
	{
		int32 Count = 0;
		for (const TArray<FVector2D>& Outline : Loops)
		{
			if (SignedArea(Outline) <= 0.0)
			{
				continue;
			}
			TArray<TArray<FVector2D>> Islands;
			for (const TArray<FVector2D>& Island : Loops)
			{
				if (SignedArea(Island) < 0.0 && Contains(Outline, Island[0]))
				{
					Islands.Add(Island);
				}
			}
			Raised(Out, Loops.Num() > 1 ? FString::Printf(TEXT("%s.%d"), *Name, ++Count) : Name, LocalSurface, F, Outline, Islands, Z0, Z1, Hit);
		}
	}

	TArray<double> Stations(double From, double To, double Step, TConstArrayView<double> Knots)
	{
		TArray<double> All;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(To - From) / Step - StationMergeMeters));
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			All.Add(FMath::Lerp(From, To, double(Index) / Steps));
		}
		for (const double Knot : Knots)
		{
			if (Knot > FMath::Min(From, To) && Knot < FMath::Max(From, To))
			{
				All.Add(Knot);
			}
		}
		All.Sort();
		if (To < From)
		{
			Algo::Reverse(All);
		}
		TArray<double> Clean;
		for (const double Station : All)
		{
			if (Clean.IsEmpty() || FMath::Abs(Station - Clean.Last()) > StationMergeMeters)
			{
				Clean.Add(Station);
			}
		}
		return Clean;
	}
}
