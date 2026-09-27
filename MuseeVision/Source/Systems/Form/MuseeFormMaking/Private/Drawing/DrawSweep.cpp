#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanRuns.h"
#include "Kernel/Sweep.h"
#include "Kernel/Tolerances.h"
#include "Stations/Stations.h"

/**
 * A sweep in plan runs its section along frames laid on the plan path. A sweep on a face is drawn the same way in the
 * face's development, (U along the face, Z up, W out of it), where the face is flat, and every point is then carried onto
 * the face: a vertical face is developable, so the development is an isometry and the section keeps its true size on a
 * curved wall, lying on it rather than on a tangent plane. A chamfered end moves the end's section along the travel onto
 * its cutting plane, so the sides run on to the plane and the cap lies in it.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/** Two distances along a face closer than the weld tolerance are one. */
		constexpr double SameAlongFace = WeldToleranceMeters;

		/**
		 * A face's station that falls within a chord tolerance (1 mm) of one of the path's is not added: the path's chord
		 * already stands on the face there within a hair (the face bends by the square of that millimetre over its radius),
		 * and a shorter chord has no direction a frame can be set by (a direction under 0.1 mm normalises to nothing).
		 */
		constexpr double NearStationMeters = DefaultChordToleranceMeters;

		/** The frame of a sweep's end cut on a plane through its origin: the section carried along the travel onto the plane. */
		bool CutFrame(FSweepFrame& Frame, const FVector& PlaneNormal, FString& OutWhy)
		{
			const FVector Normal = PlaneNormal.GetSafeNormal();
			const double Along = FVector::DotProduct(Frame.Along, Normal);
			if (FMath::Abs(Along) < FMath::Cos(FMath::DegreesToRadians(FFormSweep::MaxCutDegrees)))
			{
				OutWhy = FString::Printf(TEXT("a chamfered end leans more than %.0f degrees from square to the path"), FFormSweep::MaxCutDegrees);
				return false;
			}
			Frame.AxisA -= Frame.Along * (FVector::DotProduct(Frame.AxisA, Normal) / Along);
			Frame.AxisB -= Frame.Along * (FVector::DotProduct(Frame.AxisB, Normal) / Along);
			Frame.Along = Normal * FMath::Sign(Along);
			return true;
		}

		/**
		 * A vertical face in plan and its development: U along its line, Z up, W out of it (to the line's left). A
		 * development's point or direction is carried onto the face at its U.
		 */
		struct FFaceDevelopment
		{
			TArray<FSweepFrame> Line;

			double Length() const { return Line.Last().U; }

			FVector Point(const FVector& Developed) const
			{
				const FSweepFrame F = FrameAt(Line, Developed.X);
				return F.Origin + FVector::UpVector * Developed.Y - F.NormalA * Developed.Z;
			}

			FVector Direction(double U, const FVector& Developed) const
			{
				const FSweepFrame F = FrameAt(Line, U);
				return F.Along * Developed.X + FVector::UpVector * Developed.Y - F.NormalA * Developed.Z;
			}

			/** A direction of the drawing's frame, as the development sees it at U. */
			FVector Developed(double U, const FVector& Direction) const
			{
				const FSweepFrame F = FrameAt(Line, U);
				return FVector(FVector::DotProduct(Direction, F.Along), Direction.Z, -FVector::DotProduct(Direction, F.NormalA));
			}
		};

		/**
		 * The path in the development, raised by the elevation, with a station added wherever it crosses one of the face's:
		 * each chord of the path is then carried onto one flat strip of the face, and follows the face within its tolerance.
		 */
		FProfileStations DevelopedPath(const FProfileStations& Path, double Elevation, const FFaceDevelopment& Face)
		{
			FProfileStations Out;
			Out.bClosed = Path.bClosed;
			Out.ClosedLengthMeters = Path.ClosedLengthMeters;
			auto Raised = [Elevation](FProfileVertex Vertex) { Vertex.Point.Y += Elevation; return Vertex; };
			for (int32 Index = 0; Index < Path.Vertices.Num(); ++Index)
			{
				const FProfileVertex& From = Path.Vertices[Index];
				Out.Vertices.Add(Raised(From));
				if (Index >= Path.NumSegments())
				{
					continue;
				}
				const FProfileVertex& To = Path.Vertices[(Index + 1) % Path.Vertices.Num()];
				const double ToLength = Path.LengthAtSegmentEnd(Index);
				const double Low = FMath::Min(From.Point.X, To.Point.X);
				const double High = FMath::Max(From.Point.X, To.Point.X);
				// A station the face would add within a millimetre of either end stands there already (see NearStationMeters).
				const double Chord = FVector2D::Distance(From.Point, To.Point);
				TArray<double> Crossed;
				for (const FSweepFrame& Station : Face.Line)
				{
					const double Alpha = Station.U > Low + SameAlongFace && Station.U < High - SameAlongFace
						? (Station.U - From.Point.X) / (To.Point.X - From.Point.X) : -1.0;
					if (Alpha * Chord > NearStationMeters && (1.0 - Alpha) * Chord > NearStationMeters)
					{
						Crossed.Add(Alpha);
					}
				}
				Crossed.Sort();
				for (const double Alpha : Crossed)
				{
					FProfileVertex Between;
					Between.Point = FMath::Lerp(From.Point, To.Point, Alpha);
					Between.NormalIn = From.NormalOut;
					Between.NormalOut = From.NormalOut;
					Between.LengthMeters = FMath::Lerp(From.LengthMeters, ToLength, Alpha);
					Out.Vertices.Add(Raised(Between));
				}
			}
			return Out;
		}

		/**
		 * A closed section clipped to the side of one of its axes at or above zero (Axis 0: A, 1: B), with a crisp edge along
		 * the axis where it was cut, facing back across it. False when the section crosses the axis more than once each
		 * way, or lies wholly behind it.
		 */
		bool TrimSink(FProfileStations& Section, int32 Axis, FString& OutWhy)
		{
			// A point within the weld tolerance of the surface lies on it: the section's own point there is the cut.
			auto Height = [Axis](const FVector2D& P)
			{
				const double H = Axis == 0 ? P.X : P.Y;
				return FMath::Abs(H) <= WeldToleranceMeters ? 0.0 : H;
			};
			const int32 Count = Section.Vertices.Num();
			TArray<FProfileVertex> Kept;
			int32 Crossings = 0;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const FProfileVertex& Here = Section.Vertices[Index];
				const FProfileVertex& Next = Section.Vertices[(Index + 1) % Count];
				const double H0 = Height(Here.Point);
				const double H1 = Height(Next.Point);
				if (H0 >= 0.0)
				{
					Kept.Add(Here);
				}
				Crossings += (H0 >= 0.0) != (H1 >= 0.0) ? 1 : 0;
				if ((H0 > 0.0 && H1 < 0.0) || (H0 < 0.0 && H1 > 0.0))
				{
					FProfileVertex Cut;
					Cut.Point = FMath::Lerp(Here.Point, Next.Point, H0 / (H0 - H1));
					Cut.NormalIn = Cut.NormalOut = (Here.NormalOut + Next.NormalIn).GetSafeNormal();
					Cut.bCrisp = true;
					Kept.Add(Cut);
				}
			}
			if (Crossings == 0 && Kept.IsEmpty())
			{
				OutWhy = TEXT("the section lies wholly behind the surface its sink is trimmed to");
				return false;
			}
			if (Crossings > 2)
			{
				OutWhy = TEXT("a trimmed section crosses the surface it is fixed to more than once each way");
				return false;
			}
			// The edge along the cut runs between the two cut points; each keeps its own side's normal as it arrives or leaves.
			const int32 Kept0 = Kept.Num();
			for (int32 Index = 0; Index < Kept0; ++Index)
			{
				FProfileVertex& From = Kept[Index];
				FProfileVertex& To = Kept[(Index + 1) % Kept0];
				if (Crossings == 2 && Height(From.Point) == 0.0 && Height(To.Point) == 0.0)
				{
					const FVector2D Step = To.Point - From.Point;
					From.NormalOut = To.NormalIn = FVector2D(Step.Y, -Step.X).GetSafeNormal();
					From.bCrisp = To.bCrisp = true;
				}
			}
			Section.Vertices = Kept;
			double Length = 0.0;
			for (int32 Index = 0; Index < Kept0; ++Index)
			{
				Section.Vertices[Index].LengthMeters = Length;
				Length += FVector2D::Distance(Kept[Index].Point, Kept[(Index + 1) % Kept0].Point);
			}
			Section.ClosedLengthMeters = Length;
			return true;
		}

		/** Caps an open sweep's ends, cut where the drawing chamfers them. The runs' end frames take the cut. */
		bool CutAndCap(TArray<TArray<FSweepFrame>>& Runs, const FProfileStations& Section, const FVector& StartCut, const FVector& EndCut,
			TFunctionRef<FVector(const FSweepFrame&, const FVector&)> Developed, FMeshBuilder& Mesh, FString& OutWhy)
		{
			FSweepFrame& First = Runs[0][0];
			FSweepFrame& Last = Runs.Last().Last();
			if ((!StartCut.IsNearlyZero() && !CutFrame(First, Developed(First, StartCut), OutWhy))
				|| (!EndCut.IsNearlyZero() && !CutFrame(Last, Developed(Last, EndCut), OutWhy)))
			{
				return false;
			}
			for (const TArray<FSweepFrame>& Run : Runs)
			{
				Sweep(Mesh, Run, Section);
			}
			if (Section.bClosed && !(Cap(Mesh, First, Section, false) && Cap(Mesh, Last, Section, true)))
			{
				OutWhy = TEXT("the section's outline crosses itself, so its ends can't be capped");
				return false;
			}
			return true;
		}

		/** Every vertex and triangle of a development carried onto its face. The carrying keeps handedness, so windings hold. */
		void CarryOnto(const FFaceDevelopment& Face, const FMeshBuilder& Developed, FMeshBuilder& Mesh)
		{
			TArray<int32> Carried;
			for (int32 Vertex = 0; Vertex < Developed.NumVertices(); ++Vertex)
			{
				const FVector& P = Developed.Position(Vertex);
				Carried.Add(Mesh.AddVertex(Face.Point(P), Face.Direction(P.X, Developed.Normal(Vertex)), Developed.UV(Vertex)));
			}
			for (int32 Triangle = 0; Triangle < Developed.NumTriangles(); ++Triangle)
			{
				const FIntVector& T = Developed.Triangle(Triangle);
				Mesh.UseSlot(Developed.Slots()[Developed.TriangleSlot(Triangle)]);
				Mesh.AddTriangle(Carried[T.X], Carried[T.Y], Carried[T.Z]);
			}
		}

		bool DrawOnFace(const FFormSweep& Shape, const FProfileStations& Section, FMeshBuilder& Mesh, FString& OutWhy)
		{
			if (!CheckProfile(Shape.Face, TEXT("the face"), false, OutWhy))
			{
				return false;
			}
			const TArray<TArray<FSweepFrame>> Lines = PlanRuns(Resolve(Shape.Face), 0.0);
			if (Lines.Num() != 1)
			{
				OutWhy = TEXT("a sweep's face is one straight or smooth stretch");
				return false;
			}
			const FFaceDevelopment Face{Lines[0]};
			const FProfileStations Path = DevelopedPath(Resolve(Shape.Path), Shape.ElevationMeters, Face);
			for (const FProfileVertex& Vertex : Path.Vertices)
			{
				if (Vertex.Point.X < -SameAlongFace || Vertex.Point.X > Face.Length() + SameAlongFace)
				{
					OutWhy = FString::Printf(TEXT("the path runs off its face (U %.4f m of %.4f m)"), Vertex.Point.X, Face.Length());
					return false;
				}
			}
			TArray<TArray<FSweepFrame>> Runs = PlanRuns(Path, 0.0);
			if (Runs.IsEmpty())
			{
				OutWhy = TEXT("the path has fewer than two stations, or doubles back too sharply to mitre");
				return false;
			}
			FMeshBuilder Developed(Mesh.NumUVChannels());
			auto AsDeveloped = [&Face](const FSweepFrame& End, const FVector& Direction) { return Face.Developed(End.Origin.X, Direction); };
			const bool bEnds = Path.bClosed || CutAndCap(Runs, Section, Shape.StartCut, Shape.EndCut, AsDeveloped, Developed, OutWhy);
			if (!bEnds)
			{
				return false;
			}
			if (Path.bClosed)
			{
				for (const TArray<FSweepFrame>& Run : Runs)
				{
					Sweep(Developed, Run, Section);
				}
			}
			CarryOnto(Face, Developed, Mesh);
			return true;
		}
	}

	bool DrawSweep(const FFormSweep& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		if (!CheckProfile(Shape.Section, TEXT("the section"), false, OutWhy) || !CheckProfile(Shape.Path, TEXT("the path"), false, OutWhy))
		{
			return false;
		}
		FProfileStations Section = Resolve(Shape.Section);
		for (const int32 Axis : {0, 1})
		{
			if ((Axis == 0 ? Shape.bTrimSinkA : Shape.bTrimSinkB) && (!Section.bClosed || !TrimSink(Section, Axis, OutWhy)))
			{
				OutWhy = OutWhy.IsEmpty() ? TEXT("only a closed section's sink can be trimmed") : OutWhy;
				return false;
			}
		}
		if (Shape.IsOnFace())
		{
			return DrawOnFace(Shape, Section, Mesh, OutWhy);
		}
		TArray<TArray<FSweepFrame>> Runs = PlanRuns(Resolve(Shape.Path), Shape.ElevationMeters);
		if (Runs.IsEmpty())
		{
			OutWhy = TEXT("the path has fewer than two stations, or doubles back too sharply to mitre");
			return false;
		}
		if (Shape.Path.Closed)
		{
			for (const TArray<FSweepFrame>& Run : Runs)
			{
				Sweep(Mesh, Run, Section);
			}
			return true;
		}
		auto InPlan = [](const FSweepFrame&, const FVector& Direction) { return Direction; };
		return CutAndCap(Runs, Section, Shape.StartCut, Shape.EndCut, InPlan, Mesh, OutWhy);
	}
}
