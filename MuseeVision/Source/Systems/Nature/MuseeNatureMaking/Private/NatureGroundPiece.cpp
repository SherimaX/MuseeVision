#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FormFace.h"
#include "FormMake.h"
#include "FormMesh.h"
#include "Growth/Organs.h"
#include "Growth/Plant.h"
#include "MuseeCollision.h"
#include "MuseeUnits.h"
#include "NatureAnchors.h"
#include "NaturePieces.h"

namespace Musee::NatureMaking
{
	namespace
	{
		/**
		 * An ellipse's edge is divided so no chord strays more than a millimetre from it: the museum's chord tolerance, the
		 * one number Form divides every curve by (Docs/Architecture.md, Form).
		 */
		constexpr double ChordToleranceMeters = 0.001;

		/** The ground's surface key, which Form's face takes; the piece brings its own material. */
		const FName GroundSurface(TEXT("ground"));

		/** A face of a piece is its top when it looks within a degree of straight up. */
		const double UpwardCosine = FMath::Cos(FMath::DegreesToRadians(1.0));

		/** A face lies at a piece's top when it is within a millimetre of its highest upward face (metres). */
		constexpr double TopToleranceMeters = 0.001;

		bool Inside(const TArray<FVector2D>& Corners, const FVector2D& Point)
		{
			bool bInside = false;
			for (int32 Index = 0, Previous = Corners.Num() - 1; Index < Corners.Num(); Previous = Index++)
			{
				const FVector2D& A = Corners[Index];
				const FVector2D& B = Corners[Previous];
				if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < A.X + (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y))
				{
					bInside = !bInside;
				}
			}
			return bInside;
		}

		/** A made face over a region in a frame's plane, or why it can't be made. */
		bool MakeFace(const FName Name, const FBedRegion& Region, FFormMadePiece& OutPiece, FString& OutWhy)
		{
			FFormFace Face;
			Face.Outline = FFormProfile::Polyline(Region.Outline, true);
			for (const TArray<FVector2D>& Hole : Region.Holes)
			{
				Face.Holes.Add(FFormProfile::Polyline(Hole, true));
			}
			TArray<FFormMadePiece> Made;
			TArray<FMuseeFinding> Findings;
			Musee::FormMaking::Make(FFormDrawing::Of(Name, GroundSurface, MoveTemp(Face)), Made, Findings);
			if (Made.Num() != 1)
			{
				OutWhy = Findings.IsEmpty() ? FString(TEXT("its outline draws no surface")) : Findings[0].Summary;
				return false;
			}
			OutPiece = MoveTemp(Made[0]);
			return true;
		}
	}

	TArray<FVector2D> PlanPolygon(const FNatureOutline& Outline, const FMuseeFrame& Anchor)
	{
		if (!Outline.IsEllipse())
		{
			return Outline.Polygon;
		}
		const FVector2D Axes = Outline.SemiAxes();
		const double Largest = FMath::Max(Axes.X, Axes.Y);
		const int32 Stations = FMath::Max(12, FMath::CeilToInt(Tau / (2.0 * FMath::Acos(1.0 - ChordToleranceMeters / FMath::Max(Largest, ChordToleranceMeters)))));
		TArray<FVector2D> Out;
		for (int32 S = 0; S < Stations; ++S)
		{
			const double A = Tau * S / Stations;
			const FVector P = Anchor.PointToParent(FVector(Axes.X * FMath::Cos(A), Axes.Y * FMath::Sin(A), 0.0));
			Out.Add(FVector2D(P.X, P.Y));
		}
		return Out;
	}

	FBedRegion RegionOf(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, const FNatureBed& Entry, const FMuseeFrame& Bed)
	{
		FBedRegion Out;
		if (!Entry.Outline.IsValid())
		{
			return Out;
		}
		const TArray<FVector2D> Plan2 = PlanPolygon(Entry.Outline, Bed);
		const auto ToBed = [&Bed](const TArray<FVector2D>& Points)
		{
			TArray<FVector2D> Local;
			for (const FVector2D& P : Points)
			{
				const FVector L = Bed.PointFromParent(FVector(P.X, P.Y, Bed.OriginMeters.Z));
				Local.Add(FVector2D(L.X, L.Y));
			}
			return Local;
		};
		Out.Outline = ToBed(Plan2);
		// The water that lies wholly in the bed is cut out of it: nothing grows on the ground under a pond.
		for (const FNatureWater& Water : Plan.Water)
		{
			const FMuseeStageAnchor* Datum = Stage.FindAnchor(Water.Anchor, AnchorKind::Water);
			if (!Datum || !Water.Outline.IsValid())
			{
				continue;
			}
			const TArray<FVector2D> Hole = PlanPolygon(Water.Outline, Datum->Frame);
			if (!Hole.ContainsByPredicate([&Plan2](const FVector2D& P) { return !Inside(Plan2, P); }))
			{
				Out.Holes.Add(ToBed(Hole));
			}
		}
		return Out;
	}

	bool MakeGroundPiece(const FMuseeStageContext& Stage, const FNatureBed& Entry, const FBedRegion& Region, const FMuseeFrame& Bed, FFormMadePiece& OutPiece,
		FString& OutWhy)
	{
		const FNatureGround& Ground = Entry.Ground;
		if (Ground.Working.IsNone() || Ground.History.IsNone())
		{
			OutWhy = TEXT("its ground needs its material with its working and history");
			return false;
		}
		if (Region.Outline.Num() < 3)
		{
			OutWhy = TEXT("its ground needs the bed's outline, an ellipse or a polygon");
			return false;
		}
		FMuseeFrame Frame;
		if (!PieceFrame(Stage, Bed, Entry.Body, Frame, OutWhy) || !MakeFace(FName(Entry.Bed.Child(TEXT("ground")).ToString()), Region, OutPiece, OutWhy))
		{
			return false;
		}
		OutPiece.Material = Ground.Material;
		OutPiece.Working = Ground.Working;
		OutPiece.History = Ground.History;
		OutPiece.Body = Entry.Body;
		OutPiece.Collision = Collision::Walkable.Profile();
		OutPiece.Frame = Frame;
		return true;
	}

	namespace
	{
		/**
		 * A carpet over the top of one of the stage's own pieces (the grounds' terrain, cut round every building on it): the
		 * piece's faces that look straight up at its highest level, laid again as a surface of their own in the bed's frame
		 * (the piece is placed at the bed raised to the carpet's height, so it stands that high over the top), UV0 metres
		 * across the plan, the shoots' greens varying over it. Fails, saying why, when the stage has no such piece or it has
		 * no top.
		 */
		bool LayOnTop(TConstArrayView<FFormMadePiece> StagePieces, FName On, const FMuseeFrame& Bed, const FNatureSpecies& Species, FRandomStream& Random,
			FFormMadePiece& OutPiece, FString& OutWhy)
		{
			const FFormMadePiece* Under = StagePieces.FindByPredicate([On](const FFormMadePiece& Piece) { return Piece.Name == On; });
			if (!Under)
			{
				OutWhy = FString::Printf(TEXT("the stage has no piece %s to lie on"), *On.ToString());
				return false;
			}
			const UE::Geometry::FDynamicMesh3& Mesh = *Under->Mesh;
			const auto InStage = [Under](const FVector& Engine) { return Under->Frame.PointToParent(FromEngine(Engine)); };
			TArray<int32> Upward;
			double Highest = -TNumericLimits<double>::Max();
			for (const int32 Triangle : Mesh.TriangleIndicesItr())
			{
				if (Under->Frame.Rotation.RotateVector(Mesh.GetTriNormal(Triangle)).Z >= UpwardCosine)
				{
					FVector A, B, C;
					Mesh.GetTriVertices(Triangle, A, B, C);
					Upward.Add(Triangle);
					Highest = FMath::Max(Highest, FMath::Max3(InStage(A).Z, InStage(B).Z, InStage(C).Z));
				}
			}
			FFormMesh Top;
			for (const int32 Triangle : Upward)
			{
				FVector Corners[3];
				Mesh.GetTriVertices(Triangle, Corners[0], Corners[1], Corners[2]);
				for (FVector& Corner : Corners)
				{
					Corner = InStage(Corner);
				}
				if (FMath::Min3(Corners[0].Z, Corners[1].Z, Corners[2].Z) < Highest - TopToleranceMeters)
				{
					continue;
				}
				int32 Indices[3];
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					const FVector Local = Bed.PointFromParent(Corners[Corner]);
					Indices[Corner] = Top.AddVertex(Local, FVector::UpVector, FVector2D(Local.X, Local.Y));
					Top.SetColor(Indices[Corner], Vary(AnyColour(Species.LeafColours, Random), Random, 6.0, 0.1, 0.15));
				}
				Top.AddTriangle(Indices[0], Indices[1], Indices[2]);
			}
			if (Top.IsEmpty())
			{
				OutWhy = FString::Printf(TEXT("the piece %s has no top to lie on"), *On.ToString());
				return false;
			}
			Top.Write(OutPiece.Mesh);
			return true;
		}
	}

	bool MakeCarpetPiece(const FMuseeStageContext& Stage, const FNatureBed& Entry, const FNatureSpecies& Species, const FBedRegion& Region, const FMuseeFrame& Bed,
		TConstArrayView<FFormMadePiece> StagePieces, FFormMadePiece& OutPiece, FString& OutWhy)
	{
		const bool bOnPiece = !Entry.On.IsNone();
		if (!bOnPiece && Region.Outline.Num() < 3)
		{
			OutWhy = TEXT("a carpet needs the bed's outline, or the stage's piece it lies on, to grow over");
			return false;
		}
		FRandomStream Random(Seed(Entry.Bed.Child(Species.Key.ToString())));
		// A mown sward stands at the height it is mown to; a moss at its shoots' length.
		const double Depth = Entry.Mown > 0.0f ? double(Entry.Mown) : Draw(Species.LeafLengthMeters, Random);
		FMuseeFrame Raised = Bed;
		Raised.OriginMeters += Bed.Rotation.GetAxisZ() * Depth;
		FMuseeFrame Frame;
		const FName Name(FString::Printf(TEXT("%s.%s.%s"), *Entry.Bed.ToString(), *Species.Key.ToString(), *Organ::Leaf.ToString()));
		if (!PieceFrame(Stage, Raised, Entry.Body, Frame, OutWhy))
		{
			return false;
		}
		if (bOnPiece)
		{
			if (!LayOnTop(StagePieces, Entry.On, Bed, Species, Random, OutPiece, OutWhy))
			{
				return false;
			}
			OutPiece.Name = Name;
		}
		else if (!MakeFace(Name, Region, OutPiece, OutWhy))
		{
			return false;
		}
		// The shoots' greens vary from place to place over the carpet.
		else if (UE::Geometry::FDynamicMeshColorOverlay* Colours = OutPiece.Mesh->HasAttributes() ? OutPiece.Mesh->Attributes()->PrimaryColors() : nullptr)
		{
			for (const int32 Element : Colours->ElementIndicesItr())
			{
				const FLinearColor Tint = Vary(AnyColour(Species.LeafColours, Random), Random, 6.0, 0.1, 0.15);
				Colours->SetElement(Element, FVector4f(Tint.R, Tint.G, Tint.B, 1.0f));
			}
		}
		OutPiece.Surface = FName(FString::Printf(TEXT("plant.%s"), *Organ::Leaf.ToString()));
		OutPiece.Material = Entry.Cast.IsSet() ? Entry.Cast.Material : Species.MaterialOf(Organ::Leaf);
		OutPiece.Working = Entry.Working;
		OutPiece.History = Entry.History;
		OutPiece.Body = Entry.Body;
		OutPiece.Collision = NAME_None;
		OutPiece.Frame = Frame;
		return true;
	}
}
