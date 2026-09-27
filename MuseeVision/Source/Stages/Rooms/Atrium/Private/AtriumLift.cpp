#include "AtriumParts.h"

/**
 * The lift installation the Atrium serves both wells with (§3.6), everything but the car: the bronze lining of the
 * floor's opening, the two guide rails with their brackets, the glass landing with its leaves, shoe, head ring and sill,
 * the glass tube up to the collar with its ring frame, the collar hung from the compression ring, and the collar iris,
 * the closure of atrium.up, drawn shut.
 *
 * A turned solid is notched radially: the lining's slots and the shoe's, head ring's and ring frame's notches round the
 * rails are radial gaps clearing the rail's foot, the nearest honest form of the T-shaped notches the sheet draws. The
 * racks on the rails wait for polish (§12).
 */
namespace Musee::Atrium
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;

		/** The half-angle a rail's foot takes out of a ring about the axis whose inner face is at this radius. */
		double RailGap(double Radius)
		{
			return HalfAngle(Plan::FootSlotHalf, Radius);
		}

		/** A ring r0 → r1 by z0 → z1 round the axis, broken where the rails stand, from one bearing to another (clockwise). */
		void RingRoundRails(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, double R0, double R1, double Z0, double Z1,
			double From, double To, const Musee::Collision::FClass* Collision)
		{
			const double Gap = RailGap(R0);
			const double End = To <= From ? To + 360.0 : To;
			// A rail at either end starts or stops the ring a gap clear of it; one within it breaks it in two.
			TArray<double> Breaks = {From, End};
			for (const double Rail : {Plan::RailBearings[0], Plan::RailBearings[1], Plan::RailBearings[0] + 360.0, Plan::RailBearings[1] + 360.0})
			{
				if (FMath::IsNearlyEqual(Rail, From))
				{
					Breaks[0] = From + Gap;
				}
				else if (FMath::IsNearlyEqual(Rail, End))
				{
					Breaks[1] = End - Gap;
				}
				else if (Rail > From && Rail < End)
				{
					Breaks.Add(Rail - Gap);
					Breaks.Add(Rail + Gap);
				}
			}
			Breaks.Sort();
			for (int32 Index = 0; Index + 1 < Breaks.Num(); Index += 2)
			{
				Out.Add(Drawn(FString::Printf(TEXT("%s.%d"), *Name, Index / 2 + 1), LocalSurface, Band(R0, R1, Z0, Z1, Breaks[Index], Breaks[Index + 1]),
					Collision));
			}
		}

		/** The lining of the floor's opening, slotted where the rails' feet stand in it. */
		void Lining(TArray<FFormDrawing>& Out)
		{
			RingRoundRails(Out, TEXT("lining"), TEXT("lining"), Plan::LiningInner, Plan::LiningOuter, Plan::SlabBottom, Plan::SlabTop,
				Plan::RailBearings[0], Plan::RailBearings[0], Solid);
		}

		/** The two T-section guide rails in the well's notches, and the brackets that fix their feet to the rings. */
		void Rails(TArray<FFormDrawing>& Out)
		{
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::RailBearings)); ++Index)
			{
				const double Bearing = Plan::RailBearings[Index];
				auto At = [Bearing](double U, double T) { return U * Outward(Bearing) + T * Along(Bearing); };
				const double Blade = Plan::BladeSlotHalf;
				const double Foot = Plan::FootSlotHalf;
				const FFormProfile Tee = FFormProfile::Polyline({At(Plan::RailBladeIn, -Blade), At(Plan::RailFootIn, -Blade),
					At(Plan::RailFootIn, -Foot), At(Plan::RailFootOut, -Foot), At(Plan::RailFootOut, Foot), At(Plan::RailFootIn, Foot),
					At(Plan::RailFootIn, Blade), At(Plan::RailBladeIn, Blade)}, true);
				const TCHAR* Side = Bearing < 90.0 ? TEXT("north") : TEXT("south");
				Out.Add(Drawn(FString::Printf(TEXT("rail.%s"), Side), TEXT("rail"), Prism(Tee, Plan::RailBottom, Plan::RailTop), Solid,
					FMuseeFrame(FVector(0.0, 0.0, Plan::RailBottom))));
				for (int32 Level = 0; Level < int32(UE_ARRAY_COUNT(Plan::BracketLevels)); ++Level)
				{
					const double Z = Plan::BracketLevels[Level];
					Out.Add(Drawn(FString::Printf(TEXT("rail.%s.bracket.%d"), Side, Level + 1), TEXT("rail"), Prism(AtBearing(Bearing, Plan::BracketIn,
						Plan::BracketOut, -Plan::BracketHalfWidth, Plan::BracketHalfWidth), Z, Z + Plan::BracketHeight), Solid, FMuseeFrame(FVector(0.0, 0.0, Z))));
				}
			}
		}

		/** The landing's panes round the car's way, its bronze shoe and head ring, and the sill across its door. */
		void Landing(TArray<FFormDrawing>& Out)
		{
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::LandingPanes)); ++Index)
			{
				const Plan::FSpan& Span = Plan::LandingPanes[Index];
				AsGlass(Out.Add_GetRef(Drawn(FString::Printf(TEXT("%s%d"), Pieces::LandingGlass, Index + 1), TEXT("landing.glass"), Band(Plan::LandingGlassIn,
					Plan::LandingGlassOut, Plan::LandingGlassBottom, Plan::LandingGlassTop, FromDoor(Span.From), FromDoor(Span.To)), Solid)));
			}
			DrawLeafPair(Out, TEXT("landing"), Plan::LandingLeafIn, Plan::LandingLeafOut, Plan::LandingLeafBottom, Plan::LandingGlassTop,
				TEXT("landing.glass"), TEXT("landing.bronze"));
			RingRoundRails(Out, TEXT("landing.shoe"), TEXT("landing.bronze"), Plan::LandingRingIn, Plan::LandingRingOut, Plan::LandingShoeBottom,
				Plan::LandingShoeTop, FromDoor(Plan::LandingDoorHalf), FromDoor(-Plan::LandingDoorHalf), Walkable);
			RingRoundRails(Out, TEXT("landing.head-ring"), TEXT("landing.bronze"), Plan::LandingRingIn, Plan::LandingRingOut, Plan::HeadRingBottom,
				Plan::HeadRingTop, Plan::RailBearings[0], Plan::RailBearings[0], Solid);
			Out.Add(Drawn(Pieces::Sill, TEXT("landing.sill"), Band(Plan::SillIn, Plan::SillOut, Plan::MarbleBottom, Plan::SillTop,
				FromDoor(-Plan::SillHalf), FromDoor(Plan::SillHalf)), Walkable));
		}

		/** The tube: the landing's glass carried up in two tiers of six panes to the collar's hub, with its ring frame between them. */
		void Tube(TArray<FFormDrawing>& Out)
		{
			for (int32 Tier = 0; Tier < int32(UE_ARRAY_COUNT(Plan::TubeTiers)); ++Tier)
			{
				for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::TubePanes)); ++Index)
				{
					const Plan::FSpan& Span = Plan::TubePanes[Index];
					AsGlass(Out.Add_GetRef(Drawn(FString::Printf(TEXT("%st%d.%d"), Pieces::TubeGlass, Tier + 1, Index + 1), TEXT("landing.glass"),
						Band(Plan::LandingGlassIn, Plan::LandingGlassOut, Plan::TubeTiers[Tier][0], Plan::TubeTiers[Tier][1], FromDoor(Span.From),
						FromDoor(Span.To)), Solid)));
				}
			}
			RingRoundRails(Out, TEXT("tube.ring-frame"), TEXT("landing.bronze"), Plan::RingFrameIn, Plan::RingFrameOut, Plan::TubeTiers[0][1],
				Plan::TubeTiers[1][0], Plan::RailBearings[0], Plan::RailBearings[0], Solid);
		}

		/**
		 * One blade of the collar iris in its pin's frame: its open place is a 110° arc of the middle circle about the
		 * iris's axis (which lies 3.745 west of the pin), 1.250 wide with round ends, the pin beyond its near end.
		 */
		FFormProfile BladeOutline()
		{
			const FVector2D Axis(-Plan::IrisPin, 0.0);
			const double Half = 0.5 * Plan::IrisBladeWidth;
			const double Inner = Plan::IrisMiddle - Half;
			const double Outer = Plan::IrisMiddle + Half;
			const double Arc = Plan::IrisArcDeg;
			const FVector2D FarEnd = Axis + Plan::IrisMiddle * FVector2D(FMath::Cos(FMath::DegreesToRadians(Arc)), FMath::Sin(FMath::DegreesToRadians(Arc)));
			const FVector2D NearEnd = Axis + FVector2D(Plan::IrisMiddle, 0.0);
			FFormProfile Blade = FFormProfile::From(Axis + FVector2D(Inner, 0.0));
			Blade.ArcAbout(Axis, Inner, Inner, 0.0, Arc)
				.ArcAbout(FarEnd, Half, Half, Arc + 180.0, Arc)
				.ArcAbout(Axis, Outer, Outer, Arc, 0.0)
				.ArcAbout(NearEnd, Half, Half, 0.0, -180.0);
			return Blade.Close();
		}

		/**
		 * The collar with its coffers and the pocket its iris stows in; and the seven blades (bodies atrium.up.iris.blade.1
		 * to .7, each at its pin on its top face), drawn shut: each turned 52° on its pin from its open place. A blade's
		 * gilt underside and pearl top are one extrusion's two faces, which Form can't yet finish apart: it takes the top's.
		 */
		void Collar(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(Pieces::Collar, TEXT("collar"), TurnedAround(Section(Plan::Collar, true)), Solid));
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				const double Pin = 360.0 * Blade / Plan::IrisBlades;
				const FMuseeFrame Shut(FVector(0.0, 0.0, -Plan::IrisBlade), FQuat(FVector::UpVector, FMath::DegreesToRadians(Pin + Plan::IrisSwingDeg)));
				FFormDrawing& Drawing = Out.Add_GetRef(Drawn(FString::Printf(TEXT("collar.iris.blade.%d"), Blade + 1), TEXT("up.iris.top"),
					Prism(BladeOutline(), 0.0, Plan::IrisBlade), Solid, Shut));
				Drawing.Body = IrisBladeBody(Blade);
			}
		}
	}

	FMuseeKey IrisBladeBody(int32 Blade)
	{
		return Key(*FString::Printf(TEXT("up.iris.blade.%d"), Blade + 1));
	}

	FMuseeFrame IrisPinFrame(int32 Blade)
	{
		const double Pin = FMath::DegreesToRadians(360.0 * Blade / Plan::IrisBlades);
		return FMuseeFrame(FVector(Plan::IrisPin * FMath::Cos(Pin), Plan::IrisPin * FMath::Sin(Pin), Plan::IrisTop - Plan::IrisPitch * Blade));
	}

	void DrawLeafPair(TArray<FFormDrawing>& Out, const TCHAR* Owner, double R0, double R1, double Z0, double Z1, const TCHAR* GlassSurface,
		const TCHAR* FinSurface)
	{
		const double Middle = 0.5 * (R0 + R1);
		const double FinArc = FMath::RadiansToDegrees(Plan::FinWidth / Middle);
		for (const double Side : {1.0, -1.0})
		{
			const TCHAR* Name = Side > 0.0 ? TEXT("north") : TEXT("south");
			const FMuseeKey Body = Key(*FString::Printf(TEXT("%s.leaf.%s"), Owner, Name));
			const double Leading = FromDoor(Side * FinArc);
			const double Trailing = FromDoor(Side * Plan::LeafSpan);
			FFormDrawing& Glass = AsGlass(Out.Add_GetRef(Drawn(FString::Printf(TEXT("%s.leaf.%s"), Owner, Name), GlassSurface,
				Band(R0, R1, Z0, Z1, FMath::Min(Leading, Trailing), FMath::Max(Leading, Trailing)), Solid)));
			Glass.Body = Body;
			const FFormProfile Fin = AtBearing(Plan::DoorBearing, Middle - 0.5 * Plan::FinDepth, Middle + 0.5 * Plan::FinDepth,
				FMath::Min(0.0, Side * Plan::FinWidth), FMath::Max(0.0, Side * Plan::FinWidth));
			Out.Add_GetRef(Drawn(FString::Printf(TEXT("%s.leaf.%s.fin"), Owner, Name), FinSurface, Prism(Fin, Z0, Z1), Solid,
				FMuseeFrame(FVector(0.0, 0.0, Z0)))).Body = Body;
		}
	}

	void DrawLift(TArray<FFormDrawing>& Out)
	{
		Lining(Out);
		Rails(Out);
		Landing(Out);
		Tube(Out);
		Collar(Out);
	}
}
