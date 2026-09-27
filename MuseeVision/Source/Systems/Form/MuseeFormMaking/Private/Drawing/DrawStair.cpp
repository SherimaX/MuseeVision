#include "Drawing/Draw.h"

#include "Kernel/Extrude.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/Profile.h"
#include "Stations/Stations.h"

namespace Musee::Form::Drawing
{
	namespace
	{
		/**
		 * The flight's side, (x along the climb, z up): the risers and treads from the first riser's foot to the top
		 * nosing, a plumb cut down to the soffit, and the soffit back down to the lower floor. The soffit runs parallel
		 * to the nosings' line, WaistMeters under it square to the pitch.
		 */
		TArray<FVector2D> FlightSide(const FFormStairSteps& Steps, double WaistMeters, double& OutFootMeters)
		{
			const double R = Steps.RiserMeters;
			const double G = Steps.GoingMeters;
			const double Pitch = FMath::Atan2(R, G);
			const double Drop = WaistMeters / FMath::Cos(Pitch);
			TArray<FVector2D> Side;
			Side.Add(FVector2D(0.0, 0.0));
			for (int32 Step = 0; Step < Steps.Risers; ++Step)
			{
				Side.Add(FVector2D(Step * G, (Step + 1) * R));
				if (Step + 1 < Steps.Risers)
				{
					Side.Add(FVector2D((Step + 1) * G, (Step + 1) * R));
				}
			}
			const double Top = (Steps.Risers - 1) * G;
			Side.Add(FVector2D(Top, Steps.Risers * R - Drop));
			// The soffit, z = R + x R / G - Drop, meets the lower floor here.
			OutFootMeters = (Drop - R) * G / R;
			Side.Add(FVector2D(OutFootMeters, 0.0));
			return Side;
		}
	}

	bool DrawStair(const FFormStair& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		const FFormStairSteps Steps = Shape.SetOut();
		if (Steps.Risers < 2 || Steps.GoingMeters <= 0.0 || Shape.WidthMeters <= 0.0 || Shape.WaistMeters <= 0.0)
		{
			OutWhy = TEXT("the stair can't be set out: give its rise and its risers or run, a width and a waist, for two risers or more");
			return false;
		}
		double Foot = 0.0;
		const TArray<FVector2D> Side = FlightSide(Steps, Shape.WaistMeters, Foot);
		if (Foot <= 0.0 || Foot >= (Steps.Risers - 1) * Steps.GoingMeters)
		{
			OutWhy = FString::Printf(TEXT("a waist of %.3f m under risers of %.4f m makes a soffit that misses the lower floor"),
				Shape.WaistMeters, Steps.RiserMeters);
			return false;
		}
		FProfile Outline(Side[0]);
		for (int32 Index = 1; Index < Side.Num(); ++Index)
		{
			Outline.LineTo(Side[Index], EProfileJoint::Crisp);
		}
		Outline.Close(EProfileJoint::Crisp, EProfileJoint::Crisp);
		FPlaneFrame Plane;
		Plane.Origin = FVector(0.0, -0.5 * Shape.WidthMeters, 0.0);
		Plane.AxisU = FVector::ForwardVector;
		Plane.AxisV = FVector::UpVector;
		Plane.Normal = FVector::RightVector;
		if (!Extrude(Mesh, Plane, Outline.Resolve(DefaultChordToleranceMeters), {}, Shape.WidthMeters, true))
		{
			OutWhy = TEXT("the flight's side crosses itself");
			return false;
		}
		return true;
	}
}
