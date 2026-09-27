#include "Growth/NatureSurface.h"
#include "Growth/Organs.h"
#include "NatureAnchors.h"
#include "NaturePieces.h"

namespace Musee::NatureMaking
{
	namespace
	{
		/**
		 * The water's edge is divided so no chord strays more than a millimetre from the true ellipse: the museum's
		 * chord tolerance, the one number Form divides every curve by (Docs/Architecture.md, Form).
		 */
		constexpr double ChordToleranceMeters = 0.001;

		/** The surface is laid in rings from the middle out, so no triangle runs the whole width of the water. */
		constexpr int32 SurfaceRings = 6;

		const FName WaterSurface(TEXT("water"));

		/** How many stations the edge of an ellipse needs, a multiple of four so both axes fall on a station. */
		int32 EdgeStations(const FVector2D& SemiAxes)
		{
			const double Largest = FMath::Max(SemiAxes.X, SemiAxes.Y);
			const double Step = 2.0 * FMath::Acos(1.0 - ChordToleranceMeters / Largest);
			return 4 * FMath::Max(3, FMath::CeilToInt(Tau / Step / 4.0));
		}
	}

	bool PieceFrame(const FMuseeStageContext& Stage, const FMuseeFrame& AnchorFrame, const FMuseeKey& Body, FMuseeFrame& OutFrame, FString& OutWhy)
	{
		if (Body.IsNone())
		{
			OutFrame = AnchorFrame;
			return true;
		}
		const FMuseeStageAnchor* Carrier = Stage.FindAnchor(Body);
		if (!Carrier || Carrier->Kind != AnchorKind::Body)
		{
			OutWhy = FString::Printf(TEXT("its body %s is not a Body anchor of the stage"), *Body.ToString());
			return false;
		}
		OutFrame = Carrier->Frame.FrameFromParent(AnchorFrame);
		return true;
	}

	bool MakeWaterPiece(const FMuseeStageContext& Stage, const FNatureWater& Water, FFormMadePiece& OutPiece, FString& OutWhy)
	{
		const FMuseeStageAnchor* Datum = Stage.FindAnchor(Water.Anchor);
		if (!Datum || Datum->Kind != AnchorKind::Water)
		{
			OutWhy = FString::Printf(TEXT("its anchor %s is not a Water anchor of the stage"), *Water.Anchor.ToString());
			return false;
		}
		if (!Water.Outline.IsValid() || Water.Depth <= 0.0f || Water.Freeboard < 0.0f || Water.Material.IsNone())
		{
			OutWhy = TEXT("it needs an ellipse of two semi-axes, a depth, a freeboard of zero or more and a material");
			return false;
		}
		if (!PieceFrame(Stage, Datum->Frame, Water.Body, OutPiece.Frame, OutWhy))
		{
			return false;
		}
		const FVector2D Axes = Water.Outline.SemiAxes();
		const int32 Stations = EdgeStations(Axes);
		FNatureSurface Surface;
		const int32 Middle = Surface.Vertex(FVector::ZeroVector, FVector::UpVector, FVector2D::ZeroVector, FLinearColor::White);
		TArray<int32> Previous;
		for (int32 Ring = 1; Ring <= SurfaceRings; ++Ring)
		{
			const double Share = double(Ring) / SurfaceRings;
			TArray<int32> Current;
			for (int32 S = 0; S < Stations; ++S)
			{
				const double Angle = Tau * S / Stations;
				const FVector Point(Axes.X * Share * FMath::Cos(Angle), Axes.Y * Share * FMath::Sin(Angle), 0.0);
				Current.Add(Surface.Vertex(Point, FVector::UpVector, FVector2D(Point.X, Point.Y), FLinearColor::White));
			}
			for (int32 S = 0; S < Stations; ++S)
			{
				const int32 Next = (S + 1) % Stations;
				Ring == 1 ? Surface.Triangle(Middle, Current[S], Current[Next]) : Surface.Quad(Previous[S], Previous[Next], Current[Next], Current[S]);
			}
			Previous = MoveTemp(Current);
		}
		OutPiece.Name = FName(Water.Key.Child(TEXT("surface")).ToString());
		OutPiece.Surface = WaterSurface;
		OutPiece.Material = Water.Material;
		OutPiece.Body = Water.Body;
		OutPiece.Collision = NAME_None;
		OutPiece.bNanite = false;
		Surface.Write(OutPiece.Mesh);
		return true;
	}
}
