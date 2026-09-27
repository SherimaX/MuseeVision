#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The gilt rosettes (§3.4), one on the centre of every coffer's panel: 750, five bays of 8 × 19 less the lanterns' two.
 * Each is cast in staff and gilt, as the rosettes in the coffers of the Louvre's and the Petit Palais's vaults are: twelve
 * broad outer petals, twelve inner ones between them cupped up round the boss, each petal domed and sunk along its
 * midrib, and a domed boss in a bead ring. It is carved as a relief on the panel's tangent plane, its ground 2 mm into
 * the panel so each petal's edge runs into the plaster (SalonStructure.cpp:889-937, as built). All are cast from one
 * mould: one drawing, set at every coffer's centre (rosette.1 to rosette.750, bay by bay, row by row, column by column).
 */
namespace Musee::Salon
{
	namespace
	{
		/** One ring's petal at (along its axis, across it), metres proud of the panel, or zero off it. */
		double Petal(const Plan::FPetals& P, double AlongAxis, double Across)
		{
			if (AlongAxis <= P.Inner || AlongAxis >= P.Outer)
			{
				return 0.0;
			}
			const double V = (AlongAxis - P.Inner) / (P.Outer - P.Inner);
			// Widest a third of the way out, rounded at its base, pointed at its tip.
			const double HalfWidth = 0.5 * P.Width * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Lerp(Plan::PetalBase, 1.0, V)), Plan::PetalFullness)
				* (1.0 - Plan::PetalTaper * V);
			if (HalfWidth <= 0.0 || FMath::Abs(Across) >= HalfWidth)
			{
				return 0.0;
			}
			const double U = Across / HalfWidth;
			const double Crown = FMath::Lerp(P.HeightAtBase, P.HeightAtTip, V) * FMath::Sqrt(1.0 - U * U);
			const double Midrib = P.Midrib * FMath::Exp(-FMath::Square(U / Plan::MidribSpread))
				* FMath::Sin(UE_DOUBLE_PI * FMath::Min(1.0, Plan::MidribReach * V));
			return FMath::Max(0.0, Crown - Midrib);
		}

		/** The rosette's height over its centre at (X, Y), metres: the highest of its petals, the bead and the boss. */
		double Flower(double X, double Y)
		{
			const double Rho = FMath::Sqrt(X * X + Y * Y);
			if (Rho >= Plan::RosetteRadius)
			{
				return 0.0;
			}
			double Height = 0.0;
			const double Alpha = FMath::Atan2(Y, X);
			for (const Plan::FPetals* Ring : {&Plan::OuterPetals, &Plan::InnerPetals})
			{
				for (int32 K = 0; K < Plan::PetalsPerRing; ++K)
				{
					const double Axis = FMath::DegreesToRadians(Ring->OffsetDeg + 360.0 * K / Plan::PetalsPerRing);
					Height = FMath::Max(Height, Petal(*Ring, Rho * FMath::Cos(Alpha - Axis), Rho * FMath::Sin(Alpha - Axis)));
				}
			}
			const double FromBead = FMath::Abs(Rho - Plan::BeadRadius);
			if (FromBead < Plan::BeadTube)
			{
				Height = FMath::Max(Height, Plan::BeadHeight + FMath::Sqrt(Plan::BeadTube * Plan::BeadTube - FromBead * FromBead));
			}
			if (Rho < Plan::BossRadius)
			{
				Height = FMath::Max(Height, Plan::BossHeight * FMath::Sqrt(1.0 - FMath::Square(Rho / Plan::BossRadius)));
			}
			return Height;
		}

		/** Half the side of the square ground the rosette is carved on, metres. */
		constexpr double RosetteHalf = Plan::RosetteRadius + Plan::RosetteGrid;

		/** Where the rosette stands on the panel of the coffer between column lines C and C + 1 of a bay, in row J. */
		FMuseeFrame RosetteFrame(int32 Bay, int32 C, int32 J)
		{
			const double X = 0.5 * (ColumnX(Bay, C) + ColumnX(Bay, C + 1));
			const double Phi = 0.5 * (RowAngle(J) + RowAngle(J + 1));
			const FVector In(0.0, FMath::Cos(Phi), -FMath::Sin(Phi));
			const FVector AlongVault = FVector::ForwardVector;
			const FVector Across = FVector::CrossProduct(In, AlongVault);
			const FVector Ground = VaultPoint(X, Phi, Plan::VaultRadius + Plan::CofferDepth) - In * Plan::Sink;
			return FrameOfAxes(Ground - AlongVault * RosetteHalf - Across * RosetteHalf, AlongVault, Across);
		}

		/** The rosette's cast: a relief on its square ground. */
		FFormRelief Rosette()
		{
			FFormRelief Shape;
			Shape.SizeMeters = FVector2D(2.0 * RosetteHalf, 2.0 * RosetteHalf);
			Shape.GridMeters = Plan::RosetteGrid;
			Shape.BackMeters = Plan::Sink;
			Shape.HeightMeters = [](double U, double V)
			{
				const double Height = Flower(U - RosetteHalf, V - RosetteHalf);
				return Height > 0.0 ? Plan::Sink + Height : 0.0;
			};
			return Shape;
		}
	}

	void DrawRosettes(TArray<FFormDrawing>& Out)
	{
		FFormDrawing& Rosettes = Out.Add_GetRef(Drawn(TEXT("rosette"), TEXT("vault.rosette"), Rosette(), nullptr));
		for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
		{
			for (int32 J = 0; J < Plan::Rows; ++J)
			{
				for (int32 C = 0; C < Plan::Columns; ++C)
				{
					const bool bLantern = J == Plan::LanternRow && (C == Plan::LanternColumn || C == Plan::LanternColumn + 1);
					if (!bLantern)
					{
						Rosettes.Repeats.Add(RosetteFrame(Bay, C, J));
					}
				}
			}
		}
	}
}
