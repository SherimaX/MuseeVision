#include "CubeParts.h"
#include "CubeStrata.h"

/**
 * The ground's cut face round the shaft (§3.8, §7): a height field on the inward-facing cylinder r 2.660, from the head
 * plate's underside to the foot plate's top, drawn by Form from the stratigraphic rule at a 12.5 mm step and consolidated
 * in place as a soil monolith is. Its back face is r 2.860; beyond the plates' r 2.770 it reaches up to the slab and down
 * to the roof as two thin collars. The ammonite lies in the chalk at bearing 58, coil to the shaft, half exposed.
 *
 * One piece per layer. The rule's contacts wave and the solution pipe drops the clay 0.75 into the chalk, but a relief's
 * band is bounded by two heights, so each layer's piece is cut at its contact's mean height: the face's relief follows the
 * rule everywhere (the pieces share their stations), while its finish changes at the mean (Form's gap). The relief is drawn
 * from a ground plane set back by the rule's deepest recess, so the face lies behind the nominal face where the rule says
 * it does. The flints in the clay, the flint bands and the heart urchin wait for polish.
 */
namespace Musee::Cube
{
	namespace
	{
		/** The layers' bands, top down: the head plate, the four mean contacts, the foot plate. */
		TArray<double> LayerLevels()
		{
			using namespace Plan;
			return {GroundTop, Hardcore.Mean, TopsoilSubsoil.Mean, SubsoilClay.Mean, ClayChalk.Mean, GroundBottom};
		}

		FFormRelief LayerFace(double Bottom, double Top)
		{
			using namespace Plan;
			const double Recess = Strata::DeepestRecess();
			FFormRelief Shape;
			// Clockwise on the plan from due east, so the path's right, where the ground stands proud, is the axis.
			Shape.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, GroundFace, GroundFace, 0.0, true);
			Shape.OffsetMeters = -Recess;
			Shape.BottomMeters = Bottom;
			Shape.TopMeters = Top;
			Shape.GridMeters = GroundGrid;
			Shape.BackMeters = GroundBack - GroundFace - Recess;
			Shape.HeightMeters = [Recess](double U, double V) { return Recess + Strata::ProudAt(U / GroundFace, V); };
			return Shape;
		}

		/** The ammonite's plane ground, square to the shaft behind the chalk's face: x round the face, y up, z towards the axis. */
		FMuseeFrame AmmoniteGround(double Behind)
		{
			using namespace Plan;
			const double Ground = GroundFace - AmmoniteProud + Behind;
			const FVector2D Outwards = Outward(AmmoniteBearing);
			const FVector2D Round = Along(AmmoniteBearing);
			const FVector X(-Round.X, -Round.Y, 0.0);
			const FVector Centre(Ground * Outwards.X, Ground * Outwards.Y, AmmoniteZ);
			return FrameOfAxes(Centre - 0.5 * AmmoniteField * (X + FVector::UpVector), X, FVector::UpVector);
		}

		void Ammonite(TArray<FFormDrawing>& Out)
		{
			using namespace Plan;
			// Its ground lies behind the deepest the chalk's face can be; its mid-plane 4 mm proud of the nominal face.
			const double Behind = Strata::DeepestRecess() + AmmoniteProud;
			FFormRelief Shape;
			Shape.SizeMeters = FVector2D(AmmoniteField, AmmoniteField);
			Shape.GridMeters = AmmoniteGrid;
			Shape.BackMeters = AmmoniteBack;
			Shape.HeightMeters = [Behind](double U, double V) { return Strata::AmmoniteAt(U - 0.5 * AmmoniteField, V - 0.5 * AmmoniteField, Behind); };
			Out.Add(Drawn(FString::Printf(TEXT("%sammonite"), Pieces::Ground), TEXT("ground.fossil"), MoveTemp(Shape), Solid, AmmoniteGround(Behind)));
		}
	}

	void DrawGround(TArray<FFormDrawing>& Out)
	{
		using namespace Plan;
		const TArray<double> Levels = LayerLevels();
		for (int32 Layer = 0; Layer < Strata::Layers; ++Layer)
		{
			const FString Name = Strata::LayerName(Strata::ELayer(Layer));
			Out.Add(Drawn(FString::Printf(TEXT("%s%s"), Pieces::Ground, *Name), *FString::Printf(TEXT("ground.%s"), *Name),
				LayerFace(Levels[Layer + 1], Levels[Layer]), Solid));
		}
		const FString Top = Strata::LayerName(Strata::ELayer::CrushedStone);
		const FString Bottom = Strata::LayerName(Strata::ELayer::Chalk);
		Out.Add(Drawn(FString::Printf(TEXT("%s%s.collar"), Pieces::Ground, *Top), *FString::Printf(TEXT("ground.%s"), *Top),
			Ring(GroundCollarInner, GroundBack, GroundTop, MatingPlane), Solid));
		Out.Add(Drawn(FString::Printf(TEXT("%s%s.collar"), Pieces::Ground, *Bottom), *FString::Printf(TEXT("ground.%s"), *Bottom),
			Ring(GroundCollarInner, GroundBack, RoofTop, GroundBottom), Solid));
		Ammonite(Out);
	}
}
