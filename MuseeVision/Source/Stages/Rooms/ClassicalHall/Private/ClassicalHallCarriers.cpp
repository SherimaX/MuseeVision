#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The carriers of the works in the round (§3.5, sized by the casts in §8): each stands with its top centre on its Mount
 * anchor, its front the way the work faces. Niche plinths on the sills and niche pedestals on the floor, running back
 * over the sill into the niche; the gallery's consoles on the panels' centre lines and the tribune's; the herms; the
 * axis and ambulatory plinths; and the centrepiece's turned pedestal.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		/** The carrier's frame on the floor under its top: x the way the work faces, y across, z up. */
		FMuseeFrame FloorFrame(const Plan::FCarrierPlan& Carrier)
		{
			const double Facing = Carrier.FacingDeg < 0.0 ? FMath::RadiansToDegrees(FMath::Atan2(-Carrier.Y, -Carrier.X)) : Carrier.FacingDeg;
			return Turned(FVector(Carrier.X, Carrier.Y, 0.0), Facing);
		}

		/** A moulding ring round a plan outline (drawn so it faces out), in a frame, its section's B from a height. */
		FFormDrawing Ring(const FString& Name, const TCHAR* LocalSurface, const FMuseeFrame& Frame, FFormProfile Section, FFormProfile Around, double From)
		{
			FFormSweep Shape;
			Shape.Section = MoveTemp(Section);
			Shape.Path = MoveTemp(Around);
			Shape.ElevationMeters = From;
			return Drawn(Name, LocalSurface, MoveTemp(Shape), Solid, Frame);
		}

		/** A die with the niche carriers' base and cap, in a frame, its top Width × Depth centred on the frame's origin. */
		void MouldedDie(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FMuseeFrame& Frame, double Width, double Depth,
			double Foot, double Top)
		{
			const double C = Plan::NicheCarrierCapProud;
			const double X = 0.5 * Depth - C;
			const double Y = 0.5 * Width - C;
			Out.Add(PrismIn(Name, LocalSurface, Frame, Rect(-X, -Y, X, Y), Foot - Plan::NicheCarrierBed, Top, Solid));
			Out.Add(Ring(Name + TEXT(".base"), LocalSurface, Frame, Written(Plan::NicheCarrierBase), RectOutward(-X, -Y, X, Y), Foot));
			Out.Add(Ring(Name + TEXT(".cap"), LocalSurface, Frame, Written(Plan::NicheCarrierCap), RectOutward(-X, -Y, X, Y), Top));
		}

		/**
		 * A niche pedestal: its die stands on the floor in front of the mouth and runs back over the sill into the niche,
		 * drawn in the niche's section (x into the wall, y up) and pushed across; its cap round the top. The wall's
		 * skirting and dado rail return round its foot.
		 */
		void NichePedestal(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier, const FNiche& Niche)
		{
			const double C = Plan::NicheCarrierCapProud;
			const double Axial = FVector::DotProduct(FVector(Carrier.X, Carrier.Y, 0.0) - Niche.Origin, Niche.In);
			const double Front = Axial - 0.5 * Carrier.Depth + C;
			const double Back = Axial + 0.5 * Carrier.Depth - C;
			const double Half = 0.5 * Carrier.Width - C;
			const FVector2D Section[] = {{Front, -Plan::Sink}, {0.0, -Plan::Sink}, {0.0, Plan::DadoTop}, {Back, Plan::DadoTop}, {Back, Carrier.Top}, {Front, Carrier.Top}};
			const FMuseeFrame Across = FrameOfAxes(Niche.Origin + Niche.Lateral() * Half, Niche.In, FVector::UpVector);
			Out.Add(Drawn(Name, TEXT("plinth"), Prism(FFormProfile::Polygon(Section), 2.0 * Half), Solid, Across));
			Out.Add(Ring(Name + TEXT(".cap"), TEXT("plinth"), Niche.PlanFrame(), Written(Plan::NicheCarrierCap), RectOutward(Front, -Half, Back, Half), Carrier.Top));
		}

		/**
		 * A console: the quadrant bracket's section, (−0.03, 1.14) (0.06, 1.14), the quarter ellipse about (0.06, 1.50) out to
		 * 0.03 short of its front, (front, 1.50) (front, 1.55) (−0.03, 1.55), pushed across its width from its back.
		 */
		void Console(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier)
		{
			const FMuseeFrame Floor = FloorFrame(Carrier);
			const FVector Facing = Floor.DirectionToParent(FVector::ForwardVector);
			const FVector Across = Floor.DirectionToParent(FVector::RightVector);
			const double Proud = Carrier.Depth;
			const FVector Back = Floor.OriginMeters - Facing * (0.5 * Proud) + Across * (0.5 * Carrier.Width);
			const FFormProfile Section = FPen(Plan::ConsoleBack, Plan::ConsoleFoot).To(Plan::ConsoleCurveA, Plan::ConsoleFoot)
				.Ellipse(FVector2D(Plan::ConsoleCurveA, Plan::ConsoleCurveB), Proud - Plan::ConsoleCurveA - Plan::ConsoleNib, Plan::ConsoleCurveHigh, -90.0, 0.0)
				.To(Proud, Plan::ConsoleCurveB).To(Proud, Plan::ConsoleTop).To(Plan::ConsoleBack, Plan::ConsoleTop).Closed();
			Out.Add(Drawn(Name, TEXT("console"), Prism(Section, Carrier.Width), Solid, FrameOfAxes(Back, Facing, FVector::UpVector)));
		}

		/**
		 * A herm: a moulded plinth 0.40 square × 0.14, the shaft and a moulded cap 0.09 from 1.16. The shaft is lofted as the
		 * mason works it, widening from 0.26 square at 0.13 to 0.32 at 1.17.
		 */
		void Herm(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier)
		{
			const FMuseeFrame Floor = FloorFrame(Carrier);
			MouldedDie(Out, Name + TEXT(".plinth"), TEXT("herm"), Floor, Carrier.Width, Carrier.Depth, 0.0, Plan::HermPlinthHigh);
			FFormLoft Shaft;
			for (const TPair<double, double>& Station : {TPair<double, double>(Plan::HermShaftFoot, Plan::HermShaftFootSide),
				TPair<double, double>(Plan::HermShaftTop, Plan::HermShaftTopSide)})
			{
				FFormLoftSection& Section = Shaft.Sections.AddDefaulted_GetRef();
				Section.Outline = Rect(-0.5 * Station.Value, -0.5 * Station.Value, 0.5 * Station.Value, 0.5 * Station.Value);
				Section.AtMeters = Station.Key;
			}
			Out.Add(Drawn(Name + TEXT(".shaft"), TEXT("herm"), MoveTemp(Shaft), Solid, Floor));
			MouldedDie(Out, Name + TEXT(".cap"), TEXT("herm"), Floor, Carrier.Width, Carrier.Depth, Plan::HermCapFoot, Carrier.Top);
		}

		/**
		 * An axis or ambulatory plinth: its die with the base (0.03, −0.01) (0.03, 0.06) cyma to (0, 0.10) convex first and the
		 * cap (0, H − 0.075) cyma to (0.03, H − 0.02) concave first (0.03, H), round it.
		 */
		void Plinth(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier)
		{
			const FMuseeFrame Floor = FloorFrame(Carrier);
			const double X = 0.5 * Carrier.Depth - Plan::PlinthCapProud;
			const double Y = 0.5 * Carrier.Width - Plan::PlinthCapProud;
			Out.Add(PrismIn(Name, TEXT("plinth"), Floor, Rect(-X, -Y, X, Y), -Plan::Sink, Carrier.Top, Solid));
			Out.Add(Ring(Name + TEXT(".base"), TEXT("plinth"), Floor, Written(Plan::PlinthBaseProfile), RectOutward(-X, -Y, X, Y), 0.0));
			Out.Add(Ring(Name + TEXT(".cap"), TEXT("plinth"), Floor, Written(Plan::PlinthCapProfile), RectOutward(-X, -Y, X, Y), Carrier.Top));
		}

		/** The centrepiece's pedestal, turned about O: Ø 2.20 × 0.30. */
		void Pedestal(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier)
		{
			FFormLathe Shape;
			Shape.Section = Written(Plan::CentrePedestal);
			Out.Add(Drawn(Name, TEXT("pedestal"), MoveTemp(Shape), Solid, FloorFrame(Carrier)));
		}
	}

	void DrawCarriers(TArray<FFormDrawing>& Out)
	{
		for (const Plan::FCarrierPlan& Carrier : Plan::Carriers)
		{
			const FString Name = FString(Carrier.Anchor) + TEXT(".carrier");
			switch (Carrier.Kind)
			{
			case Plan::ECarrier::NichePlinth:
				MouldedDie(Out, Name, TEXT("plinth"), FrameOfAxes(FVector(Carrier.X, Carrier.Y, 0.0), NicheOf(Carrier.Anchor)->In, NicheOf(Carrier.Anchor)->Lateral()),
					Carrier.Width, Carrier.Depth, Plan::DadoTop, Carrier.Top);
				break;
			case Plan::ECarrier::NichePedestal:
				NichePedestal(Out, Name, Carrier, *NicheOf(Carrier.Anchor));
				break;
			case Plan::ECarrier::Console:
				Console(Out, Name, Carrier);
				break;
			case Plan::ECarrier::Herm:
				Herm(Out, Name, Carrier);
				break;
			case Plan::ECarrier::Plinth:
				Plinth(Out, Name, Carrier);
				break;
			case Plan::ECarrier::Pedestal:
				Pedestal(Out, Name, Carrier);
				break;
			}
		}
	}
}
