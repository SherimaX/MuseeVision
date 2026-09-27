#include "Drawing/Draw.h"

#include "Kernel/MeshBuilder.h"

/**
 * Each letter is cast as the founder casts it: its bed on the outline, plumb sides up to the bevel, the bevel's band in to
 * the face, and the face over the same triangles as the bed, moved in along each point's mitre (FFormGlyph). The letters
 * are set out along the line by their advances and the tracking between them, the words by their spaces and stops, and
 * the line is centred on its place.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/** Where one letter stands: its baseline's middle, its axis along the line and its up, in the drawing's plane. */
		struct FLetterPlace
		{
			FVector2D Middle = FVector2D::ZeroVector;
			FVector2D Along = FVector2D(1.0, 0.0);
			FVector2D Up = FVector2D(0.0, 1.0);
		};

		/** Whether a character is a letter of the typeface: neither a space nor a stop. */
		bool IsLetter(TCHAR Character)
		{
			return Character != TEXT(' ') && Character != FFormLettering::Interpunct;
		}

		/** How far the pen moves on over the character at Index, in cap heights: tracking only between two letters. */
		double LetterWidth(const FFormLettering& Shape, int32 Index)
		{
			const TCHAR Character = Shape.Text[Index];
			if (Character == TEXT(' '))
			{
				return Shape.WordSpaceCaps;
			}
			if (Character == FFormLettering::Interpunct)
			{
				return Shape.StopGapCaps;
			}
			const FFormGlyph* Glyph = Shape.Typeface.Find(Character);
			const bool bLetterFollows = Index + 1 < Shape.Text.Len() && IsLetter(Shape.Text[Index + 1]);
			return (Glyph ? Glyph->Advance : 0.0) + (bLetterFollows ? Shape.TrackingCaps : 0.0);
		}

		/** The place of the letter whose middle lies Along metres from the line's middle. */
		FLetterPlace PlaceAt(const FFormLettering& Shape, double Along)
		{
			FLetterPlace Place;
			if (Shape.BaselineRadiusMeters <= 0.0)
			{
				Place.Middle = FVector2D(Along, 0.0);
				return Place;
			}
			const double Radius = Shape.BaselineRadiusMeters;
			const double Sense = Shape.bTopsInward ? -1.0 : 1.0;
			const double Angle = FMath::DegreesToRadians(Shape.CentreDegrees) + Sense * Along / Radius;
			const FVector2D Radial(FMath::Cos(Angle), FMath::Sin(Angle));
			Place.Middle = Radial * Radius;
			Place.Along = FVector2D(-Radial.Y, Radial.X) * Sense;
			Place.Up = Shape.bTopsInward ? -Radial : Radial;
			return Place;
		}

		/** The outward normal of a letter's outline edge in the plane: the right of its travel where the solid is on its left. */
		FVector2D EdgeOut(const FVector2D& From, const FVector2D& To, double Handedness)
		{
			const FVector2D Direction = (To - From).GetSafeNormal();
			return FVector2D(Direction.Y, -Direction.X) * Handedness;
		}

		/** One letter, cast at its place. */
		void CastLetter(const FFormLettering& Shape, const FFormGlyph& Glyph, const FLetterPlace& Place, FMeshBuilder& Mesh)
		{
			const double Cap = Shape.CapMeters;
			const double Bevel = Shape.BevelMeters;
			const double Proud = Shape.ProudMeters;
			const double Shoulder = Proud - Bevel;
			// A letter's own (x, y) turned into the drawing's plane keeps or turns over its outlines' winding.
			const double Handedness = FVector2D::CrossProduct(Place.Along, Place.Up) >= 0.0 ? 1.0 : -1.0;
			auto Local = [&](const FVector2D& Point) { return FVector2D((Point.X - 0.5 * Glyph.Advance) * Cap, Point.Y * Cap); };
			auto InPlane = [&](const FVector2D& L) { return Place.Middle + Place.Along * L.X + Place.Up * L.Y; };
			const int32 Count = Glyph.Points.Num();
			TArray<FVector2D> Bed, Face, BedLocal, FaceLocal;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				BedLocal.Add(Local(Glyph.Points[Index]));
				FaceLocal.Add(Local(Glyph.Points[Index] + Glyph.Mitres[Index] * (Bevel / Cap)));
				Bed.Add(InPlane(BedLocal.Last()));
				Face.Add(InPlane(FaceLocal.Last()));
			}
			for (const bool bFace : {false, true})
			{
				TArray<int32> Vertices;
				for (int32 Index = 0; Index < Count; ++Index)
				{
					const FVector2D& P = bFace ? Face[Index] : Bed[Index];
					Vertices.Add(Mesh.AddVertex(FVector(P.X, P.Y, bFace ? Proud : 0.0), bFace ? FVector::UpVector : -FVector::UpVector,
						bFace ? FaceLocal[Index] : BedLocal[Index]));
				}
				for (int32 Corner = 0; Corner + 2 < Glyph.Triangles.Num(); Corner += 3)
				{
					Mesh.AddTriangle(Vertices[Glyph.Triangles[Corner]], Vertices[Glyph.Triangles[Corner + 1]], Vertices[Glyph.Triangles[Corner + 2]]);
				}
			}
			for (int32 Contour = 0; Contour < Glyph.ContourStarts.Num(); ++Contour)
			{
				const int32 First = Glyph.ContourStarts[Contour];
				const int32 End = Contour + 1 < Glyph.ContourStarts.Num() ? Glyph.ContourStarts[Contour + 1] : Count;
				const int32 Points = End - First;
				auto Next = [First, Points](int32 Index) { return First + (Index - First + 1) % Points; };
				auto Previous = [First, Points](int32 Index) { return First + (Index - First + Points - 1) % Points; };
				double Travelled = 0.0;
				for (int32 Index = First; Index < End; ++Index)
				{
					const int32 To = Next(Index);
					const FVector2D Out = EdgeOut(Bed[Index], Bed[To], Handedness);
					// At a smooth point the side's normal is the mean of the two edges' there, so the curve reads round.
					auto SideNormal = [&](int32 At, bool bStart)
					{
						if (!Glyph.Smooth[At])
						{
							return FVector(Out.X, Out.Y, 0.0);
						}
						const FVector2D Other = bStart ? EdgeOut(Bed[Previous(At)], Bed[At], Handedness) : EdgeOut(Bed[At], Bed[Next(At)], Handedness);
						const FVector2D Mean = (Out + Other).GetSafeNormal();
						return FVector(Mean.X, Mean.Y, 0.0);
					};
					const double Length = FVector2D::Distance(Bed[Index], Bed[To]);
					const FVector N0 = SideNormal(Index, true);
					const FVector N1 = SideNormal(To, false);
					const int32 A0 = Mesh.AddVertex(FVector(Bed[Index].X, Bed[Index].Y, 0.0), N0, FVector2D(Travelled, 0.0));
					const int32 B0 = Mesh.AddVertex(FVector(Bed[To].X, Bed[To].Y, 0.0), N1, FVector2D(Travelled + Length, 0.0));
					const int32 B1 = Mesh.AddVertex(FVector(Bed[To].X, Bed[To].Y, Shoulder), N1, FVector2D(Travelled + Length, Shoulder));
					const int32 A1 = Mesh.AddVertex(FVector(Bed[Index].X, Bed[Index].Y, Shoulder), N0, FVector2D(Travelled, Shoulder));
					Mesh.AddQuad(A0, B0, B1, A1);
					if (Bevel > 0.0)
					{
						// The bevel's band leans in from the side's top to the face; its normal leans out and up by its slope.
						const FVector Lean = FVector(Out.X * Bevel, Out.Y * Bevel, (Bed[Index] - Face[Index]).Size()).GetSafeNormal();
						const FVector Band0 = (N0 * Bevel + FVector::UpVector * (Bed[Index] - Face[Index]).Size()).GetSafeNormal();
						const FVector Band1 = (N1 * Bevel + FVector::UpVector * (Bed[To] - Face[To]).Size()).GetSafeNormal();
						const int32 C0 = Mesh.AddVertex(FVector(Bed[Index].X, Bed[Index].Y, Shoulder), Band0.IsNearlyZero() ? Lean : Band0,
							FVector2D(Travelled, Shoulder));
						const int32 D0 = Mesh.AddVertex(FVector(Bed[To].X, Bed[To].Y, Shoulder), Band1.IsNearlyZero() ? Lean : Band1,
							FVector2D(Travelled + Length, Shoulder));
						const int32 D1 = Mesh.AddVertex(FVector(Face[To].X, Face[To].Y, Proud), Band1.IsNearlyZero() ? Lean : Band1,
							FVector2D(Travelled + Length, Proud));
						const int32 C1 = Mesh.AddVertex(FVector(Face[Index].X, Face[Index].Y, Proud), Band0.IsNearlyZero() ? Lean : Band0,
							FVector2D(Travelled, Proud));
						Mesh.AddQuad(C0, D0, D1, C1);
					}
					Travelled += Length;
				}
			}
		}

		/**
		 * One stop, cast in the middle of its gap: a triangular pyramid, point down, its base on the bed and its apex over the
		 * triangle's incentre, where its three faces meet.
		 */
		void CastStop(const FFormLettering& Shape, const FLetterPlace& Place, FMeshBuilder& Mesh)
		{
			const double Cap = Shape.CapMeters;
			auto InPlane = [&](double X, double Y) { return Place.Middle + Place.Along * (X * Cap) + Place.Up * (Y * Cap); };
			const FVector2D Corners[3] = {InPlane(-Shape.StopHalfWidthCaps, Shape.StopTopCaps), InPlane(Shape.StopHalfWidthCaps, Shape.StopTopCaps),
				InPlane(0.0, Shape.StopPointCaps)};
			const double Opposite[3] = {FVector2D::Distance(Corners[1], Corners[2]), FVector2D::Distance(Corners[2], Corners[0]),
				FVector2D::Distance(Corners[0], Corners[1])};
			const FVector2D Incentre = (Corners[0] * Opposite[0] + Corners[1] * Opposite[1] + Corners[2] * Opposite[2])
				/ (Opposite[0] + Opposite[1] + Opposite[2]);
			const FVector Apex(Incentre.X, Incentre.Y, Shape.StopApexMeters);
			Mesh.UseSlot(FFormLettering::StopPart);
			int32 Bed[3];
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				Bed[Corner] = Mesh.AddVertex(FVector(Corners[Corner].X, Corners[Corner].Y, 0.0), -FVector::UpVector, (Corners[Corner] - Incentre));
			}
			Mesh.AddTriangle(Bed[0], Bed[1], Bed[2]);
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const FVector A(Corners[Corner].X, Corners[Corner].Y, 0.0);
				const FVector B(Corners[(Corner + 1) % 3].X, Corners[(Corner + 1) % 3].Y, 0.0);
				FVector Facet = FVector::CrossProduct(B - A, Apex - A).GetSafeNormal();
				const FVector Outward = 0.5 * (A + B) - FVector(Incentre.X, Incentre.Y, 0.0);
				Facet = FVector::DotProduct(Facet, Outward) >= 0.0 ? Facet : -Facet;
				const double Run = FVector::Distance(A, B);
				const int32 VA = Mesh.AddVertex(A, Facet, FVector2D(0.0, 0.0));
				const int32 VB = Mesh.AddVertex(B, Facet, FVector2D(Run, 0.0));
				const int32 VApex = Mesh.AddVertex(Apex, Facet, FVector2D(0.5 * Run, FVector::Distance(0.5 * (A + B), Apex)));
				Mesh.AddTriangle(VA, VB, VApex);
			}
			Mesh.UseSlot(NAME_None);
		}

		bool CheckLettering(const FFormLettering& Shape, FString& OutWhy)
		{
			if (Shape.CapMeters <= 0.0 || Shape.ProudMeters <= 0.0 || Shape.BevelMeters < 0.0 || Shape.BevelMeters >= Shape.ProudMeters)
			{
				OutWhy = TEXT("lettering has a cap height, stands proud, and its bevel is shallower than its letters are proud");
				return false;
			}
			if (Shape.BevelMeters / Shape.CapMeters > Shape.Typeface.MaxBevel)
			{
				OutWhy = FString::Printf(TEXT("a bevel of %.4f cap heights is deeper than %s's mitres allow (%.4f)"), Shape.BevelMeters / Shape.CapMeters,
					*Shape.Typeface.Key.ToString(), Shape.Typeface.MaxBevel);
				return false;
			}
			bool bAnyLetter = false;
			for (const TCHAR Character : Shape.Text)
			{
				if (Character == FFormLettering::Interpunct)
				{
					if (Shape.StopGapCaps <= 0.0 || Shape.StopHalfWidthCaps <= 0.0 || Shape.StopTopCaps <= Shape.StopPointCaps || Shape.StopApexMeters <= 0.0)
					{
						OutWhy = TEXT("a stop needs its gap, its triangle (its top above its point) and its apex");
						return false;
					}
					continue;
				}
				FString Why;
				const FFormGlyph* Glyph = Shape.Typeface.Find(Character);
				if (Character != TEXT(' ') && (!Glyph || !Glyph->IsWellFormed(&Why)))
				{
					OutWhy = FString::Printf(TEXT("%s has no whole letter %c %s"), *Shape.Typeface.Key.ToString(), Character, *Why);
					return false;
				}
				bAnyLetter |= Character != TEXT(' ');
			}
			return bAnyLetter;
		}
	}

	bool DrawLettering(const FFormLettering& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		if (!CheckLettering(Shape, OutWhy))
		{
			if (OutWhy.IsEmpty())
			{
				OutWhy = TEXT("the lettering has no letters");
			}
			return false;
		}
		double Total = 0.0;
		for (int32 Index = 0; Index < Shape.Text.Len(); ++Index)
		{
			Total += LetterWidth(Shape, Index);
		}
		double Pen = -0.5 * Total;
		for (int32 Index = 0; Index < Shape.Text.Len(); ++Index)
		{
			const TCHAR Character = Shape.Text[Index];
			if (Character == FFormLettering::Interpunct)
			{
				CastStop(Shape, PlaceAt(Shape, (Pen + 0.5 * Shape.StopGapCaps) * Shape.CapMeters), Mesh);
			}
			else if (const FFormGlyph* Glyph = IsLetter(Character) ? Shape.Typeface.Find(Character) : nullptr)
			{
				CastLetter(Shape, *Glyph, PlaceAt(Shape, (Pen + 0.5 * Glyph->Advance) * Shape.CapMeters), Mesh);
			}
			Pen += LetterWidth(Shape, Index);
		}
		return true;
	}
}
