#include "Drawing/Draw.h"

#include "Kernel/Loft.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Tolerances.h"
#include "Stations/Stations.h"

/**
 * A loft's sections are read point for point: each drawn step of one section has its fellow in every other, a line's end
 * for a line's end and an arc's stations for an arc's, the arc divided into as many chords as the largest of its fellows
 * needs at the chord tolerance. An arc that doesn't start where the drawing stands is joined by a line, in every section
 * alike, so the points keep step even where that line has no length in one of them.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/** One section's points, with the joint the drawing gives at each. */
		struct FSectionPoints
		{
			TArray<FVector2D> Points;
			TArray<EFormJoint> Joints;

			void Add(const FVector2D& Point, EFormJoint Joint)
			{
				Points.Add(Point);
				Joints.Add(Joint);
			}
		};

		FVector2D ArcPoint(const FFormArc& Arc, double Degrees)
		{
			const double T = FMath::DegreesToRadians(Degrees);
			const double RadiusA = Arc.RadiiMeters[0];
			const double RadiusB = Arc.RadiiMeters.Num() > 1 ? Arc.RadiiMeters[1] : RadiusA;
			return FVector2D(Arc.CenterMeters[0] + RadiusA * FMath::Cos(T), Arc.CenterMeters[1] + RadiusB * FMath::Sin(T));
		}

		int32 ArcChords(const FFormArc& Arc)
		{
			const double Radius = FMath::Max(Arc.RadiiMeters[0], Arc.RadiiMeters.Num() > 1 ? Arc.RadiiMeters[1] : Arc.RadiiMeters[0]);
			return FMath::Max(1, ArcSegmentCount(Radius, FMath::DegreesToRadians(FMath::Abs(Arc.ToDegrees - Arc.FromDegrees)), DefaultChordToleranceMeters));
		}

		/** The sections are drawn alike: as many steps, lines and arcs in the same order. */
		bool Alike(const FFormLoft& Shape, TArray<int32>& OutChords, FString& OutWhy)
		{
			const TArray<FFormProfileStep>& First = Shape.Sections[0].Outline.Steps;
			OutChords.Init(0, First.Num());
			double Below = -TNumericLimits<double>::Max();
			for (const FFormLoftSection& Section : Shape.Sections)
			{
				if (!CheckProfile(Section.Outline, TEXT("a loft's section"), true, OutWhy))
				{
					return false;
				}
				if (Section.AtMeters <= Below + WeldToleranceMeters)
				{
					OutWhy = TEXT("a loft's sections rise one above another along z");
					return false;
				}
				Below = Section.AtMeters;
				const TArray<FFormProfileStep>& Steps = Section.Outline.Steps;
				bool bAlike = Steps.Num() == First.Num();
				for (int32 Step = 0; bAlike && Step < Steps.Num(); ++Step)
				{
					bAlike = Steps[Step].IsArc() == First[Step].IsArc();
					OutChords[Step] = FMath::Max(OutChords[Step], bAlike && Steps[Step].IsArc() ? ArcChords(Steps[Step].Arc) : 0);
				}
				if (!bAlike)
				{
					OutWhy = TEXT("a loft's sections are drawn alike, step for step");
					return false;
				}
			}
			return true;
		}

		FSectionPoints SectionPoints(const FFormProfile& Outline, TConstArrayView<int32> Chords)
		{
			FSectionPoints Out;
			Out.Add(Outline.StartPoint(), EFormJoint::Auto);
			for (int32 Step = 0; Step < Outline.Steps.Num(); ++Step)
			{
				const FFormProfileStep& Drawn = Outline.Steps[Step];
				Out.Joints.Last() = Drawn.Joint;
				if (!Drawn.IsArc())
				{
					Out.Add(FVector2D(Drawn.Line[0], Drawn.Line[1]), EFormJoint::Auto);
					continue;
				}
				Out.Add(ArcPoint(Drawn.Arc, Drawn.Arc.FromDegrees), EFormJoint::Auto);
				for (int32 Chord = 1; Chord <= Chords[Step]; ++Chord)
				{
					const double Degrees = FMath::Lerp(Drawn.Arc.FromDegrees, Drawn.Arc.ToDegrees, double(Chord) / Chords[Step]);
					Out.Add(ArcPoint(Drawn.Arc, Degrees), Chord < Chords[Step] ? EFormJoint::Smooth : EFormJoint::Auto);
				}
			}
			return Out;
		}

		double SignedArea(TConstArrayView<FVector2D> Points)
		{
			double Twice = 0.0;
			for (int32 Index = 0; Index < Points.Num(); ++Index)
			{
				Twice += FVector2D::CrossProduct(Points[Index], Points[(Index + 1) % Points.Num()]);
			}
			return 0.5 * Twice;
		}

		/** The direction of the first edge of some length leaving (Step 1) or arriving at (Step -1) a point, going round. */
		FVector2D EdgeFrom(TConstArrayView<FVector2D> Points, int32 Index, int32 Step)
		{
			const int32 Count = Points.Num();
			for (int32 Walk = 1; Walk < Count; ++Walk)
			{
				const FVector2D Edge = Points[(Index + Step * Walk + Count * Count) % Count] - Points[Index];
				if (Edge.Size() > WeldToleranceMeters)
				{
					return Edge.GetSafeNormal() * Step;
				}
			}
			return FVector2D::ZeroVector;
		}

		/** Whether a point is an arris round the section: drawn crisp, or, left to the drawing's rule, where it turns. */
		bool IsCrispAround(const FSectionPoints& Section, int32 Index)
		{
			const EFormJoint Joint = Section.Joints[Index];
			if (Joint != EFormJoint::Auto)
			{
				return Joint == EFormJoint::Crisp;
			}
			const FVector2D In = EdgeFrom(Section.Points, Index, -1);
			const FVector2D Out = EdgeFrom(Section.Points, Index, 1);
			return In.IsZero() || Out.IsZero()
				|| FMath::Atan2(FMath::Abs(FVector2D::CrossProduct(In, Out)), FVector2D::DotProduct(In, Out)) > TangentJointRadians;
		}

		/** Whether the side keeps an arris round section K: drawn crisp, or turning there by more than the crease rule. */
		bool IsCrispAcross(const FFormLoft& Shape, const FLoftGrid& Grid, int32 K)
		{
			const EFormJoint Joint = Shape.Sections[K].Joint;
			if (Joint != EFormJoint::Auto || K == 0 || K + 1 == Grid.Rings.Num())
			{
				return Joint == EFormJoint::Crisp;
			}
			for (int32 I = 0; I < Grid.Rings[K].Num(); ++I)
			{
				const FVector In = Grid.Rings[K][I] - Grid.Rings[K - 1][I];
				const FVector Out = Grid.Rings[K + 1][I] - Grid.Rings[K][I];
				if (FMath::Atan2(FVector::CrossProduct(In, Out).Size(), FVector::DotProduct(In, Out)) > CreaseAngleRadians)
				{
					return true;
				}
			}
			return false;
		}
	}

	bool DrawLoft(const FFormLoft& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		TArray<int32> Chords;
		if (Shape.Sections.Num() < 2)
		{
			OutWhy = TEXT("a loft has two sections at least");
			return false;
		}
		if (!Alike(Shape, Chords, OutWhy))
		{
			return false;
		}
		TArray<FSectionPoints> Sections;
		bool bReturns = true;
		double Turning = 0.0;
		for (const FFormLoftSection& Section : Shape.Sections)
		{
			FSectionPoints& Points = Sections.Add_GetRef(SectionPoints(Section.Outline, Chords));
			bReturns &= FVector2D::Distance(Points.Points[0], Points.Points.Last()) <= WeldToleranceMeters;
			const double Area = SignedArea(Points.Points);
			if (Area * Turning < 0.0)
			{
				OutWhy = TEXT("a loft's sections all run the same way round");
				return false;
			}
			Turning = FMath::Abs(Area) > FMath::Abs(Turning) ? Area : Turning;
		}
		FLoftGrid Grid;
		for (int32 K = 0; K < Sections.Num(); ++K)
		{
			FSectionPoints& Section = Sections[K];
			if (bReturns)
			{
				// Every drawing returns to its start: the last point is the first again.
				Section.Points.Pop();
				Section.Joints.Pop();
			}
			if (Turning < 0.0)
			{
				Algo::Reverse(Section.Points);
				Algo::Reverse(Section.Joints);
			}
			TArray<FVector>& Ring = Grid.Rings.AddDefaulted_GetRef();
			for (const FVector2D& Point : Section.Points)
			{
				Ring.Add(FVector(Point, Shape.Sections[K].AtMeters));
			}
		}
		Grid.CrispAround.Init(false, Grid.Rings[0].Num());
		for (const FSectionPoints& Section : Sections)
		{
			for (int32 I = 0; I < Section.Points.Num(); ++I)
			{
				Grid.CrispAround[I] |= IsCrispAround(Section, I);
			}
		}
		for (int32 K = 0; K < Grid.Rings.Num(); ++K)
		{
			Grid.CrispAcross.Add(IsCrispAcross(Shape, Grid, K));
		}
		if (!Loft(Mesh, Grid, FFormLoft::SidePart, FFormLoft::EndPart))
		{
			OutWhy = TEXT("a loft's end section crosses itself");
			return false;
		}
		return true;
	}
}
