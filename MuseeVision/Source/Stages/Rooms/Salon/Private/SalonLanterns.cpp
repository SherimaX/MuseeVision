#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The five lanterns inside the roof (§3.4): each bay's shaft rising from the crown of its vault to the roof's top, and
 * at the shaft's mouth a bronze grid carrying the inner pane (bay 2's flashed opal, the others clear), with bay 4's
 * linen scrim stretched on its angle frame below the grid and sagging under its own weight, a fixed diffuser (D-014).
 * The weather glazing over each shaft is the roof's.
 */
namespace Musee::Salon
{
	namespace
	{
		/** The shaft's four walls, from 4 mm inside the vault's face (so they meet the ribs with no gap) up to the roof's top. */
		void Shaft(TArray<FFormDrawing>& Out, const FString& Name, const FBox2D& L)
		{
			const double T = Plan::ShaftWall;
			const double Foot = Plan::VaultRadius - Plan::SealIn;
			const double Phi = RowAngle(Plan::LanternRow);
			const double Bottom = Plan::Springing + Foot * FMath::Sin(Phi);
			Out.Add(Box(Name + TEXT(".north"), TEXT("lantern.shaft"), FVector(L.Min.X - T, L.Min.Y - T, Bottom), FVector(L.Max.X + T, L.Min.Y, Plan::RoofTop),
				Solid));
			Out.Add(Box(Name + TEXT(".south"), TEXT("lantern.shaft"), FVector(L.Min.X - T, L.Max.Y, Bottom), FVector(L.Max.X + T, L.Max.Y + T, Plan::RoofTop),
				Solid));

			// The end walls stand on the vault's curve across the crown row, and run out under the long walls' thickness.
			const double Half = L.Max.Y + T;
			const double Top = Plan::RoofTop - Plan::Springing;
			const double Edge = FMath::RadiansToDegrees(FMath::Acos(Half / Foot));
			FFormProfile End = FFormProfile::From(FVector2D(Half, Top));
			End.LineTo(FVector2D(-Half, Top), EFormJoint::Crisp).LineTo(FVector2D(-Half, Foot * FMath::Sin(FMath::DegreesToRadians(Edge))), EFormJoint::Crisp)
				.ArcAbout(FVector2D::ZeroVector, Foot, Foot, 180.0 - Edge, Edge, EFormJoint::Crisp);
			End.Close();
			Out.Add(FaceSlab(Name + TEXT(".west"), TEXT("lantern.shaft"), FaceFrame(FVector(L.Min.X - T, 0.0, Plan::Springing), FMuseeFrame::East()), End, {}, T,
				Solid));
			Out.Add(FaceSlab(Name + TEXT(".east"), TEXT("lantern.shaft"), FaceFrame(FVector(L.Max.X + T, 0.0, Plan::Springing), FMuseeFrame::West()), End, {}, T,
				Solid));
		}

		/** The grid under the inner pane: its perimeter, one bar along the axis, five cross bars at the sixths. */
		void Grid(TArray<FFormDrawing>& Out, const FString& Name, const FBox2D& L)
		{
			const double F = Plan::GridFrame;
			const double B = 0.5 * Plan::GridBar;
			const double Z0 = Plan::GridBottom;
			const double Z1 = Plan::InnerPaneZ;
			Out.Add(PlanSlab(Name + TEXT(".frame"), TEXT("lantern.grid"), FFormProfile::Rectangle(L.Min, L.Max),
				{FFormProfile::Rectangle(L.Min + FVector2D(F, F), L.Max - FVector2D(F, F))}, Z0, Z1, Solid));
			Out.Add(Box(Name + TEXT(".axis"), TEXT("lantern.grid"), FVector(L.Min.X + F, -B, Z0), FVector(L.Max.X - F, B, Z1), Solid));
			for (int32 K = 1; K < Plan::GridDivisions; ++K)
			{
				const double X = FMath::Lerp(L.Min.X, L.Max.X, double(K) / Plan::GridDivisions);
				Out.Add(Box(FString::Printf(TEXT("%s.bar.%d"), *Name, K), TEXT("lantern.grid"), FVector(X - B, L.Min.Y + F, Z0),
					FVector(X + B, L.Max.Y - F, Z1), Solid));
			}
		}

		/**
		 * Bay 4's linen, stretched at 13.490 on its angle frame round the shaft's mouth and sagging 30 mm at its middle, as a
		 * cloth held on its four sides hangs under its own weight.
		 */
		void Scrim(TArray<FFormDrawing>& Out, const FString& Name, const FBox2D& L)
		{
			FFormCloth Linen;
			Linen.SizeMeters = L.Max - L.Min;
			Linen.SagMeters = Plan::ScrimSag;
			Linen.ThicknessMeters = Plan::ScrimThickness;
			Linen.GridMeters = Plan::ScrimGrid;
			FFormDrawing Cloth = Drawn(Name + TEXT(".scrim"), TEXT("lantern.scrim"), MoveTemp(Linen), nullptr, FMuseeFrame(FVector(L.Min.X, L.Min.Y, Plan::ScrimZ)));
			Cloth.bNanite = false;
			Out.Add(MoveTemp(Cloth));

			// The angle: its upright leg against the shaft, its flat leg under the cloth's edge.
			const double Leg = Plan::ScrimFrameLeg;
			const double Thick = Plan::ScrimFrameThickness;
			const double Z0 = Plan::ScrimFrameBottom;
			const double Z1 = Plan::ScrimFrameTop;
			FFormSweep Angle;
			Angle.Section = Polygon({{0.0, Z0}, {Leg, Z0}, {Leg, Z0 + Thick}, {Thick, Z0 + Thick}, {Thick, Z1}, {0.0, Z1}});
			Angle.Path = FFormProfile::Polyline({L.Min, FVector2D(L.Max.X, L.Min.Y), L.Max, FVector2D(L.Min.X, L.Max.Y)}, true);
			Out.Add(Drawn(Name + TEXT(".scrim.frame"), TEXT("lantern.grid"), MoveTemp(Angle), nullptr));
		}
	}

	void DrawLanterns(TArray<FFormDrawing>& Out)
	{
		for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
		{
			const FString Name = FString::Printf(TEXT("lantern.bay%d"), Bay + 1);
			const FBox2D L = Lantern(Bay);
			Shaft(Out, Name + TEXT(".shaft"), L);
			Grid(Out, Name + TEXT(".grid"), L);
			FFormDrawing Pane = Box(Name + TEXT(".pane"), Bay == Plan::OpalBay ? TEXT("lantern.pane.opal") : TEXT("lantern.pane.clear"),
				FVector(L.Min.X, L.Min.Y, Plan::InnerPaneZ), FVector(L.Max.X, L.Max.Y, Plan::InnerPaneZ + Plan::InnerPane), Solid);
			Pane.bNanite = false;
			Out.Add(MoveTemp(Pane));
			if (Bay == Plan::ScrimBay)
			{
				Scrim(Out, Name, L);
			}
		}
	}
}
