#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The wall mouldings and panels (§3.4): the skirting and the dado rail swept round the whole perimeter, from one side
 * of the port's rough opening to the other, across the niches' mouths, back into each tribune niche's bay by its
 * sliver reveal and round every niche pedestal that stands on the floor; the framed panels (the gallery's sixteen at
 * each bay's centre ± 1.60, the tribune's ten flat at each niche ± 15°) and the lunette panel on the lintel.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		const FVector2D O = FVector2D::ZeroVector;

		FVector2D InPlan(const FVector& Point)
		{
			return FVector2D(Point.X, Point.Y);
		}

		/** A point in a niche's plan: Axial into the wall from its mouth plane, Lateral along it. */
		FVector2D AtNiche(const FNiche& Niche, double Axial, double Lateral)
		{
			return InPlan(Niche.Origin + Niche.In * Axial + Niche.Lateral() * Lateral);
		}

		/** The die of a pedestal standing on the floor in a niche's mouth: its front (axial, − into the room) and half-width. */
		struct FDie
		{
			double Front = 0.0;
			double Back = 0.0;
			double Half = 0.0;
		};

		TOptional<FDie> PedestalIn(const FNiche& Niche)
		{
			for (const Plan::FCarrierPlan& Carrier : Plan::Carriers)
			{
				if (Carrier.Kind == Plan::ECarrier::NichePedestal && Niche.AnchorLocal() == Carrier.Anchor)
				{
					const double Axial = FVector::DotProduct(FVector(Carrier.X, Carrier.Y, 0.0) - Niche.Origin, Niche.In);
					return FDie{Axial - 0.5 * Carrier.Depth + Plan::NicheCarrierCapProud, Axial + 0.5 * Carrier.Depth - Plan::NicheCarrierCapProud,
						0.5 * Carrier.Width - Plan::NicheCarrierCapProud};
				}
			}
			return {};
		}

		/** Round a niche pedestal's die, from the wall on its −lateral side to the wall on its +lateral side. */
		void RoundPedestal(FFormProfile& Path, const FNiche& Niche)
		{
			if (const TOptional<FDie> Die = PedestalIn(Niche))
			{
				for (const FVector2D& Corner : {AtNiche(Niche, 0.0, -Die->Half), AtNiche(Niche, Die->Front, -Die->Half), AtNiche(Niche, Die->Front, Die->Half),
					AtNiche(Niche, 0.0, Die->Half)})
				{
					Path.LineTo(Corner, EFormJoint::Crisp);
				}
			}
		}

		/** The plan angle of the point of the tribune's face at a lateral offset from a niche's axis. */
		double FaceDeg(const FNiche& Niche, double Lateral)
		{
			return FMath::RadiansToDegrees(FMath::Atan2(Niche.In.Y, Niche.In.X) + FMath::Asin(Lateral / Plan::TribuneRadius));
		}

		/** The niches of one gallery wall, in the order the perimeter walks past them. */
		TArray<const FNiche*> WallNiches(bool bEast)
		{
			TArray<const FNiche*> Out;
			for (const FNiche& Niche : Niches())
			{
				if (!Niche.bTribune && (Niche.In.X > 0.0) == bEast)
				{
					Out.Add(&Niche);
				}
			}
			// The west wall is walked north, the east wall south: both run from each niche's −lateral side to its +lateral side.
			Out.Sort([bEast](const FNiche& A, const FNiche& B) { return bEast ? A.Origin.Y < B.Origin.Y : A.Origin.Y > B.Origin.Y; });
			return Out;
		}

		/**
		 * The perimeter at the wall's face, with the room on its right: west along the south wall from the rough opening,
		 * up the west wall, round the tribune (into each niche's bay and out), down the east wall, and west to the opening.
		 */
		FFormProfile Perimeter()
		{
			const double Opening = Plan::PortClearHalf + Plan::LiningAllowance;
			const double Half = Plan::GalleryHalf;
			const double JambY = FMath::Sqrt(FMath::Square(Plan::TribuneRadius) - Half * Half);
			FFormProfile Path = FFormProfile::From(FVector2D(-Opening, Plan::SouthFaceY));
			Path.LineTo(FVector2D(-Half, Plan::SouthFaceY), EFormJoint::Crisp);
			for (const FNiche* Niche : WallNiches(false))
			{
				RoundPedestal(Path, *Niche);
			}
			Path.LineTo(FVector2D(-Half, JambY), EFormJoint::Crisp);
			double Deg = FMath::RadiansToDegrees(FMath::Acos(-Half / Plan::TribuneRadius));
			for (const double NicheDeg : {150.0, 210.0, 270.0, 330.0, 30.0})
			{
				const FNiche& Niche = *Niches().FindByPredicate([NicheDeg](const FNiche& N) { return N.bTribune && N.In.Equals(Direction(NicheDeg), 1e-9); });
				double Reach = FaceDeg(Niche, -Niche.BayHalf());
				while (Reach < Deg)
				{
					Reach += 360.0;
				}
				Path.ArcAbout(O, Plan::TribuneRadius, Plan::TribuneRadius, Deg, Reach, EFormJoint::Crisp);
				Path.LineTo(AtNiche(Niche, 0.0, -Niche.BayHalf()), EFormJoint::Crisp);
				RoundPedestal(Path, Niche);
				Path.LineTo(AtNiche(Niche, 0.0, Niche.BayHalf()), EFormJoint::Crisp);
				Deg = FaceDeg(Niche, Niche.BayHalf());
				while (Deg < Reach)
				{
					Deg += 360.0;
				}
				Path.LineTo(O + Polar(Plan::TribuneRadius, Deg), EFormJoint::Crisp);
			}
			Path.ArcAbout(O, Plan::TribuneRadius, Plan::TribuneRadius, Deg, 360.0 + FMath::RadiansToDegrees(FMath::Acos(Half / Plan::TribuneRadius)),
				EFormJoint::Crisp);
			for (const FNiche* Niche : WallNiches(true))
			{
				RoundPedestal(Path, *Niche);
			}
			Path.LineTo(FVector2D(Half, Plan::SouthFaceY), EFormJoint::Crisp);
			Path.LineTo(FVector2D(Opening, Plan::SouthFaceY), EFormJoint::Crisp);
			return Path;
		}

		/** The skirting 0.14 × 0.025 and the dado rail, its top 0.46 over the niches' sills, round the perimeter. */
		void SkirtingAndRail(TArray<FFormDrawing>& Out)
		{
			const FFormProfile Path = Perimeter();
			FFormSweep Skirting;
			Skirting.Section = Written(Plan::Skirting);
			Skirting.Path = Path;
			Out.Add(Drawn(TEXT("wall.skirting"), TEXT("moulding"), MoveTemp(Skirting), nullptr));
			FFormSweep Rail;
			Rail.Section = Written(Plan::DadoRail);
			Rail.Path = Path;
			Out.Add(Drawn(TEXT("wall.dado-rail"), TEXT("moulding"), MoveTemp(Rail), nullptr));
		}

		/**
		 * A framed panel on a wall, its frame standing at the face (x the viewer's right, y up, z out of the wall): the
		 * panel 0.02 proud, and round it the frame (−0.012, −0.01) (0.045, −0.01) (0.045, 0.012) cyma to (0.012, 0.035)
		 * concave first (−0.012, 0.035).
		 */
		void Panel(TArray<FFormDrawing>& Out, const FString& Name, const FVector& Face, const FVector& Outward, double Half, double Foot, double Top)
		{
			const FMuseeFrame Frame = FrameOfAxes(Face, FVector::CrossProduct(FVector::UpVector, Outward), FVector::UpVector);
			const FMuseeFrame Sunk(Frame.PointToParent(FVector(0.0, 0.0, -Plan::Sink)), Frame.Rotation);
			Out.Add(Drawn(Name, TEXT("panel"), Prism(Rect(-Half, Foot, Half, Top), Plan::PanelProud + Plan::Sink), nullptr, Sunk));
			FFormSweep Moulding;
			Moulding.Section = Written(Plan::PanelFrame);
			Moulding.Path = RectOutward(-Half, Foot, Half, Top);
			Out.Add(Drawn(Name + TEXT(".frame"), TEXT("moulding"), MoveTemp(Moulding), nullptr, Frame));
		}

		void Panels(TArray<FFormDrawing>& Out)
		{
			const double Half = 0.5 * Plan::PanelWidth;
			for (const double Side : {-1.0, 1.0})
			{
				for (int32 Bay = 0; Bay < Plan::Bays; ++Bay)
				{
					for (const double Along : {1.0, -1.0})
					{
						const FVector Face(Side * Plan::GalleryHalf, Plan::BayCentres[Bay] + Along * Plan::GalleryPanelOffset, 0.0);
						Panel(Out, FString::Printf(TEXT("gallery.panel.%s%d%s"), Side > 0.0 ? TEXT("e") : TEXT("w"), Bay + 1, Along > 0.0 ? TEXT("s") : TEXT("n")),
							Face, FVector(-Side, 0.0, 0.0), Half, Plan::PanelFoot, Plan::PanelTop);
					}
				}
			}
			for (const FNiche& Niche : Niches())
			{
				if (!Niche.bTribune)
				{
					continue;
				}
				const double Deg = FMath::RadiansToDegrees(FMath::Atan2(Niche.In.Y, Niche.In.X));
				for (const double Offset : {-Plan::TribunePanelOffsetDeg, Plan::TribunePanelOffsetDeg})
				{
					const FVector Radial = Direction(Deg + Offset);
					Panel(Out, FString::Printf(TEXT("tribune.panel.%s.%s"), *Niche.Name, Offset < 0.0 ? TEXT("a") : TEXT("b")), Radial * Plan::TribuneRadius, -Radial,
						Half, Plan::PanelFoot, Plan::PanelTop);
				}
			}
			Panel(Out, TEXT("lintel.lunette"), FVector(0.0, Plan::ScreenY, 0.0), FVector(0.0, 1.0, 0.0), 0.5 * Plan::LunetteWidth, Plan::LunetteFoot,
				Plan::LunetteTop);
		}
	}

	void DrawMouldings(TArray<FFormDrawing>& Out)
	{
		SkirtingAndRail(Out);
		Panels(Out);
	}
}
