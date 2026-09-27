#include "ChenghuaiParts.h"
#include "ChenghuaiPlanGarden.h"
#include "ChenghuaiPlanHalls.h"
#include "ChenghuaiTrades.h"
#include "Math/RandomStream.h"

/**
 * The garden's landform and stonework (§3.9; ChenghuaiGarden.cpp:79-321, ChenghuaiRockery.cpp:178-215): the pond's basin,
 * one landform over the pond's edge (its bank falling from the garden's ground to the bed, the bed rising over the Classical
 * Hall's tribune beneath), the lake rocks along its edge, the paths of pebble mosaic between bluestone kerbs (path 0 dividing round 石兄), the
 * zigzag bridge on its piers with its low kerbs, the flower hall's terrace, the water pavilion's base in the pond and the
 * twelve engraved stones in the covered walk's wall. The garden's ground and its water are Nature's.
 */
namespace Musee::Chenghuai
{
	namespace
	{
		TArray<FVector2D> Points(TConstArrayView<Plan::FPlanPoint> Plan)
		{
			TArray<FVector2D> Out;
			for (const Plan::FPlanPoint& Point : Plan)
			{
				Out.Add(FVector2D(Point.X, Point.Y));
			}
			return Out;
		}

		const FVector2D Tribune(Plan::TribuneX, Plan::TribuneY);

		/** The pond's bed at a point: shallow over the tribune, deep beyond it, a straight ramp between (§3.9). */
		double BedAt(double Radius)
		{
			return FMath::Lerp(Plan::BedShallow, Plan::BedDeep, FMath::Clamp((Radius - Plan::TribuneClear) / Plan::BedRamp, 0.0, 1.0));
		}

		/** The distance from a point to the nearest chord of a closed loop. */
		double ToLoop(TConstArrayView<FVector2D> Loop, const FVector2D& Point)
		{
			double Least = TNumericLimits<double>::Max();
			for (int32 I = 0, J = Loop.Num() - 1; I < Loop.Num(); J = I++)
			{
				const FVector2D Chord = Loop[I] - Loop[J];
				const double Along = FMath::Clamp(FVector2D::DotProduct(Point - Loop[J], Chord) / FMath::Max(Chord.SizeSquared(), UE_DOUBLE_SMALL_NUMBER), 0.0, 1.0);
				Least = FMath::Min(Least, FVector2D::Distance(Loop[J] + Chord * Along, Point));
			}
			return Least;
		}

		/**
		 * The pond's basin (§3.9), one landform over the pond's edge: its top the bank, falling in a straight slope from the
		 * garden's ground at the edge to the bed 0.45 in, and the bed, bed(r), beyond; its underside the bed's slab, on the
		 * footprint's floor under the deep part and held at -0.50 over the Classical Hall's tribune beneath.
		 */
		void Basin(TArray<FFormDrawing>& Out)
		{
			const TArray<FVector2D> Edge = Points(Plan::PondEdge);
			FFormHeightfield Shape;
			Shape.Outline = FFormProfile::Polyline(Edge, true);
			Shape.TopMeters = [Edge](double X, double Y)
			{
				const FVector2D At(X, Y);
				const double In = FMath::Min(ToLoop(Edge, At) / Plan::BankReach, 1.0);
				return FMath::Lerp(Plan::GardenGround, BedAt(FVector2D::Distance(At, Tribune)), In);
			};
			Shape.BottomMeters = [](double X, double Y)
			{
				return FVector2D::Distance(FVector2D(X, Y), Tribune) < Plan::TribuneClear ? Plan::HallClearZ : Plan::BedDeep - Plan::BedSlab;
			};
			Shape.GridMeters = Plan::PondGrid;
			Draw(Out, TEXT("pond.bed"), TEXT("pond.bed"), MoveTemp(Shape), EHit::Walkable);
		}

		/** The lake rocks along the edge: broad stones at the water, and on some an upper stone, each an irregular prism. */
		void EdgeRocks(TArray<FFormDrawing>& Out)
		{
			FRandomStream Random(Musee::Seed(Key(TEXT("pond.edge-rocks"))));
			const TArray<FVector2D> Edge = Points(Plan::PondEdge);
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			double Carry = 0.0;
			int32 Count = 0;
			for (int32 Index = 0; Index < Edge.Num(); ++Index)
			{
				const FVector2D A = Edge[Index];
				const FVector2D B = Edge[(Index + 1) % Edge.Num()];
				const double Length = FVector2D::Distance(A, B);
				const FVector2D Along = (B - A) / Length;
				double At = Carry;
				while (At < Length)
				{
					const FVector2D Centre = A + Along * At;
					++Count;
					for (const bool bUpper : {false, true})
					{
						const double Radius = (bUpper ? Plan::UpperRock : Plan::BroadRock) * (1.0 + Plan::RockSpread * (2.0 * Random.FRand() - 1.0));
						const double Turn = Random.FRand() * 2.0 * UE_DOUBLE_PI;
						TArray<FVector2D> Outline;
						for (int32 Side = 0; Side < Plan::RockSides; ++Side)
						{
							const double T = Turn + 2.0 * UE_DOUBLE_PI * Side / Plan::RockSides;
							const double R = Radius * FMath::Lerp(Plan::RockRadius0, Plan::RockRadius1, Random.FRand());
							const FVector2D Local(Plan::RockStretch * R * FMath::Cos(T), R * FMath::Sin(T));
							Outline.Add(Centre + Along * Local.X + FVector2D(-Along.Y, Along.X) * Local.Y);
						}
						const double Top = bUpper ? FMath::Lerp(Plan::UpperTop0, Plan::UpperTop1, Random.FRand()) : Plan::BroadTop1 * Random.FRand();
						if (bUpper && Random.FRand() >= Plan::UpperShare)
						{
							continue;
						}
						// Over the tribune a broad stone stands on the bed there, never below -0.50.
						const double Reach = Plan::RockRadius1 * Plan::RockStretch * Radius;
						const bool bOverHall = FVector2D::Distance(Centre, Tribune) < Plan::TribuneClear + Reach;
						const double Under = Plan::WaterLevel - Plan::BroadUnderWater;
						const double Bottom = bUpper ? Plan::UpperBottom : (bOverHall ? FMath::Max(Under, Plan::HallClearZ) : Under);
						Raised(Out, FString::Printf(TEXT("pond.rock.%d%s"), Count, bUpper ? TEXT(".upper") : TEXT("")), TEXT("edge-rock"), Stage, Outline, {},
							Bottom, Top, EHit::Walkable);
					}
					At += FMath::Lerp(Plan::RockSpacing0, Plan::RockSpacing1, Random.FRand());
				}
				Carry = At - Length;
			}
		}

		/** A path along a plan line: its pebble bed between two kerbs, each swept on the ground. */
		void Path(TArray<FFormDrawing>& Out, const FString& Name, FFormProfile Line)
		{
			auto Sweep = [&](const FString& Part, const TCHAR* Surface, double A0, double A1, double B0, double B1)
			{
				FFormSweep Shape;
				Shape.Section = Polygon(Rectangle(A0, B0, A1, B1));
				Shape.Path = Line;
				Shape.ElevationMeters = Plan::GardenGround;
				Draw(Out, Name + Part, Surface, MoveTemp(Shape), EHit::Walkable);
			};
			const double Bed = Plan::PathBedHalf;
			Sweep(TEXT(".bed"), TEXT("garden.path"), -Bed, Bed, Plan::PathBedBottom, Plan::PathBedTop);
			Sweep(TEXT(".kerb.1"), TEXT("garden.bluestone"), -Bed - Plan::PathKerb, -Bed, Plan::PathKerbBottom, Plan::PathKerbTop);
			Sweep(TEXT(".kerb.2"), TEXT("garden.bluestone"), Bed, Bed + Plan::PathKerb, Plan::PathKerbBottom, Plan::PathKerbTop);
		}

		void Paths(TArray<FFormDrawing>& Out)
		{
			// Path 0 divides round 石兄: straight to the fork, round the rock by both sides, straight on to the terrace.
			const FVector2D Fork(Plan::ForkX, Plan::ForkY);
			const double West = FMath::RadiansToDegrees(FMath::Atan2(Plan::Path0Y - Plan::ForkY, Plan::ForkX0 - Plan::ForkX));
			const double East = FMath::RadiansToDegrees(FMath::Atan2(Plan::Path0Y - Plan::ForkY, Plan::ForkX1 - Plan::ForkX));
			Path(Out, TEXT("garden.path.0.west"), FFormProfile::Polyline({FVector2D(Plan::Path0X0, Plan::Path0Y), FVector2D(Plan::ForkX0, Plan::Path0Y)}, false));
			// The fork's ends lie just north of the rock's centre: the north branch turns through -90°, the south through +90°.
			Path(Out, TEXT("garden.path.0.north"), FFormProfile::From(FVector2D(Plan::ForkX0, Plan::Path0Y))
				.ArcAbout(Fork, Plan::ForkRadius, Plan::ForkRadius, West, East));
			Path(Out, TEXT("garden.path.0.south"), FFormProfile::From(FVector2D(Plan::ForkX0, Plan::Path0Y))
				.ArcAbout(Fork, Plan::ForkRadius, Plan::ForkRadius, West + 360.0, East));
			Path(Out, TEXT("garden.path.0.east"), FFormProfile::Polyline({FVector2D(Plan::ForkX1, Plan::Path0Y), FVector2D(Plan::Path0X1, Plan::Path0Y)}, false));
			Path(Out, TEXT("garden.path.1"), SmoothPath(Points(Plan::Path1)));
			Path(Out, TEXT("garden.path.2"), SmoothPath(Points(Plan::Path2)));
			Path(Out, TEXT("garden.path.3"), FFormProfile::Polyline({FVector2D(Plan::Path3X0, Plan::Path3Y), FVector2D(Plan::Path3X1, Plan::Path3Y)}, false));
			Path(Out, TEXT("garden.path.4"), SmoothPath(Points(Plan::Path4)));
			Path(Out, TEXT("garden.path.5"), SmoothPath(Points(Plan::Path5)));
			Path(Out, TEXT("garden.path.6"), SmoothPath(Points(Plan::Path6)));
		}

		/** The zigzag bridge: a slab per run, 0.50 past its turns, on piers from the bed, a low kerb on both its edges. */
		void Bridge(TArray<FFormDrawing>& Out)
		{
			const TArray<FVector2D> Turns = Points(Plan::Bridge);
			const double Half = 0.5 * Plan::BridgeWidth;
			const double Slab = Plan::BridgeTop - Plan::BridgeSlab;
			for (int32 Run = 0; Run + 1 < Turns.Num(); ++Run)
			{
				const FVector2D A = Turns[Run];
				const FVector2D B = Turns[Run + 1];
				const double Length = FVector2D::Distance(A, B);
				const FVector2D Along = (B - A) / Length;
				const FBuildingFrame F = FBuildingFrame::Of(A, Along, FVector2D(-Along.Y, Along.X));
				const FString Name = FString::Printf(TEXT("bridge.run.%d"), Run + 1);
				const double U0 = -Plan::BridgePast;
				const double U1 = Length + Plan::BridgePast;
				Block(Out, Name, TEXT("garden.bluestone"), F, U0, U1, -Half, Half, Slab, Plan::BridgeTop, EHit::Walkable);
				for (const double Side : {-1.0, 1.0})
				{
					const double V0 = Side * Half;
					const double V1 = Side * (Half - Plan::BridgeKerb);
					Block(Out, Name + (Side < 0.0 ? TEXT(".kerb.1") : TEXT(".kerb.2")), TEXT("bridge.kerb"), F, U0, U1, FMath::Min(V0, V1), FMath::Max(V0, V1),
						Plan::BridgeTop, Plan::BridgeTop + Plan::BridgeKerbHigh, EHit::Solid);
				}
				// Piers at even centres no more than 1.60 apart; the turn a run starts from has the last run's.
				const int32 Piers = FMath::Max(1, FMath::CeilToInt32(Length / Plan::BridgePierPitch));
				for (int32 K = Run == 0 ? 0 : 1; K <= Piers; ++K)
				{
					const FVector2D At = FMath::Lerp(A, B, double(K) / Piers);
					const double Radius = FVector2D::Distance(At, Tribune);
					const double Foot = FMath::Max(BedAt(Radius) - Plan::BridgePierUnderBed, Radius < Plan::TribuneClear ? Plan::HallClearZ : -TNumericLimits<double>::Max());
					// Each pier drawn about its own centre, square to its run.
					const double P = 0.5 * Plan::BridgePier;
					const FBuildingFrame Pier = FBuildingFrame::Of(At, Along, FVector2D(-Along.Y, Along.X));
					Block(Out, FString::Printf(TEXT("%s.pier.%d"), *Name, K), TEXT("garden.bluestone.submerged"), Pier, -P, P, -P, P, Foot, Slab, EHit::Solid);
				}
			}
		}

		void Stones(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			int32 Count = 0;
			for (const double Bay : Plan::StoneBays)
			{
				for (const double Offset : Plan::StoneOffsets)
				{
					Block(Out, FString::Printf(TEXT("walk.stone.%d"), ++Count), TEXT("shutiao"), Stage, Plan::StoneX0, Plan::StoneX1, Bay + Offset,
						Bay + Offset + Plan::StoneLength, Plan::StoneZ0, Plan::StoneZ1, EHit::Solid);
				}
			}
		}

		void TerraceAndBase(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			const Plan::FPlanRect& T = Plan::Terrace;
			Block(Out, TEXT("terrace"), TEXT("garden.bluestone"), Stage, T.X0, T.X1, T.Y1, T.Y0, Plan::FootingBottom, Plan::TerraceTop, EHit::Walkable);
			// The water pavilion's west part in the pond: a bluestone base from the bed to its platform's footing.
			const FHallSpec Pavilion = Plan::WaterPavilion();
			const double Foot = Plan::FootingProud;
			const FVector2D South = Pavilion.Frame.Plan(Pavilion.PlatformU0 - Foot, 0.0);
			const FVector2D North = Pavilion.Frame.Plan(Pavilion.PlatformU1 + Foot, 0.0);
			const double West = Pavilion.Frame.Plan(0.0, Pavilion.PlatformFront - Foot).X;
			Block(Out, TEXT("water-pavilion.base"), TEXT("garden.bluestone.submerged"), Stage, West, Plan::PavilionBaseX1, FMath::Min(South.Y, North.Y),
				FMath::Max(South.Y, North.Y), Plan::BedDeep, Plan::FootingBottom, EHit::Solid);
		}
	}

	void DrawGarden(TArray<FFormDrawing>& Out)
	{
		Basin(Out);
		EdgeRocks(Out);
		Paths(Out);
		Bridge(Out);
		Stones(Out);
		TerraceAndBase(Out);
	}
}
