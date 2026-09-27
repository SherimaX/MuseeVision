#include "SphereParts.h"
#include "SpherePlan.h"

#include "Algo/Find.h"

/**
 * The neck, the airlock (§3.4, as built ElanSphereTop.cpp:191-256): the pearl throat standing on the Atrium's collar,
 * its gilt trellis of courses and ribs, the bronze flange at the mouth with the neck iris's pocket and pins, the neck
 * iris (bodies sphere.neck.iris.blade.1 to .7, drawn shut), and the two hard-chromed rails with their brackets.
 */
namespace Musee::Sphere
{
	namespace
	{
		/** The outer half-width of a dog's housing: its inside and one plate. */
		double HousingHalf()
		{
			return 0.5 * Plan::HousingInside + Plan::HousingPlate;
		}

		/**
		 * The throat: whole from the plane up to the dogs' housings, then four bands between them up to the mouth, each
		 * band's ends parallel to the housing's sides, which pass through the wall.
		 */
		void Throat(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(Pieces::Throat, TEXT("throat"), Ring(Plan::ThroatBore, Plan::ThroatOuter, Plan::PortZ, Plan::HousingBottom), Solid));
			const int32 Dogs = UE_ARRAY_COUNT(Plan::DogBearings);
			for (int32 Dog = 0; Dog < Dogs; ++Dog)
			{
				const double From = Plan::DogBearings[Dog];
				const double To = Plan::DogBearings[(Dog + 1) % Dogs] + (Dog + 1 == Dogs ? 360.0 : 0.0);
				Out.Add(Drawn(Pieces::ThroatBand(Dog), TEXT("throat"), Slab(Sector(Plan::ThroatBore, Plan::ThroatOuter, From, To, HousingHalf()), {},
					Plan::Mouth - Plan::HousingBottom), Solid, Raised(Plan::HousingBottom)));
			}
		}

		/** The ribs' bearings, 11.25° + 22.5°k. */
		double RibBearing(int32 Rib)
		{
			return Plan::RibFirstBearing + 360.0 * Rib / Plan::Ribs;
		}

		/**
		 * The trellis on the bore: sixteen ribs, broken 10 mm short of the light channel, and six courses running between
		 * the ribs and stopped 10 mm short of the rail brackets. The ribs stand proud of the courses and run through.
		 */
		void Trellis(TArray<FFormDrawing>& Out)
		{
			const double RibHalf = 0.5 * Plan::RibWidth;
			const FFormProfile RibPlan = Rectangle(FVector2D(Plan::RibInner, -RibHalf), FVector2D(Plan::ThroatBore, RibHalf));
			for (int32 Rib = 0; Rib < Plan::Ribs; ++Rib)
			{
				const double Bearing = RibBearing(Rib);
				Out.Add(Drawn(FString::Printf(TEXT("neck.rib.%02d.lower"), Rib + 1), TEXT("throat.gilt"), Slab(RibPlan, {}, Plan::RibBreakBottom - Plan::PortZ),
					Solid, OnBearing(Bearing, 0.0, Plan::PortZ)));
				Out.Add(Drawn(FString::Printf(TEXT("neck.rib.%02d.upper"), Rib + 1), TEXT("throat.gilt"), Slab(RibPlan, {}, Plan::Mouth - Plan::RibBreakTop),
					Solid, OnBearing(Bearing, 0.0, Plan::RibBreakTop)));
			}

			// A course's stretches: from rib to rib, and where a rail stands between two ribs, from the rib to the bracket.
			struct FStretch
			{
				double From;
				double To;
				double FromOffset;
				double ToOffset;
			};
			TArray<FStretch> Stretches;
			const double BracketStop = Plan::BracketHalf + Plan::StopClear;
			for (int32 Rib = 0; Rib < Plan::Ribs; ++Rib)
			{
				const double From = RibBearing(Rib);
				const double To = RibBearing(Rib + 1);
				const double* Rail = Algo::FindByPredicate(Plan::RailBearings, [&](double R) { return (R > From && R < To) || (R + 360.0 > From && R + 360.0 < To); });
				if (Rail)
				{
					const double At = *Rail > From ? *Rail : *Rail + 360.0;
					Stretches.Add({From, At, RibHalf, BracketStop});
					Stretches.Add({At, To, BracketStop, RibHalf});
				}
				else
				{
					Stretches.Add({From, To, RibHalf, RibHalf});
				}
			}
			for (int32 Course = 0; Course < int32(UE_ARRAY_COUNT(Plan::CourseCentres)); ++Course)
			{
				const double Bottom = Plan::CourseCentres[Course] - 0.5 * Plan::CourseHeight;
				for (int32 Index = 0; Index < Stretches.Num(); ++Index)
				{
					const FStretch& S = Stretches[Index];
					Out.Add(Drawn(FString::Printf(TEXT("neck.course.%d.%02d"), Course + 1, Index + 1), TEXT("throat.gilt"),
						Slab(Sector(Plan::CourseInner, Plan::ThroatBore, S.From, S.To, S.FromOffset, S.ToOffset), {}, Plan::CourseHeight), Solid,
						Raised(Bottom)));
				}
			}
		}

		/**
		 * The flange at the mouth, the neck iris's seat: its body under the pocket, the pocket's outer wall, the 3 mm top
		 * plate over it, and the seven pins through the pocket, all bored to the seats' 96-gon.
		 */
		void Flange(TArray<FFormDrawing>& Out)
		{
			const FFormProfile Outer = Circle(Plan::FlangeOuter);
			Out.Add(Drawn(Pieces::FlangeBase, TEXT("neck.seat"), Slab(Outer, {SeatPolygon()}, Plan::NeckPocketBottom - Plan::Mouth), Solid,
				Raised(Plan::Mouth)));
			Out.Add(Drawn(Pieces::FlangeWall, TEXT("neck.seat"), Ring(Plan::PocketOuter, Plan::FlangeOuter, Plan::NeckPocketBottom, Plan::NeckPocketTop),
				Solid));
			Out.Add(Drawn(Pieces::FlangePlate, TEXT("neck.seat"), Slab(Outer, {SeatPolygon()}, Plan::NeckTop - Plan::NeckPocketTop), Solid,
				Raised(Plan::NeckPocketTop)));
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				const FVector Pin = IrisPinFrame(Blade, Plan::NeckIrisTop).OriginMeters;
				Out.Add(Drawn(FString::Printf(TEXT("neck.iris.pin.%d"), Blade + 1), TEXT("neck.seat"),
					Rod(0.5 * Plan::PinDiameter, Plan::NeckPocketBottom, Plan::NeckPocketTop), Solid, FMuseeFrame(FVector(Pin.X, Pin.Y, 0.0))));
			}
		}

		/**
		 * The neck iris, drawn shut. A blade's gilt top and pearl underside are one extrusion's faces, which Form can't yet
		 * finish apart: it takes the top's, the face the Sphere sees.
		 */
		void Iris(TArray<FFormDrawing>& Out)
		{
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				FFormDrawing& Drawing = Out.Add_GetRef(Drawn(Pieces::NeckBlade(Blade), TEXT("neck.iris.top"), Slab(IrisBladeOpen(), {}, Plan::IrisBlade),
					Solid, IrisBladeShut(Blade)));
				Drawing.Body = IrisBladeBody(TEXT("neck"), Blade);
			}
		}

		/**
		 * The rails north and south, in the Atrium's T-section (the blade in the well's notch, the foot behind it), and
		 * their brackets: four to the throat's bore and one to the flange's.
		 */
		void Rails(TArray<FFormDrawing>& Out)
		{
			const double Blade = Plan::RailBladeHalf;
			const double Foot = Plan::RailFootHalf;
			const FFormProfile Section = FFormProfile::Polyline({FVector2D(Plan::RailBladeIn, -Blade), FVector2D(Plan::RailFootIn, -Blade),
				FVector2D(Plan::RailFootIn, -Foot), FVector2D(Plan::RailFootOut, -Foot), FVector2D(Plan::RailFootOut, Foot), FVector2D(Plan::RailFootIn, Foot),
				FVector2D(Plan::RailFootIn, Blade), FVector2D(Plan::RailBladeIn, Blade)}, true);
			for (const double Bearing : Plan::RailBearings)
			{
				const TCHAR* Side = Bearing < 90.0 ? TEXT("north") : TEXT("south");
				Out.Add(Drawn(FString::Printf(TEXT("neck.rail.%s"), Side), TEXT("rail"), Slab(Section, {}, Plan::RailTop - Plan::RailBottom), Solid,
					OnBearing(Bearing, 0.0, Plan::RailBottom)));
				int32 Index = 0;
				auto Bracket = [&](double Centre, double Reach)
				{
					Out.Add(Drawn(FString::Printf(TEXT("neck.rail.%s.bracket.%d"), Side, ++Index), TEXT("rail"),
						Slab(Rectangle(FVector2D(Plan::RailFootOut, -Plan::BracketHalf), FVector2D(Reach, Plan::BracketHalf)), {}, Plan::BracketHeight), Solid,
						OnBearing(Bearing, 0.0, Centre - 0.5 * Plan::BracketHeight)));
				};
				for (const double Centre : Plan::ThroatBracketCentres)
				{
					Bracket(Centre, Plan::ThroatBore);
				}
				Bracket(Plan::FlangeBracketCentre, SeatInscribed());
			}
		}
	}

	void DrawNeck(TArray<FFormDrawing>& Out)
	{
		Throat(Out);
		Trellis(Out);
		Flange(Out);
		Iris(Out);
		Rails(Out);
	}
}
