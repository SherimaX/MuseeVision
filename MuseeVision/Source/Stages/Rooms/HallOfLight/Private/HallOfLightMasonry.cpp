#include "HallOfLightEnds.h"
#include "HallOfLightParts.h"
#include "HallOfLightPlan.h"

/**
 * The masonry (sec. 3.1), in polished travertine: the floor between the linings and the plinths, with its five flush
 * viewing stones in their bronze rings; the plinths under the glass, their step lights housed at every post; the planter
 * kerbs outside the glass; and the four turned stereo stones.
 */
namespace Musee::HallOfLight
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		FRunLimit WestLining() { return FRunLimit::OnFace(InnerFace(EHallEnd::West)); }
		FRunLimit EastLining() { return FRunLimit::OnFace(InnerFace(EHallEnd::East)); }

		double ViewingX(int32 Stone)
		{
			return Plan::FirstViewingX + Plan::ViewingStep * Stone;
		}

		/** A section turned a full turn about a vertical axis standing at a point of the floor. */
		FFormLathe Turned(TConstArrayView<FVector2D> Section)
		{
			FFormLathe Lathe;
			Lathe.Section = FFormProfile::From(Section[0]);
			for (int32 Index = 1; Index < Section.Num(); ++Index)
			{
				Lathe.Section.LineTo(Section[Index], EFormJoint::Crisp);
			}
			return Lathe;
		}

		/**
		 * The floor, laid in travertine slabs between the linings' inner faces and the plinths' inner faces: an unstaggered
		 * grid of 1.20 by 0.80 with a joint on the axis, cut to the linings' arcs and round the viewing stones, its 2.5 mm
		 * joints filled flush with the tinted resin that fills the stone's voids (its finish, filled.polished).
		 */
		void Floor(TArray<FFormDrawing>& Out)
		{
			FFormLaid Shape;
			Shape.Outline = PlanOutline(WestLining(), EastLining(), -Plan::FloorHalf, Plan::FloorHalf);
			for (int32 Stone = 0; Stone < Plan::ViewingStones; ++Stone)
			{
				Shape.Holes.Add(FFormProfile::Ellipse(FVector2D(ViewingX(Stone), 0.0), Plan::ViewingDiscRadius, Plan::ViewingDiscRadius, 0.0, true));
			}
			Shape.ThicknessMeters = -Plan::FloorBottom;
			Shape.JointMeters = Plan::FloorJoint;
			Shape.FillMeters = 0.0;
			Shape.Module = {FFormProfile::Rectangle(FVector2D::ZeroVector, FVector2D(Plan::FloorSlabLength, Plan::FloorSlabWidth))};
			Shape.LatticeU = FVector2D(Plan::FloorSlabLength, 0.0);
			Shape.LatticeV = FVector2D(0.0, Plan::FloorSlabWidth);
			FFormDrawing& Laid = Out.Add_GetRef(Drawn(TEXT("floor"), Surface(TEXT("floor")), MoveTemp(Shape), Walkable,
				FMuseeFrame(FVector(0.0, 0.0, Plan::FloorBottom))));
			Laid.PartSurfaces.Add(FFormLaid::JointPart, Surface(TEXT("floor")));
		}

		/** The viewing stones: a rebated disc flush with the floor, and the bronze ring that fills its rebate. */
		void ViewingStones(TArray<FFormDrawing>& Out)
		{
			const double Disc = Plan::ViewingDiscRadius;
			const double Stone = Plan::ViewingStoneRadius;
			const double Rebate = Plan::ViewingRebateZ;
			const FVector2D Section[] = {{0.0, Plan::FloorBottom}, {Disc, Plan::FloorBottom}, {Disc, Rebate}, {Stone, Rebate}, {Stone, 0.0}, {0.0, 0.0}};
			for (int32 Index = 0; Index < Plan::ViewingStones; ++Index)
			{
				const FMuseeFrame At(FVector(ViewingX(Index), 0.0, 0.0));
				const FString Name = FString::Printf(TEXT("viewing-stone.%d"), Index + 1);
				Out.Add(Drawn(Name, Surface(TEXT("viewing-stone")), Turned(Section), Walkable, At));
				FFormLathe Ring;
				Ring.Section = FFormProfile::Rectangle(FVector2D(Stone, Rebate), FVector2D(Disc, 0.0));
				Out.Add(Drawn(Name + TEXT(".ring"), Surface(TEXT("viewing-stone.ring")), MoveTemp(Ring), Walkable, At));
			}
		}

		/**
		 * A plinth on the glass line, lining to lining, its ends cut to the linings' faces: at every post a housing for a
		 * step light let into its inner face (x +-0.050, back to d 0.060, z 0.110 to 0.170), and its two top arrises
		 * chamfered 10 mm, each chamfer cut on its plane along the run so it meets the linings as the mason cuts it.
		 */
		void Plinth(TArray<FFormDrawing>& Out, int32 Side)
		{
			const double Inner = WallY(Side, Plan::PlinthHalf);
			const double Outer = WallY(Side, -Plan::PlinthHalf);
			const double Back = WallY(Side, Plan::HousingBackD);
			const double Height = Plan::PlinthTop - Plan::PlinthBottom;
			FFormDrawing Plinth = PlanRun(FString::Printf(TEXT("plinth.%s"), SideName(Side)), Surface(TEXT("plinth")), WestLining(), EastLining(), Inner,
				Outer, Plan::PlinthBottom, Plan::PlinthTop, Solid);
			FFormExtrusion& Shape = Plinth.Shape.Get<FFormExtrusion>();
			for (int32 Post = 0; Post < Plan::Posts; ++Post)
			{
				// Drawn from its back out past the inner face, so it opens there.
				FFormPocket& Housing = Shape.Pockets.AddDefaulted_GetRef();
				Housing.Outline = FFormProfile::Rectangle(FVector2D(PostX(Post) - Plan::HousingHalf, FMath::Min(Back, 2.0 * Inner - Back)),
					FVector2D(PostX(Post) + Plan::HousingHalf, FMath::Max(Back, 2.0 * Inner - Back)));
				Housing.CoverMeters = Plan::PlinthTop - Plan::HousingTop;
				Housing.DepthMeters = Plan::HousingTop - Plan::HousingBottom;
			}
			for (const double Edge : {Inner, Outer})
			{
				FFormCut& Chamfer = Plinth.Cuts.AddDefaulted_GetRef();
				Chamfer.PointMeters = FVector(0.0, Edge, Height - Plan::PlinthChamfer);
				Chamfer.Normal = FVector(0.0, FMath::Sign(Edge - WallY(Side, 0.0)), 1.0);
			}
			Out.Add(MoveTemp(Plinth));
		}

		/** A planter kerb outside the glass, butting the plinth, its outer top arris chamfered. */
		void Kerb(TArray<FFormDrawing>& Out, int32 Side)
		{
			const double Chamfer = Plan::KerbChamfer;
			const FVector2D Section[] = {{WallY(Side, Plan::KerbOuterD), Plan::KerbBottom}, {WallY(Side, Plan::KerbInnerD), Plan::KerbBottom},
				{WallY(Side, Plan::KerbInnerD), Plan::KerbTop}, {WallY(Side, Plan::KerbOuterD + Chamfer), Plan::KerbTop},
				{WallY(Side, Plan::KerbOuterD), Plan::KerbTop - Chamfer}};
			SectionRun(Out, FString::Printf(TEXT("kerb.%s"), SideName(Side)), Surface(TEXT("kerb")), Section, WestLining(), EastLining(), Solid);
		}

		/** The stereo stones, turned drums standing on the floor, north first and then alternating (sec. 1). */
		void StereoStones(TArray<FFormDrawing>& Out)
		{
			const double Radius = Plan::StereoStoneRadius;
			const double Height = Plan::StereoStoneHeight;
			const double Chamfer = Plan::StereoStoneChamfer;
			const FVector2D Section[] = {{0.0, 0.0}, {Radius, 0.0}, {Radius, Height - Chamfer}, {Radius - Chamfer, Height}, {0.0, Height}};
			for (int32 Index = 0; Index < Plan::StereoStones; ++Index)
			{
				const FMuseeFrame At(StereoStone(Index));
				Out.Add(Drawn(FString::Printf(TEXT("stereo-stone.%d"), Index + 1), Surface(TEXT("stereo-stone")), Turned(Section), Solid, At));
			}
		}
	}

	void DrawMasonry(TArray<FFormDrawing>& Out)
	{
		Floor(Out);
		ViewingStones(Out);
		for (const int32 Side : Sides)
		{
			Plinth(Out, Side);
			Kerb(Out, Side);
		}
		StereoStones(Out);
	}
}
