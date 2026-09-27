#include "AlbionParts.h"

/**
 * The vaults' glass and what stands on it (§3.5, §3.6): on each half, between each pair of glazing bars, a column of flat
 * panes from 10.10 to the crown, each its own plane on the arc's chord and lapping 10 mm over the one below, so the
 * reflections break at every lap as real glazing's do; clear over the nave, diffusing over the aisles. On the crowns the
 * lead rolls, the aisles' cast crestings and the nave's ventilator, its standards under a lead cap (its louvres wait for
 * polish, §12).
 */
namespace Musee::Albion
{
	namespace
	{
		/** Where two lines meet: P + λu and Q + μv. */
		FVector2D Meet(const FVector2D& P, const FVector2D& U, const FVector2D& Q, const FVector2D& V)
		{
			const double Lambda = FVector2D::CrossProduct(Q - P, V) / FVector2D::CrossProduct(U, V);
			return P + U * Lambda;
		}

		/**
		 * A column of panes in section (x, z): up the panes' inner faces on the chords, over the top pane's end, and down
		 * their outer faces, stepping out at each lap where a pane's foot runs 10 mm down over the pane below it.
		 */
		FFormProfile PaneColumn(const FVaultHalf& H)
		{
			const double T = Plan::LaminatedMeters;
			const FVector2D Centre(H.CentreX(), Plan::Springing);
			TArray<FVector2D> Stations;
			for (const double S : H.PaneStations())
			{
				Stations.Add(H.At(S));
			}
			const int32 Count = Stations.Num() - 1;
			TArray<FVector2D> Directions;
			TArray<FVector2D> Outward;
			for (int32 Pane = 0; Pane < Count; ++Pane)
			{
				const FVector2D D = (Stations[Pane + 1] - Stations[Pane]).GetSafeNormal();
				FVector2D M(-D.Y, D.X);
				if (FVector2D::DotProduct(M, Stations[Pane] - Centre) < 0.0)
				{
					M = -M;
				}
				Directions.Add(D);
				Outward.Add(M);
			}
			TArray<FVector2D> Outline = Stations;
			Outline.Add(Stations[Count] + Outward[Count - 1] * T);
			for (int32 Pane = Count - 1; Pane >= 1; --Pane)
			{
				const FVector2D Foot = Stations[Pane] - Directions[Pane] * Plan::PaneLap + Outward[Pane] * T;
				Outline.Add(Foot);
				Outline.Add(Meet(Foot, -Outward[Pane], Stations[Pane] + Outward[Pane - 1] * T, Directions[Pane - 1]));
			}
			Outline.Add(Stations[0] + Outward[0] * T);
			return Polygon(Outline);
		}

		void VaultPanes(TArray<FFormDrawing>& Out)
		{
			const double Pitch = 2.0 * Plan::GlassEndY / Plan::GlazingBays;
			for (const FVaultHalf& H : VaultHalves())
			{
				const FFormProfile Column = PaneColumn(H);
				for (int32 Index = 0; Index < Plan::GlazingBays; ++Index)
				{
					FFormExtrusion Shape;
					Shape.Outline = Column;
					Shape.DepthMeters = Pitch;
					Out.Add(Glass(Drawn(FString::Printf(TEXT("vault.glass.%s.%02d"), H.Name, Index + 1), H.bNave ? TEXT("vault.glass.nave") : TEXT("vault.glass.aisle"),
						MoveTemp(Shape), Overhead, AcrossY(-Plan::GlassEndY + Pitch * (Index + 1)))));
				}
			}
		}

		/** The lead rolls on the three crowns; the aisles' crestings; the nave's ventilator standards and its cap. */
		void Ridges(TArray<FFormDrawing>& Out)
		{
			const double End = Plan::RidgeRollEndY;
			const TPair<const TCHAR*, FVector2D> Crowns[] = {{TEXT("nave"), FVector2D(0.0, Plan::NaveCrown)},
				{TEXT("west"), FVector2D(-Plan::AisleCrownX, Plan::AisleCrown)}, {TEXT("east"), FVector2D(Plan::AisleCrownX, Plan::AisleCrown)}};
			for (const TPair<const TCHAR*, FVector2D>& Crown : Crowns)
			{
				const double X = Crown.Value.X;
				const double Z = Crown.Value.Y + Plan::RidgeRollLift;
				Out.Add(Rod(FString::Printf(TEXT("lead.ridge.%s"), Crown.Key), TEXT("lead"), FVector(X, -End, Z), FVector(X, End, Z), Plan::RidgeRollRadius, Overhead));
				if (X != 0.0)
				{
					const double Half = 0.5 * Plan::CrestingMeters;
					Out.Add(Box(FString::Printf(TEXT("cresting.%s"), Crown.Key), TEXT("iron.ext"), FVector(X - Half, -End, Plan::CrestingFoot),
						FVector(X + Half, End, Plan::CrestingTop), Overhead));
				}
			}
			int32 Standard = 0;
			for (double Y = Plan::FirstLineY; Y <= -Plan::FirstLineY + 1e-6; Y += Plan::StandardPitch)
			{
				Out.Add(Box(FString::Printf(TEXT("ventilator.standard.%02d"), ++Standard), TEXT("iron.ext"),
					FVector(-Plan::StandardHalfX, Y - Plan::StandardHalfY, Plan::StandardFoot), FVector(Plan::StandardHalfX, Y + Plan::StandardHalfY, Plan::StandardTop),
					Overhead));
			}
			FFormExtrusion Cap;
			Cap.Outline = Section({FVector2D(-Plan::CapHalf, Plan::StandardTop), FVector2D(Plan::CapHalf, Plan::StandardTop), FVector2D(0.0, Plan::CapRidge)});
			Cap.DepthMeters = 2.0 * End;
			Out.Add(Drawn(TEXT("ventilator.cap"), TEXT("lead"), MoveTemp(Cap), Overhead, AcrossY(End)));
		}
	}

	void DrawRoof(TArray<FFormDrawing>& Out)
	{
		VaultPanes(Out);
		Ridges(Out);
	}
}
