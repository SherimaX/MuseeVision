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

		/** The niche carriers' base (0.015, −0.005) (0.015, 0.025) cyma to (0, 0.045) convex first, and their cap cyma to (0.02, H). */
		FFormProfile NicheBase()
		{
			return FPen(-Plan::Sink, -0.005).To(Plan::NicheCarrierBase, -0.005).To(Plan::NicheCarrierBase, 0.025).Cyma(0.0, Plan::NicheCarrierBaseTop, true)
				.To(-Plan::Sink, Plan::NicheCarrierBaseTop).Closed();
		}

		FFormProfile NicheCap()
		{
			const double C = Plan::NicheCarrierCap;
			return FPen(-Plan::Sink, 0.0).To(0.0, 0.0).Cyma(C, Plan::NicheCarrierCapHigh - 0.005, false).To(C, Plan::NicheCarrierCapHigh)
				.To(-Plan::Sink, Plan::NicheCarrierCapHigh).Closed();
		}

		/** A die with the niche carriers' base and cap, in a frame, its top Width × Depth centred on the frame's origin. */
		void MouldedDie(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FMuseeFrame& Frame, double Width, double Depth,
			double Foot, double Top)
		{
			const double C = Plan::NicheCarrierCap;
			const double X = 0.5 * Depth - C;
			const double Y = 0.5 * Width - C;
			Out.Add(PrismIn(Name, LocalSurface, Frame, Rect(-X, -Y, X, Y), Foot - 0.005, Top, Solid));
			Out.Add(Ring(Name + TEXT(".base"), LocalSurface, Frame, NicheBase(), RectOutward(-X, -Y, X, Y), Foot));
			Out.Add(Ring(Name + TEXT(".cap"), LocalSurface, Frame, NicheCap(), RectOutward(-X, -Y, X, Y), Top - Plan::NicheCarrierCapHigh));
		}

		/**
		 * A niche pedestal: its die stands on the floor in front of the mouth and runs back over the sill into the niche,
		 * drawn in the niche's section (x into the wall, y up) and pushed across; its cap round the top. The wall's
		 * skirting and dado rail return round its foot.
		 */
		void NichePedestal(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier, const FNiche& Niche)
		{
			const double C = Plan::NicheCarrierCap;
			const double Axial = FVector::DotProduct(FVector(Carrier.X, Carrier.Y, 0.0) - Niche.Origin, Niche.In);
			const double Front = Axial - 0.5 * Carrier.Depth + C;
			const double Back = Axial + 0.5 * Carrier.Depth - C;
			const double Half = 0.5 * Carrier.Width - C;
			const FVector2D Section[] = {{Front, -Plan::Sink}, {0.0, -Plan::Sink}, {0.0, Plan::DadoTop}, {Back, Plan::DadoTop}, {Back, Carrier.Top}, {Front, Carrier.Top}};
			const FMuseeFrame Across = FrameOfAxes(Niche.Origin + Niche.Lateral() * Half, Niche.In, FVector::UpVector);
			Out.Add(Drawn(Name, TEXT("plinth"), Prism(Polygon(Section), 2.0 * Half), Solid, Across));
			Out.Add(Ring(Name + TEXT(".cap"), TEXT("plinth"), Niche.PlanFrame(), NicheCap(), RectOutward(Front, -Half, Back, Half), Carrier.Top - Plan::NicheCarrierCapHigh));
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
			const double Fillet = 0.030;
			const FFormProfile Section = FPen(-0.03, Plan::ConsoleFoot).To(Plan::ConsoleCurveA, Plan::ConsoleFoot)
				.Ellipse(FVector2D(Plan::ConsoleCurveA, Plan::ConsoleCurveB), Proud - Plan::ConsoleCurveA - Fillet, Plan::ConsoleCurveHigh, -90.0, 0.0)
				.To(Proud, Plan::ConsoleCurveB).To(Proud, Plan::ConsoleTop).To(-0.03, Plan::ConsoleTop).Closed();
			Out.Add(Drawn(Name, TEXT("console"), Prism(Section, Carrier.Width), Solid, FrameOfAxes(Back, Facing, FVector::UpVector)));
		}

		/**
		 * A herm: a moulded plinth 0.40 square × 0.14, the shaft and a moulded cap 0.09 from 1.16. Form draws no prism that
		 * changes its section along its length, so the shaft, which widens from 0.26 at 0.13 to 0.32 at 1.17, stands square
		 * at its mean, 0.29.
		 */
		void Herm(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier)
		{
			const FMuseeFrame Floor = FloorFrame(Carrier);
			MouldedDie(Out, Name + TEXT(".plinth"), TEXT("herm"), Floor, Carrier.Width, Carrier.Depth, 0.0, Plan::HermPlinthHigh);
			const double Shaft = 0.25 * (Plan::HermShaftFootSide + Plan::HermShaftTopSide);
			Out.Add(PrismIn(Name + TEXT(".shaft"), TEXT("herm"), Floor, Rect(-Shaft, -Shaft, Shaft, Shaft), Plan::HermShaftFoot, Plan::HermShaftTop, Solid));
			MouldedDie(Out, Name + TEXT(".cap"), TEXT("herm"), Floor, Carrier.Width, Carrier.Depth, Plan::HermCapFoot, Carrier.Top);
		}

		/**
		 * An axis or ambulatory plinth: its die with the base (0.03, −0.01) (0.03, 0.06) cyma to (0, 0.10) convex first and the
		 * cap (0, H − 0.075) cyma to (0.03, H − 0.02) concave first (0.03, H).
		 */
		void Plinth(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier)
		{
			const FMuseeFrame Floor = FloorFrame(Carrier);
			const double C = Plan::PlinthCap;
			const double X = 0.5 * Carrier.Depth - C;
			const double Y = 0.5 * Carrier.Width - C;
			const double H = Carrier.Top;
			Out.Add(PrismIn(Name, TEXT("plinth"), Floor, Rect(-X, -Y, X, Y), -Plan::Sink, H, Solid));
			const FFormProfile Base = FPen(-Plan::Sink, -Plan::Sink).To(Plan::PlinthBase, -Plan::Sink).To(Plan::PlinthBase, Plan::PlinthBaseFillet)
				.Cyma(0.0, Plan::PlinthBaseTop, true).To(-Plan::Sink, Plan::PlinthBaseTop).Closed();
			Out.Add(Ring(Name + TEXT(".base"), TEXT("plinth"), Floor, Base, RectOutward(-X, -Y, X, Y), 0.0));
			const FFormProfile Cap = FPen(-Plan::Sink, H - Plan::PlinthCapHigh).To(0.0, H - Plan::PlinthCapHigh).Cyma(C, H - Plan::PlinthCapFillet, false).To(C, H)
				.To(-Plan::Sink, H).Closed();
			Out.Add(Ring(Name + TEXT(".cap"), TEXT("plinth"), Floor, Cap, RectOutward(-X, -Y, X, Y), 0.0));
		}

		/** The centrepiece's pedestal, turned about O: Ø 2.20 × 0.30. */
		void Pedestal(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FCarrierPlan& Carrier)
		{
			const double R = Plan::CentreRadius;
			FFormLathe Shape;
			Shape.Section = FPen(0.0, -Plan::Sink).To(R, -Plan::Sink).To(R, 0.03).Round(R - 0.02, 0.07, true).To(R - 0.05, 0.07).To(R - 0.05, 0.24)
				.Cyma(R, 0.285, true).To(R, Carrier.Top).To(0.0, Carrier.Top).Closed();
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
