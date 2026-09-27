#include "AlbionParts.h"
#include "SiteDescription.h"

/**
 * What the court declares to the site and the systems (§1, §2): its footprint, its one port on the Rotunda's south door,
 * the openings in its shell, the anchors the systems furnish, and its enclosure with the pieces that bound it.
 */
namespace Musee::Albion
{
	namespace
	{
		const FVector Vertical = FVector::UpVector;

		void Footprint(FSiteDescription& Out)
		{
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			for (const auto& Point : Plan::FootprintPoints)
			{
				Out.Footprint.OutlineMeters.Add(FVector2D(Point[0], Point[1]));
			}
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;
		}

		/**
		 * The one port: the porch's doorway on the flat port plane, tangent to the Rotunda's drum, facing north to meet
		 * rotunda.south. The court is a dead end by design (D-012): no other port, no well.
		 */
		void Ports(FSiteDescription& Out)
		{
			FSitePort& North = Out.Ports.AddDefaulted_GetRef();
			North.Key = Key(TEXT("north"));
			North.Kind = ESitePortKind::Doorway;
			North.Class = FMuseeKey(Plan::PortClass);
			// A port's x is its facing and its z is up, so its y is up × facing: east, for a port facing north (−y).
			North.Frame = FrameOfAxes(FVector(0.0, Plan::PortPlaneY, 0.0), FVector(0.0, -1.0, 0.0), FVector(1.0, 0.0, 0.0));
			North.WallDepthMeters = Plan::LiningMeters;
			North.FaceRadiusMeters = 0.0;
			North.Mates = FMuseeKey(TEXT("rotunda.south"));
		}

		/** An opening in a plane: centred on Centre, its x along XAxis, its z facing out (out of the court), sized. */
		void Opening(FSiteDescription& Out, const TCHAR* Local, EFormOpeningKind Kind, const FVector& Centre, const FVector& XAxis, const FVector2D& Size,
			const TCHAR* Glazing)
		{
			FFormOpening& Made = Out.Openings.AddDefaulted_GetRef();
			Made.Key = Key(Local);
			Made.Kind = Kind;
			Made.Enclosure = Key(TEXT("court"));
			Made.Frame = FrameOfAxes(Centre, XAxis, Vertical);
			Made.SizeMeters = Size;
			Made.Glazing = Glazing ? Surface(Glazing) : NAME_None;
		}

		/** A vault's glazing, declared in the plane of its springing: all its daylight crosses that plane. */
		void VaultOpening(FSiteDescription& Out, const TCHAR* Local, double X, double Span, const TCHAR* Glazing)
		{
			FFormOpening& Made = Out.Openings.AddDefaulted_GetRef();
			Made.Key = Key(Local);
			Made.Kind = EFormOpeningKind::Glazed;
			Made.Enclosure = Key(TEXT("court"));
			Made.Frame = FMuseeFrame(FVector(X, 0.0, Plan::Springing));
			Made.SizeMeters = FVector2D(Span, 2.0 * Plan::GlassEndY);
			Made.Glazing = Surface(Glazing);
		}

		/**
		 * Form's one list of openings (§1): the three vaults, the nave's screens, the aisles' gables, the band and the
		 * twelve lancets, every one Glazed; and the port's rough opening.
		 */
		void Openings(FSiteDescription& Out)
		{
			const FVector East(1.0, 0.0, 0.0);
			const FVector West(-1.0, 0.0, 0.0);
			const FVector South(0.0, 1.0, 0.0);
			const FVector North(0.0, -1.0, 0.0);
			VaultOpening(Out, TEXT("vault.nave"), 0.0, 2.0 * Plan::NaveHalfSpan, TEXT("vault.glass.nave"));
			VaultOpening(Out, TEXT("vault.west"), -Plan::AisleCrownX, 2.0 * Plan::AisleHalfSpan, TEXT("vault.glass.aisle"));
			VaultOpening(Out, TEXT("vault.east"), Plan::AisleCrownX, 2.0 * Plan::AisleHalfSpan, TEXT("vault.glass.aisle"));

			// The screens face out of the court: the north one north (its x east), the south one south (its x west).
			const double ScreenHigh = Plan::NaveCrown - Plan::ScreenCillTop;
			const double ScreenZ = 0.5 * (Plan::NaveCrown + Plan::ScreenCillTop);
			Opening(Out, TEXT("screen.north"), EFormOpeningKind::Glazed, FVector(0.0, -Plan::EndGlassY, ScreenZ), East,
				FVector2D(2.0 * Plan::ArcadeX, ScreenHigh), TEXT("screen.glass"));
			Opening(Out, TEXT("screen.south"), EFormOpeningKind::Glazed, FVector(0.0, Plan::EndGlassY, ScreenZ), West,
				FVector2D(2.0 * Plan::ArcadeX, ScreenHigh), TEXT("screen.glass"));
			const double GableFoot = Plan::Springing + Plan::GableCillHigh;
			const double GableZ = 0.5 * (Plan::AisleCrown + GableFoot);
			const FVector2D GableSize(2.0 * Plan::AisleHalfSpan, Plan::AisleCrown - GableFoot);
			Opening(Out, TEXT("gable.nw"), EFormOpeningKind::Glazed, FVector(-Plan::AisleCrownX, -Plan::EndGlassY, GableZ), East, GableSize, TEXT("screen.glass.gable"));
			Opening(Out, TEXT("gable.ne"), EFormOpeningKind::Glazed, FVector(Plan::AisleCrownX, -Plan::EndGlassY, GableZ), East, GableSize, TEXT("screen.glass.gable"));
			Opening(Out, TEXT("gable.sw"), EFormOpeningKind::Glazed, FVector(-Plan::AisleCrownX, Plan::EndGlassY, GableZ), West, GableSize, TEXT("screen.glass.gable"));
			Opening(Out, TEXT("gable.se"), EFormOpeningKind::Glazed, FVector(Plan::AisleCrownX, Plan::EndGlassY, GableZ), West, GableSize, TEXT("screen.glass.gable"));

			const double BandTop = Plan::BandUpperFoot + Plan::BandPanelHigh;
			Opening(Out, TEXT("band"), EFormOpeningKind::Glazed, FVector(0.0, Plan::EndGlassY, 0.5 * (Plan::BandLowerFoot + BandTop)), West,
				FVector2D(2.0 * Plan::BandEastX, BandTop - Plan::BandLowerFoot), TEXT("band.glass"));

			// The lancets on the glass line, the light's outline grown 0.01 (1.42 × 4.01).
			for (int32 Bay = 1; Bay <= Plan::Lines - 1; ++Bay)
			{
				const FArch Glazed = Lancet(Bay).Grown(Plan::LancetGlassGrow);
				const FVector2D Size(2.0 * Glazed.Half, Glazed.Crown() - Glazed.Sill);
				const double Z = 0.5 * (Glazed.Sill + Glazed.Crown());
				Opening(Out, *FString::Printf(TEXT("lancet.w%d"), Bay), EFormOpeningKind::Glazed, FVector(-Plan::SideGlassX, Glazed.Centre, Z), North, Size,
					TEXT("lancet.glass"));
				Opening(Out, *FString::Printf(TEXT("lancet.e%d"), Bay), EFormOpeningKind::Glazed, FVector(Plan::SideGlassX, Glazed.Centre, Z), South, Size,
					TEXT("lancet.glass"));
			}

			// The port's rough opening through the lining, facing north on the port plane.
			const FArch Rough = RoughOpening();
			Opening(Out, TEXT("north"), EFormOpeningKind::Port, FVector(0.0, Plan::PortPlaneY, 0.5 * (Rough.Sill + Rough.Crown())), East,
				FVector2D(2.0 * Rough.Half, Rough.Crown() - Rough.Sill), nullptr);
		}

		void Anchor(FSiteDescription& Out, const FString& Local, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key(Local), FName(Kind), Frame});
		}

		/**
		 * The anchors (§2). Walls: origin on the hanging face at the floor at the viewer's left, x to the viewer's right,
		 * y out of the face. The band's and the lancets' Glazing anchors: origin at the light's lower left seen from inside,
		 * y into the court. The cases' Mounts on their baize beds and the lectern's on its desk; the benches' eight seats.
		 */
		void Anchors(FSiteDescription& Out)
		{
			const FVector X(1.0, 0.0, 0.0);
			const FVector Y(0.0, 1.0, 0.0);
			const double SX = Plan::SideInnerX;
			const double EY = Plan::EndInnerY;
			Anchor(Out, TEXT("wall.west"), TEXT("Wall"), FrameOfAxes(FVector(-SX, EY, 0.0), -Y, X));
			Anchor(Out, TEXT("wall.east"), TEXT("Wall"), FrameOfAxes(FVector(SX, -EY, 0.0), Y, -X));
			Anchor(Out, TEXT("wall.south"), TEXT("Wall"), FrameOfAxes(FVector(SX, EY, 0.0), -X, -Y));
			Anchor(Out, TEXT("wall.north"), TEXT("Wall"), FrameOfAxes(FVector(-SX, -EY, 0.0), X, Y));

			for (const double SideX : {-1.0, 1.0})
			{
				for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::CaseY)); ++Index)
				{
					const FString Local = FString::Printf(TEXT("case.%s%d"), SideX < 0.0 ? TEXT("w") : TEXT("e"), Index == 0 ? 3 : 5);
					Anchor(Out, Local, TEXT("Mount"), FMuseeFrame(FVector(SideX * Plan::CaseX, Plan::CaseY[Index], Plan::BaizeTop)));
				}
			}
			// The lectern's desk: x up the slope, z its normal; the reader faces +x.
			const double Slope = FMath::DegreesToRadians(Plan::DeskSlopeDeg);
			const double DeskMiddle = Plan::DeskFarTop - 0.5 * Plan::DeskDepth * FMath::Sin(Slope);
			Anchor(Out, TEXT("lectern"), TEXT("Mount"), FrameOfAxes(FVector(Plan::LecternX, Plan::LecternY, DeskMiddle),
				FVector(FMath::Cos(Slope), 0.0, FMath::Sin(Slope)), Y));

			for (int32 Panel = 0; Panel < 2 * Plan::BandColumns; ++Panel)
			{
				const double East = Plan::BandEastX - Plan::BandPanelWidth * (Panel % Plan::BandColumns);
				const double Foot = Panel < Plan::BandColumns ? Plan::BandUpperFoot : Plan::BandLowerFoot;
				Anchor(Out, FString::Printf(TEXT("band.t%d"), Panel + 1), TEXT("Glazing"), FrameOfAxes(FVector(East, Plan::EndGlassY, Foot), -X, -Y));
			}
			for (int32 Bay = 1; Bay <= Plan::Lines - 1; ++Bay)
			{
				const double YB = Plan::BayY(Bay);
				Anchor(Out, FString::Printf(TEXT("lancet.w%d"), Bay), TEXT("Glazing"),
					FrameOfAxes(FVector(-Plan::SideGlassX, YB + Plan::LancetHalf, Plan::LancetAnchorZ), -Y, X));
				Anchor(Out, FString::Printf(TEXT("lancet.e%d"), Bay), TEXT("Glazing"),
					FrameOfAxes(FVector(Plan::SideGlassX, YB - Plan::LancetHalf, Plan::LancetAnchorZ), Y, -X));
			}

			// The seats: on each bench, the outer two face the aisle and the inner two the nave, as the sheet's table sets them.
			for (const bool bWest : {true, false})
			{
				const double S = bWest ? -1.0 : 1.0;
				const TCHAR* Bench = bWest ? TEXT("west") : TEXT("east");
				int32 Seat = 0;
				for (const bool bOuter : {true, false})
				{
					const double SeatX = S * (bOuter ? Plan::SeatOuter : Plan::SeatInner);
					const FVector Facing = (bOuter ? S : -S) * X;
					for (const double SeatY : Plan::SeatY)
					{
						Anchor(Out, FString::Printf(TEXT("bench.%s.seat.%d"), Bench, ++Seat), TEXT("Seat"),
							FMuseeFrame(FVector(SeatX, SeatY, Plan::BenchTop), FRotationMatrix::MakeFromX(Facing).ToQuat()));
					}
				}
			}
		}

		/** Whether a piece bounds the court (with its porch): the court's leaves, floors, the porch's shell, the glass. */
		bool BoundsCourt(const FString& Name)
		{
			auto Starts = [&Name](const TCHAR* Prefix) { return Name.StartsWith(Prefix, ESearchCase::CaseSensitive); };
			auto Has = [&Name](const TCHAR* Part) { return Name.Contains(Part, ESearchCase::CaseSensitive); };
			const bool bLeaf = (Starts(TEXT("wall.")) || Starts(TEXT("porch.wall."))) && Has(TEXT(".inner."));
			const bool bPorchFace = Starts(TEXT("wall.north.outer."));
			const bool bCill = Starts(TEXT("wall.")) && Name.EndsWith(TEXT(".cill"), ESearchCase::CaseSensitive);
			const bool bPorch = Starts(TEXT("porch.lining.")) || Name == TEXT("porch.ceiling.boards");
			const bool bFloor = Starts(TEXT("floor.")) || Starts(TEXT("girder."));
			const bool bGlass = Starts(TEXT("vault.glass.")) || ((Starts(TEXT("screen.")) || Starts(TEXT("gable."))) && Has(TEXT(".glass.")))
				|| Starts(TEXT("screen.south.strip.")) || ((Starts(TEXT("band.")) || Starts(TEXT("lancet."))) && Name.EndsWith(TEXT(".glass"), ESearchCase::CaseSensitive));
			return bLeaf || bPorchFace || bCill || bPorch || bFloor || bGlass;
		}

		/**
		 * The enclosure: the court and its porch as one space, since the court door is an opening the two would share and
		 * an opening names one enclosure (the sheet's fallback, §1). Bounded by the court's leaves and floors, the porch's
		 * leaves, lining and ceiling, the girders and every glass.
		 */
		void Enclosures(FSiteDescription& Out)
		{
			FFormEnclosure& Court = Out.Enclosures.AddDefaulted_GetRef();
			Court.Key = Key(TEXT("court"));
			Court.Kind = EFormEnclosureKind::Enclosed;
			for (const FFormDrawing& Piece : Out.Pieces)
			{
				if (BoundsCourt(Piece.Name.ToString()))
				{
					Court.Pieces.Add(Piece.Name);
				}
			}
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Ports(Out);
		Openings(Out);
		Anchors(Out);
		Enclosures(Out);
	}
}
