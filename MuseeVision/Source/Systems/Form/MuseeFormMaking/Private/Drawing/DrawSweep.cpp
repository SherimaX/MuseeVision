#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Drawing/RisingPath.h"
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
		 * the axis where it was cut, facing back across it. A point where the clipped outline only touches the axis and turns
		 * straight back (a hollow that dipped behind it) is no corner and goes. False when what is left meets the axis along
		 * more than one edge, or nothing is left.
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
				if ((H0 > 0.0 && H1 < 0.0) || (H0 < 0.0 && H1 > 0.0))
				{
					FProfileVertex Cut;
					Cut.Point = FMath::Lerp(Here.Point, Next.Point, H0 / (H0 - H1));
					Cut.NormalIn = Cut.NormalOut = (Here.NormalOut + Next.NormalIn).GetSafeNormal();
					Cut.bCrisp = true;
					Kept.Add(Cut);
				}
			}
			for (bool bDropped = true; bDropped && Kept.Num() >= 3;)
			{
				bDropped = false;
				for (int32 Index = 0; Index < Kept.Num() && Kept.Num() >= 3; ++Index)
				{
					const FVector2D In = Kept[Index].Point - Kept[(Index + Kept.Num() - 1) % Kept.Num()].Point;
					const FVector2D Out = Kept[(Index + 1) % Kept.Num()].Point - Kept[Index].Point;
					const bool bBack = FVector2D::DotProduct(In, Out) < 0.0 && FMath::Abs(FVector2D::CrossProduct(In, Out)) <= TangentJointRadians * In.Size() * Out.Size();
					if (In.Size() <= WeldToleranceMeters || bBack)
					{
						Kept.RemoveAt(Index--);
						bDropped = true;
					}
				}
			}
			if (Kept.Num() < 3)
			{
				OutWhy = TEXT("the section lies wholly behind the surface its sink is trimmed to");
				return false;
			}
			// The edge along the cut: each end keeps its own side's normal as it arrives or leaves.
			int32 Along = 0;
			for (int32 Index = 0; Index < Kept.Num(); ++Index)
			{
				FProfileVertex& From = Kept[Index];
				FProfileVertex& To = Kept[(Index + 1) % Kept.Num()];
				if (Height(From.Point) == 0.0 && Height(To.Point) == 0.0)
				{
					const FVector2D Step = To.Point - From.Point;
					From.NormalOut = To.NormalIn = FVector2D(Step.Y, -Step.X).GetSafeNormal(StepSquaredMeters);
					From.bCrisp = To.bCrisp = true;
					++Along;
				}
			}
			if (Along > 1)
			{
				OutWhy = TEXT("a trimmed section meets the surface it is fixed to along more than one edge");
				return false;
			}
			Section.Vertices = Kept;
			double Length = 0.0;
			for (int32 Index = 0; Index < Kept.Num(); ++Index)
			{
				Section.Vertices[Index].LengthMeters = Length;
				Length += FVector2D::Distance(Kept[Index].Point, Kept[(Index + 1) % Kept.Num()].Point);
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

		/** A corner of the development as it is cut at the face's stations: where it lies, and what it carries. */
		struct FDevelopedCorner
		{
			FVector Position = FVector::ZeroVector;
			FVector Normal = FVector::ZeroVector;
			FVector2D UV = FVector2D::ZeroVector;
		};

		/**
		 * Where an edge of the development crosses the line of a face's station, set out from its corner of lower U, so the
		 * two triangles either side of the edge cut it at one point.
		 */
		FDevelopedCorner Crossing(const FDevelopedCorner& A, const FDevelopedCorner& B, double U)
		{
			const bool bFromA = A.Position.X <= B.Position.X;
			const FDevelopedCorner& Low = bFromA ? A : B;
			const FDevelopedCorner& High = bFromA ? B : A;
			const double Alpha = (U - Low.Position.X) / (High.Position.X - Low.Position.X);
			return {FMath::Lerp(Low.Position, High.Position, Alpha), FMath::Lerp(Low.Normal, High.Normal, Alpha).GetSafeNormal(),
				FMath::Lerp(Low.UV, High.UV, Alpha)};
		}

		/**
		 * A convex piece of the development cut along the line of a face's station, U: the part at or before it and the
		 * part at or after it. A corner within a chord tolerance of the line lies on it (NearStationMeters), in both parts,
		 * and only an edge from one side to the other is cut: a rule of the corners alone, so every triangle sharing an
		 * edge cuts it alike and the carried surface stays closed.
		 */
		void CutAt(TConstArrayView<FDevelopedCorner> Piece, double U, TArray<FDevelopedCorner>& Before, TArray<FDevelopedCorner>& After)
		{
			auto Side = [U](const FDevelopedCorner& Corner)
			{
				const double Off = Corner.Position.X - U;
				return Off < -NearStationMeters ? -1 : (Off > NearStationMeters ? 1 : 0);
			};
			for (int32 Index = 0; Index < Piece.Num(); ++Index)
			{
				const FDevelopedCorner& Here = Piece[Index];
				const FDevelopedCorner& Next = Piece[(Index + 1) % Piece.Num()];
				const int32 HereSide = Side(Here);
				const int32 NextSide = Side(Next);
				if (HereSide <= 0)
				{
					Before.Add(Here);
				}
				if (HereSide >= 0)
				{
					After.Add(Here);
				}
				if (HereSide * NextSide < 0)
				{
					const FDevelopedCorner Cut = Crossing(Here, Next, U);
					Before.Add(Cut);
					After.Add(Cut);
				}
			}
		}

		/** A convex piece carried onto the face as a fan of triangles. */
		void CarryPiece(const FFaceDevelopment& Face, TConstArrayView<FDevelopedCorner> Piece, FMeshBuilder& Mesh)
		{
			if (Piece.Num() < 3)
			{
				return;
			}
			TArray<int32> Carried;
			for (const FDevelopedCorner& Corner : Piece)
			{
				Carried.Add(Mesh.AddVertex(Face.Point(Corner.Position), Face.Direction(Corner.Position.X, Corner.Normal), Corner.UV));
			}
			for (int32 Index = 1; Index + 1 < Carried.Num(); ++Index)
			{
				Mesh.AddTriangle(Carried[0], Carried[Index], Carried[Index + 1]);
			}
		}

		/**
		 * Every triangle of a development carried onto its face, cut first along the lines of the face's stations it spans:
		 * each piece then lies on one flat strip of the face, so a surface drawn along the face (a surround's back up a jamb)
		 * follows it as the face's own chords do, never cutting across its curve. The carrying keeps handedness, so windings
		 * hold.
		 */
		void CarryOnto(const FFaceDevelopment& Face, const FMeshBuilder& Developed, FMeshBuilder& Mesh)
		{
			TArray<double> Stations;
			for (const FSweepFrame& Station : Face.Line)
			{
				Stations.Add(Station.U);
			}
			for (int32 Triangle = 0; Triangle < Developed.NumTriangles(); ++Triangle)
			{
				const FIntVector& T = Developed.Triangle(Triangle);
				TArray<FDevelopedCorner> Rest;
				for (const int32 Vertex : {T.X, T.Y, T.Z})
				{
					Rest.Add({Developed.Position(Vertex), Developed.Normal(Vertex), Developed.UV(Vertex)});
				}
				Mesh.UseSlot(Developed.Slots()[Developed.TriangleSlot(Triangle)]);
				const double Low = FMath::Min3(Rest[0].Position.X, Rest[1].Position.X, Rest[2].Position.X);
				const double High = FMath::Max3(Rest[0].Position.X, Rest[1].Position.X, Rest[2].Position.X);
				for (const double U : Stations)
				{
					if (U <= Low + NearStationMeters || U >= High - NearStationMeters)
					{
						continue;
					}
					TArray<FDevelopedCorner> Before, After;
					CutAt(Rest, U, Before, After);
					CarryPiece(Face, Before, Mesh);
					Rest = MoveTemp(After);
				}
				CarryPiece(Face, Rest, Mesh);
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
		auto InPlan = [](const FSweepFrame&, const FVector& Direction) { return Direction; };
		if (Shape.Rises())
		{
			TArray<TArray<FSweepFrame>> Rising;
			return RisingRuns(Shape, Rising, OutWhy) && CutAndCap(Rising, Section, Shape.StartCut, Shape.EndCut, InPlan, Mesh, OutWhy);
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
		return CutAndCap(Runs, Section, Shape.StartCut, Shape.EndCut, InPlan, Mesh, OutWhy);
	}
}
