#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The coffered barrel vault (§3.4), bay by bay between the arches' and end walls' faces. A coffer is the space the
 * ribs leave: each bay is drawn as its shell behind the coffers' panels, a plain band at each end, the transverse ribs
 * on its column lines (turned about the vault's axis) and the longitudinal ribs on its row lines (run along it), each
 * rib carrying the coffers' three steps on both its sides so that where two cross, their steps mitre. Over the crown
 * row the lantern's opening takes two coffers: the ribs round it are halved and stop inside the shaft's walls.
 */
namespace Musee::Salon
{
	namespace
	{
		/** Where the ribs' and bands' backs stop: the panel's depth, run 2 mm into the shell. */
		double Back()
		{
			return Plan::CofferDepth + Plan::Sink;
		}

		/** Half a shaft wall: where a rib halved at the lantern stops, inside the wall. */
		double InsideShaft()
		{
			return 0.5 * Plan::ShaftWall;
		}

		/** The coffer's steps on one side of a rib line at S (Side +1 towards increasing S), from the rib's face to the panel. */
		void AddSteps(TArray<FVector2D>& Out, double S, double Side, bool bOutward)
		{
			const int32 Count = UE_ARRAY_COUNT(Plan::CofferSection);
			for (int32 Step = 0; Step < Count - 1; ++Step)
			{
				const int32 Index = bOutward ? Step + 1 : Count - 1 - Step;
				Out.Add(FVector2D(S + Side * Plan::CofferSection[Index].X, Plan::CofferSection[Index].Y));
			}
		}

		/** The rib section's parts, (along, depth into the vault), drawn anticlockwise with the depth up. */
		enum class ERibSides : uint8 { Both, Minus, Plus };

		TArray<FVector2D> RibSection(double S, ERibSides Sides, double Cut)
		{
			TArray<FVector2D> Points;
			const double Face = Plan::CofferSection[1].X;
			const double Panel = Plan::CofferSection[UE_ARRAY_COUNT(Plan::CofferSection) - 1].X;
			if (Sides == ERibSides::Plus)
			{
				Points.Add(FVector2D(Cut, 0.0));
			}
			else
			{
				Points.Add(FVector2D(S - Face, 0.0));
			}
			if (Sides == ERibSides::Minus)
			{
				Points.Add(FVector2D(Cut, 0.0));
				Points.Add(FVector2D(Cut, Back()));
			}
			else
			{
				AddSteps(Points, S, 1.0, true);
				Points.Add(FVector2D(S + Panel, Back()));
			}
			if (Sides == ERibSides::Plus)
			{
				Points.Add(FVector2D(Cut, Back()));
			}
			else
			{
				Points.Add(FVector2D(S - Panel, Back()));
				AddSteps(Points, S, -1.0, false);
				Points.Pop();
			}
			return Points;
		}

		/** A section drawn in (x, depth) as the lathe's (radius, distance west of its frame): anticlockwise stays so. */
		FFormProfile Transverse(TConstArrayView<FVector2D> XD, double EastX)
		{
			TArray<FVector2D> Points;
			for (const FVector2D& P : XD)
			{
				Points.Add(FVector2D(Plan::VaultRadius + P.Y, EastX - P.X));
			}
			return Polygon(Points);
		}

		/** A transverse piece of a bay's vault turned about its axis between two row angles (radians). */
		FFormDrawing TurnedAbout(const FString& Name, const TCHAR* LocalSurface, TConstArrayView<FVector2D> XD, double EastX, double From, double To)
		{
			FFormLathe Shape;
			Shape.Section = Transverse(XD, EastX);
			Shape.FromDegrees = FMath::RadiansToDegrees(From);
			Shape.ToDegrees = FMath::RadiansToDegrees(To);
			return Drawn(Name, LocalSurface, MoveTemp(Shape), Solid, FaceFrame(FVector(EastX, 0.0, Plan::Springing), FMuseeFrame::West()));
		}

		/**
		 * A longitudinal rib's section, (along the intrados at R 7.000, depth) mapped round the vault at row angle Phi into
		 * the plane across it (y, z): steps of constant depth are arcs about the vault's axis; the closing edge is a line.
		 */
		FFormProfile Longitudinal(TConstArrayView<FVector2D> SD, double Phi)
		{
			auto Angle = [Phi](const FVector2D& P) { return 180.0 - FMath::RadiansToDegrees(Phi + P.X / Plan::VaultRadius); };
			auto At = [&Angle](const FVector2D& P)
			{
				const double T = FMath::DegreesToRadians(Angle(P));
				const double R = Plan::VaultRadius + P.Y;
				return FVector2D(R * FMath::Cos(T), Plan::Springing + R * FMath::Sin(T));
			};
			FFormProfile Profile = FFormProfile::From(At(SD[0]));
			for (int32 Index = 1; Index < SD.Num(); ++Index)
			{
				const FVector2D& A = SD[Index - 1];
				const FVector2D& B = SD[Index];
				if (FMath::IsNearlyEqual(A.Y, B.Y))
				{
					const double R = Plan::VaultRadius + A.Y;
					Profile.ArcAbout(FVector2D(0.0, Plan::Springing), R, R, Angle(A), Angle(B), EFormJoint::Crisp);
				}
				else
				{
					Profile.LineTo(At(B), EFormJoint::Crisp);
				}
			}
			return Profile.Close();
		}

		/** A longitudinal rib run along the vault between two x. */
		FFormDrawing RibAlong(const FString& Name, TConstArrayView<FVector2D> SD, double Phi, double West, double East)
		{
			FFormSweep Shape;
			Shape.Section = Longitudinal(SD, Phi);
			Shape.Path = FFormProfile::Polyline({FVector2D(West, 0.0), FVector2D(East, 0.0)}, false);
			return Drawn(Name, TEXT("vault.coffer"), MoveTemp(Shape), Solid);
		}

		/** The shell behind the panels between two x and two row angles. */
		FFormDrawing ShellBetween(const FString& Name, double West, double East, double EastX, double From, double To)
		{
			const double Panel = Plan::CofferDepth;
			const double Outer = Panel + Plan::VaultShell;
			return TurnedAbout(Name, TEXT("vault.coffer"), {{West, Panel}, {East, Panel}, {East, Outer}, {West, Outer}}, EastX, From, To);
		}

		void BayVault(TArray<FFormDrawing>& Out, int32 Bay)
		{
			const FString Name = FString::Printf(TEXT("vault.bay%d"), Bay + 1);
			const double West = Plan::Bays[Bay].West - Plan::SealIn;
			const double East = Plan::Bays[Bay].East + Plan::SealIn;
			const double Crown0 = RowAngle(Plan::LanternRow);
			const double Crown1 = RowAngle(Plan::LanternRow + 1);
			const int32 L0 = Plan::LanternColumn;
			const int32 L1 = Plan::LanternColumn + 2;
			const double LanternWest = ColumnX(Bay, L0) - InsideShaft();
			const double LanternEast = ColumnX(Bay, L1) + InsideShaft();

			// The shell, open over the lantern.
			Out.Add(ShellBetween(Name + TEXT(".shell.north"), West, East, East, 0.0, Crown0));
			Out.Add(ShellBetween(Name + TEXT(".shell.south"), West, East, East, Crown1, UE_DOUBLE_PI));
			Out.Add(ShellBetween(Name + TEXT(".shell.crown.west"), West, LanternWest, East, Crown0, Crown1));
			Out.Add(ShellBetween(Name + TEXT(".shell.crown.east"), LanternEast, East, East, Crown0, Crown1));

			// The plain bands at the bay's ends, sealed into the arches or end walls, stepping down into the first coffers.
			const double C0 = ColumnX(Bay, 0);
			const double C8 = ColumnX(Bay, Plan::Columns);
			TArray<FVector2D> WestBand = RibSection(C0, ERibSides::Plus, West);
			TArray<FVector2D> EastBand = RibSection(C8, ERibSides::Minus, East);
			Out.Add(TurnedAbout(Name + TEXT(".band.west"), TEXT("vault.band"), WestBand, East, 0.0, UE_DOUBLE_PI));
			Out.Add(TurnedAbout(Name + TEXT(".band.east"), TEXT("vault.band"), EastBand, East, 0.0, UE_DOUBLE_PI));

			// The transverse ribs; those at the lantern stop at its opening, and its edges' ribs keep only their outer half there.
			for (int32 C = 1; C < Plan::Columns; ++C)
			{
				const FString Rib = FString::Printf(TEXT("%s.rib.c%d"), *Name, C);
				const TArray<FVector2D> Full = RibSection(ColumnX(Bay, C), ERibSides::Both, 0.0);
				if (C < L0 || C > L1)
				{
					Out.Add(TurnedAbout(Rib, TEXT("vault.coffer"), Full, East, 0.0, UE_DOUBLE_PI));
					continue;
				}
				Out.Add(TurnedAbout(Rib + TEXT(".north"), TEXT("vault.coffer"), Full, East, 0.0, Crown0));
				Out.Add(TurnedAbout(Rib + TEXT(".south"), TEXT("vault.coffer"), Full, East, Crown1, UE_DOUBLE_PI));
				if (C == L0)
				{
					Out.Add(TurnedAbout(Rib + TEXT(".crown"), TEXT("vault.coffer"), RibSection(ColumnX(Bay, C), ERibSides::Minus, LanternWest), East, Crown0,
						Crown1));
				}
				else if (C == L1)
				{
					Out.Add(TurnedAbout(Rib + TEXT(".crown"), TEXT("vault.coffer"), RibSection(ColumnX(Bay, C), ERibSides::Plus, LanternEast), East, Crown0,
						Crown1));
				}
			}

			// The longitudinal ribs; those at the springings are half ribs, and the two along the lantern are halved beside it.
			for (int32 K = 0; K <= Plan::Rows; ++K)
			{
				const FString Rib = FString::Printf(TEXT("%s.row.%02d"), *Name, K);
				const double Phi = RowAngle(K);
				if (K == 0 || K == Plan::Rows)
				{
					Out.Add(RibAlong(Rib, RibSection(0.0, K == 0 ? ERibSides::Plus : ERibSides::Minus, 0.0), Phi, C0, C8));
					continue;
				}
				const TArray<FVector2D> Full = RibSection(0.0, ERibSides::Both, 0.0);
				if (K != Plan::LanternRow && K != Plan::LanternRow + 1)
				{
					Out.Add(RibAlong(Rib, Full, Phi, C0, C8));
					continue;
				}
				Out.Add(RibAlong(Rib + TEXT(".west"), Full, Phi, C0, LanternWest));
				Out.Add(RibAlong(Rib + TEXT(".east"), Full, Phi, LanternEast, C8));
				Out.Add(RibAlong(Rib + TEXT(".lantern"), RibSection(0.0, K == Plan::LanternRow ? ERibSides::Minus : ERibSides::Plus, 0.0), Phi,
					LanternWest, LanternEast));
			}
		}
	}

	void DrawVault(TArray<FFormDrawing>& Out)
	{
		for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
		{
			BayVault(Out, Bay);
		}
	}
}
