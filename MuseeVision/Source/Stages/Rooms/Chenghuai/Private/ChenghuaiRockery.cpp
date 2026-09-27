#include "ChenghuaiLattice.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlanGarden.h"
#include "ChenghuaiTrades.h"
#include "Math/RandomStream.h"

/**
 * The rockery (假山) and its pavilion (§3.9; ChenghuaiRockery.cpp:22-172, 401-558). The rockery is heaped as a Suzhou
 * rockery is: its mass rising from its foot to its upper outline and the summit, broken into stones about a metre across
 * (a jittered Voronoi of them), each set at its own height and lean with a dark crevice round it, settling into the ground
 * at the foot; a path climbs it in steps to the summit's plateau under the pavilion and falls gently away, the stone beside
 * it standing a kerb's height over it or falling away. It is drawn as one relief over the plan.
 *
 * The hexagonal pavilion (亭 · 見山) stands on the summit: a bluestone platform, six chestnut columns with lintels, fretwork
 * and goose-neck seats on four faces, and its roof: six slabs bent both ways, each its sector's bed and the soffit 0.14
 * under it (the sheet's sweep, concave from the apex and lifting to the corners), with its hips and corner beams following
 * the bed along the corners, and its finial. Its tiles wait for Form's repeated members (K3-FORM-A).
 */
namespace Musee::Chenghuai
{
	namespace
	{
		TArray<FVector2D> Outline(TConstArrayView<Plan::FPlanPoint> Points)
		{
			TArray<FVector2D> Out;
			for (const Plan::FPlanPoint& Point : Points)
			{
				Out.Add(FVector2D(Point.X, Point.Y));
			}
			return Out;
		}

		/** The distance from a point to a loop's boundary: negative inside it. */
		double SignedDistance(TConstArrayView<FVector2D> Loop, const FVector2D& Point)
		{
			double Least = TNumericLimits<double>::Max();
			for (int32 I = 0, J = Loop.Num() - 1; I < Loop.Num(); J = I++)
			{
				const FVector2D AB = Loop[J] - Loop[I];
				const double T = FMath::Clamp(FVector2D::DotProduct(Point - Loop[I], AB) / AB.SizeSquared(), 0.0, 1.0);
				Least = FMath::Min(Least, FVector2D::Distance(Loop[I] + AB * T, Point));
			}
			return Contains(Loop, Point) ? -Least : Least;
		}

		/** The rockery's setting-out, made once: its outlines, its path measured along, and each stone's own draw. */
		struct FRockery
		{
			TArray<FVector2D> Foot;
			TArray<FVector2D> Upper;
			TArray<FVector2D> Path;
			TArray<double> Along;
			double SummitAt = 0.0;
			FVector2D Hex = FVector2D(Plan::HexX, Plan::HexY);
			/** Each stone's draw: its seed's jitter in its cell, its lift, its lean. */
			struct FStone
			{
				FVector2D Seed;
				double Lift;
				FVector2D Lean;
			};
			int32 I0 = 0;
			int32 J0 = 0;
			int32 NI = 0;
			int32 NJ = 0;
			TArray<FStone> Stones;

			FRockery()
			{
				Foot = Outline(Plan::RockeryFoot);
				Upper = Outline(Plan::RockeryUpper);
				Path = Outline(Plan::RockeryPath);
				double Best = TNumericLimits<double>::Max();
				Along.Add(0.0);
				for (int32 K = 1; K < Path.Num(); ++K)
				{
					Along.Add(Along.Last() + FVector2D::Distance(Path[K], Path[K - 1]));
				}
				for (int32 K = 0; K < Path.Num(); ++K)
				{
					const double D = FVector2D::Distance(Path[K], Hex);
					if (D < Best)
					{
						Best = D;
						SummitAt = Along[K];
					}
				}
				// The stones' cells over the sampled field, two cells past it each way, drawn in a fixed order from the rockery's stream.
				const double Cell = Plan::StoneCell;
				I0 = FMath::FloorToInt32(Plan::RockeryX0 / Cell) - 3;
				J0 = FMath::FloorToInt32(Plan::RockeryY0 / Cell) - 3;
				NI = FMath::CeilToInt32(Plan::RockeryX1 / Cell) + 3 - I0;
				NJ = FMath::CeilToInt32(Plan::RockeryY1 / Cell) + 3 - J0;
				FRandomStream Random(Musee::Seed(Key(TEXT("rockery"))));
				for (int32 Index = 0; Index < NI * NJ; ++Index)
				{
					FStone Stone;
					Stone.Seed = FVector2D(FMath::Lerp(Plan::StoneJitter0, Plan::StoneJitter0 + Plan::StoneJitter1, Random.FRand()),
						FMath::Lerp(Plan::StoneJitter0, Plan::StoneJitter0 + Plan::StoneJitter1, Random.FRand()));
					Stone.Lift = (Random.FRand() - Plan::StoneLiftFrom) * Plan::StoneLift;
					Stone.Lean = FVector2D(Random.FRandRange(-1.0, 1.0), Random.FRandRange(-1.0, 1.0)) * Plan::StoneLean;
					Stones.Add(Stone);
				}
			}

			const FStone& StoneAt(int32 I, int32 J) const
			{
				return Stones[FMath::Clamp(J - J0, 0, NJ - 1) * NI + FMath::Clamp(I - I0, 0, NI - 1)];
			}

			/** The massing: rising from the foot to the upper outline and round the summit; OutFoot is the distance outside the foot. */
			double Massing(const FVector2D& Q, double& OutFoot) const
			{
				OutFoot = SignedDistance(Foot, Q);
				const double ToUpper = SignedDistance(Upper, Q);
				const double ToHex = FVector2D::Distance(Q, Hex);
				return Plan::FootRise * FMath::SmoothStep(Plan::FootFrom, Plan::FootTo, -OutFoot)
					+ Plan::UpperRise * FMath::SmoothStep(Plan::UpperFrom, Plan::UpperTo, -ToUpper)
					+ Plan::CrownRise * FMath::SmoothStep(0.0, Plan::CrownTo, Plan::CrownReach - ToHex);
			}

			/** The heaped mass before the path is cut: each stone at its own height and lean, a crevice round it, settled at the foot. */
			double Heaped(const FVector2D& Q) const
			{
				double Outside = 0.0;
				const double H = Massing(Q, Outside);
				if (Outside > Plan::FootOutside)
				{
					return -Plan::FootOutside;
				}
				const double Cell = Plan::StoneCell;
				const int32 CI = FMath::FloorToInt32(Q.X / Cell);
				const int32 CJ = FMath::FloorToInt32(Q.Y / Cell);
				double D1 = TNumericLimits<double>::Max();
				double D2 = TNumericLimits<double>::Max();
				FVector2D Nearest = Q;
				const FStone* Own = nullptr;
				for (int32 DI = -2; DI <= 2; ++DI)
				{
					for (int32 DJ = -2; DJ <= 2; ++DJ)
					{
						const FStone& Stone = StoneAt(CI + DI, CJ + DJ);
						const FVector2D Centre = (FVector2D(CI + DI, CJ + DJ) + Stone.Seed) * Cell;
						const double D = FVector2D::DistSquared(Centre, Q);
						if (D < D1)
						{
							D2 = D1;
							D1 = D;
							Nearest = Centre;
							Own = &Stone;
						}
						else if (D < D2)
						{
							D2 = D;
						}
					}
				}
				const double Edge = 0.5 * (FMath::Sqrt(D2) - FMath::Sqrt(D1));
				double Ignored = 0.0;
				const double Top = Massing(Nearest, Ignored) + Own->Lift + FVector2D::DotProduct(Own->Lean, Q - Nearest)
					- Plan::Crevice * (1.0 - FMath::SmoothStep(0.0, Plan::CreviceWidth, Edge));
				const double Stone = FMath::Lerp(H, Top, FMath::SmoothStep(Plan::SettleFrom, Plan::SettleTo, H));
				return FMath::Max(Stone, H * Plan::MassFloor) - Plan::Sink * FMath::Max(0.0, Outside + Plan::SinkOutside);
			}

			/** The path's walking level a distance along it: up in steps to the summit, down again gently. */
			double PathLevel(double S) const
			{
				const double Total = Along.Last();
				const double Up = FMath::Clamp(S / FMath::Max(SummitAt - Plan::PathShort, Plan::RockeryGrid), 0.0, 1.0);
				const double Down = FMath::Clamp((Total - S) / FMath::Max(Total - SummitAt - Plan::PathShort, Plan::RockeryGrid), 0.0, 1.0);
				const double Z = Plan::Summit * FMath::Min(Up, Down) + Plan::PathLead;
				return FMath::FloorToDouble(Z / Plan::PathRiser + 0.5) * Plan::PathRiser + Plan::PathOver;
			}

			void NearestOnPath(const FVector2D& Q, double& OutDistance, double& OutAlong) const
			{
				OutDistance = TNumericLimits<double>::Max();
				OutAlong = 0.0;
				for (int32 K = 0; K + 1 < Path.Num(); ++K)
				{
					const FVector2D AB = Path[K + 1] - Path[K];
					const double T = FMath::Clamp(FVector2D::DotProduct(Q - Path[K], AB) / AB.SizeSquared(), 0.0, 1.0);
					const double D = FVector2D::Distance(Path[K] + AB * T, Q);
					if (D < OutDistance)
					{
						OutDistance = D;
						OutAlong = FMath::Lerp(Along[K], Along[K + 1], T);
					}
				}
			}

			/** The surface: the heaped mass levelled to the summit's plateau, with the path's ledge cut into it. */
			double Height(const FVector2D& Q) const
			{
				const double ToHex = FVector2D::Distance(Q, Hex);
				if (ToHex < Plan::SummitFlat)
				{
					return Plan::Summit;
				}
				double H = Heaped(Q);
				if (ToHex < Plan::SummitBlend)
				{
					H = FMath::Lerp(Plan::Summit, H, (ToHex - Plan::SummitFlat) / (Plan::SummitBlend - Plan::SummitFlat));
				}
				double D = 0.0;
				double S = 0.0;
				NearestOnPath(Q, D, S);
				if (D < Plan::PathHalf + Plan::PathBeside)
				{
					const double Z = PathLevel(S);
					if (D < Plan::PathHalf)
					{
						return Z;
					}
					const double T = (D - Plan::PathHalf) / Plan::PathBeside;
					H = FMath::Lerp(FMath::Max(Z + Plan::PathKerbOver, H), H, T * T);
				}
				return H;
			}
		};

		const FRockery& Rockery()
		{
			static const FRockery Made;
			return Made;
		}

		/** A corner of the pavilion's hexagon at a radius: corner K at 60° K from east. */
		FVector2D Corner(int32 K, double Radius)
		{
			const double Angle = FMath::DegreesToRadians(60.0 * K);
			return FVector2D(Plan::HexX, Plan::HexY) + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		}

		/** The pavilion's plan is a hexagon: six sectors of 60°, each face's normal half-way between its corners. */
		constexpr double HexSectorDeg = 60.0;

		/**
		 * The roof's bed at a plan point (§3.9): z = base + (apex − base)(1 − f)^1.35 + 0.45 c³ f², f the point's fraction out
		 * to the eave along the normal of its sector's face, and c its angle from that normal over half a sector.
		 */
		double RoofBed(const FVector2D& Point)
		{
			const FVector2D Off = Point - FVector2D(Plan::HexX, Plan::HexY);
			const double Half = 0.5 * HexSectorDeg;
			const double Angle = FMath::RadiansToDegrees(FMath::Atan2(Off.Y, Off.X));
			const double FromFace = Angle - (HexSectorDeg * FMath::FloorToDouble(Angle / HexSectorDeg) + Half);
			const double Apothem = Plan::HexEave * FMath::Cos(FMath::DegreesToRadians(Half));
			const double F = FMath::Clamp(Off.Size() * FMath::Cos(FMath::DegreesToRadians(FromFace)) / Apothem, 0.0, 1.0);
			const double C = FMath::Abs(FromFace) / Half;
			return Plan::HexRoofBase + (Plan::HexApex - Plan::HexRoofBase) * FMath::Pow(1.0 - F, Plan::HexRoofPower) + Plan::HexCornerLift * C * C * C * F * F;
		}

		void Seat(TArray<FFormDrawing>& Out, int32 Face, const FVector2D& P, const FVector2D& Q)
		{
			const FVector2D Centre(Plan::HexX, Plan::HexY);
			const FVector2D Along = (Q - P).GetSafeNormal();
			const FVector2D In = (Centre - 0.5 * (P + Q)).GetSafeNormal();
			const double Floor = Plan::HexFloor;
			const double End = Plan::HexColumnR + Plan::SeatClear;
			const FVector2D S0 = P + Along * End;
			const FVector2D S1 = Q - Along * End;
			const FVector Across(In.X, In.Y, 0.0);
			auto At = [](const FVector2D& Plan2, double Z) { return FVector(Plan2.X, Plan2.Y, Z); };
			const FString Name = FString::Printf(TEXT("pavilion.seat.%d"), Face);
			const double PlankZ = Floor + Plan::SeatZ - 0.5 * Plan::SeatPlankThick;
			Bar(Out, Name + TEXT(".plank"), TEXT("timber.garden"), At(S0 + In * Plan::SeatInside, PlankZ), At(S1 + In * Plan::SeatInside, PlankZ), Across,
				Plan::SeatPlank, Plan::SeatPlankThick, EHit::Walkable);
			const double BaseTop = Floor + Plan::SeatZ - Plan::SeatPlankThick;
			Bar(Out, Name + TEXT(".base"), TEXT("timber.garden"), At(S0 + In * Plan::SeatInside, 0.5 * (Floor + BaseTop)),
				At(S1 + In * Plan::SeatInside, 0.5 * (Floor + BaseTop)), Across, Plan::SeatBaseWide, BaseTop - Floor, EHit::Solid);
			// The goose-neck back (美人靠): balusters rising and leaning out over the drop, each a square bar carved on its curve.
			const FBuildingFrame Out2 = FBuildingFrame::Of(S0, -In, Along);
			const double Length = FVector2D::Distance(S0, S1);
			const int32 Balusters = FMath::Max(3, FMath::RoundToInt32(Length / Plan::BalusterPitch));
			TArray<FVector2D> Neck;
			for (int32 K = 0; K < Plan::BalusterStations; ++K)
			{
				const double T = double(K) / (Plan::BalusterStations - 1);
				Neck.Add(FVector2D(Plan::BalusterLean0 + Plan::BalusterLean * FMath::Sin(Plan::BalusterBow * T), Floor + Plan::BalusterFoot + Plan::BalusterRise * T));
			}
			const double Half = 0.5 * Plan::Baluster;
			for (int32 K = 0; K <= Balusters; ++K)
			{
				const double V = Length * K / Balusters;
				ReliefAlongV(Out, FString::Printf(TEXT("%s.baluster.%d"), *Name, K + 1), TEXT("timber.garden"), Out2, Neck, V - Half, V + Half, Plan::MemberGrid,
					Plan::ReliefBack, [](double, double) { return Plan::Baluster - Plan::ReliefBack; });
			}
			const FVector2D Back = -In * Plan::BackRailOut;
			const double RailZ = Floor + Plan::BackRailZ;
			Bar(Out, Name + TEXT(".back-rail"), TEXT("timber.garden"), At(S0 + Back, RailZ), At(S1 + Back, RailZ), Across, Plan::BackRailWide, Plan::BackRailHigh,
				EHit::Solid);
		}

		void Pavilion(TArray<FFormDrawing>& Out)
		{
			const FVector2D Centre(Plan::HexX, Plan::HexY);
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			const double Floor = Plan::HexFloor;
			Raised(Out, TEXT("pavilion.platform"), TEXT("garden.bluestone"), Stage, RegularPolygon(Centre, Plan::HexPlatform, 6, 0.0), {},
				Plan::HexPlatformBottom, Floor, EHit::Walkable);
			for (int32 K = 0; K < 6; ++K)
			{
				const FVector2D P = Corner(K, Plan::HexColumns);
				const FVector2D Q = Corner(K + 1, Plan::HexColumns);
				const FString Column = FString::Printf(TEXT("pavilion.column.%d"), K + 1);
				const double Base = 0.5 * Plan::HexBase;
				Block(Out, Column + TEXT(".base"), TEXT("garden.bluestone"), Stage, P.X - Base, P.X + Base, P.Y - Base, P.Y + Base, Floor - Plan::HexBaseUnder,
					Floor + Plan::HexBaseOver, EHit::Solid);
				Turned(Out, Column + TEXT(".shaft"), TEXT("timber.garden"), FVector(P.X, P.Y, Floor + Plan::HexBaseOver),
					Section({{0.0, 0.0}, {Plan::HexColumnR, 0.0}, {Plan::HexColumnTop, Plan::HexColumnH - Plan::HexBaseOver},
						{0.0, Plan::HexColumnH - Plan::HexBaseOver}}, false), EHit::Solid);
				const FVector2D Along = (Q - P).GetSafeNormal();
				const FVector Out3(Along.Y, -Along.X, 0.0);
				const double LintelZ = Floor + Plan::HexLintelZ;
				Bar(Out, FString::Printf(TEXT("pavilion.lintel.%d"), K + 1), TEXT("timber.garden"), FVector(P.X, P.Y, LintelZ), FVector(Q.X, Q.Y, LintelZ), Out3,
					Plan::HexLintelWide, Plan::HexLintelHigh, EHit::None);
				const bool bSeated = MakeArrayView(Plan::HexSeatedFaces).Contains(K);
				if (!bSeated)
				{
					continue;
				}
				// The fretwork under the lintel between the columns, and the seat below.
				const FBuildingFrame Face = FBuildingFrame::Of(P, Along, FVector2D(-Along.Y, Along.X));
				const double Clear = Plan::HexColumnR + Plan::HexFretworkClear;
				DrawPanel(Out, FString::Printf(TEXT("pavilion.fretwork.%d"), K + 1), TEXT("timber.garden"), Face, Clear, -0.5 * Plan::LatticeDepth,
					Floor + Plan::HexFretworkZ, FVector2D::Distance(P, Q) - 2.0 * Clear, Plan::HexFretwork, ELattice::StepBrocade, false);
				Seat(Out, K, P, Q);
			}
			// The roof: a slab over each sector, its bed bent both ways and the soffit 0.14 under it.
			for (int32 K = 0; K < 6; ++K)
			{
				const FVector2D P = Corner(K, Plan::HexEave);
				FFormHeightfield Slab;
				Slab.Outline = FFormProfile::Polyline({Centre, P, Corner(K + 1, Plan::HexEave)}, true);
				Slab.TopMeters = [](double X, double Y) { return RoofBed(FVector2D(X, Y)); };
				Slab.BottomMeters = [](double X, double Y) { return RoofBed(FVector2D(X, Y)) - Plan::HexSoffit; };
				Slab.GridMeters = Plan::HexRoofGrid;
				Draw(Out, FString::Printf(TEXT("pavilion.roof.%d"), K + 1), TEXT("roof.bed"), MoveTemp(Slab), EHit::None);
				// The hip (垂脊) up this sector's first corner, on the bed and rising at the corner; and the corner beam (角梁) under it.
				const FVector2D Hip = (P - Centre).GetSafeNormal();
				const FBuildingFrame HipFrame = FBuildingFrame::Of(Centre, Hip, FVector2D(-Hip.Y, Hip.X));
				const double Reach = Plan::HexEave;
				TArray<FVector2D> Line;
				for (int32 S = 0; S < Plan::HipStations; ++S)
				{
					const double T = double(S) / (Plan::HipStations - 1);
					const double U = Reach * FMath::Lerp(Plan::HipFrom, 1.0, T);
					const double Z = RoofBed(Centre + Hip * U) + Plan::HipOver + Plan::HipLift * FMath::Pow(T, 6.0);
					Line.Add(FVector2D(U, Z + Plan::HipUnder));
				}
				ReliefAlongV(Out, FString::Printf(TEXT("pavilion.hip.%d"), K + 1), TEXT("roof.ridge"), HipFrame, Line, Plan::HipTop[0].A, -Plan::HipTop[0].A,
					Plan::MemberGrid, Plan::ReliefBack, [](double, double V) { return TopAt(Plan::HipTop, V) - Plan::HipUnder; });
				auto Under = [&](double Fraction, double Drop)
				{
					const FVector2D At = FMath::Lerp(Centre, P, Fraction);
					return FVector(At.X, At.Y, RoofBed(At) - Plan::HexSoffit - Drop);
				};
				Bar(Out, FString::Printf(TEXT("pavilion.corner-beam.%d"), K + 1), TEXT("timber.garden"), Under(Plan::CornerBeamFrom, Plan::CornerBeamUnder0),
					Under(Plan::CornerBeamTo, Plan::CornerBeamUnder1), FVector(-Hip.Y, Hip.X, 0.0), Plan::CornerBeamWide, Plan::CornerBeamHigh, EHit::None);
			}
			Turned(Out, TEXT("pavilion.finial"), TEXT("roof.ridge"), FVector(Centre.X, Centre.Y, Plan::FinialZ), Section(Plan::FinialSection, false), EHit::None);
		}
	}

	double RockeryHeight(const FVector2D& Point)
	{
		return FMath::Max(0.0, Rockery().Height(Point));
	}

	void DrawRockery(TArray<FFormDrawing>& Out)
	{
		const FVector2D Size(Plan::RockeryX1 - Plan::RockeryX0, Plan::RockeryY1 - Plan::RockeryY0);
		ReliefOnPlan(Out, TEXT("rockery"), TEXT("rockery"), FBuildingFrame::Stage(), Plan::RockeryX0, Plan::RockeryY0, Size, 0.0, Plan::RockeryGrid,
			Plan::RockeryBack, [](double X, double Y) { return RockeryHeight(FVector2D(X, Y)); }).Collision = CollisionOf(EHit::Walkable);
		Pavilion(Out);
	}
}
