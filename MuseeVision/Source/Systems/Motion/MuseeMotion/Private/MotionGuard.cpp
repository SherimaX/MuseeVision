#include "MotionGuard.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "MotionControl.h"
#include "MuseeCollision.h"
#include "MuseeUnits.h"

namespace Musee::Motion
{
	namespace
	{
		/** Below this the step is a turn, not a slide: a thousandth of a millimetre, in engine units. */
		constexpr double StillEngineUnits = 1.0e-4;

		/** The solid parts of a body: what it would strike with. Controls are sensed, never struck. */
		TArray<const UPrimitiveComponent*> SolidParts(const USceneComponent& Body)
		{
			TArray<USceneComponent*> Children;
			Body.GetChildrenComponents(true, Children);
			TArray<const UPrimitiveComponent*> Parts;
			for (const USceneComponent* Child : Children)
			{
				const UPrimitiveComponent* Part = Cast<UPrimitiveComponent>(Child);
				if (Part && !Part->IsA<UMotionControl>() && Part->IsCollisionEnabled())
				{
					Parts.Add(Part);
				}
			}
			return Parts;
		}

		/** A part's box corners in the world, with the body at a pose. */
		TArray<FVector> Corners(const UPrimitiveComponent& Part, const USceneComponent& Body, const FTransform& BodyPose)
		{
			const FBox Local = Part.CalcBounds(FTransform::Identity).GetBox();
			const FTransform PartPose = Part.GetComponentTransform().GetRelativeTransform(Body.GetComponentTransform()) * BodyPose;
			TArray<FVector> Points;
			for (int32 Corner = 0; Corner < 8; ++Corner)
			{
				const FVector Point((Corner & 1) ? Local.Max.X : Local.Min.X, (Corner & 2) ? Local.Max.Y : Local.Min.Y, (Corner & 4) ? Local.Max.Z : Local.Min.Z);
				Points.Add(PartPose.TransformPosition(Point));
			}
			return Points;
		}

		/** The ranges of points along three directions. */
		void Extents(const TArray<FVector>& Points, const FVector (&Axes)[3], FVector2D (&Out)[3])
		{
			for (int32 Axis = 0; Axis < 3; ++Axis)
			{
				Out[Axis] = FVector2D(TNumericLimits<double>::Max(), TNumericLimits<double>::Lowest());
				for (const FVector& Point : Points)
				{
					const double Along = FVector::DotProduct(Point, Axes[Axis]);
					Out[Axis].X = FMath::Min(Out[Axis].X, Along);
					Out[Axis].Y = FMath::Max(Out[Axis].Y, Along);
				}
			}
		}

		bool Occupied(const UWorld& World, const FVector (&Axes)[3], const FVector2D (&Ranges)[3])
		{
			FVector Centre = FVector::ZeroVector;
			FVector Half;
			for (int32 Axis = 0; Axis < 3; ++Axis)
			{
				Centre += Axes[Axis] * (0.5 * (Ranges[Axis].X + Ranges[Axis].Y));
				Half[Axis] = 0.5 * (Ranges[Axis].Y - Ranges[Axis].X);
			}
			const FQuat Turn(FMatrix(Axes[0], Axes[1], Axes[2], FVector::ZeroVector));
			const FCollisionObjectQueryParams Visitor(Collision::Body.Channel);
			return World.OverlapAnyTestByObjectType(Centre, Turn, Visitor, FCollisionShape::MakeBox(Half));
		}
	}

	bool IsTravelOccupied(const UWorld& World, const USceneComponent& Body, const FTransform& From, const FTransform& To, double ClearanceMeters)
	{
		const double Clearance = ToEngine(Meters(ClearanceMeters));
		const FVector Step = To.GetLocation() - From.GetLocation();
		const double StepLength = Step.Size();
		const bool bSlide = StepLength > StillEngineUnits;
		FVector Axes[3];
		if (bSlide)
		{
			Axes[0] = Step / StepLength;
			Axes[0].FindBestAxisVectors(Axes[1], Axes[2]);
			Axes[2] = FVector::CrossProduct(Axes[0], Axes[1]);
		}
		for (const UPrimitiveComponent* Part : SolidParts(Body))
		{
			FVector2D Ranges[3];
			if (bSlide)
			{
				// The slice ahead of the part's leading face, as deep as the step and the clearance, as wide as the part and
				// the clearance on each side.
				Extents(Corners(*Part, Body, From), Axes, Ranges);
				Ranges[0] = FVector2D(Ranges[0].Y, Ranges[0].Y + StepLength + Clearance);
			}
			else
			{
				const FQuat Turn = (Part->GetComponentTransform().GetRelativeTransform(Body.GetComponentTransform()) * To).GetRotation();
				Axes[0] = Turn.GetAxisX();
				Axes[1] = Turn.GetAxisY();
				Axes[2] = Turn.GetAxisZ();
				Extents(Corners(*Part, Body, To), Axes, Ranges);
				Ranges[0] = FVector2D(Ranges[0].X - Clearance, Ranges[0].Y + Clearance);
			}
			Ranges[1] = FVector2D(Ranges[1].X - Clearance, Ranges[1].Y + Clearance);
			Ranges[2] = FVector2D(Ranges[2].X - Clearance, Ranges[2].Y + Clearance);
			if (Occupied(World, Axes, Ranges))
			{
				return true;
			}
		}
		return false;
	}

	double ReachFromAxis(const USceneComponent& Body, const FVector& Origin, const FVector& Axis)
	{
		const FVector Direction = Axis.GetSafeNormal();
		double Reach = 0.0;
		for (const UPrimitiveComponent* Part : SolidParts(Body))
		{
			for (const FVector& Point : Corners(*Part, Body, Body.GetComponentTransform()))
			{
				const FVector Offset = Point - Origin;
				Reach = FMath::Max(Reach, (Offset - Direction * FVector::DotProduct(Offset, Direction)).Size());
			}
		}
		return FromEngine<Meters>(Reach).Get();
	}
}

namespace Musee::Motion
{
	bool IsVisitorWithin(const UWorld& World, const FVector& Base, const FVector& Axis, double RadiusMeters, double HeightMeters)
	{
		const FVector Up = Axis.GetSafeNormal();
		const double Radius = ToEngine(Meters(RadiusMeters));
		const double Height = ToEngine(Meters(HeightMeters));
		// The box round the cylinder finds the candidates; each is then judged by its centre.
		const FVector Centre = Base + Up * (0.5 * Height);
		const FQuat Turn = FQuat::FindBetweenNormals(FVector::UpVector, Up);
		TArray<FOverlapResult> Found;
		World.OverlapMultiByObjectType(Found, Centre, Turn, FCollisionObjectQueryParams(Collision::Body.Channel),
			FCollisionShape::MakeBox(FVector(Radius, Radius, 0.5 * Height)));
		for (const FOverlapResult& Overlap : Found)
		{
			const UPrimitiveComponent* Visitor = Overlap.GetComponent();
			if (!Visitor)
			{
				continue;
			}
			const FVector Offset = Visitor->Bounds.Origin - Base;
			const double Along = FVector::DotProduct(Offset, Up);
			if (Along >= 0.0 && Along <= Height && (Offset - Up * Along).Size() <= Radius)
			{
				return true;
			}
		}
		return false;
	}
}
