#include "ChenghuaiHallTrades.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"

/**
 * A hall's walls (§3.5; ChenghuaiHall.cpp:912-1021): the hard gables (硬山) of brick from the eave's front to the back wall,
 * rising 1.5 cm into the boarding, on a base (下碱) of rubbed brick standing 15 mm proud outside, with the corbelled front
 * end (墀头) under the eave and a lime skin inside the room; framed openings through them; the back wall between the
 * gables, rising under the roof's slope, and a sealed eave's corbelled cornice (冰盘檐).
 */
namespace Musee::Chenghuai::Hall
{
	namespace
	{
		/** An opening's notch in a wall's (v, z) outline, from the wall's foot Z0 up to its head. */
		struct FNotch
		{
			double V0 = 0.0;
			double V1 = 0.0;
			double Head = 0.0;
		};

		/**
		 * A wall's elevation in (v, z): along its foot at Z0 from V0 to V1, notched up to each opening's head, then back along
		 * its top, which follows the roof's underside.
		 */
		TArray<FVector2D> Elevation(double V0, double V1, double Z0, TConstArrayView<FNotch> Notches, TFunctionRef<double(double)> Top,
			TConstArrayView<double> Knots)
		{
			TArray<FVector2D> Points = {FVector2D(V0, Z0)};
			for (const FNotch& Notch : Notches)
			{
				Points.Add(FVector2D(Notch.V0, Z0));
				Points.Add(FVector2D(Notch.V0, Notch.Head));
				Points.Add(FVector2D(Notch.V1, Notch.Head));
				Points.Add(FVector2D(Notch.V1, Z0));
			}
			Points.Add(FVector2D(V1, Z0));
			for (const double V : Stations(V1, V0, Plan::SlabStation, Knots))
			{
				Points.Add(FVector2D(V, Top(V)));
			}
			return Points;
		}

		/** A gable at one end: its base in runs between the openings, its body, its skin, the openings' frames, its 墀头. */
		void Gable(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& S, bool bAtU1)
		{
			const FBuildingFrame& F = Spec.Frame;
			const FString Part = FString::Printf(TEXT("gable.%d"), bAtU1 ? 2 : 1);
			// Outer and inner faces along u; the base stands proud of the outer face.
			const double Outer = bAtU1 ? Spec.U1 : Spec.U0;
			const double Inner = bAtU1 ? Spec.U1 - Spec.GableT : Spec.U0 + Spec.GableT;
			const double Outward = bAtU1 ? 1.0 : -1.0;
			const double VFront = Plan::GableFront;
			const double VBack = Spec.GableBackV > 0.0 ? Spec.GableBackV : Spec.BackWallV1;
			const double ZBase = Spec.Floor + Plan::WallBase;
			TArray<FNotch> Notches;
			for (const FGableOpening& Opening : Spec.GableOpenings)
			{
				if (Opening.bAtU1 == bAtU1)
				{
					Notches.Add({Opening.V0, Opening.V1, Spec.Floor + Opening.Top});
				}
			}
			Notches.Sort([](const FNotch& A, const FNotch& B) { return A.V0 < B.V0; });
			const TArray<double> Knots = {S.EaveR, S.BackR};
			auto Top = [&S](double V) { return S.Under(V) + Plan::GableIntoBoarding; };
			AlongU(Out, Named(Spec, Part + TEXT(".body")), Spec.WallBody, F, Elevation(VFront, VBack, ZBase, Notches, Top, Knots), {}, Outer, Inner,
				EHit::Solid);

			// The base, run by run between the openings, its ends and outer face standing proud.
			double From = VFront;
			int32 Run = 0;
			auto Base = [&](double To)
			{
				if (To - From > Plan::Skin)
				{
					Block(Out, Named(Spec, FString::Printf(TEXT("%s.base.%d"), *Part, ++Run)), Spec.WallBase, F, FMath::Min(Outer + Outward * Plan::BaseProud, Inner),
						FMath::Max(Outer + Outward * Plan::BaseProud, Inner), From, To, Spec.Floor, ZBase, EHit::Solid);
				}
			};
			for (const FNotch& Notch : Notches)
			{
				Base(Notch.V0);
				From = Notch.V1;
			}
			Base(VBack);

			// The skin inside the room, from behind the lattice to the back wall.
			if (Spec.bPlaster && !Spec.Room.IsEmpty())
			{
				const double V0 = FMath::Max(VFront, Spec.FacadeV + Plan::FramePost);
				const double V1 = Spec.bBackWall ? Spec.BackWallV0 : VBack;
				TArray<FNotch> Inside;
				for (const FNotch& Notch : Notches)
				{
					if (Notch.V1 > V0 && Notch.V0 < V1)
					{
						Inside.Add({FMath::Max(Notch.V0, V0), FMath::Min(Notch.V1, V1), Notch.Head});
					}
				}
				AlongU(Out, Named(Spec, Part + TEXT(".skin")), TEXT("room.plaster"), F, Elevation(V0, V1, Spec.Floor, Inside, Top, Knots), {}, Inner,
					Inner - Outward * Plan::Skin, EHit::Solid);
			}

			// The openings' red frames: a head under the brick and a jamb each side.
			int32 Frame = 0;
			const double U0 = FMath::Min(Outer, Inner) - Plan::Skin;
			const double U1 = FMath::Max(Outer, Inner) + Plan::Skin;
			for (const FNotch& Notch : Notches)
			{
				const FString Name = FString::Printf(TEXT("%s.opening.%d"), *Part, ++Frame);
				Block(Out, Named(Spec, Name + TEXT(".head")), Spec.Timber, F, U0, U1, Notch.V0, Notch.V1, Notch.Head - Plan::OpeningHead, Notch.Head,
					EHit::Solid);
				Block(Out, Named(Spec, Name + TEXT(".jamb.1")), Spec.Timber, F, U0, U1, Notch.V0, Notch.V0 + Plan::OpeningJamb, Spec.Floor,
					Notch.Head - Plan::OpeningHead, EHit::Solid);
				Block(Out, Named(Spec, Name + TEXT(".jamb.2")), Spec.Timber, F, U0, U1, Notch.V1 - Plan::OpeningJamb, Notch.V1, Spec.Floor,
					Notch.Head - Plan::OpeningHead, EHit::Solid);
			}

			// The corbelled front end (墀头): the stone at the base's top, five courses stepping forward and out, the board.
			const double Wide0 = FMath::Min(Outer, Inner);
			const double Wide1 = FMath::Max(Outer, Inner);
			Block(Out, Named(Spec, Part + TEXT(".corbel.stone")), TEXT("stone.kerb"), F, Wide0 - Plan::BaseProud, Wide1 + Plan::BaseProud,
				VFront, VFront + Plan::CorbelStoneDeep, ZBase - Plan::CorbelStone, ZBase, EHit::Solid);
			const double Zt = S.Zc - Plan::CorbelUnderZc;
			for (int32 K = 0; K < Plan::CorbelCourses; ++K)
			{
				const double Reach = Plan::CorbelForward * (K + 1);
				const double Side = Plan::CorbelSideways * (K + 1);
				const double Z0 = Zt + Plan::CorbelCourse * K;
				Block(Out, Named(Spec, FString::Printf(TEXT("%s.corbel.%d"), *Part, K + 1)), Spec.WallBase, F, Wide0 - (bAtU1 ? 0.0 : Side),
					Wide1 + (bAtU1 ? Side : 0.0), VFront - Reach, VFront + Plan::CorbelBack, Z0, Z0 + Plan::CorbelCourse, EHit::Solid);
			}
			const double BoardFoot = Zt + Plan::CorbelCourse * Plan::CorbelCourses;
			const double Front = VFront - Plan::CorbelBoardReach;
			const double Back = VFront + Plan::CorbelBack;
			const TArray<FVector2D> Board = {FVector2D(Front, BoardFoot), FVector2D(Back, BoardFoot), FVector2D(Back, S.Under(Back)),
				FVector2D(Front - Plan::CorbelBoardSlope, S.Under(Front - Plan::CorbelBoardSlope))};
			AlongU(Out, Named(Spec, Part + TEXT(".corbel.board")), Spec.WallBase, F, Board, {}, Wide0 - (bAtU1 ? 0.0 : Plan::PurlinIntoGable),
				Wide1 + (bAtU1 ? Plan::PurlinIntoGable : 0.0), EHit::Solid);
		}
	}

	void DrawGables(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section)
	{
		for (const bool bAtU1 : {false, true})
		{
			if (HasGable(Spec, bAtU1))
			{
				Gable(Out, Spec, Section, bAtU1);
			}
		}
	}

	void DrawBackWall(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& S)
	{
		if (!Spec.bBackWall)
		{
			return;
		}
		const FBuildingFrame& F = Spec.Frame;
		const double A = HasGable(Spec, false) ? Spec.U0 + Spec.GableT : Spec.U0;
		const double B = HasGable(Spec, true) ? Spec.U1 - Spec.GableT : Spec.U1;
		const double V0 = Spec.BackWallV0;
		const double V1 = Spec.BackWallV1;
		const double ZBase = Spec.Floor + Plan::WallBase;
		Block(Out, Named(Spec, TEXT("back-wall.base")), Spec.WallBase, F, A, B, V0, V1 + Plan::BaseProud, Spec.Floor - Plan::BackBaseUnder, ZBase,
			EHit::Solid);
		// The wall rises under the roof: its top follows the boarding's underside across its thickness.
		const TArray<FVector2D> Body = {FVector2D(V0, ZBase), FVector2D(V1, ZBase), FVector2D(V1, S.Under(V1)), FVector2D(V0, S.Under(V0))};
		AlongU(Out, Named(Spec, TEXT("back-wall.body")), Spec.WallBody, F, Body, {}, A, B, EHit::Solid);
		if (Spec.bPlaster && !Spec.Room.IsEmpty())
		{
			const double Face = V0 - Plan::Skin;
			const TArray<FVector2D> Skin = {FVector2D(Face, Spec.Floor), FVector2D(V0, Spec.Floor), FVector2D(V0, S.Under(V0)), FVector2D(Face, S.Under(Face))};
			AlongU(Out, Named(Spec, TEXT("back-wall.skin")), TEXT("room.plaster"), F, Skin, {}, A, B, EHit::Solid);
		}
		if (!Spec.bSealedBack)
		{
			return;
		}
		// The sealed eave's cornice: four courses, each stepping out under the roof's back edge.
		const double Z1 = S.Under(V1);
		for (int32 K = 0; K < Plan::CorniceCourses; ++K)
		{
			const double Za = Z1 - Plan::CorniceCourse * (Plan::CorniceCourses - K);
			const double Reach = FMath::Min(Spec.EaveBack - Plan::CorniceShort, Plan::CorniceStep * (K + 1));
			const double Zb = FMath::Min(Za + Plan::CorniceCourse, S.Under(V1 + Reach));
			Block(Out, Named(Spec, FString::Printf(TEXT("back-wall.cornice.%d"), K + 1)), Spec.WallBase, F, Spec.U0, Spec.U1, V1 - Plan::CorniceBack,
				V1 + Reach, Za, Zb, EHit::Solid);
		}
	}
}
