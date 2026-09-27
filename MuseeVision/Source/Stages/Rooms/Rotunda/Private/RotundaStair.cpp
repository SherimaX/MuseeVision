#include "RotundaParts.h"
#include "RotundaPlan.h"
#include "RotundaStairLine.h"

/**
 * The stair (§3.8), after Momo and Bramante: one wide helical flight round an open well Ø 4.40, descending anticlockwise
 * in plan (φ falling) from the raised drum's west door, by the south and east, to the landing, whose portal opens north.
 * Forty risers of 0.150: the top landing, eighteen treads, the mid landing, nineteen treads, and the last riser onto the
 * landing floor.
 *
 * Each tread is one stone, a spandrel step as a stone stair's are cut: its top level, its sides on its nosing line and on
 * the one above, and its underside dressed to the soffit, the smooth helicoid 0.40 under the pitch line (RotundaStairLine),
 * so the stones' undersides together make one soffit. Its nosing is swept along its riser. The inner string, r 2.20 to
 * 2.35, rises with the pitch line, its top 0.12 over it and its underside on the soffit. Treads and string are solids
 * between their tops and the soffit over their plans (Form's heightfield), their undersides finished as the soffit.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** A solid over a plan outline from the soffit up to its top, its underside finished as the soffit. */
		FFormDrawing Spandrel(const FString& Name, const TCHAR* LocalSurface, FFormProfile Outline, TFunction<double(double, double)> Top,
			double NearDeg, const Musee::Collision::FClass* Collision)
		{
			FFormHeightfield Shape;
			Shape.Outline = MoveTemp(Outline);
			Shape.TopMeters = MoveTemp(Top);
			Shape.BottomMeters = [NearDeg](double X, double Y) { return Stair::Soffit(Stair::AngleOf(X, Y, NearDeg)); };
			Shape.GridMeters = Plan::StairGrid;
			FFormDrawing Drawing = Drawn(Name, LocalSurface, MoveTemp(Shape), Collision);
			Drawing.PartSurfaces.Add(FFormHeightfield::BottomPart, Surface(TEXT("stair.soffit")));
			return Drawing;
		}

		void Tread(TArray<FFormDrawing>& Out, int32 K)
		{
			const double From = Stair::Front(K);
			const double To = Stair::Front(K - 1);
			const double Top = Stair::TreadTop(K);
			Out.Add(Spandrel(FString::Printf(TEXT("stair.tread.%02d"), K), TEXT("stair.tread"), AnnularSector(Plan::TreadInner - Plan::TreadInto,
				Plan::TreadOuter + Plan::TreadInto, From, To), [Top](double, double) { return Top; }, 0.5 * (From + To), Walkable));

			// The nosing over the riser below, swept inwards along the riser line so its section faces forward, down the flight.
			FFormSweep Nosing;
			Nosing.Section = Profile(Plan::Nosing, true);
			Nosing.Path = FFormProfile::Polyline({Polar(Plan::TreadOuter, From), Polar(Plan::TreadInner, From)}, false);
			Nosing.ElevationMeters = Top;
			Out.Add(Drawn(FString::Printf(TEXT("stair.tread.%02d.nosing"), K), TEXT("stair.tread"), MoveTemp(Nosing), nullptr));
		}

		/** The inner string, from the top landing's back edge to half a degree past the last riser. */
		void InnerString(TArray<FFormDrawing>& Out)
		{
			const double From = Stair::Front(Plan::Risers - 1) - Plan::StringPastDeg;
			const double To = Plan::TopLandingBackDeg;
			const double Near = 0.5 * (From + To);
			Out.Add(Spandrel(TEXT("stair.string"), TEXT("stair.string"), AnnularSector(Plan::StringInner, Plan::TreadInner, From, To),
				[Near](double X, double Y) { return Stair::StringTop(Stair::AngleOf(X, Y, Near)); }, Near, Solid));
		}
	}

	void DrawStair(TArray<FFormDrawing>& Out)
	{
		for (int32 K = 1; K < Plan::Risers; ++K)
		{
			Tread(Out, K);
		}
		InnerString(Out);
		DrawStairFittings(Out);
	}
}
