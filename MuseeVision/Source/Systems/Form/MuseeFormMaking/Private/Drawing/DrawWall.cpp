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
 * each finds its back at the same offset across the axis, so the passage's jambs and soffit run parallel to its axis. An
 * arched head, round, segmental or pointed, has a station at each of its arcs' stations; a drawn top has one at each of
 * its own, and each strip's top follows it on that chord. Openings may stand one above another in a stretch (a lunette over
 * a door): a strip under both is solid from the bed to the lower one's sill, between them, and over the upper one's head.
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

		/**
		 * The normal of a quad from two of its edges: each edge's direction first, normalised at the kernel's step (a strip
		 * a tenth of a millimetre wide, where a curved face's chord lengths and a drawing's arc lengths part, has one), then
		 * their cross, zero only where the edges run parallel. Crossing the edges themselves would square their lengths
		 * against the engine's tolerance and lose any strip under a tenth of a millimetre.
		 */
		FVector QuadNormal(const FVector& EdgeA, const FVector& EdgeB)
		{
			return FVector::CrossProduct(EdgeA.GetSafeNormal(StepSquaredMeters), EdgeB.GetSafeNormal(StepSquaredMeters)).GetSafeNormal();
		}

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

			/** The underside of the head at U: the lintel, or the arch's intrados, round, segmental or pointed. */
			double HeadAt(double U) const
			{
				return Opening->HeadAt(AcrossAt(U));
			}
		};

		/**
		 * The offsets across an opening's axis at which its arched head must have a station, so its intrados is followed
		 * within the chord tolerance: each arc's own stations, and the crown.
		 */
		TArray<double> HeadStationsAcross(const FFormWallOpening& O)
		{
			TArray<double> Across;
			if (!O.bArched)
			{
				return Across;
			}
			const double Half = O.HalfWidthMeters();
			const double Radius = O.IntradosRadiusMeters();
			if (O.bPointed)
			{
				// Each half is an arc of the radius about a centre on the springing line, Radius - Half across the axis from
				// its own jamb's far side: from the springing (angle 0 at the jamb) to the crown on the axis.
				const double Centre = Half - Radius;
				const double Crown = FMath::Acos(FMath::Clamp((Radius - Half) / Radius, -1.0, 1.0));
				for (const double Angle : ArcStations(Radius, 0.0, Crown, DefaultChordToleranceMeters))
				{
					const double Offset = FMath::Max(0.0, Centre + Radius * FMath::Cos(Angle));
					if (Offset <= O.SillHalfWidthMeters())
					{
						Across.Add(Offset);
						Across.Add(-Offset);
					}
				}
				return Across;
			}
			// A round or segmental head is one arc about a centre on the axis, at or below the springing, with its crown; where
			// it springs below the sill, only the part above the sill is the opening's.
			const double Springing = FMath::Acos(FMath::Clamp(O.SillHalfWidthMeters() / Radius, -1.0, 1.0));
			const double Crown[1] = {UE_DOUBLE_HALF_PI};
			for (const double Angle : ArcStations(Radius, UE_DOUBLE_PI - Springing, Springing, DefaultChordToleranceMeters, Crown))
			{
				Across.Add(Radius * FMath::Cos(Angle));
			}
			return Across;
		}

		/**
		 * The face's distances as drawn (an arc measured round the arc), as a caller sets out an opening's centre or a drawn
		 * top, beside Form's U, measured on the face's chords from where its runs start (a closed path's first corner): the
		 * two at every station of the plan. A drawn distance lies on its segment's chord at the same fraction, so the map
		 * between them is linear between stations. Beyond the path's ends the two run on alike. Over a long curved face the
		 * chords fall short of the curve by a few parts in a hundred thousand: 1.7 mm round three quarters of the Rotunda's
		 * drum, enough to set a doorway off its axis.
		 */
		struct FAlongFace
		{
			TArray<double> Drawn;
			TArray<double> Chords;

			explicit FAlongFace(const FProfileStations& Plan)
			{
				const int32 Count = Plan.Vertices.Num();
				if (Count < 2)
				{
					return;
				}
				int32 Start = 0;
				if (Plan.bClosed)
				{
					const int32 Corner = Plan.Vertices.IndexOfByPredicate([](const FProfileVertex& Vertex) { return Vertex.bCrisp; });
					Start = Corner == INDEX_NONE ? 0 : Corner;
				}
				Drawn.Add(0.0);
				Chords.Add(0.0);
				for (int32 Step = 0; Step < Plan.NumSegments(); ++Step)
				{
					const int32 Segment = (Start + Step) % Count;
					Drawn.Add(Drawn.Last() + Plan.SegmentDrawnMeters(Segment));
					Chords.Add(Chords.Last() + FVector2D::Distance(Plan.Vertices[Segment].Point, Plan.Vertices[(Segment + 1) % Count].Point));
				}
			}

			/** A distance along the face as drawn, as a U on its chords. */
			double ToChords(double DrawnMeters) const
			{
				if (Drawn.Num() < 2 || DrawnMeters <= 0.0)
				{
					return DrawnMeters;
				}
				if (DrawnMeters >= Drawn.Last())
				{
					return Chords.Last() + DrawnMeters - Drawn.Last();
				}
				return Interpolate(Drawn, Chords, DrawnMeters);
			}
		};

		/**
		 * The wall's top along its face: level at its height, or on the drawn line's stations (U along the face, Z above the
		 * base), held level beyond its ends.
		 */
		struct FWallTop
		{
			double Level = 0.0;
			TArray<double> U;
			TArray<double> Z;

			double At(double Along) const { return U.IsEmpty() ? Level : Interpolate(U, Z, Along); }
		};

		bool ReadTop(const FFormWall& Wall, FWallTop& Out, FString& OutWhy)
		{
			Out.Level = Wall.HeightMeters;
			if (!Wall.HasDrawnTop())
			{
				return true;
			}
			if (!CheckProfile(Wall.Top, TEXT("the top"), false, OutWhy))
			{
				return false;
			}
			for (const FProfileVertex& Vertex : Resolve(Wall.Top).Vertices)
			{
				if (!Out.U.IsEmpty() && Vertex.Point.X <= Out.U.Last() + Same)
				{
					OutWhy = FString::Printf(TEXT("the top runs back or stands vertical at %.4f m along the face: it gives one height at each distance"),
						Vertex.Point.X);
					return false;
				}
				if (Vertex.Point.Y <= Same)
				{
					OutWhy = FString::Printf(TEXT("the top comes down to the base at %.4f m along the face"), Vertex.Point.X);
					return false;
				}
				Out.U.Add(Vertex.Point.X);
				Out.Z.Add(Vertex.Point.Y);
			}
			return true;
		}

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
			const double Half = O.SillHalfWidthMeters();
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
		bool SetOutOpenings(const FFormWall& Wall, const FWallTop& Top, const TArray<TArray<FSweepFrame>>& Runs, TArray<FOpeningSpan>& OutSpans,
			FString& OutWhy)
		{
			for (int32 Index = 0; Index < Wall.Openings.Num(); ++Index)
			{
				const FFormWallOpening& O = Wall.Openings[Index];
				// Under the top at its crown and at its jambs (where it meets its sill, for an arch springing below it).
				const double Half = O.SillHalfWidthMeters();
				const bool bUnderTop = O.CrownMeters() < Top.At(O.CenterMeters) - Same && O.HeadAt(Half) < Top.At(O.CenterMeters - Half) - Same
					&& O.HeadAt(Half) < Top.At(O.CenterMeters + Half) - Same;
				const bool bHeadAboveSill = O.HeadMeters > O.SillMeters || (O.bArched && O.CrownMeters() > O.SillMeters + Same);
				if (!(O.WidthMeters > 0.0 && O.SillMeters >= 0.0 && bHeadAboveSill && bUnderTop))
				{
					OutWhy = FString::Printf(TEXT("opening %d doesn't fit: it needs a width, a head above its sill, and a crown under the wall's top"), Index);
					return false;
				}
				if (!O.HasBuildableHead())
				{
					OutWhy = FString::Printf(TEXT("opening %d's arch has a radius under half its width, which can't span it"), Index);
					return false;
				}
				FOpeningSpan& Span = OutSpans.AddDefaulted_GetRef();
				Span.Opening = &O;
				Span.Low = O.CenterMeters - O.SillHalfWidthMeters();
				Span.High = O.CenterMeters + O.SillHalfWidthMeters();
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
					// Two openings may share the face's stretch when one stands wholly above the other: a door under a lunette.
					const FFormWallOpening& A = *Span.Opening;
					const FFormWallOpening& B = *OutSpans[Other].Opening;
					const bool bAlongside = OutSpans[Other].Low < Span.High + Same && Span.Low < OutSpans[Other].High + Same;
					const bool bOneAbove = A.CrownMeters() + Same < B.SillMeters || B.CrownMeters() + Same < A.SillMeters;
					if (bAlongside && !bOneAbove)
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
		 * across a straight passage's axis), the drawn top's, and, for a wall between two faces, across from each of the back
		 * line's.
		 */
		TArray<double> RequiredStations(const FFormWall& Wall, const FWallTop& Top, TConstArrayView<FOpeningSpan> Spans,
			const TArray<TArray<FSweepFrame>>& Runs)
		{
			TArray<double> Required = Top.U;
			for (const FOpeningSpan& Span : Spans)
			{
				Required.Add(Span.Low);
				Required.Add(Span.High);
				for (const double Across : HeadStationsAcross(*Span.Opening))
				{
					Required.Add(Span.UAt(Across));
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
			/** The wall's top along its face. */
			const FWallTop* Top = nullptr;

			const FSweepFrame& Frame(int32 Station) const { return Run.Frames[Station]; }

			/** The top above the base at U. */
			double TopAt(double U) const { return Top->At(U); }

			/** The openings a station or a strip lies within, from the lowest up. */
			TArray<const FOpeningSpan*> SpansAt(double U, bool bInclusive) const
			{
				TArray<const FOpeningSpan*> Found;
				for (const FOpeningSpan& S : Spans)
				{
					if (S.Holds(U, bInclusive))
					{
						Found.Add(&S);
					}
				}
				Found.Sort([](const FOpeningSpan& A, const FOpeningSpan& B) { return A.Opening->SillMeters < B.Opening->SillMeters; });
				return Found;
			}

			/** Every height at which the wall's surfaces break at station U, ascending. */
			TArray<double> BreaksAt(double U) const
			{
				TArray<double> Breaks = {0.0, TopAt(U)};
				for (const FOpeningSpan* S : SpansAt(U, true))
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
				const FVector Normal = QuadNormal(FVector::UpVector, Frame(Station).AxisA);
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

		/** The solid bands of a strip: from the bed (or each opening's head) up to the next opening's sill, or to the top. */
		TArray<FBand> BandsOf(const FWallSetOut& SetOut, double MidU)
		{
			const FWallTop* Top = SetOut.Top;
			TFunction<double(double)> Low = [](double) { return 0.0; };
			bool bFromBed = true;
			TArray<FBand> Bands;
			for (const FOpeningSpan* S : SetOut.SpansAt(MidU, false))
			{
				const double Sill = S->Opening->SillMeters;
				if (!bFromBed || Sill > Same)
				{
					Bands.Add(FBand{Low, [Sill](double) { return Sill; }});
				}
				Low = [S](double U) { return S->HeadAt(U); };
				bFromBed = false;
			}
			Bands.Add(FBand{Low, [Top](double U) { return Top->At(U); }});
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
			const FVector2D Down = FVector2D(HR - HL, -(SetOut.Frame(Right).U - SetOut.Frame(Left).U)).GetSafeNormal(StepSquaredMeters);
			const FVector Toward = (Along.GetSafeNormal2D(StepSquaredMeters) * Down.X + FVector::UpVector * Down.Y).GetSafeNormal();
			const FVector Normal = QuadNormal(Along, Across);
			if (Normal.IsNearlyZero())
			{
				return Toward;
			}
			return FVector::DotProduct(Normal, Toward) >= 0.0 ? Normal : -Normal;
		}

		/** The top of a strip: the normal of its own quad, turned up (level where the top is level, leaning where it rakes). */
		FVector TopNormal(const FWallSetOut& SetOut, int32 Left, double TL, double TR)
		{
			const FVector A0 = SetOut.PointAt(Left, TL, 0.0);
			const FVector Along = SetOut.PointAt(Left + 1, TR, 0.0) - A0;
			const FVector Across = SetOut.PointAt(Left, TL, SetOut.Depth) - A0;
			const FVector Normal = QuadNormal(Along, Across);
			if (Normal.IsNearlyZero())
			{
				return FVector::UpVector;
			}
			return Normal.Z >= 0.0 ? Normal : -Normal;
		}

		/** The top and the bed of every strip, and the sill and head linings of the strips inside openings. */
		void DrawAcross(const FWallSetOut& SetOut, int32 Left, FMeshBuilder& Mesh)
		{
			const int32 Right = Left + 1;
			const double UL = SetOut.Frame(Left).U;
			const double UR = SetOut.Frame(Right).U;
			const double TL = SetOut.TopAt(UL);
			const double TR = SetOut.TopAt(UR);
			Mesh.UseSlot(FFormWall::TopPart);
			AcrossQuad(SetOut, Left, TL, Right, TR, TopNormal(SetOut, Left, TL, TR), FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
			const TArray<const FOpeningSpan*> Openings = SetOut.SpansAt(0.5 * (UL + UR), false);
			if (Openings.IsEmpty() || Openings[0]->Opening->SillMeters > Same)
			{
				Mesh.UseSlot(FFormWall::BedPart);
				AcrossQuad(SetOut, Left, 0.0, Right, 0.0, -FVector::UpVector, FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
			}
			Mesh.UseSlot(FFormWall::RevealPart);
			for (const FOpeningSpan* S : Openings)
			{
				const double Sill = S->Opening->SillMeters;
				if (Sill > Same)
				{
					AcrossQuad(SetOut, Left, Sill, Right, Sill, FVector::UpVector, FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
				}
				const double HL = S->HeadAt(UL);
				const double HR = S->HeadAt(UR);
				AcrossQuad(SetOut, Left, HL, Right, HR, SoffitNormal(SetOut, Left, HL, HR), FVector2D(UL, HL), FVector2D(UR, HR), Mesh);
			}
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
			const double Top = SetOut.TopAt(F.U);
			Mesh.UseSlot(FFormWall::EndPart);
			AcrossQuad(SetOut, Station, 0.0, Station, Top, SetOut.AcrossNormal(Station, bStart ? -F.Along : F.Along), FVector2D(0.0, 0.0),
				FVector2D(Top, 0.0), Mesh);
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
			if ((!Shape.HasBack() && Shape.ThicknessMeters <= 0.0) || (!Shape.HasDrawnTop() && Shape.HeightMeters <= 0.0))
			{
				OutWhy = TEXT("a wall has a thickness (or a back) and a height (or a drawn top)");
				return false;
			}
			return true;
		}
	}

	bool DrawWall(const FFormWall& Drawn, FMeshBuilder& Mesh, FString& OutWhy)
	{
		FWallTop Top;
		if (!CheckWall(Drawn, OutWhy) || !ReadTop(Drawn, Top, OutWhy))
		{
			return false;
		}
		const FProfileStations Plan = Resolve(Drawn.Plan);
		// The openings and the top are set out along the face as drawn; the wall is made on its chords.
		const FAlongFace Along(Plan);
		FFormWall Shape = Drawn;
		for (FFormWallOpening& Opening : Shape.Openings)
		{
			Opening.CenterMeters = Along.ToChords(Opening.CenterMeters);
		}
		for (double& U : Top.U)
		{
			U = Along.ToChords(U);
		}
		// The openings are set out on the face's own stations first; the wall is then divided at every station they need.
		const TArray<TArray<FSweepFrame>> FaceRuns = PathRuns(Plan, Shape.BaseMeters, {});
		if (FaceRuns.IsEmpty())
		{
			OutWhy = TEXT("the plan has fewer than two stations, or doubles back too sharply to mitre");
			return false;
		}
		TArray<FOpeningSpan> Spans;
		if (!SetOutOpenings(Shape, Top, FaceRuns, Spans, OutWhy))
		{
			return false;
		}
		TArray<TArray<FSweepFrame>> Runs = PathRuns(Plan, Shape.BaseMeters, RequiredStations(Shape, Top, Spans, FaceRuns));
		// Where the face runs on smoothly (along a curve) the back stands the wall's thickness out along the station's normal,
		// on the offset curve itself, as the face's station stands on its curve; a mitre stretched across the chords would
		// set it past that curve (0.12 mm beyond the Rotunda's drum). At a corner the stretched mitre keeps the thickness.
		for (TArray<FSweepFrame>& Run : Runs)
		{
			for (FSweepFrame& Frame : Run)
			{
				if (FVector::CrossProduct(Frame.AxisA.GetSafeNormal(), Frame.NormalA).SizeSquared() < UE_DOUBLE_SMALL_NUMBER)
				{
					Frame.AxisA = Frame.NormalA;
				}
			}
		}
		const double Depth = Shape.HasBack() ? 1.0 : Shape.ThicknessMeters;
		for (int32 Index = 0; Index < Runs.Num(); ++Index)
		{
			FWallRun Run;
			if (!FindBacks(Shape, Spans, Runs[Index], Depth, Run, OutWhy))
			{
				return false;
			}
			const FWallSetOut SetOut{Shape, Spans, Run, Depth, &Top};
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
