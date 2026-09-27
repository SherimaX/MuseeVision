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

		/**
		 * The point on the outer face at a latitude and longitude, and the outward unit normal there. The paraboloid's
		 * normal is the gradient of z - rise (1 - x^2/a^2 - y^2/b^2); the ellipsoid's, of x^2/a^2 + y^2/b^2 + z^2/rise^2.
		 */
		void OuterAt(const FFormDome& Dome, double Latitude, double Longitude, FVector& OutPoint, FVector& OutNormal)
		{
			const double A = Dome.SemiAxisXMeters;
			const double B = Dome.SemiAxisYMeters;
			const double Rise = Dome.RiseMeters;
			const double Across = FMath::Cos(Latitude);
			const FVector2D Round(FMath::Cos(Longitude), FMath::Sin(Longitude));
			if (Dome.bParaboloid)
			{
				OutPoint = FVector(A * Across * Round.X, B * Across * Round.Y, Rise * (1.0 - Across * Across));
				OutNormal = FVector(2.0 * Rise * OutPoint.X / (A * A), 2.0 * Rise * OutPoint.Y / (B * B), 1.0).GetSafeNormal();
				return;
			}
			const FVector Unit(Across * Round.X, Across * Round.Y, FMath::Sin(Latitude));
			OutPoint = FVector(A * Unit.X, B * Unit.Y, Rise * Unit.Z);
			OutNormal = FVector(Unit.X / A, Unit.Y / B, Unit.Z / Rise).GetSafeNormal();
		}

		/**
		 * The way out of the shell across its springing: down for the ellipsoid, whose faces stand vertical there; for the
		 * paraboloid, on out along its meridian (the derivative of its point by rho at rho = 1), outwards and down.
		 */
		FVector SpringingOutward(const FFormDome& Dome, double Longitude)
		{
			if (!Dome.bParaboloid)
			{
				return -FVector::UpVector;
			}
			return FVector(Dome.SemiAxisXMeters * FMath::Cos(Longitude), Dome.SemiAxisYMeters * FMath::Sin(Longitude), -2.0 * Dome.RiseMeters)
				.GetSafeNormal();
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

		/** The ring the shell springs from, between its two faces, facing out of the shell; UV0 its plan in metres. */
		void DrawSpringing(const FFormDome& Dome, TConstArrayView<double> Longitudes, FMeshBuilder& Mesh)
		{
			Mesh.UseSlot(DomePart::Outer);
			TArray<int32> Outer;
			TArray<int32> Inner;
			for (const double Longitude : Longitudes)
			{
				FVector Point, Normal;
				OuterAt(Dome, 0.0, Longitude, Point, Normal);
				const FVector Within = Point - Normal * Dome.ThicknessMeters;
				const FVector Out = Dome.Frame.DirectionToParent(SpringingOutward(Dome, Longitude));
				Outer.Add(Mesh.AddVertex(Dome.Frame.PointToParent(Point), Out, FVector2D(Point.X, Point.Y)));
				Inner.Add(Mesh.AddVertex(Dome.Frame.PointToParent(Within), Out, FVector2D(Within.X, Within.Y)));
			}
			for (int32 Column = 0; Column + 1 < Longitudes.Num(); ++Column)
			{
				Mesh.AddQuad(Outer[Column], Outer[Column + 1], Inner[Column + 1], Inner[Column]);
			}
		}

		/** The radius of the dome's tightest curve: (least)^2 / (largest) for the ellipsoid, (least plan)^2 / (2 rise) at a paraboloid's crown. */
		double TightestRadius(const FFormDome& Dome)
		{
			if (Dome.bParaboloid)
			{
				const double Least = FMath::Min(Dome.SemiAxisXMeters, Dome.SemiAxisYMeters);
				return Least * Least / (2.0 * Dome.RiseMeters);
			}
			const double Least = FMath::Min3(Dome.SemiAxisXMeters, Dome.SemiAxisYMeters, Dome.RiseMeters);
			const double Largest = FMath::Max3(Dome.SemiAxisXMeters, Dome.SemiAxisYMeters, Dome.RiseMeters);
			return Least * Least / Largest;
		}

		/** What is wrong with a dome's numbers, or empty. */
		FString DomeFaults(const FFormDome& Dome)
		{
			if (FMath::Min3(Dome.SemiAxisXMeters, Dome.SemiAxisYMeters, Dome.RiseMeters) <= 0.0)
			{
				return TEXT("a dome needs two plan semi-axes and a rise of more than zero");
			}
			// A shell thicker than its tightest curve's radius folds there.
			if (Dome.ThicknessMeters < 0.0 || Dome.ThicknessMeters >= TightestRadius(Dome))
			{
				return FString::Printf(TEXT("its thickness %.4f m must be zero or more and less than its tightest curve's radius, %.4f m"),
					Dome.ThicknessMeters, TightestRadius(Dome));
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
		// The second derivative of a parallel never exceeds its larger semi-axis, nor a meridian's the largest of all three
		// (twice the rise on a paraboloid, whose height rise sin^2 p bends by 2 rise cos 2p).
		const double Plan = FMath::Max(Shape.SemiAxisXMeters, Shape.SemiAxisYMeters);
		const double Meridian = FMath::Max(Plan, Shape.bParaboloid ? 2.0 * Shape.RiseMeters : Shape.RiseMeters);
		const TArray<double> Longitudes = CurveStations(0.0, TurnRadians, Plan, DefaultChordToleranceMeters, AxisRadians);
		const bool bOculus = Shape.OculusFraction > 0.0;
		const double OculusLatitude = bOculus ? FMath::Acos(Shape.OculusFraction) : CrownRadians;
		const TArray<double> Latitudes = CurveStations(0.0, CrownRadians, Meridian, DefaultChordToleranceMeters,
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
