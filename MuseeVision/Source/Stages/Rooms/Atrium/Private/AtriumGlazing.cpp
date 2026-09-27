#include "AtriumParts.h"

/**
 * The drum's glazing (§3.4): its double skin (the misty inner leaf in 96 panes, the clear outer leaf between the order's
 * verticals, the head closer that caps the cavity). The canopy's panes are cut on the grid that carries them
 * (AtriumCanopyGrid).
 */
namespace Musee::Atrium
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		constexpr double HalfJoint = 0.5 * Plan::PaneJoint;
		constexpr int32 PanesRound = int32(360.0 / Plan::PaneBearingStep);

		/** The tiers of a leaf: from its foot through the transoms' levels to its head. */
		TArray<double> Tiers(double Top)
		{
			TArray<double> Levels = {Plan::WallTop};
			Levels.Append(Plan::Transoms, UE_ARRAY_COUNT(Plan::Transoms));
			Levels.Add(Top);
			return Levels;
		}

		/** The bearing a joint's side stands at on a leaf of this radius. */
		double Inset(double Radius)
		{
			return FMath::RadiansToDegrees(FMath::Asin(HalfJoint / Radius));
		}

		/** A pane of a leaf between two bearings and two heights. */
		void Pane(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, double R0, double R1, double From, double To, double Z0,
			double Z1)
		{
			AsGlass(Out.Add_GetRef(Drawn(Name, LocalSurface, Band(R0, R1, Z0, Z1, From, To), Solid)));
		}

		/** The inner leaf: 24 panes round, 15° each with their joints behind the ribs, fins and mullions, in four tiers. */
		void InnerLeaf(TArray<FFormDrawing>& Out)
		{
			const TArray<double> Levels = Tiers(Plan::CanopyMeetsDrum);
			const double Side = Inset(Plan::BaseInner);
			for (int32 Tier = 0; Tier + 1 < Levels.Num(); ++Tier)
			{
				const double Z0 = Levels[Tier] + (Tier > 0 ? HalfJoint : 0.0);
				const double Z1 = Levels[Tier + 1] - (Tier + 2 < Levels.Num() ? HalfJoint : 0.0);
				for (int32 Index = 0; Index < PanesRound; ++Index)
				{
					const double From = Plan::PaneBearingStep * Index;
					Pane(Out, FString::Printf(TEXT("%st%d.%02d"), Pieces::InnerLeaf, Tier + 1, Index + 1), TEXT("drum.inner"), Plan::BaseInner,
						Plan::InnerLeafOuter, From + Side, From + Plan::PaneBearingStep - Side, Z0, Z1);
				}
			}
			// The head closer, a pearl steel ring capping the leaf and the cavity.
			Out.Add(Drawn(Pieces::Closer, TEXT("canopy.grid"), TurnedAround(Box(Plan::BaseInner, Plan::CanopyMeetsDrum, Plan::CloserOuter, Plan::CloserTop)),
				Solid));
		}

		/**
		 * The outer leaf: 15° panes between the order's verticals, in four tiers to 20.950. Over the portal (where its attic
		 * meets the cavity's outer face, r 14.050) the lowest tier starts on the portal's roof; a pane the portal's side
		 * crosses is two panes, meeting the stone there.
		 */
		void OuterLeaf(TArray<FFormDrawing>& Out)
		{
			const TArray<double> Levels = Tiers(Plan::OuterLeafTop);
			const double Side = Inset(Plan::OuterLeafInner);
			const double Portal = FMath::RadiansToDegrees(FMath::Asin(Plan::PortalHalfWidth / Plan::OuterLeafInner));
			const double PortalFrom = Plan::DoorBearing - Portal;
			const double PortalTo = Plan::DoorBearing + Portal;
			for (int32 Tier = 0; Tier + 1 < Levels.Num(); ++Tier)
			{
				const double Z0 = Levels[Tier] + (Tier > 0 ? HalfJoint : 0.0);
				const double Z1 = Levels[Tier + 1] - (Tier + 2 < Levels.Num() ? HalfJoint : 0.0);
				for (int32 Index = 0; Index < PanesRound; ++Index)
				{
					const double From = Plan::PaneBearingStep * Index + Side;
					const double To = Plan::PaneBearingStep * (Index + 1) - Side;
					const FString Name = FString::Printf(TEXT("drum.outer.t%d.%02d"), Tier + 1, Index + 1);
					if (Tier > 0 || To <= PortalFrom || From >= PortalTo)
					{
						Pane(Out, Name, TEXT("drum.outer"), Plan::OuterLeafInner, Plan::OuterLeafOuter, From, To, Z0, Z1);
						continue;
					}
					// Over the portal: the pane stands on its roof; beside it, down to the wall top.
					const double RoofFrom = FMath::Max(From, PortalFrom);
					const double RoofTo = FMath::Min(To, PortalTo);
					Pane(Out, Name + TEXT(".over-portal"), TEXT("drum.outer"), Plan::OuterLeafInner, Plan::OuterLeafOuter, RoofFrom, RoofTo,
						Plan::SeatOnPortal, Z1);
					if (From < PortalFrom)
					{
						Pane(Out, Name, TEXT("drum.outer"), Plan::OuterLeafInner, Plan::OuterLeafOuter, From, PortalFrom, Z0, Z1);
					}
					if (To > PortalTo)
					{
						Pane(Out, Name, TEXT("drum.outer"), Plan::OuterLeafInner, Plan::OuterLeafOuter, PortalTo, To, Z0, Z1);
					}
				}
			}
		}
	}

	void DrawGlazing(TArray<FFormDrawing>& Out)
	{
		InnerLeaf(Out);
		OuterLeaf(Out);
	}
}
