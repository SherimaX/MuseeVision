#include "Tests/ProofPieces.h"

#include "Tests/SurfaceWriter.h"

namespace Musee::Studio
{
	namespace
	{
		/** Vitruvius, De Architectura IV.7: the Tuscan column is seven lower diameters tall. */
		constexpr double DiametersTall = 7.0;
		/** The base and the capital are each half a lower diameter high; the plinth is half the base. */
		constexpr double BaseDiameters = 0.5;
		/** The upper diameter is the lower less a quarter. */
		constexpr double UpperToLower = 0.75;
		/** The round plinth's radius and the torus's reach, in lower diameters: the proof's reading of IV.7. */
		constexpr double PlinthRadiusDiameters = 0.65;
		constexpr double TorusRadiusDiameters = 0.125;

		/** Facets around the axis, and steps along each curved run of the profile. */
		constexpr int32 SegmentsAround = 48;
		constexpr int32 ArcSteps = 6;
		constexpr int32 ShaftSteps = 10;

		/** A point of a lathe profile: its distance from the axis and its height, in metres. */
		struct FProfilePoint
		{
			double Radius = 0.0;
			double Height = 0.0;
		};

		/** A smooth run of the profile; the profile creases where one run meets the next. */
		using FProfileRun = TArray<FProfilePoint>;

		FProfileRun Arc(double CentreRadius, double CentreHeight, double ReachRadius, double ReachHeight, double FromDegrees, double ToDegrees)
		{
			FProfileRun Run;
			for (int32 Step = 0; Step <= ArcSteps; ++Step)
			{
				const double Angle = FMath::DegreesToRadians(FMath::Lerp(FromDegrees, ToDegrees, double(Step) / ArcSteps));
				Run.Add({CentreRadius + ReachRadius * FMath::Cos(Angle), CentreHeight + ReachHeight * FMath::Sin(Angle)});
			}
			return Run;
		}

		/** The column's profile from the foot of the plinth to the top of the abacus. */
		TArray<FProfileRun> TuscanProfile(double Height)
		{
			const double Diameter = Height / DiametersTall;
			const double Lower = Diameter * 0.5;
			const double Upper = Lower * UpperToLower;
			const double Plinth = Diameter * BaseDiameters * 0.5;
			const double Base = Diameter * BaseDiameters;
			// Vitruvius divides the capital's half-diameter height into three equal parts: necking, echinus and abacus.
			const double Sixth = Diameter / 6.0;
			const double PlinthRadius = Diameter * PlinthRadiusDiameters;
			const double Torus = Diameter * TorusRadiusDiameters;

			TArray<FProfileRun> Runs;
			Runs.Add({{PlinthRadius, 0.0}, {PlinthRadius, Plinth}});
			Runs.Add({{PlinthRadius, Plinth}, {Lower, Plinth}});
			Runs.Add(Arc(Lower, Plinth + Torus, Torus, Torus, -90.0, 90.0));
			FProfileRun Shaft;
			const double NeckTop = Height - 2.0 * Sixth;
			for (int32 Step = 0; Step <= ShaftSteps; ++Step)
			{
				// Entasis: the taper grows with the square of the height, so the shaft swells against a straight cone.
				const double Along = double(Step) / ShaftSteps;
				Shaft.Add({Lower - (Lower - Upper) * Along * Along, FMath::Lerp(Base, NeckTop, Along)});
			}
			Runs.Add(Shaft);
			Runs.Add(Arc(Upper, Height - Sixth, Lower - Upper, Sixth, -90.0, 0.0));
			Runs.Add({{Lower, Height - Sixth}, {Lower, Height}});
			return Runs;
		}

		/** A profile point's outward normal in the (radius, height) plane: the segment directions turned a quarter. */
		TArray<FVector2D> RunNormals(const FProfileRun& Run)
		{
			TArray<FVector2D> Segments;
			for (int32 Index = 0; Index + 1 < Run.Num(); ++Index)
			{
				const FVector2D Along(Run[Index + 1].Radius - Run[Index].Radius, Run[Index + 1].Height - Run[Index].Height);
				Segments.Add(FVector2D(Along.Y, -Along.X).GetSafeNormal());
			}
			TArray<FVector2D> Normals;
			for (int32 Index = 0; Index < Run.Num(); ++Index)
			{
				const FVector2D Below = Segments[FMath::Max(Index - 1, 0)];
				const FVector2D Above = Segments[FMath::Min(Index, Segments.Num() - 1)];
				Normals.Add((Below + Above).GetSafeNormal());
			}
			return Normals;
		}

		/** Turns one run of the profile around the axis; Arc carries the profile's length so far, which is V. */
		void TurnRun(FSurfaceWriter& Writer, FPolygonGroupID Group, const FProfileRun& Run, double UnrolledRadius, double& InOutArc)
		{
			const TArray<FVector2D> Normals = RunNormals(Run);
			TArray<double> ArcAt;
			for (int32 Index = 0; Index < Run.Num(); ++Index)
			{
				ArcAt.Add(InOutArc);
				if (Index + 1 < Run.Num())
				{
					InOutArc += FVector2D::Distance(FVector2D(Run[Index].Radius, Run[Index].Height), FVector2D(Run[Index + 1].Radius, Run[Index + 1].Height));
				}
			}
			const auto Corner = [&](int32 Index, int32 Step)
			{
				// Positions come from the facet index taken round the circle, so the seam closes on identical vertices.
				const double Angle = UE_DOUBLE_TWO_PI * (Step % SegmentsAround) / SegmentsAround;
				const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
				const FVector2D Profile = Index + 1 < Run.Num()
					? FVector2D(Run[Index + 1].Radius - Run[Index].Radius, Run[Index + 1].Height - Run[Index].Height)
					: FVector2D(Run[Index].Radius - Run[Index - 1].Radius, Run[Index].Height - Run[Index - 1].Height);
				FSurfaceWriter::FCorner Made;
				Made.PositionMeters = Radial * Run[Index].Radius + FVector(0.0, 0.0, Run[Index].Height);
				Made.Normal = Radial * Normals[Index].X + FVector(0.0, 0.0, Normals[Index].Y);
				Made.Tangent = FVector(-Radial.Y, Radial.X, 0.0);
				Made.Bitangent = Radial * Profile.X + FVector(0.0, 0.0, Profile.Y);
				Made.UVMeters = FVector2D(UE_DOUBLE_TWO_PI * UnrolledRadius * Step / SegmentsAround, ArcAt[Index]);
				return Made;
			};
			for (int32 Step = 0; Step < SegmentsAround; ++Step)
			{
				for (int32 Index = 0; Index + 1 < Run.Num(); ++Index)
				{
					Writer.Quad(Group, Corner(Index, Step), Corner(Index, Step + 1), Corner(Index + 1, Step + 1), Corner(Index + 1, Step));
				}
			}
		}

		/** A flat disk across the axis at a height, facing up or down. */
		void Disk(FSurfaceWriter& Writer, FPolygonGroupID Group, double Radius, double Height, double Facing)
		{
			const auto Corner = [&](const FVector& Position)
			{
				FSurfaceWriter::FCorner Made;
				Made.PositionMeters = Position;
				Made.Normal = FVector(0.0, 0.0, Facing);
				Made.Tangent = FVector::ForwardVector;
				Made.Bitangent = FVector::RightVector;
				Made.UVMeters = FVector2D(Position.X, Position.Y);
				return Made;
			};
			const FSurfaceWriter::FCorner Centre = Corner(FVector(0.0, 0.0, Height));
			for (int32 Step = 0; Step < SegmentsAround; ++Step)
			{
				const double From = UE_DOUBLE_TWO_PI * Step / SegmentsAround;
				const double To = UE_DOUBLE_TWO_PI * ((Step + 1) % SegmentsAround) / SegmentsAround;
				Writer.Triangle(Group, Centre, Corner(FVector(Radius * FMath::Cos(From), Radius * FMath::Sin(From), Height)),
					Corner(FVector(Radius * FMath::Cos(To), Radius * FMath::Sin(To), Height)));
			}
		}

		/** The wall slab and its opening, in metres. */
		constexpr double WallWidth = 4.0;
		constexpr double WallThickness = 0.3;
		constexpr double WallHeight = 3.0;
		constexpr double OpeningWidth = 1.2;
		constexpr double SillHeight = 0.9;
		constexpr double HeadHeight = 2.4;
		/** 12 mm float glass, a standard nominal thickness (EN 572-2). */
		constexpr double GlassThickness = 0.012;

		bool SkipNone(int32, int32) { return false; }

		/** The stone: faces and ends cut at the opening's edges, so every edge meets its neighbour at shared vertices. */
		void BuildStone(FSurfaceWriter& Writer, FPolygonGroupID Stone)
		{
			const double HalfWidth = WallWidth * 0.5;
			const double HalfThickness = WallThickness * 0.5;
			const double Jamb = HalfWidth - OpeningWidth * 0.5;
			const double Along[] = {0.0, Jamb, Jamb + OpeningWidth, WallWidth};
			const double Up[] = {0.0, SillHeight, HeadHeight, WallHeight};
			const double Across[] = {0.0, WallThickness};
			const double Opening[] = {0.0, OpeningWidth};
			const double OpeningUp[] = {0.0, HeadHeight - SillHeight};
			const auto IsOpening = [](int32 Column, int32 Row) { return Column == 1 && Row == 1; };
			const FVector X = FVector::ForwardVector, Y = FVector::RightVector, Z = FVector::UpVector;
			const double Left = -OpeningWidth * 0.5;

			Writer.Face(Stone, FVector(-HalfWidth, HalfThickness, 0.0), X, Along, Z, Up, Y, IsOpening);
			Writer.Face(Stone, FVector(-HalfWidth, -HalfThickness, 0.0), X, Along, Z, Up, -Y, IsOpening);
			Writer.Face(Stone, FVector(-HalfWidth, -HalfThickness, 0.0), Y, Across, Z, Up, -X, &SkipNone);
			Writer.Face(Stone, FVector(HalfWidth, -HalfThickness, 0.0), Y, Across, Z, Up, X, &SkipNone);
			Writer.Face(Stone, FVector(-HalfWidth, -HalfThickness, WallHeight), X, Along, Y, Across, Z, &SkipNone);
			Writer.Face(Stone, FVector(-HalfWidth, -HalfThickness, 0.0), X, Along, Y, Across, -Z, &SkipNone);
			Writer.Face(Stone, FVector(Left, -HalfThickness, SillHeight), X, Opening, Y, Across, Z, &SkipNone);
			Writer.Face(Stone, FVector(Left, -HalfThickness, HeadHeight), X, Opening, Y, Across, -Z, &SkipNone);
			Writer.Face(Stone, FVector(Left, -HalfThickness, SillHeight), Y, Across, Z, OpeningUp, X, &SkipNone);
			Writer.Face(Stone, FVector(-Left, -HalfThickness, SillHeight), Y, Across, Z, OpeningUp, -X, &SkipNone);
		}

		/** The pane: a closed slab of glass filling the opening, bearing on the reveals. */
		void BuildPane(FSurfaceWriter& Writer, FPolygonGroupID Glass)
		{
			const double HalfGlass = GlassThickness * 0.5;
			const double Left = -OpeningWidth * 0.5;
			const double Wide[] = {0.0, OpeningWidth};
			const double High[] = {0.0, HeadHeight - SillHeight};
			const double Thick[] = {0.0, GlassThickness};
			const FVector X = FVector::ForwardVector, Y = FVector::RightVector, Z = FVector::UpVector;

			Writer.Face(Glass, FVector(Left, HalfGlass, SillHeight), X, Wide, Z, High, Y, &SkipNone);
			Writer.Face(Glass, FVector(Left, -HalfGlass, SillHeight), X, Wide, Z, High, -Y, &SkipNone);
			Writer.Face(Glass, FVector(Left, -HalfGlass, SillHeight), Y, Thick, Z, High, -X, &SkipNone);
			Writer.Face(Glass, FVector(-Left, -HalfGlass, SillHeight), Y, Thick, Z, High, X, &SkipNone);
			Writer.Face(Glass, FVector(Left, -HalfGlass, HeadHeight), X, Wide, Y, Thick, Z, &SkipNone);
			Writer.Face(Glass, FVector(Left, -HalfGlass, SillHeight), X, Wide, Y, Thick, -Z, &SkipNone);
		}
	}

	FMeshDescription TurnColumn(double HeightMeters)
	{
		FMeshDescription Description;
		FSurfaceWriter Writer(Description);
		const FPolygonGroupID Stone = Writer.AddGroup(TEXT("Stone"));
		const TArray<FProfileRun> Profile = TuscanProfile(HeightMeters);
		const double Diameter = HeightMeters / DiametersTall;
		double Arc = 0.0;
		for (const FProfileRun& Run : Profile)
		{
			TurnRun(Writer, Stone, Run, Diameter * 0.5, Arc);
		}
		Disk(Writer, Stone, Profile[0][0].Radius, 0.0, -1.0);
		Disk(Writer, Stone, Profile.Last().Last().Radius, HeightMeters, 1.0);
		return Description;
	}

	FMeshDescription BuildGlazedWall()
	{
		FMeshDescription Description;
		FSurfaceWriter Writer(Description);
		const FPolygonGroupID Stone = Writer.AddGroup(TEXT("Stone"));
		const FPolygonGroupID Glass = Writer.AddGroup(TEXT("Glass"));
		BuildStone(Writer, Stone);
		BuildPane(Writer, Glass);
		return Description;
	}

	FMeshDescription DrawLeaningSquare()
	{
		FMeshDescription Description;
		FSurfaceWriter Writer(Description);
		const FPolygonGroupID Stone = Writer.AddGroup(TEXT("Stone"));
		const double Lean = FMath::DegreesToRadians(LeanDegrees);
		const auto Corner = [Lean](double X, double Y)
		{
			FSurfaceWriter::FCorner Made;
			Made.PositionMeters = FVector(X, Y, 0.0);
			Made.Normal = FVector(X, Y, 0.0).GetSafeNormal() * FMath::Sin(Lean) + FVector::UpVector * FMath::Cos(Lean);
			Made.Tangent = (FVector::ForwardVector - Made.Normal * Made.Normal.X).GetSafeNormal();
			Made.Bitangent = FVector::CrossProduct(Made.Normal, Made.Tangent);
			Made.UVMeters = FVector2D(X, Y);
			return Made;
		};
		Writer.Quad(Stone, Corner(-0.5, -0.5), Corner(0.5, -0.5), Corner(0.5, 0.5), Corner(-0.5, 0.5));
		return Description;
	}
}
