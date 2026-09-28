#include "Ware/WareMesh.h"

#include "FormMesh.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** Where a point of the piece falls on its image, metres from the image's top-left corner. */
		struct FProjection
		{
			bool bFromAbove = false;
			double Radius = 0.0;
			double Height = 0.0;
			FVector2D Origin = FVector2D::ZeroVector;

			FVector2D At(const FVector& Point) const
			{
				// From above, the image's top lies away from the viewer (-y); from the front, the image's top is the lip.
				const double Down = bFromAbove ? Point.Y + Radius : Height - Point.Z;
				return Origin + FVector2D(Point.X + Radius, Down);
			}
		};

		/**
		 * Turns a section (radius, height; metres) about z. Each point's normal is the section's own, square to its run
		 * (the run's right, turning up for a section drawn inward and out for one drawn upward), smoothed along the run.
		 * UV0 is the projection's where one is given, else metres across the plan.
		 */
		void Turn(FFormMesh& Mesh, const TArray<FVector2D>& Section, int32 Segments, const FProjection* Projection)
		{
			const int32 Count = Section.Num();
			TArray<int32> Previous;
			for (int32 I = 0; I < Count; ++I)
			{
				const FVector2D Run = (Section[FMath::Min(I + 1, Count - 1)] - Section[FMath::Max(I - 1, 0)]).GetSafeNormal();
				const FVector2D Across(Run.Y, -Run.X);
				TArray<int32> Ring;
				for (int32 K = 0; K <= Segments; ++K)
				{
					const double Angle = 2.0 * UE_DOUBLE_PI * K / Segments;
					const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
					const FVector Point = Radial * Section[I].X + FVector(0.0, 0.0, Section[I].Y);
					const FVector Normal = Radial * Across.X + FVector(0.0, 0.0, Across.Y);
					const FVector2D UV = Projection ? Projection->At(Point) : FVector2D(Point.X, Point.Y);
					Ring.Add(Mesh.AddVertex(Point, Normal.IsNearlyZero() ? FVector::UpVector : Normal, UV));
				}
				if (!Previous.IsEmpty())
				{
					for (int32 K = 0; K < Segments; ++K)
					{
						Mesh.AddQuad(Previous[K], Previous[K + 1], Ring[K + 1], Ring[K]);
					}
				}
				Previous = MoveTemp(Ring);
			}
		}

		TArray<FVector2D> Scaled(const TArray<FVector2D>& Fractions, double Radius, double Height)
		{
			TArray<FVector2D> Out;
			for (const FVector2D& F : Fractions)
			{
				Out.Add(FVector2D(F.X * Radius, F.Y * Height));
			}
			return Out;
		}

		/** A vessel's outside from the foot's centre up to its lip, and its inside: over the lip, down the neck, to a floor. */
		void VesselSections(const FArtWareForms& Forms, const FWareSpec& Ware, TArray<FVector2D>& OutOutside, TArray<FVector2D>& OutInside)
		{
			const double Radius = 0.5 * Ware.WidthMeters;
			const double Height = Ware.HeightMeters;
			OutOutside = { FVector2D(0.0, 0.0) };
			OutOutside.Append(Scaled(Ware.Profile, Radius, Height));
			const FVector2D Lip = OutOutside.Last();
			if (Lip.X < Forms.OpenLipMeters)
			{
				OutOutside.Add(FVector2D(0.0, Lip.Y));
				return;
			}
			const double Wall = FMath::Min(Forms.WallMeters, Forms.WallOfLip * Lip.X);
			const double Floor = Lip.Y - FMath::Min(Forms.NeckDepthOfHeight * Height, Forms.NeckDepthMeters);
			OutInside = { Lip, FVector2D(Lip.X - Wall, Lip.Y), FVector2D(Lip.X - Wall, Floor), FVector2D(0.0, Floor) };
		}
	}

	FVector2D WareImageMeters(const FWareSpec& Ware)
	{
		const double Across = FMath::Max(Ware.Window.Y - Ware.Window.X, UE_DOUBLE_KINDA_SMALL_NUMBER);
		const double Down = FMath::Max(Ware.Window.W - Ware.Window.Z, UE_DOUBLE_KINDA_SMALL_NUMBER);
		const double Tall = Ware.Kind == DishKind ? Ware.WidthMeters : Ware.HeightMeters;
		return FVector2D(Ware.WidthMeters / Across, Tall / Down);
	}

	void BuildWare(const FArtWareForms& Forms, const FWareSpec& Ware, UE::Geometry::FDynamicMesh3& OutDecorated, UE::Geometry::FDynamicMesh3& OutGround)
	{
		const double Radius = 0.5 * Ware.WidthMeters;
		const FVector2D Image = WareImageMeters(Ware);
		FProjection Projection;
		Projection.bFromAbove = Ware.Kind == DishKind;
		Projection.Radius = Radius;
		Projection.Height = Ware.HeightMeters;
		Projection.Origin = FVector2D(Ware.Window.X * Image.X, Ware.Window.Z * Image.Y);
		const int32 Segments = ChordsRound(Radius, Forms.ChordToleranceMeters);
		FFormMesh Decorated, Ground;
		if (Ware.Kind == DishKind)
		{
			Turn(Decorated, Scaled(Forms.DishFace, Radius, Ware.HeightMeters), Segments, &Projection);
			Turn(Ground, Scaled(Forms.DishUnderside, Radius, Ware.HeightMeters), Segments, nullptr);
		}
		else
		{
			TArray<FVector2D> Outside, Inside;
			VesselSections(Forms, Ware, Outside, Inside);
			Turn(Decorated, Outside, Segments, &Projection);
			if (!Inside.IsEmpty())
			{
				Turn(Ground, Inside, Segments, nullptr);
			}
		}
		Decorated.Write(OutDecorated);
		Ground.Write(OutGround);
	}

	int32 ChordsRound(double Radius, double Tolerance)
	{
		const double Half = FMath::Acos(FMath::Clamp(1.0 - Tolerance / FMath::Max(Radius, Tolerance), -1.0, 1.0));
		return FMath::Max(3, FMath::CeilToInt(UE_DOUBLE_PI / FMath::Max(Half, UE_DOUBLE_KINDA_SMALL_NUMBER)));
	}
}
