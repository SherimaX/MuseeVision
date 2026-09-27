#include "SphereParts.h"
#include "SpherePlan.h"

/**
 * The platform at the centre (§3.5, as built ElanSphereTop.cpp:272-369): the steel deck, the Crema Marfil ring, the
 * bronze seat ring over the platform iris's pocket and the door sill, the platform iris (bodies
 * sphere.platform.iris.blade.1 to .7, drawn shut), the nosing, the balustrade of posts, curved panes and handrail with
 * the rail light's channel, and the eight struts that carry it all from the flange's rim.
 */
namespace Musee::Sphere
{
	namespace
	{
		/**
		 * The deck, a steel box bored to the seats' 96-gon: under the pocket it stops at the pocket's floor; out beyond the
		 * pocket its rim rises to the stone's bed. Its underside and outer face are the pearl soffit and fascia, and the
		 * rest is hidden: one extrusion's faces can't yet be finished apart, so the deck takes the soffit's finish.
		 */
		void Deck(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(TEXT("platform.deck"), TEXT("platform.soffit"), Slab(Circle(Plan::DeckOuter), {SeatPolygon()},
				Plan::PlatformPocketBottom - Plan::DeckSoffit), Solid, Raised(Plan::DeckSoffit)));
			Out.Add(Drawn(TEXT("platform.deck.rim"), TEXT("steel"), Ring(Plan::PocketOuter, Plan::DeckOuter, Plan::PlatformPocketBottom, Plan::DeckTop),
				Solid));
		}

		/** The stone ring: 24 slabs of 15° with 1 mm joints on bearings 7.5° + 15°k, from the pocket's wall to the deck's edge. */
		void Stone(TArray<FFormDrawing>& Out)
		{
			const double Slab15 = 360.0 / Plan::StoneSlabs;
			for (int32 Index = 0; Index < Plan::StoneSlabs; ++Index)
			{
				const double From = Plan::StoneFirstJoint + Slab15 * Index;
				Out.Add(Drawn(FString::Printf(TEXT("platform.floor.%02d"), Index + 1), TEXT("platform.floor"),
					Slab(Sector(Plan::PocketOuter, Plan::DeckOuter, From, From + Slab15, 0.5 * Plan::StoneJoint), {}, Plan::PlatformTop - Plan::DeckTop),
					Walkable, Raised(Plan::DeckTop)));
			}
		}

		/** An outline from the 96-gon between two bearings (rising) out to an arc of a radius and back. */
		FFormProfile FromSeatTo(double Radius, double FromBearing, double ToBearing)
		{
			const TArray<FVector2D> Inner = SeatPolygonBetween(FromBearing, ToBearing);
			const FVector2D ToOuter = AtBearing(Radius, ToBearing);
			const FVector2D FromOuter = AtBearing(Radius, FromBearing);
			FFormProfile Outline = FFormProfile::Polyline(Inner, false);
			Outline.LineTo(ToOuter, EFormJoint::Crisp);
			const double FromDeg = AngleOf(FromOuter);
			Outline.ArcAbout(FVector2D::ZeroVector, Radius, Radius, AngleOf(ToOuter, FromDeg), FromDeg, EFormJoint::Crisp);
			return Outline.Close();
		}

		/**
		 * The seat ring, the pocket's lid: 16 cast segments butted at bearings 11.25° + 22.5°k, from the seats' 96-gon to
		 * the stone; and the door sill across the car's door, cantilevered from it inside the 96-gon to r 2.245.
		 */
		void Seat(TArray<FFormDrawing>& Out)
		{
			const double Segment = 360.0 / Plan::SeatSegments;
			const double Depth = Plan::PlatformTop - Plan::PlatformPocketTop;
			for (int32 Index = 0; Index < Plan::SeatSegments; ++Index)
			{
				const double From = Plan::SeatFirstJoint + Segment * Index;
				Out.Add(Drawn(FString::Printf(TEXT("platform.seat.%02d"), Index + 1), TEXT("platform.seat"),
					Slab(FromSeatTo(Plan::PocketOuter, From, From + Segment), {}, Depth), Walkable, Raised(Plan::PlatformPocketTop)));
			}
			// The sill runs from the 96-gon in to its own arc, the other way round from the segments.
			const TArray<FVector2D> Seat = SeatPolygonBetween(Plan::SillFrom, Plan::SillTo);
			const FVector2D InnerFrom = AtBearing(Plan::SillInner, Plan::SillFrom);
			const FVector2D InnerTo = AtBearing(Plan::SillInner, Plan::SillTo);
			const double FromDeg = AngleOf(InnerFrom);
			FFormProfile Sill = FFormProfile::From(InnerFrom);
			Sill.ArcAbout(FVector2D::ZeroVector, Plan::SillInner, Plan::SillInner, FromDeg, AngleOf(InnerTo, FromDeg), EFormJoint::Crisp);
			for (int32 Index = Seat.Num() - 1; Index >= 0; --Index)
			{
				Sill.LineTo(Seat[Index], EFormJoint::Crisp);
			}
			Out.Add(Drawn(TEXT("platform.sill"), TEXT("platform.seat"), Slab(Sill.Close(), {}, Depth), Walkable, Raised(Plan::PlatformPocketTop)));
		}

		/**
		 * The platform iris, drawn shut under the seat ring, and the seven pins that tie the lid to the deck through the
		 * pocket. Shut it is a closure, not a floor: its blades are solid, never walked. A blade's pearl top and gilt
		 * underside are one extrusion's faces, which Form can't yet finish apart: it takes the top's.
		 */
		void Iris(TArray<FFormDrawing>& Out)
		{
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				FFormDrawing& Drawing = Out.Add_GetRef(Drawn(FString::Printf(TEXT("platform.iris.blade.%d"), Blade + 1), TEXT("platform.iris.top"),
					Slab(IrisBladeOpen(), {}, Plan::IrisBlade), Solid, IrisBladeShut(Blade)));
				Drawing.Body = IrisBladeBody(TEXT("platform"), Blade);
				const FVector Pin = IrisPinFrame(Blade, Plan::PlatformIrisTop).OriginMeters;
				Out.Add(Drawn(FString::Printf(TEXT("platform.iris.pin.%d"), Blade + 1), TEXT("platform.seat"),
					Rod(0.5 * Plan::PinDiameter, Plan::PlatformPocketBottom, Plan::PlatformPocketTop), Solid, FMuseeFrame(FVector(Pin.X, Pin.Y, 0.0))));
			}
		}

		/**
		 * The balustrade on r 5.320: the nosing over the deck's edge, the shoe on the stone, 32 posts on bearings
		 * 5.625° + 11.25°k, 32 curved panes between them (5 mm clear of each post), the handrail over all, and the rail
		 * light's channel inside the post line. The channel's 32 brackets under the rail are unsized in the sheet.
		 */
		void Balustrade(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(TEXT("platform.nosing"), TEXT("platform.gilt"), Ring(Plan::DeckOuter, Plan::NosingOuter, Plan::NosingBottom, Plan::NosingTop), Solid));
			Out.Add(Drawn(TEXT("platform.shoe"), TEXT("platform.gilt"), Ring(Plan::ShoeInner, Plan::ShoeOuter, Plan::PlatformTop, Plan::ShoeTop), Solid));
			const double Pitch = 360.0 / Plan::Posts;
			const double Half = 0.5 * Plan::PostSide;
			const FFormProfile PostPlan = Rectangle(FVector2D(Plan::BalustradeRadius - Half, -Half), FVector2D(Plan::BalustradeRadius + Half, Half));
			for (int32 Index = 0; Index < Plan::Posts; ++Index)
			{
				Out.Add(Drawn(FString::Printf(TEXT("platform.post.%02d"), Index + 1), TEXT("platform.gilt"), Slab(PostPlan, {}, Plan::PostTop - Plan::ShoeTop),
					Solid, OnBearing(Plan::PostFirstBearing + Pitch * Index, 0.0, Plan::ShoeTop)));
				const double Centre = Pitch * Index;
				FFormDrawing& Pane = Out.Add_GetRef(Drawn(FString::Printf(TEXT("platform.glass.%02d"), Index + 1), TEXT("platform.glass"),
					Ring(Plan::GlassInner, Plan::GlassOuter, Plan::ShoeTop, Plan::GlassTop, Centre - 0.5 * Plan::PaneDeg, Centre + 0.5 * Plan::PaneDeg), Solid));
				Pane.bNanite = false;
			}
			Out.Add(Drawn(TEXT("platform.handrail"), TEXT("platform.rail"),
				Turned(Circle(0.5 * Plan::HandrailDiameter, FVector2D(Plan::BalustradeRadius, Plan::HandrailZ))), Solid));
			Out.Add(Drawn(TEXT("platform.rail-light.channel"), TEXT("platform.gilt"),
				Ring(Plan::ChannelInner, Plan::ChannelOuter, Plan::ChannelBottom, Plan::ChannelTop), Solid));
		}

		/**
		 * The eight struts, tubes on the line from the flange's rim (r 4.300, z 11.400) to under the deck (r 5.300,
		 * z 20.170), between their end plates. A tube's ends are square to it and the plates are level, so each end meets
		 * its plate on a 6.5° slant, which Form can't yet cut: the tube runs to where its axis meets the plate.
		 */
		void Struts(TArray<FFormDrawing>& Out)
		{
			const double PlateRadius = 0.5 * Plan::StrutPlateDiameter;
			const double Outer = 0.5 * Plan::StrutOuterDiameter;
			for (int32 Index = 0; Index < Plan::Struts; ++Index)
			{
				const double Bearing = Plan::StrutFirstBearing + 360.0 * Index / Plan::Struts;
				const FVector2D FootAt = AtBearing(Plan::StrutFootR, Bearing);
				const FVector2D HeadAt = AtBearing(Plan::StrutHeadR, Bearing);
				const FVector Foot(FootAt.X, FootAt.Y, Plan::NeckTop);
				const FVector Head(HeadAt.X, HeadAt.Y, Plan::DeckSoffit);
				const FVector Along = (Head - Foot).GetSafeNormal();
				const FVector Start = Foot + Along * (Plan::StrutPlate / Along.Z);
				const FVector End = Head - Along * (Plan::StrutPlate / Along.Z);
				const FVector Side = Tangent(Bearing);
				const FString Name = FString::Printf(TEXT("platform.strut.%d"), Index + 1);
				Out.Add(Drawn(Name, TEXT("platform.gilt"), Ring(Outer - Plan::StrutWall, Outer, 0.0, FVector::Distance(Start, End)), Solid,
					FrameOfAxes(Start, Side, FVector::CrossProduct(Along, Side))));
				Out.Add(Drawn(Name + TEXT(".foot"), TEXT("platform.gilt"), Rod(PlateRadius, Plan::NeckTop, Plan::NeckTop + Plan::StrutPlate), Solid,
					FMuseeFrame(FVector(FootAt.X, FootAt.Y, 0.0))));
				Out.Add(Drawn(Name + TEXT(".head"), TEXT("platform.gilt"), Rod(PlateRadius, Plan::DeckSoffit - Plan::StrutPlate, Plan::DeckSoffit), Solid,
					FMuseeFrame(FVector(HeadAt.X, HeadAt.Y, 0.0))));
			}
		}
	}

	void DrawPlatform(TArray<FFormDrawing>& Out)
	{
		Deck(Out);
		Stone(Out);
		Seat(Out);
		Iris(Out);
		Balustrade(Out);
		Struts(Out);
	}
}
