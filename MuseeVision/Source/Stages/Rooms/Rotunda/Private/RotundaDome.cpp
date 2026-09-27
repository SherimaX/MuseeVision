#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The dome (§3.4): a hemisphere R 10.00 about (0, 0, 10.00) from the cornice's top to the eye's curb, its shell 0.60
 * thick and covered in milled lead. Between 4° and 64° of elevation it is coffered in five rings of 28, each coffer sunk
 * in two steps and a bevel to its panel on R 10.45.
 *
 * Each coffer is what a plasterer runs round its cell: the sheet's section, from the rib's face on the inner sphere down to
 * the panel, run round the cell's four sides and mitred at its corners. It is drawn in the dome's development (Form's
 * ground of revolution), where the cell between two meridians and two parallels is a rectangle, true in metres on the
 * ring's middle parallel as the sheet sets its cell out; carried onto the dome, its meridional sides lie in the meridian
 * planes of the cell's edges and its other sides on the parallels, so neighbouring coffers meet face to face along the
 * ribs' middle lines. The panels are the shell's inner face on R 10.45.
 */
namespace Musee::Rotunda
{
	namespace
	{
		const FVector2D SphereCentre(0.0, Plan::DomeCentreZ);

		/** The steps of a ring of coffers, from its cell (the arc between ribs at its mid elevation). */
		struct FCofferSteps
		{
			double Rib = 0.0;
			double Step = 0.0;
			double Bevel = 0.0;

			double Width() const { return Rib + 2.0 * Step + Bevel; }
		};

		double RingFromDeg(int32 Index) { return Plan::CofferFromDeg + Plan::CofferRingDeg * Index; }

		FCofferSteps StepsOf(int32 Index)
		{
			const double Middle = FMath::DegreesToRadians(RingFromDeg(Index) + 0.5 * Plan::CofferRingDeg);
			const double Cell = 2.0 * UE_DOUBLE_PI * Plan::DomeRadius * FMath::Cos(Middle) / Plan::CofferCells;
			return FCofferSteps{Plan::RibOfCell * Cell, Plan::StepOfCell * Cell, Plan::BevelOfCell * Cell};
		}

		/** A band between two spheres of the family, from one elevation to another. */
		FFormProfile SphericalBand(double Inner, double Outer, double FromDeg, double ToDeg)
		{
			FFormProfile Out = FFormProfile::From(OnDome(Outer, FromDeg));
			Out.ArcAbout(SphereCentre, Outer, Outer, FromDeg, ToDeg, EFormJoint::Crisp)
				.LineTo(OnDome(Inner, ToDeg), EFormJoint::Crisp)
				.ArcAbout(SphereCentre, Inner, Inner, ToDeg, FromDeg, EFormJoint::Crisp);
			return Out.Close();
		}

		FFormDrawing Turn(const TCHAR* Name, const TCHAR* LocalSurface, FFormProfile Section)
		{
			FFormLathe Shape;
			Shape.Section = MoveTemp(Section);
			return Drawn(Name, LocalSurface, MoveTemp(Shape), nullptr);
		}

		/**
		 * The plain bands, the shell and the curb. The shell's inner face is the coffers' panel sphere from 4° to the
		 * curb, the plain bands lie on it below and above the coffers, and its outer sphere runs from its foot on the
		 * drum's top to r 2.90, where the curb rises over it to the eye.
		 */
		void Shell(TArray<FFormDrawing>& Out)
		{
			const double CofferTop = Plan::CofferFromDeg + Plan::CofferRings * Plan::CofferRingDeg;
			const double Eye = EyeCurbDeg();
			const double Foot = ElevationAt(Plan::ShellRadius, Plan::DrumTop);
			const double ShellTop = FMath::RadiansToDegrees(FMath::Acos(Plan::ShellTopRadius / Plan::ShellRadius));
			Out.Add(Turn(Pieces::DomeBandLower, TEXT("dome.band"), SphericalBand(Plan::DomeRadius, Plan::PanelRadius, CorniceDeg(), Plan::CofferFromDeg)));
			Out.Add(Turn(Pieces::DomeBandUpper, TEXT("dome.band"), SphericalBand(Plan::DomeRadius, Plan::PanelRadius, CofferTop, Eye)));

			FFormProfile Structure = FFormProfile::From(OnDome(Plan::PanelRadius, Plan::CofferFromDeg));
			Structure.ArcAbout(SphereCentre, Plan::PanelRadius, Plan::PanelRadius, Plan::CofferFromDeg, Eye, EFormJoint::Crisp)
				.LineTo(OnDome(Plan::ShellRadius, Eye), EFormJoint::Crisp)
				.ArcAbout(SphereCentre, Plan::ShellRadius, Plan::ShellRadius, Eye, Foot, EFormJoint::Crisp)
				.Close();
			Out.Add(Turn(Pieces::DomeShell, TEXT("dome.coffer"), MoveTemp(Structure)));

			FFormProfile Curb = FFormProfile::From(OnDome(Plan::DomeRadius, Eye));
			for (const auto& Point : Plan::CurbPoints)
			{
				Curb.LineTo(FVector2D(Point[0], Point[1]), EFormJoint::Crisp);
			}
			Curb.LineTo(OnDome(Plan::ShellRadius, ShellTop), EFormJoint::Crisp)
				.ArcAbout(SphereCentre, Plan::ShellRadius, Plan::ShellRadius, ShellTop, Eye, EFormJoint::Crisp)
				.Close();
			Out.Add(Turn(Pieces::EyeCurb, TEXT("dome.eye-curb"), MoveTemp(Curb)));

			// The lead: Code 8 sheet on the shell's outer sphere from its foot, up the curb's outer face and over its top.
			const double Lead = Plan::LeadMeters;
			const double Outer = Plan::ShellRadius + Lead;
			const double CurbTop = Plan::CurbPoints[UE_ARRAY_COUNT(Plan::CurbPoints) - 1][1];
			const double EyeEdge = Plan::GlassRadius;
			const double OuterTop = FMath::RadiansToDegrees(FMath::Acos((Plan::ShellTopRadius + Lead) / Outer));
			FFormProfile Sheet = FFormProfile::From(OnDome(Plan::ShellRadius, Foot));
			Sheet.ArcAbout(SphereCentre, Plan::ShellRadius, Plan::ShellRadius, Foot, ShellTop, EFormJoint::Crisp)
				.LineTo(FVector2D(Plan::ShellTopRadius, CurbTop), EFormJoint::Crisp)
				.LineTo(FVector2D(EyeEdge, CurbTop), EFormJoint::Crisp)
				.LineTo(FVector2D(EyeEdge, CurbTop + Lead), EFormJoint::Crisp)
				.LineTo(FVector2D(Plan::ShellTopRadius + Lead, CurbTop + Lead), EFormJoint::Crisp)
				.LineTo(OnDome(Outer, OuterTop), EFormJoint::Crisp)
				.ArcAbout(SphereCentre, Outer, Outer, OuterTop, ElevationAt(Outer, Plan::DrumTop), EFormJoint::Crisp)
				.Close();
			Out.Add(Turn(TEXT("dome.outer"), TEXT("dome.outer"), MoveTemp(Sheet)));
		}

		/**
		 * A coffer's section on all four sides (§3.4), A in from the cell's edge and B out of the inner sphere towards the room:
		 * the rib's face, its two steps and the bevel down to the panel, drawn anticlockwise so its solid is on its left.
		 */
		FFormProfile CofferSection(const FCofferSteps& S)
		{
			const double Panel = Plan::PanelSinking;
			const double Width = S.Width();
			return FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(0.0, -Panel), FVector2D(Width, -Panel),
				FVector2D(S.Rib + 2.0 * S.Step, -Plan::SecondSinking), FVector2D(S.Rib + S.Step, -Plan::SecondSinking),
				FVector2D(S.Rib + S.Step, -Plan::FirstSinking), FVector2D(S.Rib, -Plan::FirstSinking), FVector2D(S.Rib, 0.0)}, true);
		}

		/**
		 * The coffers, ring by ring: each cell's section run round it in the development of the inner sphere, whose meridian is
		 * drawn down from the coffers' top so the development's W faces the room and V runs down the dome from 64°.
		 */
		void Coffers(TArray<FFormDrawing>& Out)
		{
			const double R = Plan::DomeRadius;
			const double Top = RingFromDeg(Plan::CofferRings);
			FFormProfile Meridian = FFormProfile::From(OnDome(R, Top));
			Meridian.ArcAbout(SphereCentre, R, R, Top, Plan::CofferFromDeg);
			auto Down = [R, Top](double Deg) { return R * FMath::DegreesToRadians(Top - Deg); };
			for (int32 Index = 0; Index < Plan::CofferRings; ++Index)
			{
				const double Middle = FMath::DegreesToRadians(RingFromDeg(Index) + 0.5 * Plan::CofferRingDeg);
				const double Reference = R * FMath::Cos(Middle);
				const double Cell = 2.0 * UE_DOUBLE_PI * Reference / Plan::CofferCells;
				FFormSweep Shape;
				Shape.Section = CofferSection(StepsOf(Index));
				Shape.Path = FFormProfile::Rectangle(FVector2D(0.0, Down(RingFromDeg(Index + 1))), FVector2D(Cell, Down(RingFromDeg(Index))));
				for (int32 Bay = 0; Bay < Plan::CofferCells; ++Bay)
				{
					FFormDrawing& Coffer = Out.Add_GetRef(Drawn(FString::Printf(TEXT("dome.coffer.ring-%d.cell-%02d"), Index + 1, Bay + 1),
						TEXT("dome.coffer"), FFormSweep(Shape), nullptr, Turned(360.0 * Bay / Plan::CofferCells)));
					Coffer.Ground = FFormGround::Revolved(Meridian, Reference);
				}
			}
		}
	}

	void DrawDome(TArray<FFormDrawing>& Out)
	{
		Shell(Out);
		Coffers(Out);
	}
}
