#include "CubeParts.h"

/**
 * The glass shaft from the Atrium's slab down to the box's roof (§3.3, §3.4): sixteen curved panes of 60 mm laminated
 * low-iron glass in four tiers, standing on the tier webs and the foot plate; inside them the gilt tier frames and the
 * joint strips over the panes' joints; outside them the cavity to the ground's cut face, closed by the head and foot
 * plates and crossed by the three webs.
 *
 * The frames are notched round each rail and its bracket: a turned solid is cut radially, so each frame is two half
 * rings whose ends stand 0.095 either side of the rail's centre line at the frame's bore, the nearest honest form of the
 * square notch.
 */
namespace Musee::Cube
{
	namespace
	{
		/** The sixteen panes, 90 degrees each between the joints on the diagonals. */
		void Glass(TArray<FFormDrawing>& Out)
		{
			using namespace Plan;
			for (int32 Tier = 0; Tier < Tiers; ++Tier)
			{
				for (int32 Pane = 0; Pane < PanesPerTier; ++Pane)
				{
					const double From = FirstJointBearing + 360.0 * Pane / PanesPerTier;
					const double To = From + 360.0 / PanesPerTier;
					AsGlass(Out.Add_GetRef(Drawn(FString::Printf(TEXT("%st%d.%d"), Pieces::Glass, Tier + 1, Pane + 1), TEXT("shaft.glass"),
						Band(GlassInner, GlassOuter, PaneEdges[Tier][0], PaneEdges[Tier][1], From, To), Solid)));
				}
			}
		}

		/** The five gilt frames inside the glass, each two half rings between the rails' notches. */
		void Frames(TArray<FFormDrawing>& Out)
		{
			using namespace Plan;
			const double Gap = HalfAngle(0.5 * FrameNotch, FrameInner);
			for (int32 Frame = 0; Frame < Tiers + 1; ++Frame)
			{
				for (int32 Half = 0; Half < 2; ++Half)
				{
					const double From = RailBearings[Half] + Gap;
					const double To = RailBearings[(Half + 1) % 2] - Gap;
					Out.Add(Drawn(FString::Printf(TEXT("%s%d.%s"), Pieces::Frame, Frame + 1, Half == 0 ? TEXT("east") : TEXT("west")), TEXT("shaft.gilt"),
						Band(FrameInner, FrameOuter, FrameBands[Frame][0], FrameBands[Frame][1], From, To), Solid));
				}
			}
		}

		/** The joint strips, 25 mm wide over each pane joint, one tier long between its frames. */
		void Strips(TArray<FFormDrawing>& Out)
		{
			using namespace Plan;
			const double Half = FMath::RadiansToDegrees(0.5 * StripWidth / (0.5 * (StripInner + StripOuter)));
			for (int32 Tier = 0; Tier < Tiers; ++Tier)
			{
				for (int32 Joint = 0; Joint < PanesPerTier; ++Joint)
				{
					const double Bearing = FirstJointBearing + 360.0 * Joint / PanesPerTier;
					Out.Add(Drawn(FString::Printf(TEXT("shaft.strip.t%d.%d"), Tier + 1, Joint + 1), TEXT("shaft.gilt"),
						Band(StripInner, StripOuter, StripRuns[Tier][0], StripRuns[Tier][1], Bearing - Half, Bearing + Half), Solid));
				}
			}
		}

		/** The head and foot plates over the glass and the cavity, and the three webs the panes stand on. */
		void Plates(TArray<FFormDrawing>& Out)
		{
			using namespace Plan;
			Out.Add(Drawn(FString::Printf(TEXT("%shead"), Pieces::Plate), TEXT("shaft.plate"), Ring(PlateInner, PlateOuter, HeadPlate[0], HeadPlate[1]), Solid));
			Out.Add(Drawn(FString::Printf(TEXT("%sfoot"), Pieces::Plate), TEXT("shaft.plate"), Ring(PlateInner, PlateOuter, FootPlate[0], FootPlate[1]), Solid));
			for (int32 Web = 1; Web < Tiers; ++Web)
			{
				const double Level = TierLevels[Web];
				Out.Add(Drawn(FString::Printf(TEXT("%s%d"), Pieces::Web, Web), TEXT("shaft.plate"),
					Ring(PlateInner, WebOuter, Level - 0.5 * WebThickness, Level + 0.5 * WebThickness), Solid));
			}
		}
	}

	void DrawShaft(TArray<FFormDrawing>& Out)
	{
		Glass(Out);
		Frames(Out);
		Strips(Out);
		Plates(Out);
	}
}
