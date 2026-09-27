#pragma once

#include "CoreMinimal.h"
#include "FormLaid.h"

/**
 * Lay's rules for setting out: a laid surface's cells, joint centre to joint centre, by a module on a lattice, by courses
 * or by rings, in its setting-out frame.
 */
namespace Musee::Form::Drawing::Laying
{
	/** The setting-out frame: where the rule's (x, y) lie in the drawing's. */
	struct FSetting
	{
		FVector2D Origin;
		double Cos = 1.0;
		double Sin = 0.0;

		FSetting(const FFormLaid& Shape)
			: Origin(Shape.OriginMeters)
			, Cos(FMath::Cos(FMath::DegreesToRadians(Shape.AngleDegrees)))
			, Sin(FMath::Sin(FMath::DegreesToRadians(Shape.AngleDegrees)))
		{
		}

		FVector2D ToDrawing(const FVector2D& P) const { return Origin + FVector2D(Cos * P.X - Sin * P.Y, Sin * P.X + Cos * P.Y); }
		FVector2D ToRule(const FVector2D& Q) const
		{
			const FVector2D D = Q - Origin;
			return FVector2D(Cos * D.X + Sin * D.Y, -Sin * D.X + Cos * D.Y);
		}
	};

	/** A cell of the rule, in the rule's frame: one loop, or a loop with a hole (a whole ring). */
	struct FCell
	{
		TArray<FVector2D> Outer;
		TArray<TArray<FVector2D>> Holes;
	};

	/** The box round points. */
	FBox2D BoundsOf(TConstArrayView<FVector2D> Points);

	/**
	 * Every cell the laid surface's rule sets out that may reach the bounds (in the rule's frame): false, with the reason,
	 * for a rule that can't be set out, or one that sets out more cells than any floor has units.
	 */
	bool SetOut(const FFormLaid& Shape, const FBox2D& Bounds, TArray<FCell>& OutCells, FString& OutWhy);
}
