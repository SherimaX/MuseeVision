#include "Facade/MuseeFacadeStructure.h"

#include "Chenghuai/ChenghuaiPlan.h"

#include "Components/SpotLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Facade/FacadeKit.h"
#include "Facade/FacadeOrder.h"
#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"

/**
 * The layout, face by face. Plan metres (x east, plan y south, up); each face line runs so that its left normal points
 * out of the building (south faces run east, east faces north, north faces west, west faces south; the drum's arcs by
 * decreasing angle). d is measured out from the existing outer face.
 */
namespace FacadeBuild
{
	namespace FP = MuseePlan::Facade;
	using namespace FacadeKit;
	using namespace FacadeOrder;
	using EK = FOpening::EKind;

	constexpr double kSkin = FP::Skin;
	constexpr double kBack = FP::WindowBack;
	constexpr double kFoot = FP::Podium - 0.05;   // the skins start inside the podium's cap

	// Chenghuai: the Sculpture Hall is retired and the north door opens onto Chenghuai's open porch (grey brick walls to
	// |x| 2.5, Chenghuai/ChenghuaiPlan.h): no Sculpture Hall dress, no north passage; the podium stops at the porch's walls
	// and the drum's skin frames the door's arch down to the ground. Chenghuai::bNorthDoorOpen switches it.
	constexpr bool kSculptureHallDress = !Chenghuai::bNorthDoorOpen;
	constexpr double kNorthPorchHalf = 2.5, kNorthDoorHalf = 2.0, kNorthDoorSpring = 4.5;

	FOrder MakeOrder(double D)
	{
		FOrder O;
		O.D = D;
		O.Base = FP::Podium;
		O.Skin = kSkin;
		return O;
	}

	/** A constant height, for a skin's bottom. */
	struct FFlat
	{
		double Z;
		double operator()(double) const { return Z; }
	};

	double AsinDeg(double X) { return FMath::RadiansToDegrees(FMath::Asin(X)); }

	// ------------------------------------------------------------------------------------------------ the drum's junctions

	/** Plan angles (from east towards south) where the Rotunda's drum meets what joins it. */
	struct FDrumAngles
	{
		double E = 26.5;   // the Hall of Light: its kerbs reach |y| 4.8
		double S0, S1, W0, W1, N0, N1;
		FDrumAngles()
		{
			const double R = FP::DrumRadius;
			S0 = 90.0 - AsinDeg(FP::VestibuleHalf / R);
			S1 = 90.0 + AsinDeg(FP::VestibuleHalf / R);
			W0 = 180.0 - AsinDeg(FP::WestPassageHalf / R);
			W1 = 180.0 + AsinDeg(FP::WestPassageHalf / R);
			const double NH = kSculptureHallDress ? FP::NorthPassageHalf : kNorthPorchHalf;   // Chenghuai
			N0 = 270.0 - AsinDeg(NH / R);
			N1 = 270.0 + AsinDeg(NH / R);
		}
	};

	/** Over the Hall of Light, the drum's skin starts clear of its vault (R 5.3 about h 2.2), eaves and collars. */
	double OverHall(double Y)
	{
		const double A = FMath::Abs(Y);
		if (A >= 4.85) { return -0.05; }
		if (A >= 4.5) { return 5.35; }
		return 2.2 + FMath::Sqrt(5.3 * 5.3 - A * A) + 0.25;
	}

	// ------------------------------------------------------------------------------------------------ the oval

	FVector2D OvalIn(double T) { return FVector2D(FP::OvalX + FP::OvalA * FMath::Cos(T), FP::OvalB * FMath::Sin(T)); }

	FVector2D OvalOut(double T, double Wall = FP::OvalWall)
	{
		const FVector2D P = OvalIn(T);
		const FVector2D G = FVector2D((P.X - FP::OvalX) / (FP::OvalA * FP::OvalA), P.Y / (FP::OvalB * FP::OvalB)).GetSafeNormal();
		return P + G * Wall;
	}

	/** The parameter on the east side where the oval's outer face reaches plan y = Y (> 0). */
	double OvalEastT(double Y)
	{
		double Lo = 0.0, Hi = 0.5 * Pi;
		for (int32 i = 0; i < 80; ++i)
		{
			const double Mid = 0.5 * (Lo + Hi);
			if (OvalOut(Mid).Y < Y) { Lo = Mid; }
			else { Hi = Mid; }
		}
		return 0.5 * (Lo + Hi);
	}

	/** The oval's outer face from the hyphen's north side (y −5.5) west round to its south side (y 5.5). */
	TArray<FVector2D> OvalWest(double Wall = FP::OvalWall)
	{
		const double Te = OvalEastT(FP::HyphenHalf);
		constexpr int32 N = 480;
		TArray<FVector2D> Out;
		for (int32 i = 0; i <= N; ++i) { Out.Add(OvalOut(-Te - (2.0 * Pi - 2.0 * Te) * i / N, Wall)); }
		return Out;
	}

	/** … and its east side between the hyphen's two faces (the hyphen's roof meets it). */
	TArray<FVector2D> OvalEast(double Wall)
	{
		const double Te = OvalEastT(FP::HyphenHalf);
		constexpr int32 N = 60;
		TArray<FVector2D> Out;
		for (int32 i = 0; i <= N; ++i) { Out.Add(OvalOut(-Te + 2.0 * Te * i / N, Wall)); }
		return Out;
	}

	// ------------------------------------------------------------------------------------------------ helpers

	/** Pedestals over the given stations, and balusters between neighbours. */
	void BalustradeFace(FSink& S, const FBalustrade& B, const FFaceLine& F, const TArray<double>& Pedestals)
	{
		for (const double U : Pedestals) { Pedestal(S, B, F.Local(U)); }
		for (int32 i = 0; i + 1 < Pedestals.Num(); ++i)
		{
			Balusters(S, B, F, Pedestals[i] + 0.5 * B.PedestalWidth + 0.03, Pedestals[i + 1] - 0.5 * B.PedestalWidth - 0.03);
		}
	}

	/**
	 * A corner block where two balustrades meet at a convex corner (the end of face F at UC), from the corner's first
	 * pedestal round the corner; an urn on it when UrnHeight > 0.
	 */
	void CornerBlock(FSink& S, const FBalustrade& B, const FFaceLine& F, double UC, double Reach, double UrnHeight)
	{
		const FLocal L = F.Local(UC);
		const double Fr = B.Front, K = B.Front - B.Width, H = B.H;
		LBox(S.Carved, L, -Reach - 0.04, Fr + 0.07, K - 0.07, Fr + 0.07, B.Z0 - 0.03, B.Z0 + 0.12 * H);
		LBox(S.Carved, L, -Reach, Fr + 0.03, K - 0.03, Fr + 0.03, B.Z0 + 0.10 * H, B.Z0 + 0.86 * H);
		LBox(S.Carved, L, -Reach - 0.06, Fr + 0.09, K - 0.09, Fr + 0.09, B.Z0 + 0.84 * H, B.Z0 + H + 0.04);
		if (UrnHeight > 0.0)
		{
			const double C = 0.5 * (Fr + K);
			Urn(S.Carved, L.At(C, C, B.Z0 + H + 0.04), UrnHeight);
		}
	}

	TArray<FOpening> OpeningsOf(const TArray<FWindow>& Ws)
	{
		TArray<FOpening> Out;
		for (const FWindow& W : Ws) { Out.Add(W.Open); }
		return Out;
	}

	TArray<FWindow> OnFace(const TArray<FWindow>& Ws, double U0, double U1)
	{
		TArray<FWindow> Out;
		for (const FWindow& W : Ws)
		{
			if (W.Open.U > U0 && W.Open.U < U1) { Out.Add(W); }
		}
		return Out;
	}

	// ================================================================================================ the podium

	void BuildPodium(FSink& S)
	{
		const FDrumAngles A;
		const FVector2D C(0.0, 0.0);
		const double R = FP::DrumRadius;
		const double X0 = FP::SalonWest, X1 = FP::SalonEast, Y = FP::SalonHalf;
		const double SX = FP::SculptureEast, SN = FP::SculptureNorth, SS = FP::SculptureSouth;
		const double NP = FP::NorthPassageHalf, WP = FP::WestPassageHalf, VH = FP::VestibuleHalf, H5 = FP::HyphenHalf;

		// One run from the Hall of Light's north side round the Sculpture Hall, the Salon, the oval and back round the
		// drum to the Chinese Wing's vestibule; a second from the vestibule's east side to the Hall of Light's south.
		TArray<FVector2D> Main, NorthEast;
		if (kSculptureHallDress)
		{
			AppendPath(Main, ArcPoints(C, R, 360.0 - A.E, A.N1));
			AppendPath(Main, {FVector2D(NP, SS), FVector2D(SX, SS), FVector2D(SX, SN), FVector2D(-SX, SN), FVector2D(-SX, SS), FVector2D(-NP, SS)});
		}
		else
		{
			// Chenghuai: the podium stops either side of the north porch.
			AppendPath(NorthEast, ArcPoints(C, R, 360.0 - A.E, A.N1));
		}
		AppendPath(Main, ArcPoints(C, R, A.N0, A.W1));
		AppendPath(Main, {FVector2D(X1, -WP), FVector2D(X1, -Y), FVector2D(FP::CabinetEast, -Y), FVector2D(FP::CabinetEast, FP::CabinetNorth),
						  FVector2D(FP::CabinetWest, FP::CabinetNorth), FVector2D(FP::CabinetWest, -Y), FVector2D(X0, -Y), FVector2D(X0, -H5)});
		AppendPath(Main, OvalWest());
		AppendPath(Main, {FVector2D(X0, H5), FVector2D(X0, Y), FVector2D(X1, Y), FVector2D(X1, WP)});
		AppendPath(Main, ArcPoints(C, R, A.W0, A.S1));
		// (Albion: the podium stops at the drum, where Albion's porch joins it.)

		TArray<FVector2D> East;
		AppendPath(East, ArcPoints(C, R, A.S0, A.E));

		const FProfile Pod = PodiumProfile();
		CapEnds(S.Base, PlanSweep(S.Base, Main, false, Pod), Pod, true, false);
		CapEnds(S.Base, PlanSweep(S.Base, East, false, Pod), Pod, false, true);
		if (NorthEast.Num() > 1) { CapEnds(S.Base, PlanSweep(S.Base, NorthEast, false, Pod), Pod, true, true); }

		// The gravel strip at its foot, with a stone edge.
		FProfile Gravel;
		Gravel.bClosed = true;
		Gravel.Add(0.50, -0.10).Add(1.45, -0.10).Add(1.45, 0.012).Add(0.50, 0.012);
		FProfile Edge;
		Edge.bClosed = true;
		Edge.Add(1.44, -0.14).Add(1.56, -0.14).Add(1.56, 0.035).Add(1.55, 0.045).Add(1.45, 0.045).Add(1.44, 0.035);
		for (const TArray<FVector2D>* Path : {&Main, &East, &NorthEast})
		{
			if (Path->Num() < 2) { continue; }
			const bool bMain = Path == &Main, bNE = Path == &NorthEast;
			CapEnds(S.Gravel, PlanSweep(S.Gravel, *Path, false, Gravel), Gravel, bMain || bNE, !bMain);
			CapEnds(S.Base, PlanSweep(S.Base, *Path, false, Edge), Edge, bMain || bNE, !bMain);
		}
	}

	// ================================================================================================ the Salon

	void BuildSalon(FSink& S)
	{
		const FOrder O = MakeOrder(FP::SalonD);
		const FOrder Oval = MakeOrder(FP::OvalD);
		const double X0 = FP::SalonWest, X1 = FP::SalonEast, Y = FP::SalonHalf;
		const double Len = X1 - X0, Wid = 2.0 * Y;
		const FFaceLine FS = FFaceLine::Straight(FVector2D(X0, Y), FVector2D(X1, Y));
		const FFaceLine FE = FFaceLine::Straight(FVector2D(X1, Y), FVector2D(X1, -Y));
		const FFaceLine FN = FFaceLine::Straight(FVector2D(X1, -Y), FVector2D(X0, -Y));
		const FFaceLine FW = FFaceLine::Straight(FVector2D(X0, -Y), FVector2D(X0, Y));
		const double Top = O.EntBottom() + 0.1;

		// The windows: round-arched, in the 4 m bays between the pilasters, a round tablet over each.
		auto Window = [](double U)
		{
			FWindow W;
			W.Open = FOpening::Make(EK::Arch, U, 0.85, 3.6, 7.7);
			W.Frame = 0.24;
			W.Transom = 0.82;
			W.Medallion = 0.46;
			W.MedallionZ = 10.35;
			return W;
		};
		TArray<FWindow> SW, NW;
		for (const double X : {-72.0, -68.0, -64.0, -60.0, -52.0, -48.0, -40.0, -36.0, -28.0, -24.0, -20.0, -16.0}) { SW.Add(Window(X - X0)); }
		for (double X = -72.0; X <= -28.0 + 1e-6; X += 4.0) { NW.Add(Window(X1 - X)); }
		FOpening Door = FOpening::Make(EK::Rect, FP::PorticoAxisX - X0, 1.5, FP::Podium, 7.8);
		Door.bBacked = false;
		Door.bGlazed = false;

		// The skin: the south face whole; the others round the passage, the cabinet and the hyphen.
		TArray<FOpening> SOpen = OpeningsOf(SW);
		SOpen.Add(Door);
		BuildSkin(S.Skin, &S.Dark, &S.Glass, FS, 0.0, Len + kSkin, FFlat{kFoot}, Top, kSkin, kBack, SOpen);
		const double WP = FP::WestPassageHalf, CabE = X1 - FP::CabinetEast, CabW = X1 - FP::CabinetWest, H5 = FP::HyphenHalf;
		BuildSkin(S.Skin, nullptr, nullptr, FE, 0.0, Y - WP, FFlat{kFoot}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, nullptr, nullptr, FE, Y - WP, Y + WP, FFlat{FP::WestPassageTop}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, nullptr, nullptr, FE, Y + WP, Wid + kSkin, FFlat{kFoot}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, nullptr, nullptr, FN, 0.0, CabE, FFlat{kFoot}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, nullptr, nullptr, FN, CabE, CabW, FFlat{FP::CabinetRoof}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, &S.Dark, &S.Glass, FN, CabW, Len + kSkin, FFlat{kFoot}, Top, kSkin, kBack, OpeningsOf(NW));
		BuildSkin(S.Skin, nullptr, nullptr, FW, 0.0, Y - H5, FFlat{kFoot}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, nullptr, nullptr, FW, Y - H5, Y + H5, FFlat{Oval.EntTop() - 0.1}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, nullptr, nullptr, FW, Y + H5, Wid + kSkin, FFlat{kFoot}, Top, kSkin, kBack, {});
		for (const FWindow& W : SW) { WindowDress(S, FS, W, kSkin, kBack); }
		for (const FWindow& W : NW) { WindowDress(S, FN, W, kSkin, kBack); }
		DoorDress(S, FS, Door, kSkin, kBack);

		// The giant order: pilasters every 4 m on the long faces; at 0.6 m from the corners and ±3 m on the ends.
		for (int32 k = 0; k < 16; ++k)
		{
			const double U = 0.6 + 4.0 * k;
			Pilaster(S, O, FS.Local(U), O.Base);
			const double XN = X1 - U;
			const bool bOverCabinet = XN < FP::CabinetEast + 0.3 && XN > FP::CabinetWest - 0.3;
			Pilaster(S, O, FN.Local(U), bOverCabinet ? FP::CabinetRoof : O.Base);
		}
		for (const double U : {0.6, 4.6, 10.6, 14.6})
		{
			Pilaster(S, O, FE.Local(U), O.Base);
			const double YW = -Y + U;
			Pilaster(S, O, FW.Local(U), FMath::Abs(YW) < H5 ? Oval.EntTop() : O.Base);
		}

		// The entablature round the Salon, over its roof; the dentils face by face.
		const TArray<FVector2D> Loop = {FVector2D(X0, Y), FVector2D(X1, Y), FVector2D(X1, -Y), FVector2D(X0, -Y)};
		PlanSweep(S.Carved, Loop, true, EntablatureProfile(O, O.Face()));
		for (const FFaceLine* F : {&FS, &FE, &FN, &FW}) { Dentils(S.Carved, O, *F, 0.25, F->Length() - 0.25, O.Face()); }

		// The balustrade, broken by the portico's pediment (x −58 … −30), pedestals over the pilasters, urns at the corners.
		FBalustrade B;
		B.Z0 = O.EntTop();
		B.H = 1.15;
		B.Front = O.Face() - 0.02;
		B.Width = 0.44;
		B.PedestalWidth = 1.2;
		B.Pitch = 0.36;
		const double GapW = -58.0, GapE = -30.0;
		BalustradeRails(S, B, {FVector2D(GapE, Y), FVector2D(X1, Y), FVector2D(X1, -Y), FVector2D(X0, -Y), FVector2D(X0, Y), FVector2D(GapW, Y)}, false);
		TArray<double> SWest, SEast, NAll;
		for (int32 k = 0; k < 16; ++k)
		{
			const double U = 0.6 + 4.0 * k;
			if (X0 + U <= GapW + 1e-6) { SWest.Add(U); }
			if (X0 + U >= GapE - 1e-6) { SEast.Add(U); }
			NAll.Add(U);
		}
		BalustradeFace(S, B, FS, SWest);
		BalustradeFace(S, B, FS, SEast);
		BalustradeFace(S, B, FN, NAll);
		BalustradeFace(S, B, FE, {0.6, 4.6, 10.6, 14.6});
		BalustradeFace(S, B, FW, {0.6, 4.6, 10.6, 14.6});
		for (const FFaceLine* F : {&FS, &FE, &FN, &FW}) { CornerBlock(S, B, *F, F->Length(), 1.2, 1.45); }
	}

	// ================================================================================================ the portico

	void BuildPortico(FSink& S)
	{
		const FOrder O = MakeOrder(FP::SalonD);
		const double Ax = FP::PorticoAxisX, Yw = FP::SalonHalf, Yc = FP::PorticoColumnY, Yf = FP::PorticoFront;
		const double Hx = FP::PorticoHalf, Ch = FP::PorticoCheek, Ys = FP::PorticoStepsEnd;
		const double XW = Ax - Hx, XE = Ax + Hx;

		// The cheek walls either side of the steps: the podium's profile round their outer faces, their cores.
		const FProfile Pod = PodiumProfile();
		CapEnds(S.Base, PlanSweep(S.Base, {FVector2D(XW - Ch, Yw), FVector2D(XW - Ch, Ys), FVector2D(XW, Ys)}, false, Pod), Pod, false, true);
		CapEnds(S.Base, PlanSweep(S.Base, {FVector2D(XE, Ys), FVector2D(XE + Ch, Ys), FVector2D(XE + Ch, Yw)}, false, Pod), Pod, true, false);
		WBox(S.Base, XW - Ch, XW, Yw, Ys, -0.1, FP::Podium - 0.005);
		WBox(S.Base, XE, XE + Ch, Yw, Ys, -0.1, FP::Podium - 0.005);
		// Pedestals with urns at the cheeks' fronts.
		for (const double Xc : {XW - 0.5 * Ch, XE + 0.5 * Ch})
		{
			const double Yp = Ys - 0.95;
			WBox(S.Carved, Xc - 0.72, Xc + 0.72, Yp - 0.82, Yp + 0.82, FP::Podium - 0.02, FP::Podium + 0.16);
			WBox(S.Carved, Xc - 0.64, Xc + 0.64, Yp - 0.74, Yp + 0.74, FP::Podium, FP::Podium + 1.05);
			WBox(S.Carved, Xc - 0.74, Xc + 0.74, Yp - 0.84, Yp + 0.84, FP::Podium + 1.02, FP::Podium + 1.16);
			Urn(S.Carved, FVector(Xc, Yp, FP::Podium + 1.16), 1.7);
		}

		// The portico's floor, and eleven steps with nosings.
		WBox(S.Base, XW, XE, Yw, Yf, -0.1, FP::Podium + 0.004);   // 4 mm over the podium's cap behind it
		const double Rise = FP::Podium / FP::PorticoRisers, Going = FP::PorticoStepGoing;
		for (int32 i = 1; i <= FP::PorticoRisers; ++i)
		{
			const double Front = Ys - (i - 1) * Going, Z = i * Rise;
			WBox(S.Base, XW, XE, Front - Going, Front, -0.1, Z, FMeshData::PosY | FMeshData::PosZ);
			WBox(S.Base, XW, XE, Front - 0.01, Front + 0.03, Z - 0.045, Z, FMeshData::PosY | FMeshData::PosZ | FMeshData::NegZ);
		}

		// Six Ionic columns.
		for (int32 k = 0; k < 6; ++k) { Column(S, O, FLocal::Make(FVector2D(Ax - 10.0 + 4.0 * k, Yc), FVector2D(1.0, 0.0))); }

		// The entablature, returning to the wall over the corner columns (closed: its inner face shows under the ceiling).
		const double Fa = 5.0 / 6.0 * O.D;
		const double Xi0 = Ax - 10.0 + 0.5 * Fa, Xi1 = Ax + 10.0 - 0.5 * Fa, Yi = Yc - 0.5 * Fa;
		PlanSweep(S.Carved, {FVector2D(Xi0, Yw + 0.02), FVector2D(Xi0, Yi), FVector2D(Xi1, Yi), FVector2D(Xi1, Yw + 0.02)}, false,
				  EntablatureProfile(O, Fa, true));
		const FFaceLine Front = FFaceLine::Straight(FVector2D(Xi0, Yi), FVector2D(Xi1, Yi));
		const FFaceLine WestSide = FFaceLine::Straight(FVector2D(Xi0, Yw), FVector2D(Xi0, Yi));
		const FFaceLine EastSide = FFaceLine::Straight(FVector2D(Xi1, Yi), FVector2D(Xi1, Yw));
		const double Fr = Fa - 0.01 * O.D;
		Dentils(S.Carved, O, Front, -Fr + 0.05, Front.Length() + Fr - 0.05, Fa);
		Dentils(S.Carved, O, WestSide, 1.6, WestSide.Length() - 0.05, Fa);
		Dentils(S.Carved, O, EastSide, 0.05, EastSide.Length() - 1.6, Fa);

		// The ceiling: a soffit at the frieze's top, beams on the columns' lines and one along the middle (coffers).
		const double Zc = O.FriezeTop();
		WBox(S.Stone, Xi0 - 0.02, Xi1 + 0.02, Yw + kSkin, Yi + 0.02, Zc, O.EntTop() - 0.1, FMeshData::NegZ);
		for (const double X : {Ax - 6.0, Ax - 2.0, Ax + 2.0, Ax + 6.0})
		{
			WBox(S.Carved, X - 0.26, X + 0.26, Yw + kSkin, Yi + 0.01, Zc - 0.52, Zc + 0.01, FMeshData::NegX | FMeshData::PosX | FMeshData::NegZ);
		}
		const double Ym = 0.5 * (Yw + O.Face() + Yi);
		WBox(S.Carved, Xi0, Xi1, Ym - 0.21, Ym + 0.21, Zc - 0.40, Zc + 0.01, FMeshData::NegY | FMeshData::PosY | FMeshData::NegZ);

		// The pediment: the tympanum in the frieze's plane, raking cornices, a tablet; the gabled roof behind.
		const double Pitch = FMath::DegreesToRadians(FP::PedimentPitchDegrees), Tn = FMath::Tan(Pitch);
		const double Ft = O.EntTop();
		const double Yt = Yi + Fr;
		const double Half = 10.0 + 0.5 * Fa;
		const double Apex = Ft + Half * Tn;
		const FLocal Lw = FLocal::Make(FVector2D(0.0, 0.0), FVector2D(1.0, 0.0));   // A = x, D = y
		const double Yback = 6.9;
		FacePrism(S.Skin, Lw, {FVector2D(Ax - Half + 0.03, Ft - 0.25), FVector2D(Ax + Half - 0.03, Ft - 0.25), FVector2D(Ax, Apex - 0.02)}, Yback, Yt);
		const FProfile Rake = CorniceSection(O, 0.45);
		const TArray<TArray<FSweepFrame>> RakeRuns = FaceRuns(Lw, {FVector2D(Ax - Half - 0.02, Ft - 0.02), FVector2D(Ax, Apex), FVector2D(Ax + Half + 0.02, Ft - 0.02)}, false, Yt);
		for (const TArray<FSweepFrame>& Run : RakeRuns) { SalonKit::Sweep(S.Carved, Run, Rake); }
		CapEnds(S.Carved, RakeRuns, Rake);
		Medallion(S.Carved, FLocal::Make(FVector2D(Ax, 0.0), FVector2D(1.0, 0.0)), Yt, Ft + 1.0, 0.78);

		// On the frieze, in gilt bronze letters: the museum's name.
		{
			const double Zf0 = O.EntBottom() + 0.625 * O.D, Zf1 = O.FriezeTop(), Cap = 0.52;
			Inscription(S.Gilt, Lw, Ax, 0.5 * (Zf0 + Zf1) - 0.5 * Cap, Cap, Yt - 0.01, Yt + 0.028, TEXT("MVS\u00C9E \u00B7 VISION"), 0.42);
		}

		// The roof: two slopes from the raking cornices' tops back over the Salon's roof, a blocking course each side,
		// a gable wall at the back.
		const double Hc = 0.895 * O.D;                              // the raking cornice's height, across its slope
		const double Xe = Half + 0.02 + Hc * FMath::Sin(Pitch);       // half the roof's width at its eaves
		const double Ze = Ft - 0.02 + Hc * FMath::Cos(Pitch) - 0.02;  // the eaves' height
		const double Zr = Ze + Xe * Tn;                               // the ridge
		const double Yr0 = Yt - 0.4;
		for (const double Side : {-1.0, 1.0})
		{
			const FVector N = FVector(Side * FMath::Sin(Pitch), 0.0, FMath::Cos(Pitch));
			S.Stone.Rect(FVector(Ax + Side * Xe, Yr0, Ze), FVector(Ax, Yr0, Zr), FVector(Ax, Yback, Zr), FVector(Ax + Side * Xe, Yback, Ze), N);
			S.Stone.Rect(FVector(Ax + Side * Xe, Yr0, Ze), FVector(Ax, Yr0, Zr), FVector(Ax, Yr0 + 0.01, Zr - 0.3), FVector(Ax + Side * Xe, Yr0 + 0.01, Ze - 0.3), FVector(0, 1, 0));
			const double Xo = Ax + Side * (Xe - 0.02), Xin = Ax + Side * (Half - 0.9);
			WBox(S.Skin, FMath::Min(Xo, Xin), FMath::Max(Xo, Xin), Yw, Yr0 + 0.02, Ft - 0.05, Ze + 0.01,
				 (Side < 0 ? FMeshData::NegX : FMeshData::PosX) | FMeshData::NegY);
		}
		FacePrism(S.Skin, Lw, {FVector2D(Ax - Xe, FP::SalonRoofTop - 0.05), FVector2D(Ax + Xe, FP::SalonRoofTop - 0.05), FVector2D(Ax + Xe, Ze), FVector2D(Ax, Zr), FVector2D(Ax - Xe, Ze)},
				  Yback - 0.12, Yback + 0.02);
	}

	// ================================================================================================ the Manet cabinet

	void BuildCabinet(FSink& S)
	{
		const FOrder O = MakeOrder(FP::CabinetD);
		const double XE = FP::CabinetEast, XW = FP::CabinetWest, YN = FP::CabinetNorth, YS = -FP::SalonHalf;
		const FFaceLine FE = FFaceLine::Straight(FVector2D(XE, YS), FVector2D(XE, YN));
		const FFaceLine FN = FFaceLine::Straight(FVector2D(XE, YN), FVector2D(XW, YN));
		const FFaceLine FW = FFaceLine::Straight(FVector2D(XW, YN), FVector2D(XW, YS));
		const double Le = YS - YN, Ln = XE - XW, Top = O.EntBottom() + 0.1;

		FWindow W;
		W.Open = FOpening::Make(EK::Arch, 0.5 * Ln, 0.55, 2.75, 4.35);
		W.Frame = 0.17;
		W.Transom = 0.55;
		W.Mullions = 1;
		W.bApron = false;
		BuildSkin(S.Skin, nullptr, nullptr, FE, 0.0, Le + kSkin, FFlat{kFoot}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, &S.Dark, &S.Glass, FN, 0.0, Ln + kSkin, FFlat{kFoot}, Top, kSkin, kBack, {W.Open});
		BuildSkin(S.Skin, nullptr, nullptr, FW, 0.0, Le, FFlat{kFoot}, Top, kSkin, kBack, {});
		WindowDress(S, FN, W, kSkin, kBack);
		for (const double U : {0.25, 0.5 * Ln - 1.25, 0.5 * Ln + 1.25, Ln - 0.25}) { Pilaster(S, O, FN.Local(U), O.Base); }
		Pilaster(S, O, FE.Local(Le - 0.25), O.Base);
		Pilaster(S, O, FE.Local(0.5 * Le), O.Base);
		Pilaster(S, O, FW.Local(0.25), O.Base);
		Pilaster(S, O, FW.Local(0.5 * Le), O.Base);

		const TArray<FVector2D> Path = {FVector2D(XE, YS + 0.05), FVector2D(XE, YN), FVector2D(XW, YN), FVector2D(XW, YS + 0.05)};
		PlanSweep(S.Carved, Path, false, EntablatureProfile(O, O.Face()));
		Dentils(S.Carved, O, FE, 0.3, Le - 0.25, O.Face());
		Dentils(S.Carved, O, FN, 0.25, Ln - 0.25, O.Face());
		Dentils(S.Carved, O, FW, 0.25, Le - 0.3, O.Face());

		FBalustrade B;
		B.Z0 = O.EntTop();
		B.H = 0.72;
		B.Front = O.Face() - 0.01;
		B.Width = 0.3;
		B.PedestalWidth = 0.5;
		B.Pitch = 0.24;
		BalustradeRails(S, B, Path, false);
		BalustradeFace(S, B, FE, {0.3, 0.5 * Le, Le - 0.25});
		BalustradeFace(S, B, FN, {0.25, 0.5 * Ln - 1.25, 0.5 * Ln + 1.25, Ln - 0.25});
		BalustradeFace(S, B, FW, {0.25, 0.5 * Le, Le - 0.3});
		CornerBlock(S, B, FE, Le, 0.5, 0.0);
		CornerBlock(S, B, FN, Ln, 0.5, 0.0);
	}

	// ================================================================================================ the oval

	void BuildOval(FSink& S)
	{
		const FOrder O = MakeOrder(FP::OvalD);
		const double X0 = FP::SalonWest, H5 = FP::HyphenHalf;
		const TArray<FVector2D> West = OvalWest();
		const FFaceLine FHN = FFaceLine::Straight(FVector2D(X0, -H5), West[0]);
		const FFaceLine FOv = FFaceLine::FromPoints(West, true);
		const FFaceLine FHS = FFaceLine::Straight(West.Last(), FVector2D(X0, H5));
		const double Lo = FOv.Length(), Top = O.EntBottom() + 0.1;

		// Pilasters round the oval, an odd number of bays so an oculus stands on the museum's axis at the west end.
		const double Margin = 0.55;
		int32 Bays = FMath::RoundToInt32((Lo - 2.0 * Margin) / 4.2);
		if (Bays % 2 == 0) { ++Bays; }
		const double Step = (Lo - 2.0 * Margin) / Bays;
		TArray<double> Piers;
		for (int32 b = 0; b <= Bays; ++b) { Piers.Add(Margin + Step * b); }
		TArray<FWindow> Ws;
		for (int32 b = 0; b < Bays; ++b)
		{
			FWindow W;
			W.Open = FOpening::Make(EK::Round, Margin + Step * (b + 0.5), 0.52, 4.35, 0.0);
			W.Frame = 0.17;
			W.Mullions = 1;
			W.bSill = false;
			Ws.Add(W);
		}
		BuildSkin(S.Skin, nullptr, nullptr, FHN, 0.0, FHN.Length() + 0.12, FFlat{kFoot}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, &S.Dark, &S.Glass, FOv, 0.0, Lo, FFlat{kFoot}, Top, kSkin, kBack, OpeningsOf(Ws));
		BuildSkin(S.Skin, nullptr, nullptr, FHS, -0.12, FHS.Length(), FFlat{kFoot}, Top, kSkin, kBack, {});
		for (const FWindow& W : Ws) { WindowDress(S, FOv, W, kSkin, kBack); }
		for (const double U : Piers) { Pilaster(S, O, FOv.Local(U), O.Base); }

		// The entablature and balustrade from the Salon's west face round to it again.
		TArray<FVector2D> Path;
		AppendPath(Path, {FVector2D(X0 + 0.05, -H5)});
		AppendPath(Path, West);
		AppendPath(Path, {FVector2D(X0 + 0.05, H5)});
		PlanSweep(S.Carved, Path, false, EntablatureProfile(O, O.Face()));
		Dentils(S.Carved, O, FOv, 0.1, Lo - 0.1, O.Face());
		Dentils(S.Carved, O, FHN, 0.3, FHN.Length() - 0.1, O.Face());
		Dentils(S.Carved, O, FHS, 0.1, FHS.Length() - 0.3, O.Face());
		FBalustrade B;
		B.Z0 = O.EntTop();
		B.H = 0.85;
		B.Front = O.Face() - 0.01;
		B.Width = 0.34;
		B.PedestalWidth = 0.6;
		B.Pitch = 0.28;
		BalustradeRails(S, B, Path, false);
		BalustradeFace(S, B, FOv, Piers);
		Balusters(S, B, FHN, 0.25, FHN.Length() - 0.2);
		Balusters(S, B, FHS, 0.2, FHS.Length() - 0.25);

		// The hyphen's roof, between the Salon's west face and the oval's east side.
		TArray<FVector2D> Roof = {FVector2D(X0, -H5)};
		AppendPath(Roof, OvalEast(FP::OvalWall - 0.02));
		AppendPath(Roof, {FVector2D(X0, H5)});
		PlanPoly(S.Stone, Roof, O.EntTop() - 0.03, true);
	}

	// ================================================================================================ the Sculpture Hall

	void BuildSculptureHall(FSink& S)
	{
		const FOrder O = MakeOrder(FP::SculptureD);
		const double X = FP::SculptureEast, YN = FP::SculptureNorth, YS = FP::SculptureSouth, L = 2.0 * X;
		const FFaceLine FS = FFaceLine::Straight(FVector2D(-X, YS), FVector2D(X, YS));
		const FFaceLine FE = FFaceLine::Straight(FVector2D(X, YS), FVector2D(X, YN));
		const FFaceLine FN = FFaceLine::Straight(FVector2D(X, YN), FVector2D(-X, YN));
		const FFaceLine FW = FFaceLine::Straight(FVector2D(-X, YN), FVector2D(-X, YS));
		const double Top = O.EntBottom() + 0.1;
		const TArray<double> Piers = {0.45, 6.0, 12.0, 17.55};

		auto Window = [](double U, bool bWide)
		{
			FWindow W;
			W.Open = FOpening::Make(EK::Arch, U, bWide ? 1.05 : 0.8, 3.2, bWide ? 5.85 : 6.05);
			W.Frame = 0.21;
			W.Transom = 0.72;
			return W;
		};
		const TArray<FWindow> Three = {Window(3.225, false), Window(9.0, true), Window(14.775, false)};
		const TArray<FWindow> Two = {Window(3.225, false), Window(14.775, false)};
		const double NP = FP::NorthPassageHalf;
		BuildSkin(S.Skin, &S.Dark, &S.Glass, FS, 0.0, X - NP, FFlat{kFoot}, Top, kSkin, kBack, OpeningsOf(OnFace(Two, 0.0, X - NP)));
		BuildSkin(S.Skin, nullptr, nullptr, FS, X - NP, X + NP, FFlat{FP::NorthPassageTop}, Top, kSkin, kBack, {});
		BuildSkin(S.Skin, &S.Dark, &S.Glass, FS, X + NP, L + kSkin, FFlat{kFoot}, Top, kSkin, kBack, OpeningsOf(OnFace(Two, X + NP, L)));
		for (const FFaceLine* F : {&FE, &FN, &FW}) { BuildSkin(S.Skin, &S.Dark, &S.Glass, *F, 0.0, L + kSkin, FFlat{kFoot}, Top, kSkin, kBack, OpeningsOf(Three)); }
		for (const FWindow& W : Two) { WindowDress(S, FS, W, kSkin, kBack); }
		for (const FFaceLine* F : {&FE, &FN, &FW})
		{
			for (const FWindow& W : Three) { WindowDress(S, *F, W, kSkin, kBack); }
		}
		Medallion(S.Carved, FS.Local(X), kSkin, 7.95, 0.45);
		for (const FFaceLine* F : {&FS, &FE, &FN, &FW})
		{
			for (const double U : Piers) { Pilaster(S, O, F->Local(U), O.Base); }
		}

		const TArray<FVector2D> Loop = {FVector2D(-X, YS), FVector2D(X, YS), FVector2D(X, YN), FVector2D(-X, YN)};
		PlanSweep(S.Carved, Loop, true, EntablatureProfile(O, O.Face()));
		FBalustrade B;
		B.Z0 = O.EntTop();
		B.H = 0.95;
		B.Front = O.Face() - 0.02;
		B.Width = 0.38;
		B.PedestalWidth = 0.9;
		B.Pitch = 0.32;
		BalustradeRails(S, B, Loop, true);
		for (const FFaceLine* F : {&FS, &FE, &FN, &FW})
		{
			Dentils(S.Carved, O, *F, 0.25, L - 0.25, O.Face());
			BalustradeFace(S, B, *F, Piers);
			CornerBlock(S, B, *F, L, 0.9, 1.15);
		}
	}

	// ================================================================================================ the Rotunda's drum

	void BuildDrum(FSink& S)
	{
		const FDrumAngles A;
		const FOrder O = MakeOrder(FP::DrumD);
		const FVector2D C(0.0, 0.0);
		const double R = FP::DrumRadius, Top = O.EntBottom() + 0.1;
		auto Arc = [&](double A0, double A1) { return FFaceLine::Arc(C, R, A0, A1, 0.2); };

		// The free arcs between the wings, from the podium, a blind window on each diagonal.
		const double Free[4][3] = {{360.0 - A.E, A.N1, 315.0}, {A.N0, A.W1, 225.0}, {A.W0, A.S1, 135.0}, {A.S0, A.E, 45.0}};
		for (const auto& Span : Free)
		{
			const FFaceLine F = Arc(Span[0], Span[1]);
			FWindow W;
			W.Open = FOpening::Make(EK::Arch, FMath::DegreesToRadians(Span[0] - Span[2]) * R, 0.75, 3.2, 6.2);
			W.Frame = 0.2;
			W.Transom = 0.75;
			W.Medallion = 0.38;
			W.MedallionZ = 8.2;
			BuildSkin(S.Skin, &S.Dark, &S.Glass, F, 0.0, F.Length(), FFlat{kFoot}, Top, kSkin, kBack, {W.Open});
			WindowDress(S, F, W, kSkin, kBack);
		}
		// Over the junctions: from above the Hall of Light's vault, the vestibule's slab, the passages' shells.
		{
			const FFaceLine F = Arc(A.E, -A.E);
			BuildSkin(S.Skin, nullptr, nullptr, F, 0.0, F.Length(), [&F](double U) { return OverHall(F.At(U, 0.0, 0.0).Y); }, Top, kSkin, kBack, {});
		}
		const double Junctions[3][3] = {{A.S1, A.S0, FP::VestibuleTop + 0.05}, {A.W1, A.W0, FP::WestPassageTop + 0.03}, {A.N1, A.N0, FP::NorthPassageTop + 0.03}};
		for (const auto& J : Junctions)
		{
			const FFaceLine F = Arc(J[0], J[1]);
			if (!kSculptureHallDress && J[0] == A.N1)
			{
				// Chenghuai: over the north door's open porch the skin comes down to the ground round the door's arch.
				BuildSkin(S.Skin, nullptr, nullptr, F, 0.0, F.Length(), [&F](double U)
				{
					const double X = F.At(U, 0.0, 0.0).X;
					return FMath::Abs(X) < kNorthDoorHalf ? kNorthDoorSpring + FMath::Sqrt(FMath::Max(0.0, kNorthDoorHalf * kNorthDoorHalf - X * X)) : -0.05;
				}, Top, kSkin, kBack, {});
				continue;
			}
			BuildSkin(S.Skin, nullptr, nullptr, F, 0.0, F.Length(), FFlat{J[2]}, Top, kSkin, kBack, {});
		}
		// Pilasters in pairs flanking the diagonals.
		for (const double Deg : {33.75, 56.25, 123.75, 146.25, 213.75, 236.25, 303.75, 326.25})
		{
			const double Rad = FMath::DegreesToRadians(Deg);
			Pilaster(S, O, FLocal::Make(FVector2D(FMath::Cos(Rad), FMath::Sin(Rad)) * R, FVector2D(FMath::Sin(Rad), -FMath::Cos(Rad))), O.Base);
		}
		// The entablature right round, the attic over it, and the wash in to the dome's shell.
		TArray<FVector2D> Ring = ArcPoints(C, R, 360.0, 0.0, 0.2);
		Ring.Pop();
		PlanSweep(S.Carved, Ring, true, EntablatureProfile(O, O.Face()));
		const FFaceLine Full = Arc(360.0, 0.0);
		Dentils(S.Carved, O, Full, 0.0, Full.Length() - 0.12, O.Face());
		const double AtticTop = FP::DrumAtticTop;
		BuildSkin(S.Skin, nullptr, nullptr, Full, 0.0, Full.Length(), FFlat{O.EntTop() - 0.1}, AtticTop - 0.2, kSkin, kBack, {});
		const double CorniceH = 0.34;
		PlanSweep(S.Carved, Ring, true, SmallCornice(kSkin + 0.03, AtticTop - CorniceH + 0.02, CorniceH));
		const double Zw = AtticTop + 0.035;
		const double RIn = FMath::Sqrt(FP::DomeShellRadius * FP::DomeShellRadius - FMath::Square(Zw - FP::DomeShellCentre)) - 0.03;
		const FVector Up(0, 0, 1);
		const int32 N = Ring.Num();
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D P0 = Ring[i], P1 = Ring[(i + 1) % N];
			const FVector2D D0 = P0.GetSafeNormal(), D1 = P1.GetSafeNormal();
			S.Stone.Rect(Flat(D0 * RIn, Zw), Flat(D1 * RIn, Zw), Flat(D1 * (R + 0.02), Zw), Flat(D0 * (R + 0.02), Zw), Up);
		}
	}

	// ================================================================================================ the passages and the vestibule

	void BuildLinks(FSink& S)
	{
		const double WP = FP::WestPassageHalf, NP = FP::NorthPassageHalf, VH = FP::VestibuleHalf, R = FP::DrumRadius;
		const double WX = -FMath::Sqrt(R * R - WP * WP), NY = -FMath::Sqrt(R * R - NP * NP), VY = FMath::Sqrt(R * R - VH * VH);
		struct FLink
		{
			FVector2D A, B;
			double Top;
		};
		const FLink Links[] = {
			{FVector2D(FP::SalonEast, WP), FVector2D(WX, WP), FP::WestPassageTop},
			{FVector2D(WX, -WP), FVector2D(FP::SalonEast, -WP), FP::WestPassageTop},
			{FVector2D(NP, NY), FVector2D(NP, FP::SculptureSouth), kSculptureHallDress ? FP::NorthPassageTop : -1.0},   // Chenghuai: no
			{FVector2D(-NP, FP::SculptureSouth), FVector2D(-NP, NY), kSculptureHallDress ? FP::NorthPassageTop : -1.0},  // north passage
			{FVector2D(-VH, VY), FVector2D(-VH, FP::CourtNorthFace), -1.0},   // Albion: its porch, not a vestibule
			{FVector2D(VH, FP::CourtNorthFace), FVector2D(VH, VY), -1.0},   // Albion: its porch, not a vestibule
		};
		for (const FLink& Lk : Links)
		{
			if (Lk.Top < 0.0) { continue; }
			const FFaceLine F = FFaceLine::Straight(Lk.A, Lk.B);
			const double L = F.Length();
			BuildSkin(S.Skin, nullptr, nullptr, F, -0.1, L + 0.1, FFlat{kFoot}, Lk.Top, kSkin, kBack, {});
			PlanSweep(S.Carved, F.Points(-0.1, L + 0.1), false, SmallCornice(kSkin + 0.03, Lk.Top - 0.3, 0.42));
		}
	}

	// ================================================================================================ the Rotunda's dome

	/**
	 * A stone covering over the Rotunda's dome, 0.5 m outside its shell (R 10.6 about h 10): clear of the shadow the
	 * baked dome casts on itself at a distance (its jagged self-shadowing is the "camouflage" seen on it from outside).
	 * Two step rings on the attic, like the Pantheon's, then the smooth dome up to a rolled ring round the eye (on and
	 * over its curb, r 2.9).
	 */
	void BuildDomeCover(FSink& S)
	{
		const double Rs = FP::DomeShellRadius, Zc = FP::DomeShellCentre, Rc = Rs + FP::DomeCoverClearance;
		// The radius of a sphere of radius R about the dome's centre at height Z.
		auto Shell = [&](double R, double Z) { return FMath::Sqrt(FMath::Max(0.0, R * R - FMath::Square(Z - Zc))); };
		const FVector Foot(0.0, 0.0, 0.0);
		// Two step rings on the attic's wash.
		double Z = FP::DrumAtticTop + 0.03;
		for (int32 k = 0; k < 2; ++k)
		{
			const double Z1 = Z + 0.40, Out = 11.25 - 0.27 * k;
			FProfile Ring;
			Ring.Add(Shell(Rs, Z) - 0.05, Z - 0.01).Add(Out, Z - 0.01).Add(Out, Z1 - 0.03);
			Ring.Arc(Out - 0.03, Z1 - 0.03, 0.03, 0.03, 0, 90, 3);
			Ring.Add(Shell(Rs, Z1) - 0.05, Z1);
			Lathe(S.Stone, Foot, Ring, 160);
			Z = Z1;
		}
		// The dome, from the upper ring to the eye.
		FProfile Dome;
		constexpr int32 N = 64;
		const double Z0 = Z - 0.01, Z1 = Zc + FMath::Sqrt(Rc * Rc - 3.0 * 3.0);   // up to r 3, just outside the curb
		for (int32 i = 0; i <= N; ++i)
		{
			const double Zi = Z0 + (Z1 - Z0) * i / N;
			Dome.Add(Shell(Rc, Zi), Zi, i > 0 && i < N);
		}
		Lathe(S.Stone, Foot, Dome, 160);
		// A rolled ring round the eye, standing on the Rotunda's curb (r 2.9, top 20.45) and over it.
		FProfile Eye;
		Eye.bClosed = true;
		Eye.Add(2.92, FP::DomeCurbTop - 0.1).Add(3.28, FP::DomeCurbTop - 0.1).Add(3.28, Z1 - 0.12);
		Eye.Arc(3.28, Z1 + 0.06, 0.08, 0.18, -90, 90, 8);
		Eye.Add(2.92, Z1 + 0.24);
		Lathe(S.Carved, Foot, Eye, 96);
	}

	// ================================================================================================ the night's lights

	/** A floodlight: where it stands and what it aims at (plan metres), its cone (degrees), candela and reach (metres). */
	struct FLightSpec
	{
		FVector At, Aim;
		float Inner, Outer, Candela, Reach;
	};

	/** Every floodlight, in a fixed order (the actor makes one spot light per entry). */
	TArray<FLightSpec> NightLights()
	{
		TArray<FLightSpec> L;
		auto Add = [&L](const FVector& At, const FVector& Aim, float Inner, float Outer, float Candela, float Reach)
		{
			L.Add(FLightSpec{At, Aim, Inner, Outer, Candela, Reach});
		};
		const double Ax = FP::PorticoAxisX, Yc = FP::PorticoColumnY, X0 = FP::SalonWest, Y = FP::SalonHalf;
		const double Up = FP::Podium + 0.06;
		// The portico: its columns grazed from their feet; the pediment from the parvis; the door from the ceiling.
		for (int32 k = 0; k < 6; ++k)
		{
			const double X = Ax - 10.0 + 4.0 * k;
			Add(FVector(X, Yc + 0.95, Up), FVector(X, Yc + 0.3, 13.0), 5.f, 13.f, 5000.f, 16.f);
		}
		for (const double Side : {-1.0, 1.0}) { Add(FVector(Ax + Side * 7.5, 22.0, 0.35), FVector(Ax + Side * 2.0, 12.3, 15.6), 16.f, 30.f, 14000.f, 30.f); }
		Add(FVector(Ax, 10.4, 13.1), FVector(Ax, 7.9, 4.5), 18.f, 36.f, 2500.f, 12.f);
		// The south front's pilasters either side of the portico, grazed from the podium's cap.
		for (int32 k = 0; k < 16; ++k)
		{
			const double X = X0 + 0.6 + 4.0 * k;
			if (X > -58.0 + 1e-6 && X < -30.0 - 1e-6) { continue; }
			Add(FVector(X, Y + 0.64, Up), FVector(X, Y + 0.42, 12.6), 4.f, 11.f, 3500.f, 14.f);
		}
		// The drum's pilasters.
		for (const double Deg : {33.75, 56.25, 123.75, 146.25, 213.75, 236.25, 303.75, 326.25})
		{
			const FVector Dir(FMath::Cos(FMath::DegreesToRadians(Deg)), FMath::Sin(FMath::DegreesToRadians(Deg)), 0.0);
			Add(Dir * (FP::DrumRadius + 0.64) + FVector(0, 0, Up), Dir * (FP::DrumRadius + 0.42) + FVector(0, 0, 9.8), 4.f, 11.f, 3000.f, 12.f);
		}
		// The Sculpture Hall's corner pilasters.
		if (kSculptureHallDress)
		{
			const double X = FP::SculptureEast, YN = FP::SculptureNorth, YS = FP::SculptureSouth;
			const FFaceLine Faces[4] = {FFaceLine::Straight(FVector2D(-X, YS), FVector2D(X, YS)), FFaceLine::Straight(FVector2D(X, YS), FVector2D(X, YN)),
										FFaceLine::Straight(FVector2D(X, YN), FVector2D(-X, YN)), FFaceLine::Straight(FVector2D(-X, YN), FVector2D(-X, YS))};
			for (const FFaceLine& F : Faces)
			{
				for (const double U : {0.45, 17.55}) { Add(F.At(U, 0.62, Up), F.At(U, 0.38, 9.3), 4.f, 11.f, 2800.f, 11.f); }
			}
		}
		// The oval's pilasters, every other one.
		{
			const TArray<FVector2D> West = OvalWest();
			const FFaceLine F = FFaceLine::FromPoints(West, true);
			const double Margin = 0.55;
			int32 Bays = FMath::RoundToInt32((F.Length() - 2.0 * Margin) / 4.2);
			if (Bays % 2 == 0) { ++Bays; }
			const double Step = (F.Length() - 2.0 * Margin) / Bays;
			for (int32 b = 0; b <= Bays; b += 2) { Add(F.At(Margin + Step * b, 0.55, Up), F.At(Margin + Step * b, 0.33, 6.3), 4.f, 12.f, 1800.f, 8.f); }
		}
		return L;
	}

	FSink BuildAll(bool bDomeCover)
	{
		FSink S;
		if (bDomeCover) { BuildDomeCover(S); }
		BuildPodium(S);
		BuildSalon(S);
		BuildPortico(S);
		BuildCabinet(S);
		BuildOval(S);
		if (kSculptureHallDress) { BuildSculptureHall(S); }
		BuildDrum(S);
		BuildLinks(S);
		return S;
	}
}

// ==================================================================================================== the actor

AMuseeFacadeStructure::AMuseeFacadeStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Stone = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Stone"));
	Stone->SetupAttachment(RootComponent);
	Stone->bUseAsyncCooking = true;
	Stone->bUseComplexAsSimpleCollision = true;
	Stone->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Stone->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	Glass = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Glass"));
	Glass->SetupAttachment(RootComponent);
	Glass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glass->SetCastShadow(false);
	// The sun (channel 0) and the exterior's own lights (channel 1, which reach nothing inside).
	Stone->SetLightingChannels(true, true, false);
	Glass->SetLightingChannels(true, true, false);

	const int32 NumLights = FacadeBuild::NightLights().Num();
	for (int32 k = 0; k < NumLights; ++k)
	{
		USpotLightComponent* L = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("Floodlight%02d"), k + 1));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		Floodlights.Add(L);
	}
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	Parameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MPC_Musee.MPC_Musee")));

	auto Soft = [](const TCHAR* Path) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)); };
	// The exterior's weathered stone (Scripts/exterior_materials.py: streaks under the ledges, a damp foot, lichen on the north).
	AshlarMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Ashlar.MI_Ext_Ashlar"));
	StoneMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Dressing.MI_Ext_Dressing"));
	ShaftMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Shaft.MI_Ext_Shaft"));
	CarvedMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Carved.MI_Ext_Carved"));
	BronzeMaterial = Soft(TEXT("/Game/Museum/Materials/USD/MI_bronze_dark.MI_bronze_dark"));
	BackingMaterial = Soft(TEXT("/Game/Museum/Materials/USD/MI_steel_dark.MI_steel_dark"));
	GravelMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Gravel.MI_Ext_Gravel"));
	GlassMaterial = Soft(TEXT("/Game/Museum/Materials/M_Glass.M_Glass"));
	BaseMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Base.MI_Ext_Base"));
	GiltMaterial = Soft(TEXT("/Game/Museum/Materials/M_Gilt_Aged.M_Gilt_Aged"));

	AddTags();
}

void AMuseeFacadeStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
	PlaceLights();
	SetNight(0.f, 1.f);   // the editor shows the day
	AddTags();
}

void AMuseeFacadeStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never shows an older build.
	Build();
	ApplyMaterials();
	PlaceLights();
	LoadedParameters = Parameters.LoadSynchronous();
	LastDaylight = -1.f;
	float Daylight = 1.f;
	if (LoadedParameters) { Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, LoadedParameters, TEXT("Daylight")); }
	SetNight(1.f - FMath::SmoothStep(NightLightsFadeFrom, NightLightsOffAt, Daylight), Daylight);
}

void AMuseeFacadeStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!LoadedParameters) { return; }
	const float Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, LoadedParameters, TEXT("Daylight"));
	if (FMath::Abs(Daylight - LastDaylight) < 0.005f) { return; }
	SetNight(1.f - FMath::SmoothStep(NightLightsFadeFrom, NightLightsOffAt, Daylight), Daylight);
}

void AMuseeFacadeStructure::PlaceLights()
{
	const TArray<FacadeBuild::FLightSpec> Specs = FacadeBuild::NightLights();
	for (int32 k = 0; k < Floodlights.Num() && k < Specs.Num(); ++k)
	{
		USpotLightComponent* L = Floodlights[k];
		if (!L) { continue; }
		const FacadeBuild::FLightSpec& S = Specs[k];
		L->SetRelativeLocationAndRotation(S.At * MuseePlan::Cm, (S.Aim - S.At).Rotation());
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetInnerConeAngle(S.Inner);
		L->SetOuterConeAngle(S.Outer);
		L->SetAttenuationRadius(S.Reach * MuseePlan::Cm);
		L->SetSourceRadius(4.f);
		L->SetCastShadows(bFloodlightShadows);
		// Off MegaLights' stochastic path: forty grazing spots overlapping on the columns boiled visibly in a still
		// frame (the flicker audit, 2026-09-26); the deferred path shades them steadily, at night only.
		L->bAllowMegaLights = bFloodlightsMegaLights;
		L->MarkRenderStateDirty();
		L->SetLightingChannels(false, true, false);
	}
}

void AMuseeFacadeStructure::SetNight(float Level, float Daylight)
{
	NightLevel = bNightLights ? FMath::Clamp(Level, 0.f, 1.f) : 0.f;
	LastDaylight = Daylight;
	const bool bOn = NightLevel > 0.001f;
	const TArray<FacadeBuild::FLightSpec> Specs = FacadeBuild::NightLights();
	for (int32 k = 0; k < Floodlights.Num() && k < Specs.Num(); ++k)
	{
		if (USpotLightComponent* L = Floodlights[k])
		{
			L->SetIntensity(Specs[k].Candela * FloodlightScale * NightLevel);
			L->SetVisibility(bOn);
		}
	}
}

void AMuseeFacadeStructure::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Exterior")));
	Tags.AddUnique(FName(TEXT("musee.exterior")));
	// Nothing of it moves or ticks.
	Tags.AddUnique(MuseeBake::BakeableTag());
}

TArray<FString> AMuseeFacadeStructure::GetReplacedImportPrims()
{
	return {};
}

void AMuseeFacadeStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const FacadeOrder::FSink Parts = FacadeBuild::BuildAll(bCoverRotundaDome);
	Stone->ClearAllMeshSections();
	Glass->ClearAllMeshSections();
	Parts.Skin.Write(Stone, 0, true);
	Parts.Stone.Write(Stone, 1, true);
	Parts.Shafts.Write(Stone, 2, true);
	Parts.Carved.Write(Stone, 3, true);
	Parts.Bronze.Write(Stone, 4, true);
	Parts.Dark.Write(Stone, 5, true);
	Parts.Gravel.Write(Stone, 6, true);
	Parts.Base.Write(Stone, 7, true);
	Parts.Gilt.Write(Stone, 8, false);
	Parts.Glass.Write(Glass, 0, false);
	TriangleCount = 0;
	for (const FacadeKit::FMeshData* M : {&Parts.Skin, &Parts.Stone, &Parts.Shafts, &Parts.Carved, &Parts.Bronze, &Parts.Dark, &Parts.Gravel, &Parts.Base, &Parts.Gilt, &Parts.Glass})
	{
		TriangleCount += M->Indices.Num() / 3;
	}
	UE_LOG(LogMusee, Log, TEXT("Facade: %d triangles (skin %d, stone %d, shafts %d, carved %d, bronze %d, backing %d, gravel %d, glass %d)."), TriangleCount,
		   Parts.Skin.Indices.Num() / 3, Parts.Stone.Indices.Num() / 3, Parts.Shafts.Indices.Num() / 3, Parts.Carved.Indices.Num() / 3,
		   Parts.Bronze.Indices.Num() / 3, Parts.Dark.Indices.Num() / 3, Parts.Gravel.Indices.Num() / 3, Parts.Glass.Indices.Num() / 3);
}

void AMuseeFacadeStructure::ApplyMaterials()
{
	// Each slot takes its material, or the first fallback that exists (materials still being made are skipped).
	auto Load = [](std::initializer_list<const TSoftObjectPtr<UMaterialInterface>*> Chain) -> UMaterialInterface*
	{
		for (const TSoftObjectPtr<UMaterialInterface>* Ref : Chain)
		{
			if (Ref && !Ref->IsNull())
			{
				if (UMaterialInterface* M = Ref->LoadSynchronous()) { return M; }
			}
		}
		return nullptr;
	};
	const TSoftObjectPtr<UMaterialInterface> Moulding(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Plaster_Moulding.M_Plaster_Moulding")));
	const TSoftObjectPtr<UMaterialInterface> Bronze(FSoftObjectPath(TEXT("/Game/Museum/Materials/USD/MI_bronze.MI_bronze")));
	auto Set = [](UProceduralMeshComponent* C, int32 Section, UMaterialInterface* M)
	{
		if (C && M && Section < C->GetNumSections()) { C->SetMaterial(Section, M); }
	};
	Set(Stone, 0, Load({&AshlarMaterial, &StoneMaterial}));
	Set(Stone, 1, Load({&StoneMaterial}));
	Set(Stone, 2, Load({&ShaftMaterial, &StoneMaterial}));
	Set(Stone, 3, Load({&CarvedMaterial, &Moulding, &StoneMaterial}));
	Set(Stone, 4, Load({&BronzeMaterial, &Bronze}));
	Set(Stone, 5, Load({&BackingMaterial, &BronzeMaterial, &Bronze}));
	Set(Stone, 6, Load({&GravelMaterial, &StoneMaterial}));
	Set(Stone, 7, Load({&BaseMaterial, &StoneMaterial}));
	const TSoftObjectPtr<UMaterialInterface> PlainGilt(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Gilt.M_Gilt")));
	Set(Stone, 8, Load({&GiltMaterial, &PlainGilt, &BronzeMaterial}));
	Set(Glass, 0, Load({&GlassMaterial}));
}
