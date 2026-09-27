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

		/** The floor: laid between the linings' inner faces and the plinths' inner faces, round the viewing stones. */
		void Floor(TArray<FFormDrawing>& Out)
		{
			FFormExtrusion Shape;
			Shape.Outline = PlanOutline(WestLining(), EastLining(), -Plan::FloorHalf, Plan::FloorHalf);
			for (int32 Stone = 0; Stone < Plan::ViewingStones; ++Stone)
			{
				Shape.Holes.Add(FFormProfile::Ellipse(FVector2D(ViewingX(Stone), 0.0), Plan::ViewingDiscRadius, Plan::ViewingDiscRadius, 0.0, true));
			}
			Shape.DepthMeters = -Plan::FloorBottom;
			Out.Add(Drawn(TEXT("floor"), Surface(TEXT("floor")), MoveTemp(Shape), Walkable, FMuseeFrame(FVector(0.0, 0.0, Plan::FloorBottom))));
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
		 * The course of a plinth that holds the step lights: its plan runs lining to lining, and at every post a pocket is
		 * cut back from its inner face (x +-0.050, back to d 0.060).
		 */
		FFormProfile HousingCourse(int32 Side)
		{
			const FEndFace West = InnerFace(EHallEnd::West);
			const FEndFace East = InnerFace(EHallEnd::East);
			const double Inner = WallY(Side, Plan::PlinthHalf);
			const double Outer = WallY(Side, -Plan::PlinthHalf);
			const double Back = WallY(Side, Plan::HousingBackD);
			FFormProfile Outline = FFormProfile::From(West.At(Inner));
			AddFaceArc(Outline, West, Inner, Outer);
			Outline.LineTo(East.At(Outer), EFormJoint::Crisp);
			AddFaceArc(Outline, East, Outer, Inner);
			for (int32 Post = Plan::Posts - 1; Post >= 0; --Post)
			{
				const double X = PostX(Post);
				Outline.LineTo(FVector2D(X + Plan::HousingHalf, Inner), EFormJoint::Crisp);
				Outline.LineTo(FVector2D(X + Plan::HousingHalf, Back), EFormJoint::Crisp);
				Outline.LineTo(FVector2D(X - Plan::HousingHalf, Back), EFormJoint::Crisp);
				Outline.LineTo(FVector2D(X - Plan::HousingHalf, Inner), EFormJoint::Crisp);
			}
			return Outline.Close();
		}

		/**
		 * A plinth on the glass line, lining to lining. Form draws no blind pocket in a run, so it is laid in three
		 * courses: the bed below the housings, the course the housings are cut into, and the upper course with its
		 * chamfered arrises.
		 */
		void Plinth(TArray<FFormDrawing>& Out, int32 Side)
		{
			const FString Name = FString::Printf(TEXT("plinth.%s"), SideName(Side));
			const FName Stone = Surface(TEXT("plinth"));
			const double Inner = WallY(Side, Plan::PlinthHalf);
			const double Outer = WallY(Side, -Plan::PlinthHalf);
			Out.Add(PlanRun(Name + TEXT(".bed"), Stone, WestLining(), EastLining(), Inner, Outer, Plan::PlinthBottom, Plan::HousingBottom, Solid));

			FFormExtrusion Housings;
			Housings.Outline = HousingCourse(Side);
			Housings.DepthMeters = Plan::HousingTop - Plan::HousingBottom;
			Out.Add(Drawn(Name + TEXT(".housings"), Stone, MoveTemp(Housings), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::HousingBottom))));

			const double Half = Plan::PlinthHalf;
			const double Arris = Plan::PlinthTop - Plan::PlinthChamfer;
			const FVector2D Upper[] = {{WallY(Side, -Half), Plan::HousingTop}, {WallY(Side, Half), Plan::HousingTop}, {WallY(Side, Half), Arris},
				{WallY(Side, Half - Plan::PlinthChamfer), Plan::PlinthTop}, {WallY(Side, Plan::PlinthChamfer - Half), Plan::PlinthTop},
				{WallY(Side, -Half), Arris}};
			SectionRun(Out, Name + TEXT(".upper"), Stone, Upper, WestLining(), EastLining(), Solid);
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
