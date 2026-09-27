#include "AlbionCoursing.h"

#include "Algo/Sort.h"

namespace Musee::Albion
{
	namespace
	{
		/** Heights closer than a micrometre are one. */
		constexpr double SameHeight = 1.0e-6;

		/** How the openings of a block cross one band of it. */
		struct FCrossing
		{
			/** Through the whole band: they cut it into separate stones. */
			TArray<const FArch*> Through;
			/** Their heads rise into it from below. */
			TArray<const FArch*> FromBelow;
			/** Their sills sit in it and they run on above. */
			TArray<const FArch*> FromAbove;
			/** Wholly inside it. */
			TArray<const FArch*> Inside;
		};

		FCrossing Cross(const FLeaf& Leaf, const FLeafBlock& Block, double A, double B)
		{
			FCrossing Crossing;
			for (const FArch& O : Leaf.Openings)
			{
				const double Crown = O.Crown();
				if (O.Centre <= Block.U0 || O.Centre >= Block.U1 || Crown <= A + SameHeight || O.Sill >= B - SameHeight)
				{
					continue;
				}
				const bool bBelow = O.Sill <= A + SameHeight;
				const bool bAbove = Crown >= B - SameHeight;
				(bBelow && bAbove ? Crossing.Through : bBelow ? Crossing.FromBelow : bAbove ? Crossing.FromAbove : Crossing.Inside).Add(&O);
			}
			auto ByCentre = [](const FArch* L, const FArch* R) { return L->Centre < R->Centre; };
			Algo::Sort(Crossing.Through, ByCentre);
			Algo::Sort(Crossing.FromBelow, ByCentre);
			Algo::Sort(Crossing.FromAbove, ByCentre);
			return Crossing;
		}

		/**
		 * One stone of a band between two bounds: the block's end, or the side of an opening that runs through the band.
		 * Its outline runs anticlockwise in (u, z): along the bed, round the heads rising into it, up the right bound,
		 * back along the top, round the openings whose sills sit in it, and down the left bound.
		 */
		FFormProfile Stone(const FLeafBlock& Block, const FCrossing& Crossing, const FArch* Left, const FArch* Right, double A, double B)
		{
			const double Low = Left ? Left->Centre : Block.U0;
			const double High = Right ? Right->Centre : Block.U1;
			auto Within = [Low, High](const FArch* O) { return O->Centre > Low && O->Centre < High; };
			FFormProfile P = FFormProfile::From(FVector2D(Left ? Left->Centre + Left->HalfAt(A) : Block.U0, A));
			for (const FArch* O : Crossing.FromBelow)
			{
				if (Within(O))
				{
					P.LineTo(FVector2D(O->Centre - O->HalfAt(A), A), EFormJoint::Crisp);
					O->OverHead(P, A, A);
				}
			}
			if (Right)
			{
				P.LineTo(FVector2D(Right->Centre - Right->HalfAt(A), A), EFormJoint::Crisp);
				Right->UpLeft(P, A, B);
			}
			else
			{
				P.LineTo(FVector2D(Block.U1, A), EFormJoint::Crisp).LineTo(FVector2D(Block.U1, B), EFormJoint::Crisp);
			}
			for (int32 Index = Crossing.FromAbove.Num() - 1; Index >= 0; --Index)
			{
				const FArch* O = Crossing.FromAbove[Index];
				if (Within(O))
				{
					P.LineTo(FVector2D(O->Centre + O->HalfAt(B), B), EFormJoint::Crisp);
					O->DownRight(P, B, O->Sill);
					P.LineTo(FVector2D(O->Centre - O->Half, O->Sill), EFormJoint::Crisp);
					O->UpLeft(P, O->Sill, B);
				}
			}
			if (Left)
			{
				P.LineTo(FVector2D(Left->Centre + Left->HalfAt(B), B), EFormJoint::Crisp);
				Left->DownRight(P, B, A);
			}
			else
			{
				P.LineTo(FVector2D(Block.U0, B), EFormJoint::Crisp);
			}
			return P.Close();
		}

		/** One band of a block: its stones between the openings that run through it, each pierced by those inside it. */
		void LayBand(const FLeaf& Leaf, const FLeafBlock& Block, double A, double B, const FString& Name, TArray<FFormDrawing>& Out)
		{
			const bool bRed = IsRed(0.5 * (A + B));
			const TCHAR* Finish = Block.bInside ? (bRed ? TEXT("wall.red") : TEXT("wall.buff")) : (bRed ? TEXT("ext.red") : TEXT("ext.buff"));
			const FCrossing Crossing = Cross(Leaf, Block, A, B);
			const int32 Stones = Crossing.Through.Num() + 1;
			for (int32 Index = 0; Index < Stones; ++Index)
			{
				const FArch* Left = Index > 0 ? Crossing.Through[Index - 1] : nullptr;
				const FArch* Right = Index < Crossing.Through.Num() ? Crossing.Through[Index] : nullptr;
				const double Low = Left ? Left->Centre : Block.U0;
				const double High = Right ? Right->Centre : Block.U1;
				FFormExtrusion Shape;
				Shape.Outline = Stone(Block, Crossing, Left, Right, A, B);
				for (const FArch* O : Crossing.Inside)
				{
					if (O->Centre > Low && O->Centre < High)
					{
						Shape.Holes.Add(O->Outline());
					}
				}
				Shape.DepthMeters = Leaf.DepthMeters;
				const FString Piece = Stones > 1 ? FString::Printf(TEXT("%s.%d"), *Name, Index + 1) : Name;
				Out.Add(Drawn(Piece, Finish, MoveTemp(Shape), Solid, Leaf.Frame));
			}
		}

		/** The heights a block is cut at: its bed, its top, and every edge of a red course between them. */
		TArray<double> Cuts(const FLeafBlock& Block)
		{
			TArray<double> Heights = {Block.Z0};
			for (int32 Course = 0; Course < Plan::RedCourses; ++Course)
			{
				const double Foot = Plan::FirstRed + Plan::RedPitch * Course;
				for (const double Z : {Foot, Foot + Plan::CourseMeters})
				{
					if (Z > Block.Z0 + SameHeight && Z < Block.Z1 - SameHeight)
					{
						Heights.Add(Z);
					}
				}
			}
			Heights.Add(Block.Z1);
			return Heights;
		}
	}

	bool IsRed(double Z)
	{
		for (int32 Course = 0; Course < Plan::RedCourses; ++Course)
		{
			const double Foot = Plan::FirstRed + Plan::RedPitch * Course;
			if (Z >= Foot && Z < Foot + Plan::CourseMeters)
			{
				return true;
			}
		}
		return false;
	}

	void LayLeaf(const FLeaf& Leaf, TArray<FFormDrawing>& Out)
	{
		for (int32 BlockIndex = 0; BlockIndex < Leaf.Blocks.Num(); ++BlockIndex)
		{
			const FLeafBlock& Block = Leaf.Blocks[BlockIndex];
			const TArray<double> Heights = Cuts(Block);
			for (int32 Index = 0; Index + 1 < Heights.Num(); ++Index)
			{
				LayBand(Leaf, Block, Heights[Index], Heights[Index + 1], FString::Printf(TEXT("%s.%d.%02d"), *Leaf.Name, BlockIndex + 1, Index + 1), Out);
			}
		}
	}
}
