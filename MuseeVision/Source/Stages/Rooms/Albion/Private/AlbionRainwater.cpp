#include "AlbionParts.h"

/**
 * The rainwater goods (§3.5): ten cast downpipes Ø 0.104 standing 0.14 off the outer faces, three on each side wall and
 * one either side of each end's nave, each from its head under the parapet, tapering from the head's outlet, down to its
 * shoe over the gully, held to the wall by a holderbat every 1.80 from 1.20.
 */
namespace Musee::Albion
{
	namespace
	{
		/** A downpipe: its name, the point on the wall's face behind it, the face's outward normal, its head's top. */
		struct FDownpipe
		{
			FString Name;
			FVector2D Face;
			FVector2D Out;
			double HeadTop;
		};

		TArray<FDownpipe> Downpipes()
		{
			TArray<FDownpipe> All;
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::SidePipeY)); ++Index)
			{
				const double Y = Plan::SidePipeY[Index];
				All.Add({FString::Printf(TEXT("rainwater.west.%d"), Index + 1), FVector2D(-Plan::SideOuterX, Y), FVector2D(-1.0, 0.0), Plan::SideHeadTop});
				All.Add({FString::Printf(TEXT("rainwater.east.%d"), Index + 1), FVector2D(Plan::SideOuterX, Y), FVector2D(1.0, 0.0), Plan::SideHeadTop});
			}
			All.Add({TEXT("rainwater.nw"), FVector2D(-Plan::EndPipeX, -Plan::EndOuterY), FVector2D(0.0, -1.0), Plan::EndHeadTop});
			All.Add({TEXT("rainwater.ne"), FVector2D(Plan::EndPipeX, -Plan::EndOuterY), FVector2D(0.0, -1.0), Plan::EndHeadTop});
			All.Add({TEXT("rainwater.sw"), FVector2D(-Plan::EndPipeX, Plan::EndOuterY), FVector2D(0.0, 1.0), Plan::EndHeadTop});
			All.Add({TEXT("rainwater.se"), FVector2D(Plan::EndPipeX, Plan::EndOuterY), FVector2D(0.0, 1.0), Plan::EndHeadTop});
			return All;
		}

		/** A box standing out from the wall's face: Along either side of the pipe's line, from the face out to Reach. */
		FFormDrawing OffTheWall(const FDownpipe& P, const FString& Name, double Half, double Reach, double Z0, double Z1)
		{
			const FVector2D Side(P.Out.Y, -P.Out.X);
			const FVector2D A = P.Face - Side * Half;
			const FVector2D B = P.Face + Side * Half + P.Out * Reach;
			return Box(Name, TEXT("iron.ext"), FVector(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y), Z0), FVector(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y), Z1),
				Overhead);
		}

		void Downpipe(const FDownpipe& P, TArray<FFormDrawing>& Out)
		{
			const double HeadFoot = P.HeadTop - Plan::HeadHigh;
			const double LipFoot = P.HeadTop - Plan::LipHigh;
			Out.Add(OffTheWall(P, P.Name + TEXT(".head"), Plan::HeadAlongHalf, Plan::HeadOut, HeadFoot, LipFoot));
			Out.Add(OffTheWall(P, P.Name + TEXT(".lip"), Plan::LipAlongHalf, Plan::LipOut, LipFoot, P.HeadTop));

			const FVector2D Axis = P.Face + P.Out * Plan::PipeOff;
			const FVector Foot(Axis.X, Axis.Y, 0.0);
			const double R = Plan::PipeRadius;
			FFormProfile Pipe = FFormProfile::From(FVector2D(0.0, Plan::PipeShoeFoot));
			Pipe.LineTo(FVector2D(Plan::ShoeRadius, Plan::PipeShoeFoot), EFormJoint::Crisp).LineTo(FVector2D(Plan::ShoeRadius, Plan::PipeShoeTop), EFormJoint::Crisp)
				.LineTo(FVector2D(R, Plan::PipeShoeTop), EFormJoint::Crisp).LineTo(FVector2D(R, HeadFoot - Plan::PipeTaper), EFormJoint::Crisp)
				.LineTo(FVector2D(Plan::OutletRadius, HeadFoot), EFormJoint::Crisp).LineTo(FVector2D(0.0, HeadFoot), EFormJoint::Crisp);
			Out.Add(Lathe(P.Name + TEXT(".pipe"), TEXT("iron.ext"), Foot, MoveTemp(Pipe), Solid));

			int32 Index = 0;
			for (double Z = Plan::HolderbatFirst; Z + Plan::HolderbatHalf < HeadFoot - Plan::PipeTaper; Z += Plan::HolderbatPitch)
			{
				const FString Name = FString::Printf(TEXT("%s.holderbat.%d"), *P.Name, ++Index);
				const double Collar = R + Plan::HolderbatProud;
				FFormProfile Band = FFormProfile::From(FVector2D(R, Z - Plan::HolderbatHalf));
				Band.LineTo(FVector2D(Collar, Z - Plan::HolderbatHalf), EFormJoint::Crisp).LineTo(FVector2D(Collar, Z + Plan::HolderbatHalf), EFormJoint::Crisp)
					.LineTo(FVector2D(R, Z + Plan::HolderbatHalf), EFormJoint::Crisp).Close();
				Out.Add(Lathe(Name, TEXT("iron.ext"), Foot, MoveTemp(Band), Solid));
				const FVector2D Back = Axis - P.Out * Collar;
				Out.Add(Beam(Name + TEXT(".arm"), TEXT("iron.ext"), FVector(Back.X, Back.Y, Z), FVector(P.Face.X, P.Face.Y, Z), Plan::HolderbatArm[0],
					Plan::HolderbatArm[1], Solid));
			}
		}
	}

	void DrawRainwater(TArray<FFormDrawing>& Out)
	{
		for (const FDownpipe& P : Downpipes())
		{
			Downpipe(P, Out);
		}
	}
}
