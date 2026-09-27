#include "RotundaParts.h"
#include "RotundaPlan.h"
#include "TimeMuseumDial.h"

/**
 * The dial's inlay (§3.1, §5): the hour ring, the hour lines, the gilt meridian and the minute ticks, laid where Time's
 * dial puts the bronze node's point of shadow, so the shadow the museum's sun casts lands on them (nothing is typed by
 * hand). Each is cast bronze proud of the face with a bevelled top edge, riding the lifting drum. The numerals wait for
 * Form's lettering and the ported glyphs.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** A bar's section across it: its width on the face, its crest proud of it, its top edges bevelled. */
		FFormProfile BarSection(double Width, double Proud, double Bevel)
		{
			const double Half = 0.5 * Width;
			return FFormProfile::Polyline({FVector2D(-Half, 0.0), FVector2D(Half, 0.0), FVector2D(Half, Proud - Bevel), FVector2D(Half - Bevel, Proud),
				FVector2D(-Half + Bevel, Proud), FVector2D(-Half, Proud - Bevel)}, true);
		}

		/** A radial bar on the face at a mark, from an inner radius to an outer one. */
		void Bar(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* Surface, const Time::FDialMark& Mark, double Inner, double Outer,
			double Width, double Proud, double Bevel)
		{
			const FVector Radial(Mark.Direction, 0.0);
			const FVector Tangent = FVector::CrossProduct(FVector::UpVector, Radial);
			FFormExtrusion Shape = Slab(BarSection(Width, Proud, Bevel), {}, Outer - Inner);
			// The section lies in (tangent, up) and is pushed out along the radius (the frame's z, their cross product).
			OnDrum(Out, Name, Surface, MoveTemp(Shape), nullptr, FrameOfAxes(Radial * Inner, Tangent, FVector::UpVector));
		}

		/** The tick a minute takes: the longest whose interval divides it. */
		const Plan::FTick& TickOf(int32 MinuteOfHour)
		{
			for (const Plan::FTick& Tick : Plan::Ticks)
			{
				if (MinuteOfHour % Tick.EveryMinutes == 0)
				{
					return Tick;
				}
			}
			return Plan::Ticks[UE_ARRAY_COUNT(Plan::Ticks) - 1];
		}

		void HourRing(TArray<FFormDrawing>& Out, double Radius)
		{
			const double Half = 0.5 * Plan::HourRingWidth;
			const double Crest = Plan::HourRingProud;
			const double Bevel = Plan::HourRingBevel;
			FFormLathe Ring;
			Ring.Section = FFormProfile::Polyline({FVector2D(Radius - Half, -Plan::InlayGroove), FVector2D(Radius + Half, -Plan::InlayGroove),
				FVector2D(Radius + Half, Crest - Bevel), FVector2D(Radius + Half - Bevel, Crest), FVector2D(Radius - Half + Bevel, Crest),
				FVector2D(Radius - Half, Crest - Bevel)}, true);
			OnDrum(Out, TEXT("dial.hour-ring"), TEXT("dial.inlay"), MoveTemp(Ring), nullptr);
		}
	}

	void DrawDialInlay(TArray<FFormDrawing>& Out, const Time::FMuseumDial& Dial)
	{
		const double RingInner = Dial.ShadowRadiusMeters() - 0.5 * Plan::HourRingWidth;
		HourRing(Out, Dial.ShadowRadiusMeters());
		for (const Time::FDialMark& Mark : Dial.Marks(1))
		{
			const int32 Hour = Mark.Minute / 60;
			if (Mark.IsHour() && Hour == Plan::MeridianHour)
			{
				Bar(Out, TEXT("dial.meridian"), TEXT("dial.meridian"), Mark, Plan::HourLineInner, RingInner, Plan::MeridianWidth,
					Plan::HourLineProud, Plan::HourLineBevel);
			}
			else if (Mark.IsHour())
			{
				Bar(Out, FString::Printf(TEXT("dial.hour-line.%02d"), Hour), TEXT("dial.inlay"), Mark, Plan::HourLineInner, RingInner,
					Plan::HourLineWidth, Plan::HourLineProud, Plan::HourLineBevel);
			}
			else
			{
				const Plan::FTick& Tick = TickOf(Mark.MinuteOfHour());
				Bar(Out, FString::Printf(TEXT("dial.tick.%04d"), Mark.Minute), TEXT("dial.inlay"), Mark, Tick.Inner, RingInner, Tick.Width,
					Tick.Proud, Tick.Bevel);
			}
		}
	}
}
