#include "GroundsParts.h"

/**
 * The parvis (§3.3): bush-hammered travertine laid as a mason divides the as-built rectangle, x -60 ... -28 from the
 * Salon strip's edge (y 9.16) to y 23.00. Seventeen courses run east-west, each 0.8140 deep, set out so the joint at
 * y 17.30 falls on the portico's first nosing; slabs 1.1852 long, 27 to a full course with the middle slab centred on the axis,
 * alternate courses offset half a slab and cut at the ends; 6 mm joints; each slab -0.120 to +0.024. The kerbs round it
 * (x -60, x -28, y 23) take their 60 mm off the slabs beside them; the strip's edge is the paving's north side.
 *
 * Form lays no surface yet, so each slab is drawn as its own piece; the lamp standards at the parvis's south corners
 * stand bedded in the slabs until a laid surface is cut round what is set into it.
 */
namespace Musee::Grounds
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;

		/** One edge of a slab: on the paving's boundary it runs to the boundary, inside it stops half a joint short. */
		double Edge(double Line, double Boundary, double Toward)
		{
			return FMath::IsNearlyEqual(Line, Boundary, UE_KINDA_SMALL_NUMBER) ? Boundary : Line + Toward * 0.5 * Plan::PavingJoint;
		}
	}

	FBox2D ParvisPaving()
	{
		return FBox2D(FVector2D(Plan::ParvisWestX + Plan::KerbHalf, Plan::SalonStripEdgeY), FVector2D(Plan::ParvisEastX - Plan::KerbHalf, Plan::ParvisSouthY - Plan::KerbHalf));
	}

	void DrawPaving(TArray<FFormDrawing>& Out)
	{
		const FBox2D Paving = ParvisPaving();
		const double West = Paving.Min.X;
		const double East = Paving.Max.X;
		const double North = Paving.Min.Y;
		const double South = Paving.Max.Y;
		for (int32 Course = 0; Course < Plan::ParvisCourses; ++Course)
		{
			const double Y0 = North + Plan::CourseDepth * Course;
			const double Y1 = FMath::Min(South, North + Plan::CourseDepth * (Course + 1));
			// A full course's middle slab is centred on the axis; the next course is offset half a slab.
			const double Offset = Course % 2 == 0 ? 0.0 : 0.5 * Plan::SlabLength;
			const double First = Plan::AxisX - 0.5 * Plan::SlabLength * Plan::SlabsPerCourse + Offset - (Offset > 0.0 ? Plan::SlabLength : 0.0);
			int32 Slab = 0;
			for (double X0 = First; X0 < East; X0 += Plan::SlabLength)
			{
				const double Left = FMath::Max(West, X0);
				const double Right = FMath::Min(East, X0 + Plan::SlabLength);
				if (Right - Left <= Plan::PavingJoint)
				{
					continue;
				}
				const FVector2D Min(Edge(Left, West, 1.0), Edge(Y0, North, 1.0));
				const FVector2D Max(Edge(Right, East, -1.0), Edge(Y1, South, -1.0));
				Out.Add(Laid(FString::Printf(TEXT("parvis.c%02d.s%02d"), Course + 1, ++Slab), TEXT("parvis.paving"), Rectangle(Min, Max), {},
					Plan::PavingBottom, Plan::ParvisTop, Walkable));
			}
		}
	}
}
