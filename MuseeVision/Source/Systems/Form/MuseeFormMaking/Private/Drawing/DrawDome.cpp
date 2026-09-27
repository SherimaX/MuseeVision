#include "Drawing/Draw.h"

#include "Kernel/MeshBuilder.h"
#include "Stations/Stations.h"

namespace Musee::Form::Drawing
{
	namespace
	{
		/** The latitudes run from the springing to the crown; the longitudes once round the plan. */
		constexpr double CrownRadians = UE_DOUBLE_HALF_PI;
		constexpr double TurnRadians = UE_DOUBLE_TWO_PI;

		/** The plan's axes are always stations, so the dome meets anything drawn on them vertex for vertex. */
		const double AxisRadians[] = { UE_DOUBLE_HALF_PI, UE_DOUBLE_PI, 1.5 * UE_DOUBLE_PI };

		/** One face of the shell as it is drawn: how far inside the outer face it lies, which way it faces, its parts. */
		struct FDomeFace
		{
			double DepthMeters = 0.0;
			double Facing = 1.0;
			FName Part;
		};

		/** The point on the outer face at a latitude and longitude, and the outward unit normal there. */
		void OuterAt(const FFormDome& Dome, double Latitude, double Longitude, FVector& OutPoint, FVector& OutNormal)
		{
			const double Across = FMath::Cos(Latitude);
			const FVector Unit(Across * FMath::Cos(Longitude), Across * FMath::Sin(Longitude), FMath::Sin(Latitude));
			OutPoint = FVector(Dome.SemiAxisXMeters * Unit.X, Dome.SemiAxisYMeters * Unit.Y, Dome.RiseMeters * Unit.Z);
			OutNormal = FVector(Unit.X / Dome.SemiAxisXMeters, Unit.Y / Dome.SemiAxisYMeters, Unit.Z / Dome.RiseMeters).GetSafeNormal();
		}

		/** The arc length at each point of a row, measured along its chords from the first. */
		TArray<double> ArcLengths(TConstArrayView<FVector> Points)
		{
			TArray<double> Out;
			Out.Add(0.0);
			for (int32 Index = 1; Index < Points.Num(); ++Index)
			{
				Out.Add(Out.Last() + FVector::Dist(Points[Index - 1], Points[Index]));
			}
			return Out;
		}

		/**
		 * One face over the latitude and longitude stations, in the dome's frame: UV0 in metres, u along each parallel from
		 * longitude 0 and v up each meridian from the springing. Rows from OculusRow up are drawn in the oculus part, on a
		 * row of their own vertices where the two parts meet.
		 */
		void DrawDomeFace(const FFormDome& Dome, TConstArrayView<double> Latitudes, TConstArrayView<double> Longitudes, int32 OculusRow,
			const FDomeFace& Face, FMeshBuilder& Mesh)
		{
			const int32 Rows = Latitudes.Num();
			const int32 Columns = Longitudes.Num();
			TArray<TArray<FVector>> Points;
			TArray<TArray<FVector>> Normals;
			for (const double Latitude : Latitudes)
			{
				TArray<FVector>& Row = Points.AddDefaulted_GetRef();
				TArray<FVector>& Facing = Normals.AddDefaulted_GetRef();
				for (const double Longitude : Longitudes)
				{
					FVector Point, Normal;
					OuterAt(Dome, Latitude, Longitude, Point, Normal);
					Row.Add(Point - Normal * Face.DepthMeters);
					Facing.Add(Normal * Face.Facing);
				}
			}
			TArray<TArray<double>> Along;
			for (const TArray<FVector>& Row : Points)
			{
				Along.Add(ArcLengths(Row));
			}
			TArray<TArray<double>> Up;
			Up.SetNum(Columns);
			for (int32 Column = 0; Column < Columns; ++Column)
			{
				TArray<FVector> Meridian;
				for (int32 Row = 0; Row < Rows; ++Row)
				{
					Meridian.Add(Points[Row][Column]);
				}
				Up[Column] = ArcLengths(Meridian);
			}
			const int32 Split = OculusRow == INDEX_NONE ? Rows - 1 : OculusRow;
			const FName Parts[2] = { Face.Part, DomePart::Oculus };
			const int32 Bands[3] = { 0, Split, Rows - 1 };
			for (int32 Band = 0; Band < 2; ++Band)
			{
				if (Bands[Band] == Bands[Band + 1])
				{
					continue;
				}
				Mesh.UseSlot(Parts[Band]);
				TArray<int32> Previous;
				for (int32 Row = Bands[Band]; Row <= Bands[Band + 1]; ++Row)
				{
					TArray<int32> Current;
					for (int32 Column = 0; Column < Columns; ++Column)
					{
						Current.Add(Mesh.AddVertex(Dome.Frame.PointToParent(Points[Row][Column]), Dome.Frame.DirectionToParent(Normals[Row][Column]),
							FVector2D(Along[Row][Column], Up[Column][Row])));
					}
					for (int32 Column = 0; Row > Bands[Band] && Column + 1 < Columns; ++Column)
					{
						Mesh.AddQuad(Previous[Column], Previous[Column + 1], Current[Column + 1], Current[Column]);
					}
					Previous = MoveTemp(Current);
				}
			}
		}

		/** The flat ring the shell springs from, between its two faces, facing down; UV0 its plan in metres. */
		void DrawSpringing(const FFormDome& Dome, TConstArrayView<double> Longitudes, FMeshBuilder& Mesh)
		{
			Mesh.UseSlot(DomePart::Outer);
			const FVector Down = Dome.Frame.DirectionToParent(-FVector::UpVector);
			TArray<int32> Outer;
			TArray<int32> Inner;
			for (const double Longitude : Longitudes)
			{
				FVector Point, Normal;
				OuterAt(Dome, 0.0, Longitude, Point, Normal);
				const FVector Within = Point - Normal * Dome.ThicknessMeters;
				Outer.Add(Mesh.AddVertex(Dome.Frame.PointToParent(Point), Down, FVector2D(Point.X, Point.Y)));
				Inner.Add(Mesh.AddVertex(Dome.Frame.PointToParent(Within), Down, FVector2D(Within.X, Within.Y)));
			}
			for (int32 Column = 0; Column + 1 < Longitudes.Num(); ++Column)
			{
				Mesh.AddQuad(Outer[Column], Outer[Column + 1], Inner[Column + 1], Inner[Column]);
			}
		}

		/** What is wrong with a dome's numbers, or empty. */
		FString DomeFaults(const FFormDome& Dome)
		{
			const double Least = FMath::Min3(Dome.SemiAxisXMeters, Dome.SemiAxisYMeters, Dome.RiseMeters);
			const double Largest = FMath::Max3(Dome.SemiAxisXMeters, Dome.SemiAxisYMeters, Dome.RiseMeters);
			if (Least <= 0.0)
			{
				return TEXT("a dome needs two plan semi-axes and a rise of more than zero");
			}
			// An ellipsoid's tightest curve has the radius (least semi-axis)^2 / (largest); a thicker shell folds there.
			if (Dome.ThicknessMeters < 0.0 || Dome.ThicknessMeters >= Least * Least / Largest)
			{
				return FString::Printf(TEXT("its thickness %.4f m must be zero or more and less than its tightest curve's radius, %.4f m"),
					Dome.ThicknessMeters, Least * Least / Largest);
			}
			if (Dome.OculusFraction < 0.0 || Dome.OculusFraction >= 1.0)
			{
				return FString::Printf(TEXT("its oculus fraction %.4f must be zero or more and less than one"), Dome.OculusFraction);
			}
			return FString();
		}
	}

	bool DrawDome(const FFormDome& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		OutWhy = DomeFaults(Shape);
		if (!OutWhy.IsEmpty())
		{
			return false;
		}
		// The second derivative of a parallel never exceeds its larger semi-axis, nor a meridian's the largest of all three.
		const double Plan = FMath::Max(Shape.SemiAxisXMeters, Shape.SemiAxisYMeters);
		const TArray<double> Longitudes = CurveStations(0.0, TurnRadians, Plan, DefaultChordToleranceMeters, AxisRadians);
		const bool bOculus = Shape.OculusFraction > 0.0;
		const double OculusLatitude = bOculus ? FMath::Acos(Shape.OculusFraction) : CrownRadians;
		const TArray<double> Latitudes = CurveStations(0.0, CrownRadians, FMath::Max(Plan, Shape.RiseMeters), DefaultChordToleranceMeters,
			bOculus ? TArray<double>{ OculusLatitude } : TArray<double>());
		int32 OculusRow = INDEX_NONE;
		for (int32 Row = 0; bOculus && Row < Latitudes.Num(); ++Row)
		{
			if (OculusRow == INDEX_NONE || FMath::Abs(Latitudes[Row] - OculusLatitude) < FMath::Abs(Latitudes[OculusRow] - OculusLatitude))
			{
				OculusRow = Row;
			}
		}
		if (Shape.ThicknessMeters == 0.0)
		{
			DrawDomeFace(Shape, Latitudes, Longitudes, OculusRow, { 0.0, -1.0, DomePart::Inner }, Mesh);
			return true;
		}
		DrawDomeFace(Shape, Latitudes, Longitudes, OculusRow, { 0.0, 1.0, DomePart::Outer }, Mesh);
		DrawDomeFace(Shape, Latitudes, Longitudes, OculusRow, { Shape.ThicknessMeters, -1.0, DomePart::Inner }, Mesh);
		DrawSpringing(Shape, Longitudes, Mesh);
		return true;
	}
}
