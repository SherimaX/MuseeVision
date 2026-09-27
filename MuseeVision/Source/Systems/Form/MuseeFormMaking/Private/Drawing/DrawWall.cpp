#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Tolerances.h"
#include "Kernel/Triangulation.h"
#include "Stations/Stations.h"

/**
 * A wall is drawn as strips between the stations of its plan path. Along the path a distance U (metres on the face)
 * and a height Z above the bed name a point of the face; the back is the same point ThicknessMeters to the path's
 * right, so an opening's reveals are cut square through the wall. Each strip is split round the openings that cross
 * it, and every strip edge carries every height at which its neighbours break (a sill, a head), so the strips, the
 * reveals, the top and the bed meet edge to edge and the wall is one closed solid.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/** Two heights or distances along the wall closer than the weld tolerance are one. */
		constexpr double Same = WeldToleranceMeters;

		struct FWallSetOut
		{
			const FFormWall& Wall;
			TConstArrayView<FSweepFrame> Run;

			/** The opening whose clear span holds U, strictly or (bInclusive) at its jambs too. */
			const FFormWallOpening* OpeningAt(double U, bool bInclusive) const
			{
				for (const FFormWallOpening& O : Wall.Openings)
				{
					const double Half = 0.5 * O.WidthMeters;
					const double Reach = bInclusive ? Half + Same : Half - Same;
					if (FMath::Abs(U - O.CenterMeters) <= Reach)
					{
						return &O;
					}
				}
				return nullptr;
			}

			/** The underside of an opening's head at U: the lintel, or the arch. */
			static double HeadAt(const FFormWallOpening& O, double U)
			{
				if (!O.bArched)
				{
					return O.HeadMeters;
				}
				const double Radius = 0.5 * O.WidthMeters;
				const double Across = FMath::Clamp(U - O.CenterMeters, -Radius, Radius);
				return O.HeadMeters + FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Across * Across));
			}

			/** Every height at which the wall's surfaces break at station U, ascending. */
			TArray<double> BreaksAt(double U) const
			{
				TArray<double> Breaks = {0.0, Wall.HeightMeters};
				if (const FFormWallOpening* O = OpeningAt(U, true))
				{
					Breaks.Add(O->SillMeters);
					Breaks.Add(HeadAt(*O, U));
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

			FVector PointAt(int32 Station, double Z, double Depth) const
			{
				return Run[Station].Origin + Run[Station].AxisA * Depth + FVector::UpVector * Z;
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
			const FFormWallOpening* O = SetOut.OpeningAt(MidU, false);
			if (!O)
			{
				return {FBand{[](double) { return 0.0; }, [Height](double) { return Height; }}};
			}
			TArray<FBand> Bands;
			const double Sill = O->SillMeters;
			if (Sill > Same)
			{
				Bands.Add(FBand{[](double) { return 0.0; }, [Sill](double) { return Sill; }});
			}
			Bands.Add(FBand{[O](double U) { return FWallSetOut::HeadAt(*O, U); }, [Height](double) { return Height; }});
			return Bands;
		}

		/** Draws one band of a strip on both faces, triangulated in (U, Z) with every break on its sides. */
		bool DrawBand(const FWallSetOut& SetOut, int32 Left, const FBand& Band, FMeshBuilder& Mesh)
		{
			const int32 Right = Left + 1;
			const double UL = SetOut.Run[Left].U;
			const double UR = SetOut.Run[Right].U;
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
			const double Thickness = SetOut.Wall.ThicknessMeters;
			for (const bool bBack : {false, true})
			{
				Mesh.UseSlot(bBack ? FFormWall::BackPart : FFormWall::FacePart);
				TArray<int32> Vertices;
				for (int32 Index = 0; Index < Outline.Num(); ++Index)
				{
					const FSweepFrame& F = SetOut.Run[StationOf[Index]];
					const FVector Normal = bBack ? F.NormalA : -F.NormalA;
					const FVector P = SetOut.PointAt(StationOf[Index], Outline[Index].Y, bBack ? Thickness : 0.0);
					Vertices.Add(Mesh.AddVertex(P, Normal, Outline[Index]));
				}
				for (const FIntVector& T : Triangles)
				{
					Mesh.AddTriangle(Vertices[T.X], Vertices[T.Y], Vertices[T.Z]);
				}
			}
			return true;
		}

		/** A quad across the wall's thickness between two points of its face line, facing Normal. */
		void AcrossQuad(const FWallSetOut& SetOut, int32 StationA, double ZA, int32 StationB, double ZB, const FVector& Normal,
			const FVector2D& UVA, const FVector2D& UVB, FMeshBuilder& Mesh)
		{
			const double Thickness = SetOut.Wall.ThicknessMeters;
			const int32 A0 = Mesh.AddVertex(SetOut.PointAt(StationA, ZA, 0.0), Normal, UVA);
			const int32 B0 = Mesh.AddVertex(SetOut.PointAt(StationB, ZB, 0.0), Normal, UVB);
			const int32 B1 = Mesh.AddVertex(SetOut.PointAt(StationB, ZB, Thickness), Normal, UVB + FVector2D(0.0, Thickness));
			const int32 A1 = Mesh.AddVertex(SetOut.PointAt(StationA, ZA, Thickness), Normal, UVA + FVector2D(0.0, Thickness));
			Mesh.AddQuad(A0, B0, B1, A1);
		}

		/** The top and the bed of every strip, and the sill and head linings of the strips inside openings. */
		void DrawAcross(const FWallSetOut& SetOut, int32 Left, FMeshBuilder& Mesh)
		{
			const int32 Right = Left + 1;
			const double UL = SetOut.Run[Left].U;
			const double UR = SetOut.Run[Right].U;
			const double Height = SetOut.Wall.HeightMeters;
			Mesh.UseSlot(FFormWall::TopPart);
			AcrossQuad(SetOut, Left, Height, Right, Height, FVector::UpVector, FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
			const FFormWallOpening* O = SetOut.OpeningAt(0.5 * (UL + UR), false);
			if (!O || O->SillMeters > Same)
			{
				Mesh.UseSlot(FFormWall::BedPart);
				AcrossQuad(SetOut, Left, 0.0, Right, 0.0, -FVector::UpVector, FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
			}
			if (!O)
			{
				return;
			}
			Mesh.UseSlot(FFormWall::RevealPart);
			if (O->SillMeters > Same)
			{
				AcrossQuad(SetOut, Left, O->SillMeters, Right, O->SillMeters, FVector::UpVector, FVector2D(UL, 0.0), FVector2D(UR, 0.0), Mesh);
			}
			const double HL = FWallSetOut::HeadAt(*O, UL);
			const double HR = FWallSetOut::HeadAt(*O, UR);
			// The soffit faces down, and under an arch in towards the arch's centre, along the face's own U and Z.
			const FVector Along = (SetOut.PointAt(Right, 0.0, 0.0) - SetOut.PointAt(Left, 0.0, 0.0)).GetSafeNormal();
			const FVector2D Chord(UR - UL, HR - HL);
			const FVector2D Down = FVector2D(Chord.Y, -Chord.X).GetSafeNormal();
			const FVector Normal = (Along * Down.X + FVector::UpVector * Down.Y).GetSafeNormal();
			AcrossQuad(SetOut, Left, HL, Right, HR, Normal, FVector2D(UL, HL), FVector2D(UR, HR), Mesh);
		}

		/** The lining of a jamb: its whole height at one station, split at every break, across the thickness. */
		void DrawJamb(const FWallSetOut& SetOut, int32 Station, const FFormWallOpening& O, bool bLeft, FMeshBuilder& Mesh)
		{
			const FSweepFrame& F = SetOut.Run[Station];
			const FVector Normal = bLeft ? F.Along : -F.Along;
			const double Head = FWallSetOut::HeadAt(O, F.U);
			TArray<double> Heights;
			for (const double Z : SetOut.BreaksAt(F.U))
			{
				if (Z >= O.SillMeters - Same && Z <= Head + Same)
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
			const FSweepFrame& F = SetOut.Run[Station];
			Mesh.UseSlot(FFormWall::EndPart);
			AcrossQuad(SetOut, Station, 0.0, Station, SetOut.Wall.HeightMeters, bStart ? -F.Along : F.Along, FVector2D(0.0, 0.0),
				FVector2D(SetOut.Wall.HeightMeters, 0.0), Mesh);
		}

		/** Every distance along the path at which the wall must have a station: jambs, and the stations of each arch. */
		TArray<double> RequiredStations(const FFormWall& Wall)
		{
			TArray<double> Required;
			for (const FFormWallOpening& O : Wall.Openings)
			{
				const double Radius = 0.5 * O.WidthMeters;
				Required.Add(O.CenterMeters - Radius);
				Required.Add(O.CenterMeters + Radius);
				if (O.bArched)
				{
					for (const double Angle : ArcStations(Radius, UE_DOUBLE_PI, 0.0, DefaultChordToleranceMeters))
					{
						Required.Add(O.CenterMeters + Radius * FMath::Cos(Angle));
					}
				}
			}
			Required.Sort();
			return Required;
		}

		bool CheckOpenings(const FFormWall& Wall, FString& OutWhy)
		{
			for (int32 Index = 0; Index < Wall.Openings.Num(); ++Index)
			{
				const FFormWallOpening& O = Wall.Openings[Index];
				const bool bFits = O.WidthMeters > 0.0 && O.SillMeters >= 0.0 && O.HeadMeters > O.SillMeters
					&& O.CrownMeters() < Wall.HeightMeters - Same;
				if (!bFits)
				{
					OutWhy = FString::Printf(TEXT("opening %d doesn't fit: it needs a width, a head above its sill, and a crown under the wall's top"), Index);
					return false;
				}
				for (int32 Other = Index + 1; Other < Wall.Openings.Num(); ++Other)
				{
					const FFormWallOpening& P = Wall.Openings[Other];
					if (FMath::Abs(P.CenterMeters - O.CenterMeters) < 0.5 * (P.WidthMeters + O.WidthMeters) + Same)
					{
						OutWhy = FString::Printf(TEXT("openings %d and %d overlap or touch"), Index, Other);
						return false;
					}
				}
			}
			return true;
		}

		/** True when every opening lies inside one of the runs, clear of its ends. */
		bool OpeningsInsideRuns(const FFormWall& Wall, const TArray<TArray<FSweepFrame>>& Runs)
		{
			for (const FFormWallOpening& O : Wall.Openings)
			{
				const bool bInside = Runs.ContainsByPredicate([&O](const TArray<FSweepFrame>& Run)
				{
					return O.CenterMeters - 0.5 * O.WidthMeters > Run[0].U + Same && O.CenterMeters + 0.5 * O.WidthMeters < Run.Last().U - Same;
				});
				if (!bInside)
				{
					return false;
				}
			}
			return true;
		}

		bool DrawRun(const FFormWall& Wall, TConstArrayView<FSweepFrame> Run, bool bStartsPath, bool bEndsPath, FMeshBuilder& Mesh)
		{
			const FWallSetOut SetOut{Wall, Run};
			for (int32 Left = 0; Left + 1 < Run.Num(); ++Left)
			{
				for (const FBand& Band : BandsOf(SetOut, 0.5 * (Run[Left].U + Run[Left + 1].U)))
				{
					if (!DrawBand(SetOut, Left, Band, Mesh))
					{
						return false;
					}
				}
				DrawAcross(SetOut, Left, Mesh);
			}
			for (const FFormWallOpening& O : Wall.Openings)
			{
				for (int32 Station = 0; Station < Run.Num(); ++Station)
				{
					const double Offset = Run[Station].U - O.CenterMeters;
					if (FMath::Abs(FMath::Abs(Offset) - 0.5 * O.WidthMeters) <= Same)
					{
						DrawJamb(SetOut, Station, O, Offset < 0.0, Mesh);
					}
				}
			}
			if (bStartsPath)
			{
				DrawEnd(SetOut, 0, true, Mesh);
			}
			if (bEndsPath)
			{
				DrawEnd(SetOut, Run.Num() - 1, false, Mesh);
			}
			return true;
		}
	}

	bool DrawWall(const FFormWall& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		if (!CheckProfile(Shape.Plan, TEXT("the plan"), false, OutWhy))
		{
			return false;
		}
		if (Shape.ThicknessMeters <= 0.0 || Shape.HeightMeters <= 0.0)
		{
			OutWhy = TEXT("a wall has a thickness and a height");
			return false;
		}
		if (!CheckOpenings(Shape, OutWhy))
		{
			return false;
		}
		const TArray<TArray<FSweepFrame>> Runs = PathRuns(Resolve(Shape.Plan), Shape.BaseMeters, RequiredStations(Shape));
		if (Runs.IsEmpty())
		{
			OutWhy = TEXT("the plan has fewer than two stations, or doubles back too sharply to mitre");
			return false;
		}
		if (!OpeningsInsideRuns(Shape, Runs))
		{
			OutWhy = TEXT("an opening crosses a corner or an end of the wall; an opening lies within one straight or smooth stretch");
			return false;
		}
		for (int32 Index = 0; Index < Runs.Num(); ++Index)
		{
			const bool bStart = !Shape.Plan.Closed && Index == 0;
			const bool bEnd = !Shape.Plan.Closed && Index == Runs.Num() - 1;
			if (!DrawRun(Shape, Runs[Index], bStart, bEnd, Mesh))
			{
				OutWhy = TEXT("a strip of the wall crosses itself between two stations");
				return false;
			}
		}
		return true;
	}
}
