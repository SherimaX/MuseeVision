#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Tolerances.h"
#include "Kernel/Triangulation.h"
#include "Stations/Stations.h"

/**
 * A wall is drawn as strips between the stations of its plan path. Along the path a distance U (metres on the face)
 * and a height Z above the bed name a point of the face; its back lies straight across the wall from it: square to the
 * face, or along a straight passage's axis where one passes, and on the drawn back line when the wall fills between two
 * faces. Each strip is split round the openings that cross it, and every strip edge carries every height at which its
 * neighbours break (a sill, a head), so the strips, the reveals, the top and the bed meet edge to edge and the wall is one
 * closed solid. A straight passage's stations stand where its jambs and its arch's stations stand across its axis, and
 * each finds its back at the same offset across the axis, so the passage's jambs and soffit run parallel to its axis.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/** Two heights or distances along the wall closer than the weld tolerance are one. */
		constexpr double Same = WeldToleranceMeters;

		/**
		 * Where the two faces of a wall between faces touch (a flat port face tangent to a drum), its wedge runs out to a
		 * feather edge ten weld tolerances (10 micrometres) thick: sharp to any eye, and still one closed shell, whose face
		 * and back don't weld into one line. It is the finest station spacing the kernel keeps (DrawnProfile's merge).
		 */
		constexpr double FeatherMeters = 10.0 * WeldToleranceMeters;

		/** The value of a piecewise-linear function at X, from its knots (X ascending), held level beyond them. */
		double Interpolate(TConstArrayView<double> Xs, TConstArrayView<double> Ys, double X)
		{
			if (X <= Xs[0])
			{
				return Ys[0];
			}
			for (int32 Index = 1; Index < Xs.Num(); ++Index)
			{
				if (X <= Xs[Index])
				{
					const double Span = Xs[Index] - Xs[Index - 1];
					return Span > 0.0 ? FMath::Lerp(Ys[Index - 1], Ys[Index], (X - Xs[Index - 1]) / Span) : Ys[Index];
				}
			}
			return Ys.Last();
		}

		/**
		 * An opening set out on the face: the U of its jambs, and its offset across its own axis at any U. A square-cut
		 * opening's offset is its U from its centre; a straight passage's is measured across its axis at the face's stations.
		 */
		struct FOpeningSpan
		{
			const FFormWallOpening* Opening = nullptr;
			double Low = 0.0;
			double High = 0.0;
			TArray<double> StationU;
			TArray<double> StationAcross;
			/** A straight passage's axis in plan: the face's normal where the axis meets it, pointing into the wall. */
			FVector Axis = FVector::ZeroVector;

			bool IsStraight() const { return !StationU.IsEmpty(); }
			double AcrossAt(double U) const { return IsStraight() ? Interpolate(StationU, StationAcross, U) : U - Opening->CenterMeters; }
			double UAt(double Across) const { return IsStraight() ? Interpolate(StationAcross, StationU, Across) : Opening->CenterMeters + Across; }

			bool Holds(double U, bool bInclusive) const
			{
				return bInclusive ? U >= Low - Same && U <= High + Same : U > Low + Same && U < High - Same;
			}

			/** The underside of the head at U: the lintel, or the arch. */
			double HeadAt(double U) const
			{
				if (!Opening->bArched)
				{
					return Opening->HeadMeters;
				}
				const double Radius = 0.5 * Opening->WidthMeters;
				const double Across = FMath::Clamp(AcrossAt(U), -Radius, Radius);
				return Opening->HeadMeters + FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Across * Across));
			}
		};

		/** A back line in plan, as its points and the normals out of the wall there. */
		struct FBackLine
		{
			TArray<FVector> Points;
			TArray<FVector> Normals;
		};

		/** Where a line straight across the wall from a point of its face meets the back line, and the back's normal there. */
		bool HitBack(const FBackLine& Line, const FVector& From, const FVector& Direction, FVector& OutPoint, FVector& OutNormal)
		{
			auto Cross = [](const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; };
			const FVector2D Origin(From.X, From.Y);
			const FVector2D Across(Direction.X, Direction.Y);
			double Nearest = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index + 1 < Line.Points.Num(); ++Index)
			{
				const FVector2D P0(Line.Points[Index].X, Line.Points[Index].Y);
				const FVector2D Edge = FVector2D(Line.Points[Index + 1].X, Line.Points[Index + 1].Y) - P0;
				const double Denominator = Cross(Across, Edge);
				if (FMath::Abs(Denominator) < UE_DOUBLE_SMALL_NUMBER)
				{
					continue;
				}
				const FVector2D ToEdge = P0 - Origin;
				const double T = Cross(ToEdge, Edge) / Denominator;
				const double S = Cross(ToEdge, Across) / Denominator;
				if (S >= -UE_DOUBLE_KINDA_SMALL_NUMBER && S <= 1.0 + UE_DOUBLE_KINDA_SMALL_NUMBER && T >= -Same && T < Nearest)
				{
					Nearest = T;
					const double Along = FMath::Clamp(S, 0.0, 1.0);
					const FVector2D Hit = P0 + Edge * Along;
					OutPoint = FVector(Hit.X, Hit.Y, From.Z);
					OutNormal = FMath::Lerp(Line.Normals[Index], Line.Normals[Index + 1], Along).GetSafeNormal();
				}
			}
			return Nearest < TNumericLimits<double>::Max();
		}

		/** A run's own back, ThicknessMeters behind its face. */
		FBackLine OwnBack(TConstArrayView<FSweepFrame> Run, double Thickness)
		{
			FBackLine Line;
			for (const FSweepFrame& F : Run)
			{
				Line.Points.Add(F.Origin + F.AxisA * Thickness);
				Line.Normals.Add(F.NormalA);
			}
			return Line;
		}

		/** The drawn back line at the wall's base, every run of it in order. */
		FBackLine DrawnBack(const FFormWall& Wall)
		{
			FBackLine Line;
			for (const TArray<FSweepFrame>& Run : PathRuns(Resolve(Wall.Back), Wall.BaseMeters, {}))
			{
				for (const FSweepFrame& F : Run)
				{
					Line.Points.Add(F.Origin);
					Line.Normals.Add(F.NormalA);
				}
			}
			return Line;
		}

		/**
		 * Sets a straight passage out on the run that holds its centre: its axis, and the offsets across it of the face's
		 * stations either side of the centre for as long as they keep running across (a ring turns back beyond them).
		 */
		bool SetOutStraight(const FFormWallOpening& O, const TArray<TArray<FSweepFrame>>& Runs, FOpeningSpan& Span, FString& OutWhy)
		{
			const TArray<FSweepFrame>* Run = Runs.FindByPredicate([&O](const TArray<FSweepFrame>& R)
			{
				return O.CenterMeters > R[0].U + Same && O.CenterMeters < R.Last().U - Same;
			});
			if (!Run)
			{
				OutWhy = TEXT("a straight passage's centre lies on a corner or an end of the wall");
				return false;
			}
			const FSweepFrame Centre = FrameAt(*Run, O.CenterMeters);
			Span.Axis = Centre.NormalA;
			auto Across = [&Centre](const FSweepFrame& F) { return FVector::DotProduct(F.Origin - Centre.Origin, Centre.Along); };
			const int32 Before = Run->FindLastByPredicate([&O](const FSweepFrame& F) { return F.U <= O.CenterMeters; });
			int32 First = Before;
			while (First > 0 && Across((*Run)[First - 1]) < Across((*Run)[First]))
			{
				--First;
			}
			int32 Last = Before + 1;
			while (Last + 1 < Run->Num() && Across((*Run)[Last + 1]) > Across((*Run)[Last]))
			{
				++Last;
			}
			for (int32 Index = First; Index <= Last; ++Index)
			{
				Span.StationU.Add((*Run)[Index].U);
				Span.StationAcross.Add(Across((*Run)[Index]));
			}
			const double Half = 0.5 * O.WidthMeters;
			if (Span.StationAcross[0] > -Half || Span.StationAcross.Last() < Half)
			{
				OutWhy = TEXT("a straight passage is wider than the stretch of wall it passes through");
				return false;
			}
			Span.Low = Span.UAt(-Half);
			Span.High = Span.UAt(Half);
			return true;
		}

		/** Every opening set out on the face; false, with the reason, when one doesn't fit or two overlap. */
		bool SetOutOpenings(const FFormWall& Wall, const TArray<TArray<FSweepFrame>>& Runs, TArray<FOpeningSpan>& OutSpans, FString& OutWhy)
		{
			for (int32 Index = 0; Index < Wall.Openings.Num(); ++Index)
			{
				const FFormWallOpening& O = Wall.Openings[Index];
				if (!(O.WidthMeters > 0.0 && O.SillMeters >= 0.0 && O.HeadMeters > O.SillMeters && O.CrownMeters() < Wall.HeightMeters - Same))
				{
					OutWhy = FString::Printf(TEXT("opening %d doesn't fit: it needs a width, a head above its sill, and a crown under the wall's top"), Index);
					return false;
				}
				FOpeningSpan& Span = OutSpans.AddDefaulted_GetRef();
				Span.Opening = &O;
				Span.Low = O.CenterMeters - 0.5 * O.WidthMeters;
				Span.High = O.CenterMeters + 0.5 * O.WidthMeters;
				if (O.bStraight && !SetOutStraight(O, Runs, Span, OutWhy))
				{
					return false;
				}
			}
			for (int32 Index = 0; Index < OutSpans.Num(); ++Index)
			{
				const FOpeningSpan& Span = OutSpans[Index];
				for (int32 Other = Index + 1; Other < OutSpans.Num(); ++Other)
				{
					if (OutSpans[Other].Low < Span.High + Same && Span.Low < OutSpans[Other].High + Same)
					{
						OutWhy = FString::Printf(TEXT("openings %d and %d overlap or touch"), Index, Other);
						return false;
					}
				}
				const bool bInside = Runs.ContainsByPredicate([&Span](const TArray<FSweepFrame>& Run)
				{
					return Span.Low > Run[0].U + Same && Span.High < Run.Last().U - Same;
				});
				if (!bInside)
				{
					OutWhy = TEXT("an opening crosses a corner or an end of the wall; an opening lies within one straight or smooth stretch");
					return false;
				}
			}
			return true;
		}

		/** The distance along the face's chords of the point on them nearest a point. */
		double NearestU(const TArray<TArray<FSweepFrame>>& Runs, const FVector& Point)
		{
			double Nearest = TNumericLimits<double>::Max();
			double AtU = 0.0;
			for (const TArray<FSweepFrame>& Run : Runs)
			{
				for (int32 Index = 0; Index + 1 < Run.Num(); ++Index)
				{
					const FVector Closest = FMath::ClosestPointOnSegment(Point, Run[Index].Origin, Run[Index + 1].Origin);
					const double Distance = FVector::DistSquared(Closest, Point);
					const double Chord = FVector::Dist(Run[Index].Origin, Run[Index + 1].Origin);
					if (Distance < Nearest && Chord > 0.0)
					{
						Nearest = Distance;
						AtU = Run[Index].U + (Run[Index + 1].U - Run[Index].U) * FVector::Dist(Run[Index].Origin, Closest) / Chord;
					}
				}
			}
			return AtU;
		}

		/**
		 * Every distance along the face at which the wall must have a station: the jambs, the stations of each arch (set
		 * across a straight passage's axis), and, for a wall between two faces, across from each of the back line's.
		 */
		TArray<double> RequiredStations(const FFormWall& Wall, TConstArrayView<FOpeningSpan> Spans, const TArray<TArray<FSweepFrame>>& Runs)
		{
			TArray<double> Required;
			for (const FOpeningSpan& Span : Spans)
			{
				Required.Add(Span.Low);
				Required.Add(Span.High);
				if (Span.Opening->bArched)
				{
					const double Radius = 0.5 * Span.Opening->WidthMeters;
					for (const double Angle : ArcStations(Radius, UE_DOUBLE_PI, 0.0, DefaultChordToleranceMeters))
					{
						Required.Add(Span.UAt(Radius * FMath::Cos(Angle)));
					}
				}
			}
			if (Wall.HasBack())
			{
				for (const FVector& Point : DrawnBack(Wall).Points)
				{
					Required.Add(NearestU(Runs, Point));
				}
			}
			Required.Sort();
			return Required;
		}

		/** A run's stations with their backs found, and the back's normal at each. */
		struct FWallRun
		{
			TArray<FSweepFrame> Frames;
			TArray<FVector> BackNormals;
		};

		/**
		 * Finds each station's back: on the drawn back line, straight across the wall (along a straight passage's axis within
		 * it); or, within a straight passage in a wall of one thickness, on the wall's own back at the same offset across the
		 * axis. The station's AxisA is set so Depth reaches it.
		 */
		bool FindBacks(const FFormWall& Wall, TConstArrayView<FOpeningSpan> Spans, TArray<FSweepFrame> Run, double Depth, FWallRun& Out, FString& OutWhy)
		{
			const FBackLine Line = Wall.HasBack() ? DrawnBack(Wall) : OwnBack(Run, Wall.ThicknessMeters);
			for (FSweepFrame& F : Run)
			{
				const FOpeningSpan* Straight = Spans.FindByPredicate([&F](const FOpeningSpan& S) { return S.IsStraight() && S.Holds(F.U, true); });
				Out.BackNormals.Add(F.NormalA);
				if (!Wall.HasBack() && !Straight)
				{
					continue;
				}
				const FVector Across = Straight ? Straight->Axis : F.NormalA;
				FVector Back;
				if (!HitBack(Line, F.Origin, Across, Back, Out.BackNormals.Last()))
				{
					OutWhy = FString::Printf(TEXT("the wall's back doesn't lie behind its face at %.4f m along it"), F.U);
					return false;
				}
				const FVector Through = Back - F.Origin;
				F.AxisA = (Through.Size() < FeatherMeters ? Across * FeatherMeters : Through) / Depth;
			}
			Out.Frames = MoveTemp(Run);
			return true;
		}

		struct FWallSetOut
		{
			const FFormWall& Wall;
			TConstArrayView<FOpeningSpan> Spans;
			const FWallRun& Run;
			/** The depth that reaches the back from any station: the thickness, or 1 when each station's AxisA reaches it whole. */
			double Depth = 0.0;

			const FSweepFrame& Frame(int32 Station) const { return Run.Frames[Station]; }

			const FOpeningSpan* SpanAt(double U, bool bInclusive) const
			{
				return Spans.FindByPredicate([U, bInclusive](const FOpeningSpan& S) { return S.Holds(U, bInclusive); });
			}

			/** Every height at which the wall's surfaces break at station U, ascending. */
			TArray<double> BreaksAt(double U) const
			{
				TArray<double> Breaks = {0.0, Wall.HeightMeters};
				if (const FOpeningSpan* S = SpanAt(U, true))
				{
					Breaks.Add(S->Opening->SillMeters);
					Breaks.Add(S->HeadAt(U));
				}
				Breaks.Sort();
				TArray<double> Distinct;
				for (const double Z : Breaks)
				{
					if (Distinct.IsEmpty() || Z > Distinct.Last() + Same)
					{
						Distinct.Add(Z);
					}
				}
				return Distinct;
			}

			FVector PointAt(int32 Station, double Z, double AtDepth) const
			{
				return Frame(Station).Origin + Frame(Station).AxisA * AtDepth + FVector::UpVector * Z;
			}

			/** The wall's thickness at a station, straight across it. */
			double Width(int32 Station) const { return Frame(Station).AxisA.Size() * Depth; }

			/** The normal of a surface standing straight across the wall at a station, turned to face Toward. */
			FVector AcrossNormal(int32 Station, const FVector& Toward) const
			{
				const FVector Normal = FVector::CrossProduct(FVector::UpVector, Frame(Station).AxisA).GetSafeNormal();
				if (Normal.IsNearlyZero())
				{
					return Toward;
				}
				return FVector::DotProduct(Normal, Toward) >= 0.0 ? Normal : -Normal;
			}
		};

		/** A solid band of a strip between two heights, each a function of U: the wall under a sill, or over a head. */
		struct FBand
		{
			TFunction<double(double)> Low;
			TFunction<double(double)> High;
		};

		TArray<FBand> BandsOf(const FWallSetOut& SetOut, double MidU)
		{
			const double Height = SetOut.Wall.HeightMeters;
			const FOpeningSpan* S = SetOut.SpanAt(MidU, false);
			if (!S)
			{
				return {FBand{[](double) { return 0.0; }, [Height](double) { return Height; }}};
			}
			TArray<FBand> Bands;
			const double Sill = S->Opening->SillMeters;
			if (Sill > Same)
			{
				Bands.Add(FBand{[](double) { return 0.0; }, [Sill](double) { return Sill; }});
			}
			Bands.Add(FBand{[S](double U) { return S->HeadAt(U); }, [Height](double) { return Height; }});
			return Bands;
		}

		/** Draws one band of a strip on both faces, triangulated in (U, Z) with every break on its sides. */
		bool DrawBand(const FWallSetOut& SetOut, int32 Left, const FBand& Band, FMeshBuilder& Mesh)
		{
			const int32 Right = Left + 1;
			const double UL = SetOut.Frame(Left).U;
			const double UR = SetOut.Frame(Right).U;
			TArray<FVector2D> Outline = {FVector2D(UL, Band.Low(UL)), FVector2D(UR, Band.Low(UR))};
			TArray<int32> StationOf = {Left, Right};
			for (const double Z : SetOut.BreaksAt(UR))
			{
				if (Z > Band.Low(UR) + Same && Z < Band.High(UR) - Same) { Outline.Add(FVector2D(UR, Z)); StationOf.Add(Right); }
			}
			Outline.Add(FVector2D(UR, Band.High(UR)));
			StationOf.Add(Right);
			Outline.Add(FVector2D(UL, Band.High(UL)));
			StationOf.Add(Left);
			TArray<double> LeftBreaks = SetOut.BreaksAt(UL);
			for (int32 Index = LeftBreaks.Num() - 1; Index >= 0; --Index)
			{
				const double Z = LeftBreaks[Index];
				if (Z > Band.Low(UL) + Same && Z < Band.High(UL) - Same) { Outline.Add(FVector2D(UL, Z)); StationOf.Add(Left); }
			}
			TArray<FIntVector> Triangles;
			if (!TriangulatePolygon(Outline, {}, Triangles))
			{
				return false;
			}
			for (const bool bBack : {false, true})
			{
				Mesh.UseSlot(bBack ? FFormWall::BackPart : FFormWall::FacePart);
				TArray<int32> Vertices;
				for (int32 Index = 0; Index < Outline.Num(); ++Index)
				{
					const int32 Station = StationOf[Index];
					const FVector Normal = bBack ? SetOut.Run.BackNormals[Station] : -SetOut.Frame(Station).NormalA;
					const FVector P = SetOut.PointAt(Station, Outline[Index].Y, bBack ? SetOut.Depth : 0.0);
					Vertices.Add(Mesh.AddVertex(P, Normal, Outline[Index]));
				}
				for (const FIntVector& T : Triangles)
				{
					Mesh.AddTriangle(Vertices[T.X], Vertices[T.Y], Vertices[T.Z]);
				}
			}
			return true;
		}

		/** A quad across the wall's thickness between two points of its face line, facing Normal; its V runs across, in metres. */
		void AcrossQuad(const FWallSetOut& SetOut, int32 StationA, double ZA, int32 StationB, double ZB, const FVector& Normal,
			const FVector2D& UVA, const FVector2D& UVB, FMeshBuilder& Mesh)
		{
			const double Depth = SetOut.Depth;
			const int32 A0 = Mesh.AddVertex(SetOut.PointAt(StationA, ZA, 0.0), Normal, UVA);
			const int32 B0 = Mesh.AddVertex(SetOut.PointAt(StationB, ZB, 0.0), Normal, UVB);
			const int32 B1 = Mesh.AddVertex(SetOut.PointAt(StationB, ZB, Depth), Normal, UVB + FVector2D(0.0, SetOut.Width(StationB)));
			const int32 A1 = Mesh.AddVertex(SetOut.PointAt(StationA, ZA, Depth), Normal, UVA + FVector2D(0.0, SetOut.Width(StationA)));
			Mesh.AddQuad(A0, B0, B1, A1);
		}

		/** The soffit of a strip under a head: the normal of its own quad, turned down and, under an arch, in to its centre. */
		FVector SoffitNormal(const FWallSetOut& SetOut, int32 Left, double HL, double HR)
		{
			const int32 Right = Left + 1;
			const FVector A0 = SetOut.PointAt(Left, HL, 0.0);
			const FVector Along = SetOut.PointAt(Right, HR, 0.0) - A0;
			const FVector Across = SetOut.PointAt(Left, HL, SetOut.Depth) - A0;
			const FVector2D Down = FVector2D(HR - HL, -(SetOut.Frame(Right).U - SetOut.Frame(Left).U)).GetSafeNormal();
			const FVector Toward = (Along.GetSafeNormal2D() * Down.X + FVector::UpVector * Down.Y).GetSafeNormal();
			const FVector Normal = FVector::CrossProduct(Along, Across).GetSafeNormal();
			if (Normal.IsNearlyZero())
			{
				return Toward;
			}
			return FVector::DotProduct(Normal, Toward) >= 0.0 ? Normal : -Normal;
		}

		/** The top and the bed of every strip, and the sill and head linings of the strips inside openings. */
		void DrawAcross(const FWallSetOut& SetOut, int32 Left, FMeshBuilder& Mesh)
		{
			const int32 Right = Left + 1;
			const double UL = SetOut.Frame(Left).U;
			const double UR = SetOut.Frame(Right).U;
			const double Height = SetOut.Wall.HeightMeters;
			Mesh.UseSlot(FFormWall::TopPart);
			AcrossQuad(SetOut, Left, Height, Right, Height, FVector::UpVector, FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
			const FOpeningSpan* S = SetOut.SpanAt(0.5 * (UL + UR), false);
			if (!S || S->Opening->SillMeters > Same)
			{
				Mesh.UseSlot(FFormWall::BedPart);
				AcrossQuad(SetOut, Left, 0.0, Right, 0.0, -FVector::UpVector, FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
			}
			if (!S)
			{
				return;
			}
			Mesh.UseSlot(FFormWall::RevealPart);
			const double Sill = S->Opening->SillMeters;
			if (Sill > Same)
			{
				AcrossQuad(SetOut, Left, Sill, Right, Sill, FVector::UpVector, FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
			}
			const double HL = S->HeadAt(UL);
			const double HR = S->HeadAt(UR);
			AcrossQuad(SetOut, Left, HL, Right, HR, SoffitNormal(SetOut, Left, HL, HR), FVector2D(UL, HL), FVector2D(UR, HR), Mesh);
		}

		/** The lining of a jamb: its whole height at one station, split at every break, across the thickness. */
		void DrawJamb(const FWallSetOut& SetOut, int32 Station, const FOpeningSpan& S, bool bLow, FMeshBuilder& Mesh)
		{
			const FSweepFrame& F = SetOut.Frame(Station);
			const FVector Normal = SetOut.AcrossNormal(Station, bLow ? F.Along : -F.Along);
			const double Head = S.HeadAt(F.U);
			TArray<double> Heights;
			for (const double Z : SetOut.BreaksAt(F.U))
			{
				if (Z >= S.Opening->SillMeters - Same && Z <= Head + Same)
				{
					Heights.Add(Z);
				}
			}
			Mesh.UseSlot(FFormWall::RevealPart);
			for (int32 Index = 0; Index + 1 < Heights.Num(); ++Index)
			{
				AcrossQuad(SetOut, Station, Heights[Index], Station, Heights[Index + 1], Normal, FVector2D(Heights[Index], 0.0),
					FVector2D(Heights[Index + 1], 0.0), Mesh);
			}
		}

		/** The end of an open wall: its section at the path's first or last station. */
		void DrawEnd(const FWallSetOut& SetOut, int32 Station, bool bStart, FMeshBuilder& Mesh)
		{
			const FSweepFrame& F = SetOut.Frame(Station);
			Mesh.UseSlot(FFormWall::EndPart);
			AcrossQuad(SetOut, Station, 0.0, Station, SetOut.Wall.HeightMeters, SetOut.AcrossNormal(Station, bStart ? -F.Along : F.Along),
				FVector2D(0.0, 0.0), FVector2D(SetOut.Wall.HeightMeters, 0.0), Mesh);
		}

		bool DrawRun(const FWallSetOut& SetOut, bool bStartsPath, bool bEndsPath, FMeshBuilder& Mesh)
		{
			const int32 Stations = SetOut.Run.Frames.Num();
			for (int32 Left = 0; Left + 1 < Stations; ++Left)
			{
				for (const FBand& Band : BandsOf(SetOut, 0.5 * (SetOut.Frame(Left).U + SetOut.Frame(Left + 1).U)))
				{
					if (!DrawBand(SetOut, Left, Band, Mesh))
					{
						return false;
					}
				}
				DrawAcross(SetOut, Left, Mesh);
			}
			for (const FOpeningSpan& S : SetOut.Spans)
			{
				for (int32 Station = 0; Station < Stations; ++Station)
				{
					const double U = SetOut.Frame(Station).U;
					const bool bLow = FMath::Abs(U - S.Low) <= Same;
					if (bLow || FMath::Abs(U - S.High) <= Same)
					{
						DrawJamb(SetOut, Station, S, bLow, Mesh);
					}
				}
			}
			if (bStartsPath)
			{
				DrawEnd(SetOut, 0, true, Mesh);
			}
			if (bEndsPath)
			{
				DrawEnd(SetOut, Stations - 1, false, Mesh);
			}
			return true;
		}

		bool CheckWall(const FFormWall& Shape, FString& OutWhy)
		{
			if (!CheckProfile(Shape.Plan, TEXT("the plan"), false, OutWhy) || (Shape.HasBack() && !CheckProfile(Shape.Back, TEXT("the back"), false, OutWhy)))
			{
				return false;
			}
			if ((!Shape.HasBack() && Shape.ThicknessMeters <= 0.0) || Shape.HeightMeters <= 0.0)
			{
				OutWhy = TEXT("a wall has a thickness (or a back) and a height");
				return false;
			}
			return true;
		}
	}

	bool DrawWall(const FFormWall& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		if (!CheckWall(Shape, OutWhy))
		{
			return false;
		}
		const FProfileStations Plan = Resolve(Shape.Plan);
		// The openings are set out on the face's own stations first; the wall is then divided at every station they need.
		const TArray<TArray<FSweepFrame>> FaceRuns = PathRuns(Plan, Shape.BaseMeters, {});
		if (FaceRuns.IsEmpty())
		{
			OutWhy = TEXT("the plan has fewer than two stations, or doubles back too sharply to mitre");
			return false;
		}
		TArray<FOpeningSpan> Spans;
		if (!SetOutOpenings(Shape, FaceRuns, Spans, OutWhy))
		{
			return false;
		}
		const TArray<TArray<FSweepFrame>> Runs = PathRuns(Plan, Shape.BaseMeters, RequiredStations(Shape, Spans, FaceRuns));
		const double Depth = Shape.HasBack() ? 1.0 : Shape.ThicknessMeters;
		for (int32 Index = 0; Index < Runs.Num(); ++Index)
		{
			FWallRun Run;
			if (!FindBacks(Shape, Spans, Runs[Index], Depth, Run, OutWhy))
			{
				return false;
			}
			const FWallSetOut SetOut{Shape, Spans, Run, Depth};
			const bool bStart = !Shape.Plan.Closed && Index == 0;
			const bool bEnd = !Shape.Plan.Closed && Index == Runs.Num() - 1;
			if (!DrawRun(SetOut, bStart, bEnd, Mesh))
			{
				OutWhy = TEXT("a strip of the wall crosses itself between two stations");
				return false;
			}
		}
		return true;
	}
}
