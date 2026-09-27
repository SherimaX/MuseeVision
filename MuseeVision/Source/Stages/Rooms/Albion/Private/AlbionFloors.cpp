#include "AlbionParts.h"

/**
 * The floors (§3.3), their tops at z 0: the nave's encaustic field and its mitred border; the aisles' York flags, run a
 * centimetre under the walls; the porch's encaustic field in its black border; the court door's reveal in York stone,
 * split by the bronze strip on the wall's middle line. The tiles' pattern and the flags' courses are Matter's and Form's
 * setting-out, not separate pieces.
 */
namespace Musee::Albion
{
	namespace
	{
		/** A slab between two plan corners, its top at the floor, pierced by holes. */
		FFormDrawing Floor(const FString& Name, const FString& LocalSurface, const FVector2D& Min, const FVector2D& Max, double Thick,
			TArray<FFormProfile> Holes = {})
		{
			FFormExtrusion Shape;
			Shape.Outline = FFormProfile::Rectangle(Min, Max);
			Shape.Holes = MoveTemp(Holes);
			Shape.DepthMeters = Thick;
			return Drawn(Name, LocalSurface, MoveTemp(Shape), Walkable, FMuseeFrame(FVector(0.0, 0.0, -Thick)));
		}
	}

	void DrawFloors(TArray<FFormDrawing>& Out)
	{
		const double Tile = Plan::TileMeters;
		const double Flag = Plan::FlagMeters;
		const double B = Plan::BorderMeters;

		// The nave: the field inside a 0.30 border mitred at the arcade lines and the end walls.
		const FVector2D Field(Plan::NaveFieldHalfX, Plan::NaveFieldHalfY);
		Out.Add(Floor(TEXT("floor.nave"), TEXT("floor.nave"), -Field, Field, Tile));
		const FVector2D Nave(Plan::ArcadeX, Plan::EndInnerY);
		Out.Add(Floor(TEXT("floor.nave.border"), TEXT("floor.nave.border"), -Nave, Nave, Tile, {FFormProfile::Rectangle(-Field, Field)}));

		// The aisles, from the arcade lines to 1 cm under the walls.
		const double Wall = Plan::SideInnerX + Plan::UnderWall;
		const double End = Plan::EndInnerY + Plan::UnderWall;
		Out.Add(Floor(TEXT("floor.aisle.west"), TEXT("floor.aisle"), FVector2D(-Wall, -End), FVector2D(-Plan::ArcadeX, End), Flag));
		Out.Add(Floor(TEXT("floor.aisle.east"), TEXT("floor.aisle"), FVector2D(Plan::ArcadeX, -End), FVector2D(Wall, End), Flag));

		// The porch: its field and the black border round it, from the lining's face to the court's wall.
		const FVector2D PorchMin(-Plan::PorchFieldHalfX, Plan::PorchFieldNorth);
		const FVector2D PorchMax(Plan::PorchFieldHalfX, Plan::PorchFieldSouth);
		Out.Add(Floor(TEXT("floor.porch"), TEXT("porch.floor"), PorchMin, PorchMax, Tile));
		Out.Add(Floor(TEXT("floor.porch.border"), TEXT("porch.floor.border"), PorchMin - FVector2D(B, B), PorchMax + FVector2D(B, B), Tile,
			{FFormProfile::Rectangle(PorchMin, PorchMax)}));

		// The court door's reveal through the north wall, either side of the bronze strip on the wall's middle line.
		const double Half = Plan::CourtDoorHalf;
		const double Middle = -0.5 * (Plan::EndInnerY + Plan::EndOuterY);
		const double Strip = Plan::StripHalfWidth;
		Out.Add(Floor(TEXT("floor.door.reveal.north"), TEXT("door.reveal"), FVector2D(-Half, -Plan::EndOuterY), FVector2D(Half, Middle - Strip), Flag));
		Out.Add(Floor(TEXT("floor.door.reveal.south"), TEXT("door.reveal"), FVector2D(-Half, Middle + Strip), FVector2D(Half, -Plan::EndInnerY), Flag));
		Out.Add(Box(TEXT("floor.door.strip"), TEXT("door.strip"), FVector(-Plan::StripHalfLength, Middle - Strip, -Plan::StripMeters),
			FVector(Plan::StripHalfLength, Middle + Strip, Plan::StripProud), Walkable));
	}
}
