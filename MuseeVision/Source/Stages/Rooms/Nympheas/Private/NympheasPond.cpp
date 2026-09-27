#include "NympheasFrieze.h"
#include "NympheasParts.h"
#include "NympheasPlan.h"

/**
 * The pond (§3.5), the body that lifts (nympheas.basin): the bronze tray under everything, the bluestone lining, bed
 * and planting shelves, the travertine coping in sixteen blocks with the lily frieze carved on their die, and the gilt
 * lip that houses the line of light. Every part is set out on the water's edge and its parallels, so its joints stay
 * radial. The four telescopic posts stand in their sleeves below, collapsed as at rest.
 */
namespace Musee::Nympheas
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;
		constexpr double Turn = 2.0 * UE_DOUBLE_PI;

		/**
		 * The frieze stone's depth behind its ground: the band is cut from the die's outer 40 mm, as far as the ledge under
		 * it projects (0.425 - 0.385), so the coping behind the frieze never shows past the ledge.
		 */
		constexpr double FriezeBack = Plan::ToeD - Plan::DieD;

		FMuseeKey Basin() { return Key(TEXT("basin")); }

		/** A drawing riding on the pond's body. */
		FFormDrawing& OnBasin(FFormDrawing& Drawing)
		{
			Drawing.Body = Basin();
			return Drawing;
		}

		/**
		 * The arc length along the parallel at D, counted from the east apex northwards (t decreasing, the way the pond's
		 * paths run so their right is outward), at t = -Tau; any Tau, whole turns adding the parallel's length.
		 */
		double Northwards(double Tau, double D)
		{
			const double Turns = FMath::FloorToDouble(Tau / Turn);
			return Pond().ArcAt(Tau - Turns * Turn, D) + Turns * Pond().Length(D);
		}

		/** One coping block's stretch: from Tau0 to Tau1 northwards, the joints radial on the die's parallel. */
		struct FBlock
		{
			double Tau0 = 0.0;
			double Tau1 = 0.0;
		};

		FBlock BlockOf(int32 K)
		{
			const double Pitch = Pond().Length(Plan::DieD) / Plan::CopingBlocks;
			const double Half = 0.5 * Plan::CopingJoint;
			return {Pond().TAt((K - 0.5) * Pitch + Half, Plan::DieD), Pond().TAt((K + 0.5) * Pitch - Half, Plan::DieD)};
		}

		/** The water's edge from Tau0 to Tau1 northwards: a plan path whose right is out of the pond. */
		FFormProfile EdgeBetween(const FBlock& Block)
		{
			return FFormProfile::From(Pond().Point(-Block.Tau0)).ArcAbout(FVector2D::ZeroVector, Plan::PondA, Plan::PondB,
				-FMath::RadiansToDegrees(Block.Tau0), -FMath::RadiansToDegrees(Block.Tau1));
		}

		FFormProfile Line(FFormProfile& Profile, const Plan::FSectionPoint& P) { return Profile.LineTo(FVector2D(P.D, P.Z), EFormJoint::Crisp); }

		/**
		 * The coping's section outside the frieze, drawn with the stone on its left (d out, z up): along its bed on the
		 * tray, up the toe, plinth, ovolo and ledge, in to the frieze's back and up it, the cavetto, fillet and torus, over
		 * the seat and its wash, down the bead's rebate and the inner face.
		 */
		FFormProfile CopingSection()
		{
			const double Bed = Plan::TrayMeters;
			const double Back = Plan::DieD - FriezeBack;
			FFormProfile S = FFormProfile::From(FVector2D(0.0, Bed));
			S.LineTo(FVector2D(Plan::ToeD, Bed), EFormJoint::Crisp);
			Line(S, Plan::ToeIn);
			Line(S, Plan::PlinthOut);
			S.LineTo(FVector2D(Plan::PlinthOut.D, Plan::OvoloCentre.Z), EFormJoint::Crisp);
			S.ArcAbout(FVector2D(Plan::OvoloCentre.D, Plan::OvoloCentre.Z), Plan::OvoloRadius, Plan::OvoloRadius, 0.0, 90.0);
			S.LineTo(FVector2D(Back, Plan::BandFoot), EFormJoint::Crisp);
			S.LineTo(FVector2D(Back, Plan::BandTop), EFormJoint::Crisp);
			S.LineTo(FVector2D(Plan::DieD, Plan::BandTop), EFormJoint::Crisp);
			S.ArcAbout(FVector2D(Plan::CavettoCentre.D, Plan::CavettoCentre.Z), Plan::CavettoRadius, Plan::CavettoRadius, 180.0, 90.0, EFormJoint::Crisp);
			S.ArcAbout(FVector2D(Plan::TorusCentre.D, Plan::TorusCentre.Z), Plan::TorusRadius, Plan::TorusRadius, Plan::TorusStartDeg, 90.0,
				EFormJoint::Crisp);
			Line(S, Plan::SeatInner);
			S.LineTo(FVector2D(Plan::Wash.D, Plan::Wash.Z), EFormJoint::Smooth);
			Line(S, Plan::BeadTop);
			Line(S, Plan::BeadFoot);
			S.LineTo(FVector2D(0.0, Plan::BeadFoot.Z), EFormJoint::Crisp);
			return S.Close();
		}

		/** The lip's section, drawn with the bronze on its left: up behind the band, over the bead, down its water face. */
		FFormProfile LipSection()
		{
			FFormProfile S = FFormProfile::From(FVector2D(0.0, Plan::LipFoot));
			S.LineTo(FVector2D(0.0, Plan::BeadFoot.Z), EFormJoint::Crisp);
			S.LineTo(FVector2D(Plan::BeadFoot.D, Plan::BeadFoot.Z), EFormJoint::Crisp);
			S.LineTo(FVector2D(Plan::BeadFoot.D, Plan::BeadCentre.Z), EFormJoint::Crisp);
			S.ArcAbout(FVector2D(Plan::BeadCentre.D, Plan::BeadCentre.Z), Plan::BeadRadius, Plan::BeadRadius, 0.0, 180.0, EFormJoint::Crisp);
			for (int32 Index = int32(UE_ARRAY_COUNT(Plan::LipPoints)) - 2; Index >= 0; --Index)
			{
				S.LineTo(FVector2D(Plan::LipPoints[Index].D, Plan::LipPoints[Index].Z), EFormJoint::Crisp);
			}
			return S.Close();
		}

		/** The tray, the lining with its bed, and the two planting shelves. */
		void Vessel(TArray<FFormDrawing>& Out)
		{
			TArray<FVector2D> Toe = Pond().Stations(Plan::ToeD, 0.0, Turn, Plan::OvalStations);
			Toe.Pop();
			OnBasin(Out.Add_GetRef(Drawn(TEXT("pond.tray"), TEXT("pond.tray"), Slab(FFormProfile::Polygon(Toe), {}, 0.0, Plan::TrayMeters), Solid)));

			const double A = Plan::PondA + Plan::LiningD;
			const double B = Plan::PondB + Plan::LiningD;
			FFormBasin Lining;
			Lining.Outline = FFormProfile::Ellipse(FVector2D::ZeroVector, A, B, 0.0, true);
			Lining.WallMeters = -Plan::LiningD;
			Lining.FloorMeters = Plan::BedTop - Plan::TrayMeters;
			Lining.DepthMeters = Plan::LipFoot - Plan::BedTop;
			OnBasin(Out.Add_GetRef(Drawn(TEXT("pond.lining"), TEXT("pond.bed"), MoveTemp(Lining), Solid,
				FMuseeFrame(FVector(0.0, 0.0, Plan::TrayMeters)))));

			// Each shelf: the lining's ellipse beyond |x| = 3.20, closed by its riser.
			const double Beyond = FMath::RadiansToDegrees(FMath::Acos(Plan::ShelfX / A));
			for (const bool bEast : {true, false})
			{
				const double Mid = bEast ? 0.0 : 180.0;
				const double From = FMath::DegreesToRadians(Mid - Beyond);
				FFormProfile Outline = FFormProfile::From(FVector2D(A * FMath::Cos(From), B * FMath::Sin(From)));
				Outline.ArcAbout(FVector2D::ZeroVector, A, B, Mid - Beyond, Mid + Beyond).Close();
				OnBasin(Out.Add_GetRef(Drawn(bEast ? TEXT("pond.shelf.east") : TEXT("pond.shelf.west"), TEXT("pond.bed"),
					Slab(MoveTemp(Outline), {}, Plan::BedTop, Plan::ShelfTop), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::BedTop)))));
			}
		}

		/**
		 * The coping's sixteen blocks and the frieze carved on each block's die. The frieze's drawing is laid along each
		 * half from the east keystone, measured on the die's parallel, and mirrored north to south.
		 */
		void Coping(TArray<FFormDrawing>& Out)
		{
			const double Die = Pond().Length(Plan::DieD);
			const TSharedRef<const FFriezeDesign> Design = MakeShared<const FFriezeDesign>(0.5 * Die, Plan::BandTop - Plan::BandFoot);
			for (int32 K = 0; K < Plan::CopingBlocks; ++K)
			{
				const FBlock Block = BlockOf(K);
				FFormSweep Stone;
				Stone.Section = CopingSection();
				Stone.Path = EdgeBetween(Block);
				OnBasin(Out.Add_GetRef(Drawn(*FString::Printf(TEXT("pond.coping.%02d"), K), TEXT("pond.coping"), MoveTemp(Stone), Solid)));

				FFormRelief Frieze;
				Frieze.Path = EdgeBetween(Block);
				Frieze.OffsetMeters = Plan::DieD;
				Frieze.BottomMeters = Plan::BandFoot;
				Frieze.TopMeters = Plan::BandTop;
				Frieze.GridMeters = Plan::FriezeGrid;
				Frieze.BackMeters = FriezeBack;
				const double Start = Northwards(Block.Tau0, 0.0);
				Frieze.HeightMeters = [Design, Start, Die](double U, double V)
				{
					// The relief's U runs along the water's edge; the drawing is laid out on the die's parallel.
					const double Tau = Pond().TAt(Start + U, 0.0);
					const double Along = FMath::Fmod(Northwards(Tau, Plan::DieD) + Die, Die);
					return Design->Height(FMath::Min(Along, Die - Along), V - Plan::BandFoot);
				};
				OnBasin(Out.Add_GetRef(Drawn(*FString::Printf(TEXT("pond.frieze.%02d"), K), TEXT("pond.frieze"), MoveTemp(Frieze), Solid)));
			}

			FFormSweep Lip;
			Lip.Section = LipSection();
			Lip.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::PondA, Plan::PondB, 0.0, false);
			OnBasin(Out.Add_GetRef(Drawn(TEXT("pond.lip"), TEXT("pond.lip"), MoveTemp(Lip), Solid)));
		}

		/**
		 * The four posts, each of three nested stages collapsed in its sleeve: two tubes, each the bore of the next, round a
		 * solid ram. Each stage rides its own body, which rises with the pond as its mechanism follows the pond's height.
		 */
		void Posts(TArray<FFormDrawing>& Out)
		{
			const int32 Stages = int32(UE_ARRAY_COUNT(Plan::StageDiameters));
			for (const double X : {-Plan::PostX, Plan::PostX})
			{
				for (const double Y : {-Plan::PostY, Plan::PostY})
				{
					for (int32 Stage = 0; Stage < Stages; ++Stage)
					{
						const double Outer = 0.5 * Plan::StageDiameters[Stage];
						const double Inner = Stage + 1 < Stages ? 0.5 * Plan::StageDiameters[Stage + 1] : 0.0;
						FFormLathe Tube;
						Tube.Section = Inner > 0.0 ? FFormProfile::Rectangle(FVector2D(Inner, -Plan::StageLength), FVector2D(Outer, 0.0))
							: FFormProfile::Polyline({FVector2D(0.0, -Plan::StageLength), FVector2D(Outer, -Plan::StageLength), FVector2D(Outer, 0.0),
								FVector2D(0.0, 0.0)}, false);
						const FString Name = FString::Printf(TEXT("pond.post.%s%s.stage-%d"), Y < 0.0 ? TEXT("north") : TEXT("south"),
							X < 0.0 ? TEXT("west") : TEXT("east"), Stage + 1);
						Out.Add_GetRef(Drawn(*Name, TEXT("pond.post"), MoveTemp(Tube), Solid, FMuseeFrame(FVector(X, Y, 0.0)))).Body = PostStageBody(Stage);
					}
				}
			}
		}
	}

	void DrawPond(TArray<FFormDrawing>& Out)
	{
		Vessel(Out);
		Coping(Out);
		Posts(Out);
	}
}
