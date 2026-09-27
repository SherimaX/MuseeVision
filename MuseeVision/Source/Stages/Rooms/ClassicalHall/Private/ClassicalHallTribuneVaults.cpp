#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The tribune's vaults (§3.3), all turned about O: the annular vault over the ambulatory, a circle R 1.5009 about
 * (r 5.10, z 3.6491) springing from the outer wall's ledge and the ring beam's, with 420 coffers; the dome over the ring,
 * a spherical cap R 4.3377 about (0, 0, 0.7716) from the ring beam's inner ledge to the eye, with 72; the eye's laylight
 * and its gilt bead; and over them the vaults' masonry and the roof slab, one travertine body.
 *
 * A coffered vault of revolution is drawn from turned pieces that meet face to face: the plaster turned with the
 * coffers' rows as stepped channels round the vault; in each channel the ribs between the coffers, turned through their
 * width at each step (so, as in the Pantheon's dome, they narrow towards the axis); and on each back a cast rosette.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		const FVector2D Annular(Plan::AnnularCentreR, Plan::AnnularCentreZ);
		const FVector2D Sphere(0.0, Plan::DomeCentreZ);

		/** The ribs' fronts stand 1 mm behind the plaster's steps they cross, so no two faces lie in one surface. */
		constexpr double RibClearance = 0.001;

		double Deg(double Radians)
		{
			return FMath::RadiansToDegrees(Radians);
		}

		/** The angle of an (r, z) point about a section's centre. */
		double AngleOf(const FVector2D& Centre, double R, double Z)
		{
			return Deg(FMath::Atan2(Z - Centre.Y, R - Centre.X));
		}

		/** A coffer's steps: its rib on the face, then each riser into the vault and each setback, as angles on the face. */
		struct FSteps
		{
			double RibDeg;
			double Riser;
			double SetbackDeg;

			double Depth() const { return Plan::CofferSteps * Riser; }
		};

		/** A section's coffered row from one angle to another along the face: rib, steps down, back, steps up, rib. */
		void CofferRow(FPen& Pen, const FVector2D& C, double R, double From, double To, const FSteps& Steps)
		{
			double A = From + Steps.RibDeg;
			Pen.Arc(C, R, From, A);
			for (int32 Step = 1; Step <= Plan::CofferSteps; ++Step)
			{
				const FVector2D Down = C + Polar(R + Step * Steps.Riser, A);
				Pen.To(Down.X, Down.Y);
				if (Step < Plan::CofferSteps)
				{
					Pen.Arc(C, R + Step * Steps.Riser, A, A + Steps.SetbackDeg);
					A += Steps.SetbackDeg;
				}
			}
			double B = To - Steps.RibDeg - (Plan::CofferSteps - 1) * Steps.SetbackDeg;
			Pen.Arc(C, R + Steps.Depth(), A, B);
			for (int32 Step = Plan::CofferSteps - 1; Step >= 0; --Step)
			{
				const FVector2D Up = C + Polar(R + Step * Steps.Riser, B);
				Pen.To(Up.X, Up.Y);
				if (Step > 0)
				{
					Pen.Arc(C, R + Step * Steps.Riser, B, B + Steps.SetbackDeg);
					B += Steps.SetbackDeg;
				}
			}
			Pen.Arc(C, R, B, To);
		}

		/** The step from the last point radially to another radius at an angle. */
		void Radial(FPen& Pen, const FVector2D& C, double Radius, double AtDeg)
		{
			const FVector2D Point = C + Polar(Radius, AtDeg);
			Pen.To(Point.X, Point.Y);
		}

		// ---- the annular vault --------------------------------------------------------------------------------------------

		double AnnularOuterDeg()
		{
			return AngleOf(Annular, Plan::AnnularOuterSpringingR, Plan::Ledge);
		}

		double AnnularInnerDeg()
		{
			return AngleOf(Annular, Plan::AnnularInnerSpringingR, Plan::Ledge);
		}

		double AnnularCellDeg()
		{
			return (Plan::AnnularCofferToDeg - Plan::AnnularCofferFromDeg) / Plan::AnnularAcross;
		}

		FSteps AnnularSteps()
		{
			const double R = Plan::AnnularRadius;
			return {Deg(Plan::TribuneCofferRib / R), Plan::CofferRiser * Plan::AnnularStepScale, Deg(Plan::CofferSetback * Plan::AnnularStepScale / R)};
		}

		/** The annular vault's plaster, its seven rows of coffers as channels round it, from the outer springing over to the inner. */
		FFormProfile AnnularPlaster()
		{
			const double R = Plan::AnnularRadius;
			const double From = Plan::AnnularCofferFromDeg;
			const double To = Plan::AnnularCofferToDeg;
			const FSteps Steps = AnnularSteps();
			FPen Pen(Annular + Polar(R, AnnularOuterDeg()));
			Pen.Arc(Annular, R, AnnularOuterDeg(), From);
			for (int32 Row = 0; Row < Plan::AnnularAcross; ++Row)
			{
				CofferRow(Pen, Annular, R, From + Row * AnnularCellDeg(), From + (Row + 1) * AnnularCellDeg(), Steps);
			}
			Pen.Arc(Annular, R, To, AnnularInnerDeg());
			const double Plain = R + Plan::VaultPlaster;
			const double Coffered = R + Steps.Depth() + Plan::VaultPlaster;
			Radial(Pen, Annular, Plain, AnnularInnerDeg());
			Pen.Arc(Annular, Plain, AnnularInnerDeg(), To);
			Radial(Pen, Annular, Coffered, To);
			Pen.Arc(Annular, Coffered, To, From);
			Radial(Pen, Annular, Plain, From);
			Pen.Arc(Annular, Plain, From, AnnularOuterDeg());
			return Pen.Closed();
		}

		/** A rib's half-width at a step, in metres at its reference radius: the rib, then each setback beside it. */
		double RibHalf(int32 Level, double Rib, double Setback)
		{
			return Rib + (Level - 1) * Setback;
		}

		/** The annular vault: its plaster, its sixty ribs round in their three steps, and its 420 rosettes. */
		void AnnularVault(TArray<FFormDrawing>& Out)
		{
			FFormLathe Plaster;
			Plaster.Section = AnnularPlaster();
			Out.Add(Drawn(TEXT("tribune.vault.annular"), TEXT("vault.annular"), MoveTemp(Plaster), nullptr));

			const FSteps Steps = AnnularSteps();
			const double R = Plan::AnnularRadius;
			const double Pitch = 360.0 / Plan::AnnularRound;
			const double Setback = Plan::CofferSetback * Plan::AnnularStepScale;
			for (int32 Rib = 0; Rib < Plan::AnnularRound; ++Rib)
			{
				const double At = Plan::AnnularFirstRibDeg + Pitch * Rib;
				for (int32 Level = 1; Level <= Plan::CofferSteps; ++Level)
				{
					const double Front = R + (Level - 1) * Steps.Riser + RibClearance;
					const double Back = R + Level * Steps.Riser + (Level < Plan::CofferSteps ? RibClearance : 0.0);
					const double Half = Deg(RibHalf(Level, Plan::TribuneCofferRib, Setback) / Plan::AnnularCentreR);
					FFormLathe Shape;
					Shape.Section = AnnularSector(Annular, Front, Back, Plan::AnnularCofferFromDeg, Plan::AnnularCofferToDeg);
					Shape.FromDegrees = At - Half;
					Shape.ToDegrees = At + Half;
					Out.Add(Drawn(FString::Printf(TEXT("tribune.vault.annular.rib.%02d.%d"), Rib + 1, Level), TEXT("vault.annular"), MoveTemp(Shape), nullptr));
				}
			}
			const double BackAcross = R * FMath::DegreesToRadians(AnnularCellDeg() - 2.0 * (Steps.RibDeg + (Plan::CofferSteps - 1) * Steps.SetbackDeg));
			const double RibBack = RibHalf(Plan::CofferSteps, Plan::TribuneCofferRib, Setback);
			for (int32 Row = 0; Row < Plan::AnnularAcross; ++Row)
			{
				const double Theta = Plan::AnnularCofferFromDeg + (Row + 0.5) * AnnularCellDeg();
				const FVector2D Back = Annular + Polar(R + Steps.Depth(), Theta);
				const double BackRound = Back.X * FMath::DegreesToRadians(Pitch) - 2.0 * RibBack * Back.X / Plan::AnnularCentreR;
				const double Radius = Plan::RosetteOfBack * FMath::Min(BackAcross, BackRound);
				for (int32 Cell = 0; Cell < Plan::AnnularRound; ++Cell)
				{
					const FVector Round = Direction(Plan::AnnularFirstRibDeg + Pitch * (Cell + 0.5));
					const FVector2D Normal = -Polar(1.0, Theta);
					Out.Add(Rosette(FString::Printf(TEXT("tribune.vault.annular.rosette.%d.%02d"), Row + 1, Cell + 1), TEXT("tribune.rosette"),
						Round * Back.X + FVector::UpVector * Back.Y, Round * Normal.X + FVector::UpVector * Normal.Y,
						FVector::CrossProduct(FVector::UpVector, Round), Radius));
				}
			}
		}

		// ---- the dome -----------------------------------------------------------------------------------------------------

		double DomeSpringingDeg()
		{
			return AngleOf(Sphere, Plan::DomeSpringingR, Plan::Ledge);
		}

		double EyeDeg()
		{
			return AngleOf(Sphere, Plan::EyeRadius, Plan::EyeZ);
		}

		/** The latitude of each ring's edge on the cap, from the springing's band to the eye's. */
		double RingEdgeDeg(int32 Edge)
		{
			return AngleOf(Sphere, Plan::DomeRingEdges[Edge][0], Plan::DomeRingEdges[Edge][1]);
		}

		FSteps DomeSteps(int32 Ring)
		{
			const double R = Plan::DomeRadius;
			const double Scale = Plan::DomeStepScales[Ring];
			return {Deg(Plan::TribuneCofferRib / R), Plan::CofferRiser * Scale, Deg(Plan::CofferSetback * Scale / R)};
		}

		/** Where the eye's cut meets the plaster's back: the plenum's wall, r 1.20. */
		double EyeBackZ()
		{
			return Sphere.Y + FMath::Sqrt(FMath::Square(Plan::DomeRadius + Plan::VaultPlaster) - FMath::Square(Plan::EyeRadius));
		}

		/** The dome's plaster: from its foot on the ring beam's ledge up the face through three rows of coffers to the eye, and back. */
		FFormProfile DomePlaster()
		{
			const double R = Plan::DomeRadius;
			const double Plain = R + Plan::VaultPlaster;
			FPen Pen(Plan::DomeSpringingR, Plan::Ledge);
			Pen.Arc(Sphere, R, DomeSpringingDeg(), RingEdgeDeg(0));
			for (int32 Ring = 0; Ring < Plan::DomeRings; ++Ring)
			{
				CofferRow(Pen, Sphere, R, RingEdgeDeg(Ring), RingEdgeDeg(Ring + 1), DomeSteps(Ring));
			}
			Pen.Arc(Sphere, R, RingEdgeDeg(Plan::DomeRings), EyeDeg());
			Pen.To(Plan::EyeRadius, EyeBackZ());
			Pen.Arc(Sphere, Plain, AngleOf(Sphere, Plan::EyeRadius, EyeBackZ()), RingEdgeDeg(Plan::DomeRings));
			for (int32 Ring = Plan::DomeRings - 1; Ring >= 0; --Ring)
			{
				const double Back = R + DomeSteps(Ring).Depth() + Plan::VaultPlaster;
				Radial(Pen, Sphere, Back, RingEdgeDeg(Ring + 1));
				Pen.Arc(Sphere, Back, RingEdgeDeg(Ring + 1), RingEdgeDeg(Ring));
			}
			Radial(Pen, Sphere, Plain, RingEdgeDeg(0));
			Pen.Arc(Sphere, Plain, RingEdgeDeg(0), Deg(FMath::Asin((Plan::Ledge - Sphere.Y) / Plain)));
			return Pen.Closed();
		}

		/** One step of a meridional rib across the three rings, each ring at its own depths (the steps between lie in the ring ribs). */
		FFormProfile DomeRibSection(int32 Level)
		{
			const double R = Plan::DomeRadius;
			auto Front = [Level, R](int32 Ring) { return R + (Level - 1) * DomeSteps(Ring).Riser + RibClearance; };
			auto Back = [Level, R](int32 Ring) { return R + Level * DomeSteps(Ring).Riser + (Level < Plan::CofferSteps ? RibClearance : 0.0); };
			FPen Pen(Sphere + Polar(Front(0), RingEdgeDeg(0)));
			for (int32 Ring = 0; Ring < Plan::DomeRings; ++Ring)
			{
				if (Ring > 0)
				{
					Radial(Pen, Sphere, Front(Ring), RingEdgeDeg(Ring));
				}
				Pen.Arc(Sphere, Front(Ring), RingEdgeDeg(Ring), RingEdgeDeg(Ring + 1));
			}
			for (int32 Ring = Plan::DomeRings - 1; Ring >= 0; --Ring)
			{
				Radial(Pen, Sphere, Back(Ring), RingEdgeDeg(Ring + 1));
				Pen.Arc(Sphere, Back(Ring), RingEdgeDeg(Ring + 1), RingEdgeDeg(Ring));
			}
			return Pen.Closed();
		}

		/** The dome: its plaster, its twenty-four ribs round in their three steps, and its 72 rosettes. */
		void Dome(TArray<FFormDrawing>& Out)
		{
			FFormLathe Plaster;
			Plaster.Section = DomePlaster();
			Out.Add(Drawn(TEXT("tribune.vault.dome"), TEXT("dome"), MoveTemp(Plaster), nullptr));

			const double R = Plan::DomeRadius;
			const double Pitch = 360.0 / Plan::DomeRound;
			// The ribs' widths are set at the middle ring, where its steps and its cells are, and narrow with the radius from there.
			const double Middle = 0.5 * (RingEdgeDeg(1) + RingEdgeDeg(2));
			const double Reference = R * FMath::Cos(FMath::DegreesToRadians(Middle));
			const double Setback = Plan::CofferSetback * Plan::DomeStepScales[1];
			for (int32 Rib = 0; Rib < Plan::DomeRound; ++Rib)
			{
				const double At = Plan::DomeFirstRibDeg + Pitch * Rib;
				for (int32 Level = 1; Level <= Plan::CofferSteps; ++Level)
				{
					const double Half = Deg(RibHalf(Level, Plan::TribuneCofferRib, Setback) / Reference);
					FFormLathe Shape;
					Shape.Section = DomeRibSection(Level);
					Shape.FromDegrees = At - Half;
					Shape.ToDegrees = At + Half;
					Out.Add(Drawn(FString::Printf(TEXT("tribune.vault.dome.rib.%02d.%d"), Rib + 1, Level), TEXT("dome"), MoveTemp(Shape), nullptr));
				}
			}
			const double RibBack = RibHalf(Plan::CofferSteps, Plan::TribuneCofferRib, Setback);
			for (int32 Ring = 0; Ring < Plan::DomeRings; ++Ring)
			{
				const FSteps Steps = DomeSteps(Ring);
				const double Psi = 0.5 * (RingEdgeDeg(Ring) + RingEdgeDeg(Ring + 1));
				const FVector2D Back = Sphere + Polar(R + Steps.Depth(), Psi);
				const double BackUp = R * FMath::DegreesToRadians(RingEdgeDeg(Ring + 1) - RingEdgeDeg(Ring) - 2.0 * (Steps.RibDeg + (Plan::CofferSteps - 1) * Steps.SetbackDeg));
				const double BackRound = Back.X * FMath::DegreesToRadians(Pitch) - 2.0 * RibBack * Back.X / Reference;
				const double Radius = Plan::RosetteOfBack * FMath::Min(BackUp, BackRound);
				for (int32 Cell = 0; Cell < Plan::DomeRound; ++Cell)
				{
					const FVector Round = Direction(Plan::DomeFirstRibDeg + Pitch * (Cell + 0.5));
					const FVector2D Normal = -Polar(1.0, Psi);
					Out.Add(Rosette(FString::Printf(TEXT("tribune.vault.dome.rosette.%d.%02d"), Ring + 1, Cell + 1), TEXT("tribune.rosette"),
						Round * Back.X + FVector::UpVector * Back.Y, Round * Normal.X + FVector::UpVector * Normal.Y,
						FVector::CrossProduct(FVector::UpVector, Round), Radius));
				}
			}
		}

		/** The eye: its laylight disc closing the dome's rim flush, and the gilt bead turned round the rim. */
		void Eye(TArray<FFormDrawing>& Out)
		{
			FFormDrawing Glass = PlanPrism(TEXT("tribune.laylight.eye.glass"), TEXT("laylight.glass"), Circle(FVector2D::ZeroVector, Plan::EyeRadius), {},
				Plan::EyeZ, Plan::EyeZ + Plan::LaylightGlass, nullptr);
			Glass.bNanite = false;
			Out.Add(MoveTemp(Glass));
			FFormLathe Bead;
			Bead.Section = FPen(Plan::BeadTop[0], Plan::BeadTop[1]).Round(Plan::BeadFoot[0], Plan::BeadFoot[1], true).To(Plan::BeadOuter, Plan::BeadFoot[1])
				.To(Plan::BeadOuter, Plan::BeadTop[1]).Closed();
			Out.Add(Drawn(TEXT("tribune.eye.bead"), TEXT("eye.bead"), MoveTemp(Bead), nullptr));
		}

		/**
		 * The tribune's masonry over its vaults and the roof slab, turned in one: from the axis over the eye's plenum (its
		 * soffit the slab's, 5.15), down the plenum's wall, over the dome's back, along the ring beam's ledges, over the
		 * annular vault's back and up the outer wall's line to the roof's top.
		 */
		void Masonry(TArray<FFormDrawing>& Out)
		{
			const double Dome = Plan::DomeRadius + Plan::VaultPlaster;
			const double Vault = Plan::AnnularRadius + Plan::VaultPlaster;
			FPen Pen(0.0, Plan::RoofTop);
			Pen.To(0.0, Plan::RoofSoffit).To(Plan::EyeRadius, Plan::RoofSoffit).To(Plan::EyeRadius, EyeBackZ());
			Pen.Arc(Sphere, Dome, AngleOf(Sphere, Plan::EyeRadius, EyeBackZ()), RingEdgeDeg(Plan::DomeRings));
			for (int32 Ring = Plan::DomeRings - 1; Ring >= 0; --Ring)
			{
				const double Back = Plan::DomeRadius + DomeSteps(Ring).Depth() + Plan::VaultPlaster;
				Radial(Pen, Sphere, Back, RingEdgeDeg(Ring + 1));
				Pen.Arc(Sphere, Back, RingEdgeDeg(Ring + 1), RingEdgeDeg(Ring));
			}
			Radial(Pen, Sphere, Dome, RingEdgeDeg(0));
			Pen.Arc(Sphere, Dome, RingEdgeDeg(0), Deg(FMath::Asin((Plan::Ledge - Sphere.Y) / Dome)));
			const FVector2D Foot = Annular + Polar(Vault, AnnularInnerDeg());
			Pen.To(Foot.X, Plan::Ledge).To(Foot.X, Foot.Y);
			Pen.Arc(Annular, Vault, AnnularInnerDeg(), Plan::AnnularCofferToDeg);
			const double Coffered = Plan::AnnularRadius + AnnularSteps().Depth() + Plan::VaultPlaster;
			Radial(Pen, Annular, Coffered, Plan::AnnularCofferToDeg);
			Pen.Arc(Annular, Coffered, Plan::AnnularCofferToDeg, Plan::AnnularCofferFromDeg);
			Radial(Pen, Annular, Vault, Plan::AnnularCofferFromDeg);
			const double Wall = Deg(FMath::Acos((Plan::TribuneRadius - Annular.X) / Vault));
			Pen.Arc(Annular, Vault, Plan::AnnularCofferFromDeg, Wall).To(Plan::TribuneRadius, Plan::RoofTop);
			FFormLathe Shape;
			Shape.Section = Pen.Closed();
			Out.Add(Drawn(TEXT("shell.tribune.vault"), TEXT("shell"), MoveTemp(Shape), Solid));
		}
	}

	void DrawTribuneVaults(TArray<FFormDrawing>& Out)
	{
		AnnularVault(Out);
		Dome(Out);
		Eye(Out);
		Masonry(Out);
	}
}
