#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The Ionic order after Vignola, D 0.34, a column nine diameters high (§3.4): 26 columns (the gallery's six pairs and
 * the screen's two in giallo antico, the tribune's ring of twelve in pavonazzetto), each on its square plinth with an
 * Attic base, its shaft with entasis, and its capital: the echinus turned with the astragal, the canalis and plate,
 * a bolster each side with a volute on both its faces, and the abacus. The ten pilasters carry the same order flat on
 * the walls.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		/** The shaft: straight to 1.0983, then the entasis r(t) = 0.170 − 0.02833 (1 − cos πt/2) to r 0.14167 at 2.865. */
		FFormProfile ShaftSection()
		{
			FPen Pen(0.0, Plan::ShaftFoot);
			Pen.To(Plan::ShaftRadius, Plan::ShaftFoot).To(Plan::ShaftRadius, Plan::EntasisFrom);
			for (int32 Station = 1; Station <= Plan::EntasisStations; ++Station)
			{
				const double T = double(Station) / Plan::EntasisStations;
				const double Radius = Plan::ShaftRadius - (Plan::ShaftRadius - Plan::ShaftTopRadius) * (1.0 - FMath::Cos(0.5 * UE_DOUBLE_PI * T));
				Pen.To(Radius, FMath::Lerp(Plan::EntasisFrom, Plan::ShaftTop, T));
			}
			return Pen.To(0.0, Plan::ShaftTop).Closed();
		}

		/** A volute's spiral: 2.5 turns from the top inwards, clockwise (Turn 1) or anticlockwise (−1) seen from its face. */
		FFormProfile Spiral(double Turn)
		{
			const double Turns = Plan::VoluteTurns;
			const double K = FMath::Loge(Plan::VoluteOuter / Plan::VoluteInner) / (Turns * 2.0 * UE_DOUBLE_PI);
			const int32 Stations = FMath::CeilToInt32(Turns * Plan::VoluteStationsPerTurn);
			auto At = [&](int32 Station)
			{
				const double Theta = Turns * 2.0 * UE_DOUBLE_PI * Station / Stations;
				const double Radius = Plan::VoluteOuter * FMath::Exp(-K * Theta);
				const double Angle = 0.5 * UE_DOUBLE_PI - Turn * Theta;
				return FVector2D(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
			};
			FFormProfile Path = FFormProfile::From(At(0));
			for (int32 Station = 1; Station <= Stations; ++Station)
			{
				Path.LineTo(At(Station), EFormJoint::Smooth);
			}
			return Path;
		}

		/** A volute on a face: its spiral fillet swept on the face (B out of it) in a frame whose x is the viewer's right. */
		void Volute(TArray<FFormDrawing>& Out, const FString& Name, const FMuseeFrame& Face, double Turn, bool bEye)
		{
			FFormSweep Fillet;
			Fillet.Section = Rect(-0.5 * Plan::FilletWidth, -Plan::FilletSet, 0.5 * Plan::FilletWidth, Plan::FilletProud);
			Fillet.Path = Spiral(Turn);
			Out.Add(Drawn(Name + TEXT(".fillet"), TEXT("order.carved"), MoveTemp(Fillet), nullptr, Face));
			if (bEye)
			{
				FFormLathe Eye;
				Eye.Section = Written(Plan::VoluteEyeProfile);
				Out.Add(Drawn(Name + TEXT(".eye"), TEXT("order.carved"), MoveTemp(Eye), nullptr, Face));
			}
		}

		/** The abacus: its core, and its edge moulding (0, 0) (0, 0.012) round to (0.015, 0.027) convex (0.015, 0.035) round it. */
		void Abacus(TArray<FFormDrawing>& Out, const FString& Name, const FMuseeFrame& Frame, double Front, double Back, double Half)
		{
			const double E = Plan::AbacusEdge;
			Out.Add(PrismIn(Name + TEXT(".abacus"), TEXT("order.carved"), Frame, Rect(Back, -Half + E, Front - E, Half - E), Plan::AbacusFoot, Plan::AbacusTop,
				nullptr));
			FFormSweep Edge;
			Edge.Section = Written(Plan::AbacusEdgeProfile);
			Edge.Path = RectOutward(Back, -Half + E, Front - E, Half - E);
			Edge.ElevationMeters = Plan::AbacusFoot;
			Out.Add(Drawn(Name + TEXT(".abacus.edge"), TEXT("order.carved"), MoveTemp(Edge), nullptr, Frame));
		}

		/** A column standing at a plan point, its volutes facing along φ and back. */
		void Column(TArray<FFormDrawing>& Out, const FString& Name, const FVector2D& At, double FacingDeg, const TCHAR* Shaft)
		{
			const FVector Foot(At.X, At.Y, 0.0);
			const FMuseeFrame Frame = Turned(Foot, FacingDeg);
			const double P = 0.5 * Plan::PlinthSide;
			Out.Add(PrismIn(Name + TEXT(".plinth"), TEXT("order.carved"), Frame, Rect(-P, -P, P, P), -Plan::Sink, Plan::PlinthTop, Solid));
			FFormLathe Base;
			Base.Section = Written(Plan::ColumnBase);
			Out.Add(Drawn(Name + TEXT(".base"), TEXT("order.carved"), MoveTemp(Base), Solid, Frame));
			FFormLathe ShaftLathe;
			ShaftLathe.Section = ShaftSection();
			Out.Add(Drawn(Name + TEXT(".shaft"), Shaft, MoveTemp(ShaftLathe), Solid, Frame));
			FFormLathe Echinus;
			Echinus.Section = Written(Plan::ColumnEchinus);
			Out.Add(Drawn(Name + TEXT(".echinus"), TEXT("order.carved"), MoveTemp(Echinus), nullptr, Frame));

			const double CA = 0.5 * Plan::CanalisAcross;
			const double CD = 0.5 * Plan::CanalisDeep;
			Out.Add(PrismIn(Name + TEXT(".canalis"), TEXT("order.carved"), Frame, Rect(-CD, -CA, CD, CA), Plan::CanalisFoot, Plan::CanalisTop, nullptr));
			const double PA = 0.5 * Plan::PlateAcross;
			const double PD = 0.5 * Plan::PlateDeep;
			Out.Add(PrismIn(Name + TEXT(".plate"), TEXT("order.carved"), Frame, Rect(-PD, -PA, PD, PA), Plan::PlateFoot, Plan::PlateTop, nullptr));

			const FVector Front = Frame.DirectionToParent(FVector::ForwardVector);
			const FVector Side = FVector::CrossProduct(FVector::UpVector, Front);
			for (const double S : {-1.0, 1.0})
			{
				const FVector Axis = Foot + Side * (S * Plan::BolsterOffset) + FVector::UpVector * Plan::CanalisFoot;
				FFormLathe Bolster;
				const double H = Plan::BolsterHalf;
				const double B = Plan::BolsterBeltHalf;
				Bolster.Section = FPen(0.0, -H).To(Plan::BolsterRadius, -H).To(Plan::BolsterWaist, -B).To(Plan::BolsterBelt, -B).To(Plan::BolsterBelt, B)
					.To(Plan::BolsterWaist, B).To(Plan::BolsterRadius, H).To(0.0, H).Closed();
				const TCHAR* SideName = S > 0.0 ? TEXT("right") : TEXT("left");
				Out.Add(Drawn(FString::Printf(TEXT("%s.bolster.%s"), *Name, SideName), TEXT("order.carved"), MoveTemp(Bolster), nullptr,
					FrameOfAxes(Axis, Side, FVector::UpVector)));
				for (const double F : {-1.0, 1.0})
				{
					// Seen from this face the viewer's right is Up × (the face's normal): the volute there winds clockwise.
					const FVector Normal = Front * F;
					const FMuseeFrame Face = FrameOfAxes(Axis + Normal * H, FVector::CrossProduct(FVector::UpVector, Normal), FVector::UpVector);
					Volute(Out, FString::Printf(TEXT("%s.volute.%s.%s"), *Name, SideName, F > 0.0 ? TEXT("front") : TEXT("back")), Face, S * F > 0.0 ? 1.0 : -1.0,
						true);
				}
			}
			const double A = 0.5 * Plan::AbacusSide;
			Abacus(Out, Name, Frame, A, -A + Plan::AbacusEdge, A);
		}

		/**
		 * A pilaster on a wall: its shaft block 0.07 proud × 0.34 from 0.20 to 2.90, the Attic base and the neck swept
		 * round its three faces (from inside the wall), and its capital: the canalis, two flat volute discs with their
		 * fillets, and the abacus. Its frame stands on the wall's face, x out of the wall.
		 */
		void Pilaster(TArray<FFormDrawing>& Out, const FString& Name, const FMuseeFrame& Frame, const TCHAR* Shaft)
		{
			const double Proud = Plan::PilasterProud;
			const double Half = Plan::PilasterHalf;
			const double Inside = -Plan::PilasterReturnInside;
			Out.Add(PrismIn(Name + TEXT(".shaft"), Shaft, Frame, Rect(-Plan::Sink, -Half, Proud, Half), Plan::PilasterFoot, Plan::PilasterTop, Solid));
			const FVector2D Round3[] = {{Inside, Half}, {Proud, Half}, {Proud, -Half}, {Inside, -Half}};
			FFormSweep Base;
			Base.Section = Written(Plan::PilasterBase);
			Base.Path = FFormProfile::Polyline(Round3, false);
			Out.Add(Drawn(Name + TEXT(".base"), TEXT("order.carved"), MoveTemp(Base), Solid, Frame));
			FFormSweep Neck;
			Neck.Section = Written(Plan::PilasterNeck);
			Neck.Path = FFormProfile::Polyline(Round3, false);
			Out.Add(Drawn(Name + TEXT(".neck"), TEXT("order.carved"), MoveTemp(Neck), nullptr, Frame));

			const double CA = 0.5 * Plan::CanalisAcross;
			Out.Add(PrismIn(Name + TEXT(".canalis"), TEXT("order.carved"), Frame, Rect(-Plan::Sink, -CA, Plan::PilasterCanalisProud, CA), Plan::CanalisFoot,
				Plan::CanalisTop, nullptr));
			const FVector Outward = Frame.DirectionToParent(FVector::ForwardVector);
			const FVector Right = FVector::CrossProduct(FVector::UpVector, Outward);
			for (const double S : {-1.0, 1.0})
			{
				const FVector Centre = Frame.PointToParent(FVector(Plan::PilasterCanalisProud, S * Plan::BolsterOffset, Plan::CanalisFoot));
				const FMuseeFrame Disc = FrameOfAxes(Centre, Right, FVector::UpVector);
				// Seen from the room the viewer's right is Right: the volute on that side winds clockwise.
				const bool bRight = FVector::DotProduct(Centre - Frame.OriginMeters, Right) > 0.0;
				const FString DiscName = FString::Printf(TEXT("%s.volute.%s"), *Name, bRight ? TEXT("right") : TEXT("left"));
				FFormLathe Shape;
				Shape.Section = Rect(0.0, 0.0, Plan::BolsterRadius, Plan::PilasterDiscProud - Plan::PilasterCanalisProud);
				Out.Add(Drawn(DiscName + TEXT(".disc"), TEXT("order.carved"), MoveTemp(Shape), nullptr, Disc));
				const FMuseeFrame Face(Disc.PointToParent(FVector(0.0, 0.0, Plan::PilasterDiscProud - Plan::PilasterCanalisProud)), Disc.Rotation);
				Volute(Out, DiscName, Face, bRight ? 1.0 : -1.0, false);
			}
			Abacus(Out, Name, Frame, Plan::PilasterAbacusProud, -Plan::Sink, Plan::PilasterAbacusHalf);
		}
	}

	void DrawOrder(TArray<FFormDrawing>& Out)
	{
		for (const double Side : {-1.0, 1.0})
		{
			const TCHAR* Wall = Side > 0.0 ? TEXT("east") : TEXT("west");
			for (int32 Pair = 0; Pair < int32(UE_ARRAY_COUNT(Plan::PairCentres)); ++Pair)
			{
				for (const double Along : {1.0, -1.0})
				{
					const FVector2D At(Side * Plan::PairX, Plan::PairCentres[Pair] + Along * Plan::PairHalf);
					Column(Out, FString::Printf(TEXT("gallery.column.%s.%d%s"), Wall, Pair + 1, Along > 0.0 ? TEXT("s") : TEXT("n")), At, Side > 0.0 ? 180.0 : 0.0,
						TEXT("shaft.giallo"));
				}
			}
			Column(Out, FString::Printf(TEXT("screen.column.%s"), Wall), FVector2D(Side * Plan::ScreenColumnX, Plan::ScreenColumnY), 90.0, TEXT("shaft.giallo"));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::GalleryPilasterY)); ++Index)
			{
				const FVector Face(Side * Plan::GalleryHalf, Plan::GalleryPilasterY[Index], 0.0);
				Pilaster(Out, FString::Printf(TEXT("gallery.pilaster.%s.%d"), Wall, Index + 1), Turned(Face, Side > 0.0 ? 180.0 : 0.0), TEXT("shaft.giallo"));
			}
		}
		for (int32 Index = 0; Index < Plan::RingColumns; ++Index)
		{
			const double Deg = Plan::RingFirstColumnDeg + Index * 360.0 / Plan::RingColumns;
			Column(Out, FString::Printf(TEXT("tribune.column.%02d"), Index + 1), Polar(Plan::RingRadius, Deg), Deg, TEXT("shaft.pavonazzetto"));
		}
		for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::TribunePilasterDeg)); ++Index)
		{
			const double Deg = Plan::TribunePilasterDeg[Index];
			const FVector2D Face = Polar(Plan::TribuneRadius, Deg);
			Pilaster(Out, FString::Printf(TEXT("tribune.pilaster.%d"), Index + 1), Turned(FVector(Face.X, Face.Y, 0.0), Deg + 180.0), TEXT("shaft.pavonazzetto"));
		}
	}
}
