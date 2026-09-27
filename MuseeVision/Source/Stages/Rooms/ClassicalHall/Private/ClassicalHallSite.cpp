#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"
#include "SiteDescription.h"

/**
 * What the hall declares to the site and the systems (§1, §2): its footprint, the port where the Rotunda's stair comes
 * down and the well it receives, the openings in its shell, the Mount anchors its carriers offer the casts, and its one
 * enclosure with the pieces that bound it.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		/**
		 * Stations on an arc about a centre from one angle to another: its ends, and between them the sheet's stations,
		 * every 1.5° of the full round (240 round), so the arcs pass the sheet's points at the quarters.
		 */
		void ArcStations(TArray<FVector2D>& Out, const FVector2D& Centre, double Radius, double FromDeg, double ToDeg)
		{
			const double Step = Plan::ArcStationDeg;
			const double Sense = ToDeg > FromDeg ? 1.0 : -1.0;
			const double Tolerance = 1e-9;
			Out.Add(Centre + Polar(Radius, FromDeg));
			double Station = Step * (Sense > 0.0 ? FMath::FloorToDouble(FromDeg / Step + Tolerance) + 1.0 : FMath::CeilToDouble(FromDeg / Step - Tolerance) - 1.0);
			for (; Sense * (ToDeg - Station) > Tolerance; Station += Sense * Step)
			{
				Out.Add(Centre + Polar(Radius, Station));
			}
			Out.Add(Centre + Polar(Radius, ToDeg));
		}

		/**
		 * The footprint (§1), from the south-east corner: along the shell's south face to the sleeve, round the port
		 * cylinder about W bulging into the hall, back to the south face, up the west side, the long way round the
		 * tribune's shell by the north, and down the east side.
		 */
		void Footprint(FSiteDescription& Out)
		{
			const FVector2D W(0.0, Plan::WellAxisY);
			const double SleeveY = Plan::WellAxisY - FMath::Sqrt(FMath::Square(Plan::WellRadius) - FMath::Square(Plan::SleeveHalf));
			const double SideY = FMath::Sqrt(FMath::Square(Plan::ShellRadius) - FMath::Square(Plan::ShellHalfWidth));
			const double WellFrom = FMath::RadiansToDegrees(FMath::Atan2(SleeveY - W.Y, Plan::SleeveHalf));
			const double TribuneFrom = FMath::RadiansToDegrees(FMath::Atan2(SideY, -Plan::ShellHalfWidth));
			TArray<FVector2D>& Outline = Out.Footprint.OutlineMeters;
			Outline = {FVector2D(Plan::ShellHalfWidth, Plan::ShellSouthY), FVector2D(Plan::SleeveHalf, Plan::ShellSouthY)};
			ArcStations(Outline, W, Plan::WellRadius, WellFrom, -180.0 - WellFrom);
			Outline.Append({FVector2D(-Plan::SleeveHalf, Plan::ShellSouthY), FVector2D(-Plan::ShellHalfWidth, Plan::ShellSouthY)});
			ArcStations(Outline, FVector2D::ZeroVector, Plan::ShellRadius, TribuneFrom, 540.0 - TribuneFrom);
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.BottomMeters = Plan::SlabUnderside;
			Out.Footprint.TopMeters = Plan::RoofTop;
		}

		/** The one port: the Rotunda's served stairwell, received here as a void, the port face the well's own cylinder. */
		void Port(FSiteDescription& Out)
		{
			FSitePort& Stair = Out.Ports.AddDefaulted_GetRef();
			Stair.Key = Key(TEXT("stair"));
			Stair.Kind = ESitePortKind::Stairwell;
			Stair.Class = FMuseeKey(Plan::PortClass);
			Stair.Frame = Turned(FVector(0.0, Plan::PortFaceY, 0.0), 90.0);
			Stair.WallDepthMeters = Plan::PortFaceY - Plan::SouthFaceY;
			// The well bulges into the hall: its axis stands ahead of the port face, a negative radius; the wall behind it is the
			// gallery's flat south face (y 31.10), 0.600 behind on the axis (FSitePort).
			Stair.FaceRadiusMeters = -Plan::WellRadius;
			Stair.bFlatInnerFace = true;
			Stair.Mates = FMuseeKey(Plan::PortMates);
			Stair.bHasWell = true;
			Stair.Well.bServing = false;
			Stair.Well.MinMeters = FVector(-Plan::WellRadius, Plan::WellAxisY - Plan::WellRadius, Plan::SlabUnderside);
			Stair.Well.MaxMeters = FVector(Plan::WellRadius, Plan::WellAxisY + Plan::WellRadius, Plan::WellTop);
		}

		FFormOpening& Opening(FSiteDescription& Out, const FString& Local, EFormOpeningKind Kind, const FMuseeFrame& Frame, const FVector2D& Size)
		{
			FFormOpening& Made = Out.Openings.AddDefaulted_GetRef();
			Made.Key = Key(Local);
			Made.Kind = Kind;
			Made.Enclosure = Key(TEXT("interior"));
			Made.Frame = Frame;
			Made.SizeMeters = Size;
			return Made;
		}

		/**
		 * The openings: the rough opening at the port (the class opening grown by Site's lining allowance), facing south
		 * into the well; and the Housings of the five laylights, the four lantern wells and the eye, each looking up
		 * into its plenum under the roof slab.
		 */
		void Openings(FSiteDescription& Out)
		{
			const double Half = Plan::PortClearHalf + Plan::LiningAllowance;
			const double Sill = -Plan::LiningAllowance;
			const double Crown = Plan::PortSpringing + Half;
			const FMuseeFrame Port = FrameOfAxes(FVector(0.0, Plan::PortFaceY, 0.5 * (Sill + Crown)), FVector(-1.0, 0.0, 0.0), FVector::UpVector);
			Opening(Out, TEXT("stair"), EFormOpeningKind::Port, Port, FVector2D(2.0 * Half, Crown - Sill));

			const double Inner = Plan::LanternHalf - Plan::LanternFrame;
			const FVector2D Glass(2.0 * Barrel(Inner, Plan::VaultRadius).X, 2.0 * Inner);
			for (int32 Bay = 0; Bay < Plan::Bays; ++Bay)
			{
				const FMuseeFrame Up(FVector(0.0, Plan::BayCentres[Bay], Plan::LaylightUnderside));
				Opening(Out, FString::Printf(TEXT("lantern.bay%d"), Bay + 1), EFormOpeningKind::Housing, Up, Glass);
			}
			Opening(Out, TEXT("eye"), EFormOpeningKind::Housing, FMuseeFrame(FVector(0.0, 0.0, Plan::EyeZ)),
				FVector2D(2.0 * Plan::EyeRadius, 2.0 * Plan::EyeRadius));
		}

		/** One Mount per carrier (§2): origin at the carrier's top centre, x the way its work faces, z up. */
		void Anchors(FSiteDescription& Out)
		{
			for (const Plan::FCarrierPlan& Carrier : Plan::Carriers)
			{
				const double Facing = Carrier.FacingDeg < 0.0 ? FMath::RadiansToDegrees(FMath::Atan2(-Carrier.Y, -Carrier.X)) : Carrier.FacingDeg;
				Out.Anchors.Add({Key(Carrier.Anchor), FName(TEXT("Mount")), Turned(FVector(Carrier.X, Carrier.Y, Carrier.Top), Facing)});
			}
		}

		/** The one enclosure, bounded by the structural masonry: every shell piece, from the slab to the roof. */
		void Enclosure(FSiteDescription& Out)
		{
			FFormEnclosure& Interior = Out.Enclosures.AddDefaulted_GetRef();
			Interior.Key = Key(TEXT("interior"));
			Interior.Kind = EFormEnclosureKind::Enclosed;
			for (const FFormDrawing& Piece : Out.Pieces)
			{
				if (Piece.Name.ToString().StartsWith(ShellPrefix))
				{
					Interior.Pieces.Add(Piece.Name);
				}
			}
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Port(Out);
		Openings(Out);
		Anchors(Out);
		Enclosure(Out);
	}
}
