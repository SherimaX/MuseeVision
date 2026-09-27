#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The dome (§3.4): a hemisphere R 10.00 about (0, 0, 10.00) from the cornice's top to the eye's curb, its shell 0.60
 * thick and covered in milled lead. Between 4° and 64° of elevation it is coffered in five rings of 28, each coffer sunk
 * in two steps and a bevel to its panel on R 10.45.
 *
 * Form has no relief on a surface of revolution, so the coffers are built as what a plasterer runs: the panels are the
 * shell's inner face on R 10.45, the six ring ribs are turned whole with their steps and bevels, and each meridional rib
 * is three stacked layers, each turned about a horizontal axis through the dome's centre, so the rib follows the sphere
 * and its steps mitre cleanly into the ring ribs'. A meridional rib's sides are therefore parallel, its width the sheet's
 * at the ring's mid elevation, where the sheet's rule makes its sides radial (at most 8 % off at ring 5's edges).
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

		/** A distance up the inner sphere, as degrees of elevation. */
		double Up(double Meters) { return FMath::RadiansToDegrees(Meters / Plan::DomeRadius); }

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

		/** A point of a coffer's section: T metres up the inner sphere from an elevation, D deep into the dome. */
		FVector2D Sunk(double BaseDeg, double T, double D)
		{
			return OnDome(Plan::DomeRadius + D, BaseDeg + Up(T));
		}

		/**
		 * A ring rib at a ring boundary: its face on the inner sphere, then each side's two steps and bevel down to the
		 * panels, the side below from the ring under it and the side above from the ring over it. The lowest and highest
		 * ribs have only their coffered side; the plain band carries on from the other.
		 */
		FFormProfile RingRib(double Deg, const FCofferSteps* Below, const FCofferSteps* Above)
		{
			const double R0 = Plan::DomeRadius;
			const double R1 = R0 + Plan::FirstSinking;
			const double R2 = R0 + Plan::SecondSinking;
			const double Panel = Plan::PanelRadius;
			const double TopT = Above ? Above->Width() : 0.0;
			const double BottomT = Below ? -Below->Width() : 0.0;
			FFormProfile Out = FFormProfile::From(Sunk(Deg, TopT, Plan::PanelSinking));
			Out.ArcAbout(SphereCentre, Panel, Panel, Deg + Up(TopT), Deg + Up(BottomT), EFormJoint::Crisp);
			double FaceFrom = 0.0;
			if (Below)
			{
				const FCofferSteps& S = *Below;
				Out.LineTo(Sunk(Deg, -(S.Rib + 2.0 * S.Step), Plan::SecondSinking), EFormJoint::Crisp)
					.ArcAbout(SphereCentre, R2, R2, Deg - Up(S.Rib + 2.0 * S.Step), Deg - Up(S.Rib + S.Step), EFormJoint::Crisp)
					.LineTo(Sunk(Deg, -(S.Rib + S.Step), Plan::FirstSinking), EFormJoint::Crisp)
					.ArcAbout(SphereCentre, R1, R1, Deg - Up(S.Rib + S.Step), Deg - Up(S.Rib), EFormJoint::Crisp)
					.LineTo(Sunk(Deg, -S.Rib, 0.0), EFormJoint::Crisp);
				FaceFrom = -S.Rib;
			}
			else
			{
				Out.LineTo(Sunk(Deg, 0.0, 0.0), EFormJoint::Crisp);
			}
			const double FaceTo = Above ? Above->Rib : 0.0;
			Out.ArcAbout(SphereCentre, R0, R0, Deg + Up(FaceFrom), Deg + Up(FaceTo), EFormJoint::Crisp);
			if (Above)
			{
				const FCofferSteps& S = *Above;
				Out.LineTo(Sunk(Deg, S.Rib, Plan::FirstSinking), EFormJoint::Crisp)
					.ArcAbout(SphereCentre, R1, R1, Deg + Up(S.Rib), Deg + Up(S.Rib + S.Step), EFormJoint::Crisp)
					.LineTo(Sunk(Deg, S.Rib + S.Step, Plan::SecondSinking), EFormJoint::Crisp)
					.ArcAbout(SphereCentre, R2, R2, Deg + Up(S.Rib + S.Step), Deg + Up(S.Rib + 2.0 * S.Step), EFormJoint::Crisp);
			}
			// The bevel back up to the start on the panels (or, with no side above, the rib's end straight down to them).
			return Out.Close();
		}

		/**
		 * One meridional rib of one ring: three layers, each turned about the horizontal axis through the dome's centre
		 * square to the rib's plane, so each follows the sphere. Each layer stops where the ring ribs' own layer of the
		 * same depth begins, so the four sides of every coffer mitre at its corners.
		 */
		void MeridionalRib(TArray<FFormDrawing>& Out, int32 RingIndex, int32 Cell, const FCofferSteps& S)
		{
			const double From = RingFromDeg(RingIndex);
			const double To = From + Plan::CofferRingDeg;
			const double Theta = 360.0 * Cell / Plan::CofferCells;
			const FMuseeFrame Frame = FrameOfAxes(FVector(0.0, 0.0, Plan::DomeCentreZ), Radial(Theta), FVector::UpVector);
			const double R0 = Plan::DomeRadius;
			const double R1 = R0 + Plan::FirstSinking;
			const double R2 = R0 + Plan::SecondSinking;
			const double Inset = S.Rib + 2.0 * S.Step;
			struct FLayer
			{
				FFormProfile Section;
				double Trim;
			};
			FLayer Layers[3] = {
				{FFormProfile::Rectangle(FVector2D(R0, -S.Rib), FVector2D(R1, S.Rib)), S.Rib},
				{FFormProfile::Rectangle(FVector2D(R1, -(S.Rib + S.Step)), FVector2D(R2, S.Rib + S.Step)), S.Rib + S.Step},
				{FFormProfile::Polyline({FVector2D(R2, -Inset), FVector2D(Plan::PanelRadius, -S.Width()), FVector2D(Plan::PanelRadius, S.Width()),
					FVector2D(R2, Inset)}, true), Inset}};
			for (int32 Layer = 0; Layer < 3; ++Layer)
			{
				FFormLathe Shape;
				Shape.Section = MoveTemp(Layers[Layer].Section);
				Shape.FromDegrees = From + Up(Layers[Layer].Trim);
				Shape.ToDegrees = To - Up(Layers[Layer].Trim);
				Out.Add(Drawn(FString::Printf(TEXT("dome.coffer.ring-%d.rib-%02d.layer-%d"), RingIndex + 1, Cell, Layer + 1), TEXT("dome.coffer"),
					MoveTemp(Shape), nullptr, Frame));
			}
		}

		void Coffers(TArray<FFormDrawing>& Out)
		{
			FCofferSteps Steps[Plan::CofferRings];
			for (int32 Index = 0; Index < Plan::CofferRings; ++Index)
			{
				Steps[Index] = StepsOf(Index);
			}
			for (int32 Boundary = 0; Boundary <= Plan::CofferRings; ++Boundary)
			{
				const FCofferSteps* Below = Boundary > 0 ? &Steps[Boundary - 1] : nullptr;
				const FCofferSteps* Above = Boundary < Plan::CofferRings ? &Steps[Boundary] : nullptr;
				const FString Name = FString::Printf(TEXT("dome.coffer.ring-rib-%d"), Boundary + 1);
				Out.Add(Turn(*Name, TEXT("dome.coffer"), RingRib(RingFromDeg(Boundary), Below, Above)));
			}
			for (int32 Index = 0; Index < Plan::CofferRings; ++Index)
			{
				for (int32 Cell = 0; Cell < Plan::CofferCells; ++Cell)
				{
					MeridionalRib(Out, Index, Cell, Steps[Index]);
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
