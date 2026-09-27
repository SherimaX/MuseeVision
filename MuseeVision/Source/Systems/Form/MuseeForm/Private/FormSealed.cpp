#include "FormShells.h"
#include "MuseeCheck.h"

namespace
{
	FMuseeFinding OpenPiece(const FFormShells& Shells, const FFormEnclosure& Enclosure, const FFormPieceShell& Piece)
	{
		FMuseeMeasurement Reading;
		Reading.Label = TEXT("edges bounded by one triangle, more than two, or two running them the same way");
		Reading.Value = Piece.BoundaryEdges + Piece.NonManifoldEdges + Piece.MisorientedEdges;
		Reading.ExpectedMinimum = 0.0;
		Reading.ExpectedMaximum = 0.0;
		const TOptional<FMuseeKey> Subject = FMuseeKey::Parse(Piece.Piece.ToString());
		Reading = Reading.At(Subject.Get(FMuseeKey()), Shells.Stage, FVector::ZeroVector);
		return FMuseeFinding::Make(Piece.Drawer, FString::Printf(
			TEXT("piece %s of %s is not a closed solid: %d open, %d non-manifold and %d misoriented edges over %d triangles"),
			*Piece.Piece.ToString(), *Enclosure.Key.ToString(), Piece.BoundaryEdges, Piece.NonManifoldEdges,
			Piece.MisorientedEdges, Piece.Faces), Reading);
	}

	FMuseeFinding MissingPiece(const FFormShells& Shells, const FFormEnclosure& Enclosure, FName Piece)
	{
		FMuseeMeasurement Reading;
		Reading.Label = TEXT("pieces of that name made");
		Reading.ExpectedMinimum = 1.0;
		Reading = Reading.At(Enclosure.Key, Shells.Stage, FVector::ZeroVector);
		return FMuseeFinding::Make(FName(*Shells.Stage.ToString()), FString::Printf(
			TEXT("enclosure %s is bounded by %s, which the stage never made"), *Enclosure.Key.ToString(), *Piece.ToString()), Reading);
	}

	/**
	 * Every piece that bounds an Enclosed or Court space is a closed, consistently oriented solid: each edge of its
	 * skin is shared by exactly two triangles running it opposite ways, so light and water meet solid everywhere
	 * (Docs/Lessons.md, A1 and B10: closure proved, not hidden by overlaps).
	 */
	TArray<FMuseeFinding> CheckSealed(const FFormShells& Shells)
	{
		TArray<FMuseeFinding> Findings;
		for (const FFormEnclosure& Enclosure : Shells.Enclosures)
		{
			if (Enclosure.Kind == EFormEnclosureKind::Outdoors)
			{
				continue;
			}
			for (const FName Name : Enclosure.Pieces)
			{
				const FFormPieceShell* Piece = Shells.Pieces.FindByPredicate([Name](const FFormPieceShell& P) { return P.Piece == Name; });
				if (!Piece)
				{
					Findings.Add(MissingPiece(Shells, Enclosure, Name));
				}
				else if (!Piece->IsClosed())
				{
					Findings.Add(OpenPiece(Shells, Enclosure, *Piece));
				}
			}
		}
		return Findings;
	}

	MUSEE_CHECK("Form.Sealed", Stage, CheckSealed);
}
