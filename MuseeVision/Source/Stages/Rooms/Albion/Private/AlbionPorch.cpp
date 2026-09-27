#include "AlbionCoursing.h"
#include "AlbionParts.h"

/**
 * The porch (§3.2, §3.7): its side walls, banded inside to the ceiling and outside to the parapets; the 0.10 end lining on
 * the port plane carrying the port's rough opening (§2), banded on its porch face; the oak ceiling of boards on three
 * moulded beams and two wall plates; the lead flat over it; the parapets' copings. Nothing is built past the port plane.
 */
namespace Musee::Albion
{
	namespace
	{
		/** The porch's side walls: the porch's leaf to the lead flat, the weathered leaf to the parapet's top over the lining. */
		void PorchWalls(TArray<FFormDrawing>& Out)
		{
			const double North = Plan::LiningFaceY;
			const double South = -Plan::EndOuterY;
			for (const bool bWest : {true, false})
			{
				const TCHAR* Side = bWest ? TEXT("west") : TEXT("east");
				FLeaf Inner;
				Inner.Name = FString::Printf(TEXT("porch.wall.%s.inner"), Side);
				Inner.Frame = AcrossX(bWest ? -Plan::PorchParapetInnerX : Plan::PorchInnerX);
				Inner.DepthMeters = Plan::PorchParapetInnerX - Plan::PorchInnerX;
				Inner.Blocks.Add({North, South, 0.0, Plan::LeadFlat, true});
				LayLeaf(Inner, Out);

				FLeaf Outer;
				Outer.Name = FString::Printf(TEXT("porch.wall.%s.outer"), Side);
				Outer.Frame = AcrossX(bWest ? -Plan::PorchOuterX : Plan::PorchParapetInnerX);
				Outer.DepthMeters = Plan::PorchOuterX - Plan::PorchParapetInnerX;
				Outer.Blocks.Add({North, South, Plan::FootingZ, Plan::LeadFlat, false});
				Outer.Blocks.Add({Plan::PortPlaneY, South, Plan::LeadFlat, Plan::PorchParapetTop, false});
				LayLeaf(Outer, Out);
			}
		}

		/** The end lining on the port plane, 0.10 thick, banded on its porch face, pierced by the port's rough opening. */
		void EndLining(TArray<FFormDrawing>& Out)
		{
			FLeaf Leaf;
			Leaf.Name = TEXT("porch.lining");
			Leaf.Frame = AcrossY(Plan::LiningFaceY);
			Leaf.DepthMeters = Plan::LiningMeters;
			Leaf.Blocks.Add({-Plan::PorchOuterX, Plan::PorchOuterX, Plan::FootingZ, Plan::LeadFlat, true});
			Leaf.Openings.Add(RoughOpening());
			LayLeaf(Leaf, Out);
		}

		/** A beam's section across the porch, (y, z): 0.20 wide from its soffit to the boards, its lower arrises eased 12 mm. */
		FFormProfile BeamSection(double Y)
		{
			const double L = Y - Plan::BeamHalf;
			const double R = Y + Plan::BeamHalf;
			const double E = Plan::BeamEase;
			const double Low = Plan::BeamSoffit;
			FFormProfile P = FFormProfile::From(FVector2D(L, Plan::PorchCeiling));
			P.LineTo(FVector2D(L, Low + E), EFormJoint::Crisp);
			P.ArcAbout(FVector2D(L + E, Low + E), E, E, 180.0, 270.0, EFormJoint::Smooth);
			P.LineTo(FVector2D(R - E, Low), EFormJoint::Smooth);
			P.ArcAbout(FVector2D(R - E, Low + E), E, E, 270.0, 360.0, EFormJoint::Smooth);
			P.LineTo(FVector2D(R, Plan::PorchCeiling), EFormJoint::Smooth);
			return P.Close();
		}

		/**
		 * The ceiling: oak boards at 6.90 wall to wall, three moulded beams across and the wall plates between them, all
		 * meeting the boards' soffit; the lead flat over the whole porch at 7.20, from the port plane to the court's wall.
		 */
		void CeilingAndFlat(TArray<FFormDrawing>& Out)
		{
			const double X = Plan::PorchInnerX;
			const double North = Plan::LiningFaceY;
			const double South = -Plan::EndOuterY;
			Out.Add(Box(TEXT("porch.ceiling.boards"), TEXT("porch.ceiling"), FVector(-X, North, Plan::PorchCeiling),
				FVector(X, South, Plan::PorchCeiling + Plan::CeilingBoards), Overhead));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::BeamY)); ++Index)
			{
				FFormExtrusion Timber;
				Timber.Outline = BeamSection(Plan::BeamY[Index]);
				Timber.DepthMeters = 2.0 * X;
				Out.Add(Drawn(FString::Printf(TEXT("porch.beam.%d"), Index + 1), TEXT("porch.ceiling"), MoveTemp(Timber), Overhead, AcrossX(-X)));
			}
			// The wall plates run between the beams, in four lengths a side.
			TArray<double> Stops = {North};
			for (const double Y : Plan::BeamY)
			{
				Stops.Append({Y - Plan::BeamHalf, Y + Plan::BeamHalf});
			}
			Stops.Add(South);
			for (const bool bWest : {true, false})
			{
				const double S = bWest ? -1.0 : 1.0;
				const double Inner = S * (Plan::WallPlateX - Plan::WallPlateHalf);
				const double Outer = S * (Plan::WallPlateX + Plan::WallPlateHalf);
				for (int32 Index = 0; Index + 1 < Stops.Num(); Index += 2)
				{
					Out.Add(Box(FString::Printf(TEXT("porch.plate.%s.%d"), bWest ? TEXT("west") : TEXT("east"), Index / 2 + 1), TEXT("porch.ceiling"),
						FVector(FMath::Min(Inner, Outer), Stops[Index], Plan::WallPlateSoffit), FVector(FMath::Max(Inner, Outer), Stops[Index + 1], Plan::PorchCeiling),
						Overhead));
				}
			}
			const double Flat = Plan::PorchParapetInnerX;
			Out.Add(Box(TEXT("porch.lead-flat"), TEXT("lead"), FVector(-Flat, Plan::PortPlaneY, Plan::LeadFlat),
				FVector(Flat, South, Plan::LeadFlat + Plan::LeadMeters), Overhead));
		}

		/** The copings over the porch's parapets, x ±3.30 to ±3.65, from the port plane to the court's wall. */
		void PorchCopings(TArray<FFormDrawing>& Out)
		{
			for (const bool bWest : {true, false})
			{
				const double S = bWest ? -1.0 : 1.0;
				const double Inner = S * Plan::PorchCopingInnerX;
				const double Outer = S * Plan::PorchCopingOuterX;
				Out.Add(Box(bWest ? TEXT("porch.coping.west") : TEXT("porch.coping.east"), TEXT("ext.dressing"),
					FVector(FMath::Min(Inner, Outer), Plan::PortPlaneY, Plan::PorchParapetTop),
					FVector(FMath::Max(Inner, Outer), -Plan::EndOuterY, Plan::PorchCopingTop), Overhead));
			}
		}
	}

	void DrawPorch(TArray<FFormDrawing>& Out)
	{
		PorchWalls(Out);
		EndLining(Out);
		CeilingAndFlat(Out);
		PorchCopings(Out);
	}
}
