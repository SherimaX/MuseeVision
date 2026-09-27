#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The Manet cabinet's shell (§3.1, §3.5): its three travertine walls standing off the Salon's north wall, its flat
 * ceiling at 5.500 round the laylight's well (the well's sides rise to the etched laylight at 5.900, the diffuser of a
 * luminous ceiling in the void above), and its roof slab. Its silk, cornice (P8) and skirting are drawn with the
 * Salon's.
 */
namespace Musee::Salon
{
	namespace
	{
		/** The masonry faces behind the cabinet's silk. */
		FBox2D CabinetMasonry()
		{
			const double B = Plan::SilkBuildUp;
			return FBox2D(FVector2D(Plan::CabinetWest - B, Plan::CabinetNorth - B), FVector2D(Plan::CabinetEast + B, Plan::CabinetSouth + B));
		}
	}

	void DrawCabinet(TArray<FFormDrawing>& Out)
	{
		const FBox2D Inside = CabinetMasonry();

		// East, north and west walls, walked with the wall on the right from the Salon's wall round to it again.
		FFormWall Walls;
		Walls.Plan = FFormProfile::Polyline({FVector2D(Inside.Max.X, Inside.Max.Y), FVector2D(Inside.Max.X, Inside.Min.Y), Inside.Min,
			FVector2D(Inside.Min.X, Inside.Max.Y)}, false);
		Walls.ThicknessMeters = Plan::CabinetWall - Plan::SilkBuildUp;
		Walls.BaseMeters = Plan::WallBed;
		Walls.HeightMeters = Plan::CabinetRoofBottom - Plan::WallBed;
		FFormDrawing Wall = Drawn(Pieces::CabinetWall, TEXT("pier"), MoveTemp(Walls), Solid);
		for (const TCHAR* Part : {FFormWall::BackPart, FFormWall::TopPart, FFormWall::BedPart, FFormWall::EndPart})
		{
			Wall.PartSurfaces.Add(Part, Surface(TEXT("exterior.wall")));
		}
		Out.Add(MoveTemp(Wall));

		// The ceiling slab up to the laylight, its well glazed flush with its top by the laylight.
		FFormCeiling Ceiling;
		Ceiling.Outline = FFormProfile::Rectangle(Inside.Min, Inside.Max);
		Ceiling.Opening = FFormProfile::Rectangle(FVector2D(Plan::WellWest, Plan::WellNorth), FVector2D(Plan::WellEast, Plan::WellSouth));
		Ceiling.PaneMeters = Plan::InnerPane;
		Ceiling.PaneLevelMeters = Plan::LaylightZ - Plan::CabinetCeiling;
		Ceiling.ThicknessMeters = Ceiling.PaneLevelMeters + Ceiling.PaneMeters;
		Ceiling.GlazingSurface = Surface(TEXT("cabinet.laylight"));
		Out.Add(Drawn(Pieces::CabinetCeiling, TEXT("cabinet.ceiling"), MoveTemp(Ceiling), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::CabinetCeiling))));

		// The roof slab over the walls' outer faces, butting the Salon's north wall.
		const double Out0 = Plan::CabinetWall;
		Out.Add(PlanSlab(Pieces::CabinetRoof, TEXT("roof.paving"), FFormProfile::Rectangle(FVector2D(Plan::CabinetWest - Out0, Plan::CabinetNorth - Out0),
			FVector2D(Plan::CabinetEast + Out0, Inside.Max.Y)), {}, Plan::CabinetRoofBottom, Plan::CabinetRoofTop, Solid));
	}
}
