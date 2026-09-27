#include "AlbionParts.h"

/**
 * The columns (§3.4): ten on lines 2 to 6 of the arcades, each a monolith of its own British stone, A to E on the west
 * and F to J on the east, on a Bath stone base of a chamfered plinth and a turned moulding, under a Bath stone capital
 * of necking, bell and abacus (boasted tonight: the carving waits, §12); and four half-columns against the end walls on
 * lines 1 and 7, their shafts in the dressing, turned only on the court's side.
 */
namespace Musee::Albion
{
	namespace
	{
		/** A column: its name, where it stands, its shaft's surface, and for a half-column the side of its wall. */
		struct FColumn
		{
			FString Name;
			FVector2D At;
			FString Shaft;
			/** 0 for a whole column; +1 for a half-column whose court lies south (+y), -1 north. */
			double Half = 0.0;
		};

		TArray<FColumn> Columns()
		{
			TArray<FColumn> All;
			for (int32 Line = 2; Line <= Plan::Lines - 1; ++Line)
			{
				const TCHAR* West = Plan::WestStones[Line - 2];
				const TCHAR* East = Plan::EastStones[Line - 2];
				All.Add({FString::Printf(TEXT("column.%s"), West), FVector2D(-Plan::ArcadeX, Plan::LineY(Line)), FString::Printf(TEXT("shaft.%s"), West)});
				All.Add({FString::Printf(TEXT("column.%s"), East), FVector2D(Plan::ArcadeX, Plan::LineY(Line)), FString::Printf(TEXT("shaft.%s"), East)});
			}
			All.Add({TEXT("column.nw"), FVector2D(-Plan::ArcadeX, -Plan::EndInnerY), TEXT("dressing"), 1.0});
			All.Add({TEXT("column.ne"), FVector2D(Plan::ArcadeX, -Plan::EndInnerY), TEXT("dressing"), 1.0});
			All.Add({TEXT("column.sw"), FVector2D(-Plan::ArcadeX, Plan::EndInnerY), TEXT("dressing"), -1.0});
			All.Add({TEXT("column.se"), FVector2D(Plan::ArcadeX, Plan::EndInnerY), TEXT("dressing"), -1.0});
			return All;
		}

		/** The plinth's plan: the square with its corners chamfered, or its court half against the wall. */
		TArray<FVector2D> PlinthPlan(const FColumn& C)
		{
			const double H = Plan::PlinthHalf;
			const double K = Plan::PlinthChamfer;
			if (C.Half == 0.0)
			{
				return {C.At + FVector2D(H - K, -H), C.At + FVector2D(H, -H + K), C.At + FVector2D(H, H - K), C.At + FVector2D(H - K, H),
					C.At + FVector2D(-H + K, H), C.At + FVector2D(-H, H - K), C.At + FVector2D(-H, -H + K), C.At + FVector2D(-H + K, -H)};
			}
			const double S = C.Half;
			return {C.At + FVector2D(-H, 0.0), C.At + FVector2D(H, 0.0), C.At + FVector2D(H, S * (H - K)), C.At + FVector2D(H - K, S * H),
				C.At + FVector2D(-H + K, S * H), C.At + FVector2D(-H, S * (H - K))};
		}

		/** The capital's necking and bell as one turned section, (r, z): the bell's radius 0.255 + 0.145 t^1.8. */
		FFormProfile BellSection()
		{
			FFormProfile P = Turned(Plan::NeckingPoints);
			for (int32 Station = 1; Station <= Plan::BellStations; ++Station)
			{
				const double T = double(Station) / Plan::BellStations;
				const double R = Plan::BellRadius + Plan::BellFlare * FMath::Pow(T, Plan::BellExponent);
				P.LineTo(FVector2D(R, FMath::Lerp(Plan::BellFoot, Plan::BellTop, T)), Station == 1 ? EFormJoint::Crisp : EFormJoint::Smooth);
			}
			P.LineTo(FVector2D(0.0, Plan::BellTop), EFormJoint::Crisp);
			return P;
		}

		/**
		 * The abacus's moulding in section, (out from the core's face, z): the cavetto from the core out to 0.415 at 6.115,
		 * the half-round, the fillet, the face and its chamfer, and back over the top to the core.
		 */
		FFormProfile AbacusSection()
		{
			const double Core = Plan::AbacusCoreHalf;
			const double Reach = Plan::CavettoHalf - Core;
			FFormProfile P = FFormProfile::From(FVector2D(0.0, Plan::AbacusFoot));
			P.ArcAbout(FVector2D(Reach, Plan::AbacusFoot), Reach, Plan::CavettoTop - Plan::AbacusFoot, 180.0, 90.0, EFormJoint::Crisp);
			const double Bead = Plan::AbacusBeadRadius;
			P.ArcAbout(FVector2D(Reach, Plan::CavettoTop + Bead), Bead, Bead, -90.0, 90.0, EFormJoint::Crisp);
			P.LineTo(FVector2D(Plan::AbacusFillet[0][0] - Core, Plan::AbacusFillet[0][1]), EFormJoint::Crisp);
			P.LineTo(FVector2D(Plan::AbacusFillet[1][0] - Core, Plan::AbacusFillet[1][1]), EFormJoint::Crisp);
			const double Face = Plan::AbacusHalf - Core;
			P.LineTo(FVector2D(Face, Plan::AbacusFaceTop), EFormJoint::Crisp);
			P.LineTo(FVector2D(Face - Plan::AbacusChamfer, Plan::CapitalTop), EFormJoint::Crisp);
			P.LineTo(FVector2D(0.0, Plan::CapitalTop), EFormJoint::Crisp);
			return P.Close();
		}

		/** The abacus's core and the path its moulding runs round: the square, or its court half from the wall and back. */
		void Abacus(const FColumn& C, TArray<FFormDrawing>& Out)
		{
			const double H = Plan::AbacusCoreHalf;
			TArray<FVector2D> Path;
			FVector Min(C.At.X - H, C.At.Y - H, Plan::AbacusFoot);
			FVector Max(C.At.X + H, C.At.Y + H, Plan::CapitalTop);
			if (C.Half == 0.0)
			{
				// Anticlockwise on the plan, so the moulding lies outside the core.
				Path = {C.At + FVector2D(H, H), C.At + FVector2D(H, -H), C.At + FVector2D(-H, -H), C.At + FVector2D(-H, H)};
			}
			else if (C.Half > 0.0)
			{
				Min.Y = C.At.Y;
				Path = {C.At + FVector2D(-H, 0.0), C.At + FVector2D(-H, H), C.At + FVector2D(H, H), C.At + FVector2D(H, 0.0)};
			}
			else
			{
				Max.Y = C.At.Y;
				Path = {C.At + FVector2D(H, 0.0), C.At + FVector2D(H, -H), C.At + FVector2D(-H, -H), C.At + FVector2D(-H, 0.0)};
			}
			Out.Add(Box(C.Name + TEXT(".abacus"), TEXT("capital"), Min, Max, Solid));
			FFormSweep Moulding;
			Moulding.Section = AbacusSection();
			Moulding.Path = FFormProfile::Polyline(Path, C.Half == 0.0);
			Out.Add(Drawn(C.Name + TEXT(".abacus.moulding"), TEXT("capital"), MoveTemp(Moulding), Solid));
		}

		/** A column: plinth, base, shaft, necking and bell, abacus; a half-column turns only its court half. */
		void Column(const FColumn& C, TArray<FFormDrawing>& Out)
		{
			// A half-column on the north wall turns east through south to west (0 to 180); on the south, west through north.
			const double From = C.Half < 0.0 ? 180.0 : 0.0;
			const double To = C.Half == 0.0 ? 360.0 : From + 180.0;
			// A half turn's section is closed along the axis, so its two cut faces close it.
			auto Turn = [&C](FFormProfile Profile)
			{
				if (C.Half != 0.0)
				{
					Profile.Close();
				}
				return Profile;
			};
			const FVector Foot(C.At.X, C.At.Y, 0.0);

			FFormExtrusion Plinth;
			Plinth.Outline = FFormProfile::Polygon(PlinthPlan(C));
			Plinth.DepthMeters = Plan::PlinthTop;
			Out.Add(Drawn(C.Name + TEXT(".plinth"), TEXT("dressing"), MoveTemp(Plinth), Solid, FMuseeFrame(Foot)));
			Out.Add(Lathe(C.Name + TEXT(".base"), TEXT("dressing"), Foot, Turn(Turned(Plan::BasePoints)), Solid, From, To));

			FFormProfile Shaft = FFormProfile::From(FVector2D(0.0, Plan::ShaftFoot));
			Shaft.LineTo(FVector2D(Plan::ShaftRadius, Plan::ShaftFoot), EFormJoint::Crisp).LineTo(FVector2D(Plan::ShaftRadius, Plan::ShaftTop), EFormJoint::Crisp)
				.LineTo(FVector2D(0.0, Plan::ShaftTop), EFormJoint::Crisp);
			Out.Add(Lathe(C.Name + TEXT(".shaft"), C.Shaft, Foot, Turn(MoveTemp(Shaft)), Solid, From, To));
			Out.Add(Lathe(C.Name + TEXT(".bell"), TEXT("capital"), Foot, Turn(BellSection()), Solid, From, To));
			Abacus(C, Out);
		}
	}

	void DrawColumns(TArray<FFormDrawing>& Out)
	{
		for (const FColumn& C : Columns())
		{
			Column(C, Out);
		}
	}
}
