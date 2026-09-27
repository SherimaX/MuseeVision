#include "GroundsParts.h"
#include "GroundsRun.h"

/**
 * The basins (§3.6): the canal and the rond-point's round basin, each a ring of honed travertine copings at sitting height
 * round a floor of Belgian bluestone; Nature fills them with still water to their wetted faces. The coping (the approved
 * pond edge's language) is 0.50 wide, its top at 0.45 with a rounded outer arris; its nose overhangs the wetted face by
 * 40 mm and its underside at 0.36 throws a shadow line over the water. The canal's stones are 1.25 long, measured on the
 * outer face and mitred at the corners; the round basin's are twelve, radially jointed, a stone centred on each axis. The
 * floors lie inside the wetted faces, z -0.310 to -0.250, under 0.55 of water.
 */
namespace Musee::Grounds
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** The coping's closed section, drawn with the stone on its left: A out from the nose line, B up. */
		FFormProfile CopingSection()
		{
			FFormProfile S = FFormProfile::From(At(Plan::CopingFoot[0]));
			S.LineTo(At(Plan::CopingFoot[1]), EFormJoint::Crisp);
			S.LineTo(At(Plan::CopingFoot[2]), EFormJoint::Crisp);
			S.ArcAbout(At(Plan::CopingOuterArc), Plan::CopingOuterArcRadius, Plan::CopingOuterArcRadius, 0.0, 90.0, EFormJoint::Smooth);
			S.LineTo(At(Plan::CopingSeatInner), EFormJoint::Smooth);
			S.ArcAbout(At(Plan::CopingNoseArc), Plan::CopingNoseArcRadius, Plan::CopingNoseArcRadius, 90.0, 180.0, EFormJoint::Smooth);
			S.LineTo(At(Plan::CopingUnderside[0]), EFormJoint::Smooth);
			S.LineTo(At(Plan::CopingUnderside[1]), EFormJoint::Crisp);
			return S.Close();
		}

		void Stone(TArray<FFormDrawing>& Out, const FString& Name, FRunStone&& Made)
		{
			FFormSweep Shape;
			Shape.Section = CopingSection();
			Shape.Path = MoveTemp(Made.Path);
			Shape.StartCut = Made.StartCut;
			Shape.EndCut = Made.EndCut;
			Out.Add(Drawn(Name, TEXT("coping"), MoveTemp(Shape), Solid));
		}

		/**
		 * The canal's copings: along the nose line anticlockwise on the plan, so the stone lies outward to its right; each
		 * side divided into stones about 1.25 long on its outer face (exactly 1.25 on the long sides), their joints square
		 * to the side, the corner stones mitred.
		 */
		void Canal(TArray<FFormDrawing>& Out)
		{
			const double W = Plan::CopingWidth;
			const FVector2D Min(Plan::CanalWestX + W, Plan::CanalNorthY + W);
			const FVector2D Max(Plan::CanalEastX - W, Plan::CanalSouthY - W);
			const FVector2D Nose[] = {Min, FVector2D(Min.X, Max.Y), Max, FVector2D(Max.X, Min.Y)};
			const FRun Run = FRun::Through(Nose, true);
			int32 Count = 0;
			for (const FRunLeg& Leg : Run.Legs())
			{
				// A side's joints are set out on its outer face (0.50 longer at either end than on the nose line).
				const double Outer = Leg.Length() + 2.0 * W;
				const int32 Stones = FMath::Max(1, FMath::RoundToInt32(Outer / Plan::CopingStone));
				const FVector2D Along = Leg.DirectionAt(0.0);
				for (int32 Index = 0; Index < Stones; ++Index)
				{
					const double From = FMath::Max(0.0, Outer * Index / Stones - W);
					const double To = FMath::Min(Leg.Length(), Outer * (Index + 1) / Stones - W);
					FRunStone Made;
					Made.Path = FFormProfile::From(Leg.PointAt(From));
					Leg.Extend(Made.Path, From, To);
					// The corner stones meet on the corner's bisecting plane, a 45 degree mitre. Anticlockwise on the plan, each
					// side turns from the one before it by a right angle: the side before runs along (-y, x) of this one's direction.
					const FVector2D Before(-Along.Y, Along.X);
					if (Index == 0)
					{
						const FVector2D Mitre = (Along + Before).GetSafeNormal();
						Made.StartCut = FVector(Mitre.X, Mitre.Y, 0.0);
					}
					if (Index + 1 == Stones)
					{
						const FVector2D Mitre = (Along - Before).GetSafeNormal();
						Made.EndCut = FVector(Mitre.X, Mitre.Y, 0.0);
					}
					Stone(Out, FString::Printf(TEXT("canal.coping.%02d"), ++Count), MoveTemp(Made));
				}
			}
			const double Wet = Plan::WettedFace;
			Out.Add(Laid(TEXT("canal.bed"), TEXT("basin.bed"), Rectangle(Min - FVector2D(Wet, Wet), Max + FVector2D(Wet, Wet)), {},
				Plan::CopingBottom, Plan::BasinFloorTop, Solid));
		}

		/** The round basin's twelve stones on its nose circle, anticlockwise, each centred on a multiple of 30 degrees. */
		void Round(TArray<FFormDrawing>& Out)
		{
			const FVector2D Centre = At(Plan::RondPoint);
			const double Pitch = 360.0 / Plan::BasinStones;
			for (int32 Index = 0; Index < Plan::BasinStones; ++Index)
			{
				const double Middle = -Pitch * Index;
				FRun Run = FRun::From(OnCircle(Centre, Plan::BasinNoseRadius, Middle + 0.5 * Pitch));
				Run.ArcTo(Centre, Middle - 0.5 * Pitch, false);
				TArray<FRunStone> Stones = Run.Stones(Run.Length());
				Stone(Out, FString::Printf(TEXT("rond-point.coping.%02d"), Index + 1), MoveTemp(Stones[0]));
			}
			Out.Add(Laid(TEXT("rond-point.bed"), TEXT("basin.bed"), Circle(Centre, Plan::BasinNoseRadius + Plan::WettedFace), {},
				Plan::CopingBottom, Plan::BasinFloorTop, Solid));
		}
	}

	void DrawBasins(TArray<FFormDrawing>& Out)
	{
		Canal(Out);
		Round(Out);
	}
}
