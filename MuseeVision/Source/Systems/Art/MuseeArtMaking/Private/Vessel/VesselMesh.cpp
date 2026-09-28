#include "Vessel/VesselMesh.h"

#include "FormMesh.h"
#include "Ware/WareMesh.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/**
		 * One ring of the turned surface: the section's point carried round the axis with its normal, and pressed
		 * (FVesselPress::Pressed). An oval's pressing to an ellipse, (x, y, z) to (x, k(z) y, z), tilts the normal by the
		 * inverse transpose of its Jacobian: n to (n.x, n.y / k, n.z - k'(z) y n.y / k), y the point's unpressed y. A plan
		 * pressed to a superellipse takes its normal from the pressed surface itself: the cross of its tangents round the
		 * ring and along the section, each by central differences.
		 */
		TArray<int32> Ring(FFormMesh& Mesh, const FVector2D& Point, const FVector2D& Normal, double V, int32 Segments, const FVesselTurning& Turning,
			double ReferenceRadius)
		{
			constexpr double Step = 1e-5;
			const FVesselPress& Press = Turning.Press;
			const bool bEllipse = FMath::IsNearlyEqual(Press.ExponentAt(Point.Y), 2.0, 1e-12)
				&& FMath::IsNearlyEqual(Press.ExponentAt(Point.Y + Step), 2.0, 1e-12) && FMath::IsNearlyEqual(Press.ExponentAt(Point.Y - Step), 2.0, 1e-12);
			// along the section, its solid on the left: the outward normal turned a quarter back
			const FVector2D Along(-Normal.Y, Normal.X);
			TArray<int32> Out;
			Out.Reserve(Segments + 1);
			for (int32 K = 0; K <= Segments; ++K)
			{
				const double Angle = 2.0 * UE_DOUBLE_PI * K / Segments;
				const double S = FMath::Sin(Angle);
				const double C = FMath::Cos(Angle);
				const double Ratio = Press.RatioAt(Point.Y);
				const double Slope = Press.SlopeAt(Point.Y);
				const FVector Position = Press.Pressed(Point.X, Point.Y, Angle);
				FVector Facing = FVector(Normal.X * S, Normal.X * C / Ratio, Normal.Y - Slope * Point.X * C * Normal.X * C / Ratio).GetSafeNormal();
				if (!bEllipse)
				{
					const FVector Round = Press.Pressed(Point.X, Point.Y, Angle + Step) - Press.Pressed(Point.X, Point.Y, Angle - Step);
					const FVector Up = Press.Pressed(Point.X + Step * Along.X, Point.Y + Step * Along.Y, Angle)
						- Press.Pressed(Point.X - Step * Along.X, Point.Y - Step * Along.Y, Angle);
					const FVector Cross = FVector::CrossProduct(Round, Up);
					if (!Cross.IsNearlyZero(1e-20))
					{
						const FVector Found = Cross.GetSafeNormal();
						// the side the turned surface's own normal faces
						Facing = FVector::DotProduct(Found, FVector(Normal.X * S, Normal.X * C, Normal.Y)) >= 0.0 ? Found : -Found;
					}
				}
				Out.Add(Mesh.AddVertex(Position, Facing.IsNearlyZero() ? FVector::UpVector : Facing, FVector2D(ReferenceRadius * Angle, V)));
			}
			return Out;
		}
	}

	FString TurningOf(const FArtVessel& Vessel, const FVector2D& PlanMeters, double CreaseDegrees, double ChordTolerance, FVesselTurning& Out)
	{
		Out = FVesselTurning();
		Out.CreaseDegrees = CreaseDegrees;
		Out.ChordToleranceMeters = ChordTolerance;
		return PressOf(Vessel, SectionScale(Vessel, PlanMeters), Out.Press);
	}

	void BuildVessel(TConstArrayView<FVector2D> Section, const FVesselLayout& Layout, const FVesselTurning& Turning, UE::Geometry::FDynamicMesh3& Out)
	{
		FFormMesh Mesh;
		const int32 Count = Section.Num();
		const double Crease = FMath::DegreesToRadians(Turning.CreaseDegrees);
		const int32 Segments = ChordsRound(Layout.ReferenceRadius * FMath::Max3(Turning.Press.FootRatio, Turning.Press.MouthRatio, 1.0),
			Turning.ChordToleranceMeters);
		TArray<int32> Leaving;
		for (int32 I = 0; I < Count; ++I)
		{
			// The normals of the segments arriving at and leaving the point; an end keeps its one segment's.
			const FVector2D In = NormalOf(Section, FMath::Clamp(I - 1, 0, Count - 2));
			const FVector2D Onward = NormalOf(Section, FMath::Clamp(I, 0, Count - 2));
			const double Turn = FMath::Acos(FMath::Clamp(FVector2D::DotProduct(In, Onward), -1.0, 1.0));
			const bool bCrisp = Turn >= Crease;
			const TArray<int32> Arriving = Ring(Mesh, Section[I], bCrisp ? In : (In + Onward).GetSafeNormal(), Layout.Lengths[I], Segments, Turning,
				Layout.ReferenceRadius);
			if (I > 0)
			{
				for (int32 K = 0; K < Segments; ++K)
				{
					Mesh.AddQuad(Leaving[K], Leaving[K + 1], Arriving[K + 1], Arriving[K]);
				}
			}
			Leaving = bCrisp ? Ring(Mesh, Section[I], Onward, Layout.Lengths[I], Segments, Turning, Layout.ReferenceRadius) : Arriving;
		}
		Mesh.Write(Out);
	}
}
