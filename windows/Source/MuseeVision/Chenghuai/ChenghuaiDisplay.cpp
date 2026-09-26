#include "Chenghuai/ChenghuaiGeometry.h"

#include "Chenghuai/ChenghuaiHall.h"
#include "Chenghuai/ChenghuaiPlan.h"

/**
 * The display furniture (Hang and Rooms boards): the cases cut to the works, the tables, the main hall's carved screens.
 * Every case is 楠木 (waxed nanmu) with silk-covered backs and beds, low-iron glass, and its lamp hidden in the case
 * (a baffled strip under the canopy, or a rail at the back of a slanted case). The works themselves are actors placed
 * by Scripts/chenghuai_works.py (AChenghuaiScroll, the ceramics' meshes) against the numbers below.
 */
namespace ChenghuaiDisplayImpl
{
	namespace Kit = ChenghuaiKit;
	namespace CB = ChenghuaiBuild;
	namespace CH = Chenghuai;
	using Kit::FLocal;
	using Kit::FPart;
	using CB::FChMeshes;
	using CB::FChLight;

	FLocal Frame(double X, double Y, double UX, double UY, double VX, double VY)
	{
		FLocal L;
		L.Origin = FVector2D(X, Y);
		L.U = FVector2D(UX, UY);
		L.V = FVector2D(VX, VY);
		return L;
	}

	/** A plate along u (U0 … U1) whose section runs straight from (VA, ZA) to (VB, ZB), T thick, centred on that line. */
	void Plate(FPart& P, const TCHAR* Name, const FLocal& L, double U0, double U1, double VA, double ZA, double VB, double ZB, double T)
	{
		const double Vm = 0.5 * (VA + VB), Zm = 0.5 * (ZA + ZB);
		const FVector Side = L.Dir(0.0, VB - VA, ZB - ZA);
		Kit::Member(P, Name, L.P(U0, Vm, Zm), L.P(U1, Vm, Zm), Side, Side.Size(), T);
	}

	/** A board in the (v, z) section (an end of a slanted case), U0 … U1 thick. */
	void EndBoard(FPart& P, const TCHAR* Name, const FLocal& L, double U0, double U1, const TArray<FVector2D>& VZ)
	{
		Kit::Extrude(P, Name, VZ, L.P(0, 0, 0), L.Dir(0, 1, 0), FVector(0, 0, 1), L.U3(), U0, U1);
	}

	FChLight Lamp(const FString& Name, const FVector& Centre, const FVector& Facing, const FVector& Along, double W, double H, double Lumens)
	{
		FChLight Out;
		Out.Name = Name;
		Out.Centre = Centre;
		Out.Facing = Facing.GetSafeNormal();
		Out.Along = Along.GetSafeNormal();
		Out.Width = W;
		Out.Height = H;
		Out.Lumens = Lumens;
		Out.bShadows = true;
		Out.bDaylit = false;
		return Out;
	}

	// ------------------------------------------------------------------------------------------------ the cases

	/** The slant of a handscroll case's bed (toward the reader). */
	const double SlantTan = FMath::Tan(FMath::DegreesToRadians(11.0));
	constexpr double SlantBodyTop = 0.84, SlantBed = 0.905, SlantFront = 1.10;

	/**
	 * A slanted handscroll case (Hang board: 舒卷, 臨池): a nanmu cabinet on a black toe, a silk bed tilted 11° to the
	 * reader, a glass hood (upright front, sloped top) between nanmu ends, a lamp rail at the back under the glass. Frame
	 * L: u along the case, v from the reader's side (0) to the wall (D); floor F.
	 */
	void SlantCase(FChMeshes& Out, TArray<FChLight>* Lights, const TCHAR* Name, const FLocal& L, double U0, double U1, double D, double F)
	{
		FPart& Wood = Out[CB::Nanmu];
		const double ZF = F + SlantFront, ZB = ZF + SlantTan * D;
		if (!Lights)
		{
			Kit::LBox(Out[CB::BlackLacquer], TEXT("Case toe"), L, U0 + 0.04, U1 - 0.04, 0.05, D, F, F + 0.09);
			Kit::LBox(Wood, TEXT("Case body"), L, U0, U1, 0.0, D, F + 0.09, F + SlantBodyTop);
			Kit::LBox(Wood, TEXT("Case base moulding"), L, U0 - 0.008, U1 + 0.008, -0.008, D, F + 0.09, F + 0.13);
			Kit::LBox(Wood, TEXT("Case cornice"), L, U0 - 0.012, U1 + 0.012, -0.014, D, F + SlantBodyTop, F + 0.88);
			// Panels on the cabinet's front, a bay to each metre and a half (frame and field, the field set back).
			const int32 NP = FMath::Max(1, FMath::RoundToInt32((U1 - U0) / 1.5));
			for (int32 k = 0; k < NP; ++k)
			{
				const double A = FMath::Lerp(U0, U1, double(k) / NP) + 0.05, B = FMath::Lerp(U0, U1, double(k + 1) / NP) - 0.05;
				Kit::LBox(Wood, TEXT("Case panel bead"), L, A, B, -0.006, 0.0, F + 0.18, F + 0.19);
				Kit::LBox(Wood, TEXT("Case panel bead"), L, A, B, -0.006, 0.0, F + 0.78, F + 0.79);
				Kit::LBox(Wood, TEXT("Case panel bead"), L, A, A + 0.01, -0.006, 0.0, F + 0.19, F + 0.78);
				Kit::LBox(Wood, TEXT("Case panel bead"), L, B - 0.01, B, -0.006, 0.0, F + 0.19, F + 0.78);
			}
			// The bed: silk over a board, tilted to the reader.
			const double VA = 0.05, VB = D - 0.06;
			const double ZA = F + SlantBed, ZBd = ZA + SlantTan * (VB - VA);
			Plate(Out[CB::CaseSilk], TEXT("Case bed"), L, U0 + 0.04, U1 - 0.04, VA, ZA, VB, ZBd, 0.02);
			// Its support: a wedge of nanmu under the bed.
			EndBoard(Wood, TEXT("Case bed wedge"), L, U0 + 0.04, U1 - 0.04,
					 {FVector2D(VA, F + 0.88), FVector2D(VB, F + 0.88), FVector2D(VB, ZBd - 0.01), FVector2D(VA, ZA - 0.01)});
			// The ends, the back upstand and the lamp rail.
			for (int32 e = 0; e < 2; ++e)
			{
				const double A = e ? U1 - 0.035 : U0, B = e ? U1 : U0 + 0.035;
				EndBoard(Wood, TEXT("Case end"), L, A, B,
						 {FVector2D(-0.014, F + 0.88), FVector2D(D, F + 0.88), FVector2D(D, ZB + 0.05), FVector2D(-0.014, ZF + 0.006)});
			}
			Kit::LBox(Wood, TEXT("Case back"), L, U0 + 0.035, U1 - 0.035, D - 0.03, D, F + 0.88, ZB + 0.05);
			Kit::LBox(Wood, TEXT("Case front rail"), L, U0 + 0.035, U1 - 0.035, -0.012, 0.022, ZF - 0.024, ZF);
			Kit::LBox(Out[CB::Brass], TEXT("Lamp rail"), L, U0 + 0.05, U1 - 0.05, D - 0.1, D - 0.03, ZB - 0.075, ZB - 0.04);
			// The lamp strip on the rail's underside, out of the reader's sight (a line of light along the case read as a fitting).
			Kit::LBox(Out[CB::CaseLamp], TEXT("Lamp strip"), L, U0 + 0.06, U1 - 0.06, D - 0.092, D - 0.038, ZB - 0.077, ZB - 0.0745);
			// The glass: the upright front and the sloped top.
			Plate(Out[CB::Glass], TEXT("Case glass front"), L, U0 + 0.035, U1 - 0.035, 0.004, F + 0.88, 0.004, ZF - 0.024, 0.008);
			Plate(Out[CB::Glass], TEXT("Case glass top"), L, U0 + 0.035, U1 - 0.035, 0.0, ZF + 0.004, D - 0.03, ZB + 0.0 - 0.03 * SlantTan + 0.004, 0.008);
			Kit::LBox(Out[CB::Guard], TEXT("Case guard"), L, U0 - 0.02, U1 + 0.02, -0.02, D, F, ZB + 0.1);
			return;
		}
		// The lamp: warm, low (50 lux on the bed), down and forward from the rail.
		const double Len = U1 - U0 - 0.12;
		Lights->Add(Lamp(FString::Printf(TEXT("%s lamp"), Name), L.P(0.5 * (U0 + U1), D - 0.065, ZB - 0.082), L.Dir(0.0, -0.62, -0.78),
						 L.U3(), Len, 0.018, 70.0 * Len));
	}

	/**
	 * An upright case against a wall (Hang board section): a nanmu plinth on a black toe, a silk deck and back, nanmu
	 * ends and mullions, the glass front, a canopy with its baffled lamp strip; for scrolls, a brass hanging rail. Frame
	 * L: u along the wall, v from the glass (0) to the wall (D); floor F; the deck at Deck, the canopy's underside at
	 * Canopy, its top at Top.
	 */
	void UprightCase(FChMeshes& Out, TArray<FChLight>* Lights, const TCHAR* Name, const FLocal& L, double U0, double U1, double D, double F,
					 double Deck, double Canopy, double Top, const TArray<double>& Mullions, bool bScrollRail, double LumensPerM)
	{
		FPart& Wood = Out[CB::Nanmu];
		if (!Lights)
		{
			Kit::LBox(Out[CB::BlackLacquer], TEXT("Case toe"), L, U0 + 0.03, U1 - 0.03, 0.04, D, F, F + 0.08);
			Kit::LBox(Wood, TEXT("Case plinth"), L, U0, U1, 0.0, D, F + 0.08, Deck);
			Kit::LBox(Wood, TEXT("Case plinth moulding"), L, U0 - 0.008, U1 + 0.008, -0.01, D, Deck - 0.04, Deck);
			Kit::LBox(Wood, TEXT("Case plinth moulding"), L, U0 - 0.006, U1 + 0.006, -0.006, D, F + 0.08, F + 0.12);
			// The plinth's drawer (the conditioning drawer of the Hang board's section) and its pulls.
			if (Deck - F > 0.3)
			{
				Kit::LBox(Wood, TEXT("Case drawer bead"), L, U0 + 0.08, U1 - 0.08, -0.005, 0.0, F + 0.16, F + 0.17);
				Kit::LBox(Wood, TEXT("Case drawer bead"), L, U0 + 0.08, U1 - 0.08, -0.005, 0.0, Deck - 0.08, Deck - 0.07);
			}
			Kit::LBox(Out[CB::CaseSilk], TEXT("Case deck"), L, U0 + 0.045, U1 - 0.045, 0.02, D - 0.02, Deck, Deck + 0.012);
			Kit::LBox(Out[CB::CaseSilk], TEXT("Case back"), L, U0 + 0.045, U1 - 0.045, D - 0.035, D - 0.01, Deck + 0.012, Canopy);
			Kit::LBox(Wood, TEXT("Case back board"), L, U0 + 0.045, U1 - 0.045, D - 0.01, D, Deck, Canopy);
			for (int32 e = 0; e < 2; ++e)
			{
				Kit::LBox(Wood, TEXT("Case end"), L, e ? U1 - 0.045 : U0, e ? U1 : U0 + 0.045, 0.0, D, Deck, Canopy);
			}
			for (double M : Mullions) { Kit::LBox(Wood, TEXT("Case mullion"), L, M - 0.025, M + 0.025, -0.006, 0.05, Deck, Canopy); }
			Kit::LBox(Wood, TEXT("Case canopy"), L, U0, U1, 0.0, D, Canopy, Top - 0.05);
			Kit::LBox(Wood, TEXT("Case cornice"), L, U0 - 0.02, U1 + 0.02, -0.02, D, Top - 0.05, Top);
			Kit::LBox(Wood, TEXT("Case canopy baffle"), L, U0 + 0.045, U1 - 0.045, 0.0, 0.03, Canopy - 0.05, Canopy);
			Kit::LBox(Out[CB::CaseLamp], TEXT("Lamp strip"), L, U0 + 0.06, U1 - 0.06, 0.05, 0.1, Canopy - 0.004, Canopy);
			if (Canopy - Deck > 2.0)
			{
				// The low strip's lip: a nanmu fillet along the deck's front, hiding the strip behind it.
				Kit::LBox(Wood, TEXT("Case deck lip"), L, U0 + 0.045, U1 - 0.045, 0.02, 0.045, Deck + 0.012, Deck + 0.06);
				Kit::LBox(Out[CB::CaseLamp], TEXT("Low lamp strip"), L, U0 + 0.06, U1 - 0.06, 0.05, 0.09, Deck + 0.012, Deck + 0.016);
			}
			Plate(Out[CB::Glass], TEXT("Case glass"), L, U0 + 0.045, U1 - 0.045, 0.012, Deck, 0.012, Canopy - 0.05, 0.01);
			if (bScrollRail)
			{
				Kit::Rod(Out[CB::Brass], TEXT("Hanging rail"), L.P(U0 + 0.045, D - 0.05, Canopy - 0.05), L.P(U1 - 0.045, D - 0.05, Canopy - 0.05), 0.008, 12);
			}
			Kit::LBox(Out[CB::Guard], TEXT("Case guard"), L, U0 - 0.02, U1 + 0.02, -0.02, D, F, Top);
			return;
		}
		const double Len = U1 - U0 - 0.14;
		const bool bTall = Canopy - Deck > 2.0;
		// The canopy's strip, washing the back panel (tilted 35 degrees towards it); a tall scroll case also has a low
		// strip behind a lip at the deck's front, washing up, so a 3 m scroll is lit evenly and not only at its head.
		Lights->Add(Lamp(FString::Printf(TEXT("%s lamp"), Name), L.P(0.5 * (U0 + U1), 0.075, Canopy - 0.006), L.Dir(0.0, 0.57, -0.82), L.U3(), Len, 0.04,
						 LumensPerM * Len * (bTall ? 1.6 : 1.0)));
		if (bTall)
		{
			Lights->Add(Lamp(FString::Printf(TEXT("%s low lamp"), Name), L.P(0.5 * (U0 + U1), 0.07, Deck + 0.02), L.Dir(0.0, 0.34, 0.94), L.U3(), Len, 0.03,
							 LumensPerM * Len * 0.7));
		}
	}

	/** A free-standing glass case on a nanmu pedestal (the Ru basin's). Centre (X, Y), floor F. */
	void PedestalCase(FChMeshes& Out, TArray<FChLight>* Lights, double X, double Y, double F, double Half, double PedH, double GlassH)
	{
		const FLocal L = Frame(X - Half, Y - Half, 1, 0, 0, 1);
		const double S = 2.0 * Half, Z1 = F + PedH, Z2 = Z1 + GlassH;
		if (!Lights)
		{
			Kit::LBox(Out[CB::BlackLacquer], TEXT("Pedestal toe"), L, 0.04, S - 0.04, 0.04, S - 0.04, F, F + 0.08);
			Kit::LBox(Out[CB::Nanmu], TEXT("Pedestal"), L, 0.0, S, 0.0, S, F + 0.08, Z1 - 0.04);
			Kit::LBox(Out[CB::Nanmu], TEXT("Pedestal cap"), L, -0.015, S + 0.015, -0.015, S + 0.015, Z1 - 0.04, Z1);
			Kit::LBox(Out[CB::CaseSilk], TEXT("Pedestal deck"), L, 0.04, S - 0.04, 0.04, S - 0.04, Z1, Z1 + 0.012);
			const double G = 0.01;
			Kit::LBox(Out[CB::Glass], TEXT("Hood"), L, 0.0, G, 0.0, S, Z1, Z2);
			Kit::LBox(Out[CB::Glass], TEXT("Hood"), L, S - G, S, 0.0, S, Z1, Z2);
			Kit::LBox(Out[CB::Glass], TEXT("Hood"), L, G, S - G, 0.0, G, Z1, Z2);
			Kit::LBox(Out[CB::Glass], TEXT("Hood"), L, G, S - G, S - G, S, Z1, Z2);
			Kit::LBox(Out[CB::Glass], TEXT("Hood"), L, G, S - G, G, S - G, Z2 - G, Z2);
			Kit::LBox(Out[CB::Guard], TEXT("Pedestal guard"), L, -0.02, S + 0.02, -0.02, S + 0.02, F, Z2);
			return;
		}
		Lights->Add(Lamp(TEXT("Pedestal lamp"), FVector(X, Y, F + 2.7), FVector(0, 0, -1), FVector(1, 0, 0), 0.12, 0.12, 90.0));
	}

	// ------------------------------------------------------------------------------------------------ furniture

	/** A Ming table (画案 / 条案): top, legs set in, aprons with spandrels; optional upturned ends (翘头). */
	void Table(FChMeshes& Out, const FLocal& L, double U0, double U1, double V0, double V1, double F, double H, bool bUpturned)
	{
		FPart& W = Out[CB::Nanmu];
		const double T = 0.035, Leg = 0.045, In = 0.06;
		Kit::LBox(W, TEXT("Table top"), L, U0, U1, V0, V1, F + H - T, F + H);
		if (bUpturned)
		{
			for (int32 e = 0; e < 2; ++e)
			{
				const double A = e ? U1 - 0.09 : U0, B = e ? U1 : U0 + 0.09;
				Kit::LBox(W, TEXT("Upturned end"), L, A, B, V0, V1, F + H, F + H + 0.03);
				Kit::LBox(W, TEXT("Upturned end"), L, e ? U1 - 0.05 : U0, e ? U1 : U0 + 0.05, V0, V1, F + H + 0.03, F + H + 0.06);
			}
		}
		for (int32 a = 0; a < 2; ++a)
		{
			for (int32 b = 0; b < 2; ++b)
			{
				const double U = a ? U1 - In - Leg : U0 + In, V = b ? V1 - 0.03 - Leg : V0 + 0.03;
				Kit::LBox(W, TEXT("Table leg"), L, U, U + Leg, V, V + Leg, F, F + H - T);
			}
		}
		for (int32 b = 0; b < 2; ++b)
		{
			const double V = b ? V1 - 0.03 - Leg + 0.01 : V0 + 0.03 + 0.005;
			Kit::LBox(W, TEXT("Table apron"), L, U0 + In, U1 - In, V, V + 0.02, F + H - T - 0.07, F + H - T);
			// The stretchers low between the legs at the ends (a 条案's).
			Kit::LBox(W, TEXT("Table stretcher"), L, b ? U1 - In - Leg : U0 + In, (b ? U1 - In - Leg : U0 + In) + Leg, V0 + 0.05, V1 - 0.05, F + 0.14, F + 0.17);
		}
	}

	/**
	 * A southern official's hat chair (南官帽椅) in the Ming manner, all round members: the back legs run on up as the back
	 * posts, leaning back, to a crest rail bowed backwards and flush with them; the front legs run up to the arms, which
	 * bow a little outward to the back posts; an S-curved splat; a framed seat with a woven panel a touch below its
	 * rails; an arched front apron; stepped stretchers (步步高), the footrest lowest. Facing −v from centre (U, V).
	 */
	void Chair(FChMeshes& Out, const FLocal& L, double U, double V, double F)
	{
		FPart& W = Out[CB::Nanmu];
		const double Hw = 0.29, D = 0.46, Seat = 0.5, R = 0.017;
		const double Vf = V - 0.5 * D + R + 0.005, Vb = V + 0.5 * D - R - 0.005;
		const double Zs = F + Seat, Za = Zs + 0.23, Zc = F + 1.03, Lean = 0.045;
		auto BackV = [&](double Z) { return Vb + Lean * FMath::Clamp((Z - Zs) / (Zc - Zs), 0.0, 1.0); };
		// The seat: a frame of four rails and the woven panel.
		Kit::LBox(W, TEXT("Chair seat rail"), L, U - Hw, U + Hw, V - 0.5 * D, V - 0.5 * D + 0.05, Zs - 0.036, Zs);
		Kit::LBox(W, TEXT("Chair seat rail"), L, U - Hw, U + Hw, V + 0.5 * D - 0.05, V + 0.5 * D, Zs - 0.036, Zs);
		Kit::LBox(W, TEXT("Chair seat rail"), L, U - Hw, U - Hw + 0.05, V - 0.5 * D + 0.05, V + 0.5 * D - 0.05, Zs - 0.036, Zs);
		Kit::LBox(W, TEXT("Chair seat rail"), L, U + Hw - 0.05, U + Hw, V - 0.5 * D + 0.05, V + 0.5 * D - 0.05, Zs - 0.036, Zs);
		Kit::LBox(Out[CB::CaseSilk], TEXT("Chair seat panel"), L, U - Hw + 0.049, U + Hw - 0.049, V - 0.5 * D + 0.049, V + 0.5 * D - 0.049, Zs - 0.024, Zs - 0.008);
		for (int32 a = 0; a < 2; ++a)
		{
			const double S = a ? 1.0 : -1.0;
			const double X = U + S * (Hw - R - 0.005);
			// Front leg up to the arm; back leg up to the crest, leaning back above the seat.
			Kit::Rod(W, TEXT("Chair front leg"), L.P(X, Vf, F), L.P(X, Vf, Za), R, 12);
			Kit::Rod(W, TEXT("Chair back leg"), L.P(X, Vb, F), L.P(X, Vb, Zs), R, 12);
			Kit::Rod(W, TEXT("Chair back post"), L.P(X, Vb, Zs - 0.01), L.P(X, BackV(Zc), Zc), R, 12, 0.9 * R);
			// The arm, bowed outward, and the small post under it (联帮棍), swelling at its foot.
			const FVector A0 = L.P(X, Vf, Za), A1 = L.P(X + S * 0.02, V, Za + 0.008), A2 = L.P(X, BackV(Za + 0.015), Za + 0.015);
			Kit::Rod(W, TEXT("Chair arm"), A0 + L.Dir(0, 0.01, 0) * -1.0, A1, 0.015, 10);
			Kit::Rod(W, TEXT("Chair arm"), A1, A2, 0.015, 10);
			Kit::Rod(W, TEXT("Chair arm post"), L.P(X + S * 0.004, V - 0.02, Zs), L.P(X + S * 0.018, V - 0.02, Za + 0.005), 0.011, 8, 0.007);
			// Stepped stretchers: the side ones.
			Kit::Rod(W, TEXT("Chair side stretcher"), L.P(X, Vf, F + 0.14), L.P(X, Vb, F + 0.14), 0.011, 8);
		}
		// Footrest (front, lowest), back stretcher (highest); the footrest a flat bar worn by feet.
		Kit::LBox(W, TEXT("Chair footrest"), L, U - Hw + 0.02, U + Hw - 0.02, Vf - 0.012, Vf + 0.012, F + 0.06, F + 0.085);
		Kit::Rod(W, TEXT("Chair back stretcher"), L.P(U - Hw + 0.02, Vb, F + 0.2), L.P(U + Hw - 0.02, Vb, F + 0.2), 0.011, 8);
		// The crest rail, bowed back, its ends flush over the posts.
		const double Xl = U - (Hw - R - 0.005), Xr = U + (Hw - R - 0.005), Vc = BackV(Zc);
		const FVector C0 = L.P(Xl, Vc, Zc), C1 = L.P(U - 0.11, Vc + 0.028, Zc + 0.012), C2 = L.P(U + 0.11, Vc + 0.028, Zc + 0.012), C3 = L.P(Xr, Vc, Zc);
		Kit::Rod(W, TEXT("Chair crest rail"), C0, C1, 0.02, 12);
		Kit::Rod(W, TEXT("Chair crest rail"), C1, C2, 0.02, 12);
		Kit::Rod(W, TEXT("Chair crest rail"), C2, C3, 0.02, 12);
		// The splat: 0.16 wide, S-curved (in at the lumbar, back at the shoulders), from the back seat rail to the crest.
		const double SplatV[5] = {0.0, -0.012, 0.0, 0.03, 0.028};
		for (int32 k = 0; k < 4; ++k)
		{
			const double Z0 = Zs + (Zc - Zs) * k / 4.0, Z1 = Zs + (Zc - Zs) * (k + 1) / 4.0;
			Kit::Member(W, TEXT("Chair splat"), L.P(U, BackV(Z0) + SplatV[k], Z0 - (k ? 0.002 : 0.0)), L.P(U, BackV(Z1) + SplatV[k + 1], Z1 + (k == 3 ? -0.01 : 0.002)), L.U3(), 0.16, 0.016);
		}
		// The front apron with its shallow arch (a board and two spandrels), and the side aprons.
		Kit::LBox(W, TEXT("Chair apron"), L, U - Hw + 0.03, U + Hw - 0.03, V - 0.5 * D + 0.006, V - 0.5 * D + 0.022, Zs - 0.07, Zs - 0.036);
		for (int32 a = 0; a < 2; ++a)
		{
			const double S = a ? 1.0 : -1.0;
			Kit::LBox(W, TEXT("Chair apron spandrel"), L, a ? U + Hw - 0.13 : U - Hw + 0.03, a ? U + Hw - 0.03 : U - Hw + 0.13, V - 0.5 * D + 0.006,
					  V - 0.5 * D + 0.022, Zs - 0.12, Zs - 0.07);
			Kit::LBox(W, TEXT("Chair side apron"), L, U + S * (Hw - 0.022) - 0.008, U + S * (Hw - 0.022) + 0.008, Vf + 0.02, Vb - 0.02, Zs - 0.07, Zs - 0.036);
		}
	}

	/** The scholar's things on a desk: a Duan inkstone, a brush pot with brushes, a brush rest, a sheet of paper. */
	void DeskThings(FChMeshes& Out, const FLocal& L, double U, double V, double Z)
	{
		Kit::LBox(Out[CB::PondBed], TEXT("Inkstone"), L, U - 0.09, U + 0.09, V - 0.13, V + 0.13, Z, Z + 0.03);
		Kit::LBox(Out[CB::Nanmu], TEXT("Inkstone box"), L, U - 0.1, U + 0.1, V - 0.14, V + 0.14, Z - 0.0, Z + 0.012);
		Kit::Rod(Out[CB::Chestnut], TEXT("Brush pot"), L.P(U + 0.35, V + 0.05, Z), L.P(U + 0.35, V + 0.05, Z + 0.16), 0.065, 24);
		for (int32 k = 0; k < 6; ++k)
		{
			const double A = k * 1.05, R = 0.03;
			const FVector B = L.P(U + 0.35 + R * FMath::Cos(A), V + 0.05 + R * FMath::Sin(A), Z + 0.12);
			Kit::Rod(Out[CB::Chestnut], TEXT("Brush"), B, B + L.Dir(0.03 * FMath::Cos(A), 0.03 * FMath::Sin(A), 0.14), 0.005, 6);
		}
		Kit::LBox(Out[CB::Paper], TEXT("Paper sheet"), L, U - 0.75, U - 0.2, V - 0.2, V + 0.2, Z, Z + 0.002);
		Kit::LBox(Out[CB::BlueStone], TEXT("Paperweight"), L, U - 0.72, U - 0.22, V - 0.2, V - 0.17, Z + 0.002, Z + 0.02);
		Kit::LBox(Out[CB::BlueStone], TEXT("Paperweight"), L, U - 0.72, U - 0.22, V + 0.17, V + 0.2, Z + 0.002, Z + 0.02);
	}

	/**
	 * A carved open screen (落地罩) across a bay of the main hall: posts at its ends, fretwork legs on a solid base, a
	 * fretwork frieze under the beam and corner spandrels, all nanmu. In the plane x = X from y Y0 (the front) to Y1.
	 */
	void OpenScreen(FChMeshes& Out, double X, double Y0, double Y1, double F, double TopZ)
	{
		using ChenghuaiHall::ELattice;
		const FLocal L = Frame(X, Y0, 0, -1, 1, 0);   // u from the front back (north), v across the screen
		const double Len = Y0 - Y1, Post = 0.09, LegW = 0.55, Frieze = 0.5, Base = 0.42;
		FPart& W = Out[CB::Nanmu];
		for (int32 e = 0; e < 2; ++e)
		{
			const double A = e ? Len - Post : 0.0;
			Kit::LBox(W, TEXT("Screen post"), L, A, A + Post, -0.045, 0.045, F, TopZ);
			const double LA = e ? Len - Post - LegW : Post, LB = LA + LegW;
			Kit::LBox(W, TEXT("Screen leg post"), L, e ? LA - 0.06 : LB, e ? LA : LB + 0.06, -0.035, 0.035, F, TopZ - Frieze);
			Kit::LBox(W, TEXT("Screen base"), L, e ? LA - 0.06 : LA, e ? LB : LB + 0.06, -0.03, 0.03, F, F + Base);
			ChenghuaiHall::LatticePanel(Out, L, LA, -0.012, F + Base, LegW, TopZ - Frieze - F - Base, ELattice::StepBrocade, CB::Nanmu, false);
		}
		Kit::LBox(W, TEXT("Screen frieze rail"), L, Post, Len - Post, -0.035, 0.035, TopZ - Frieze - 0.06, TopZ - Frieze);
		Kit::LBox(W, TEXT("Screen head"), L, 0.0, Len, -0.045, 0.045, TopZ - 0.07, TopZ);
		ChenghuaiHall::LatticePanel(Out, L, Post, -0.012, TopZ - Frieze, Len - 2.0 * Post, Frieze - 0.07, ELattice::Lantern, CB::Nanmu, false);
		// The spandrels (花牙子) in the opening's upper corners.
		const double SA = Post + LegW + 0.06, SB = Len - Post - LegW - 0.06;
		for (int32 e = 0; e < 2; ++e)
		{
			const double A = e ? SB - 0.42 : SA;
			ChenghuaiHall::LatticePanel(Out, L, A, -0.012, TopZ - Frieze - 0.06 - 0.3, 0.42, 0.3, ELattice::Grid, CB::Nanmu, false);
		}
	}

	// ------------------------------------------------------------------------------------------------ lanterns

	/**
	 * A plain hexagonal paper lantern (纱灯) hung from a hook: a black-lacquered frame (top, six posts, bottom), mulberry
	 * paper panels lit from within after dusk, no characters and no tassel (a scholar's house, the user's rule: nothing
	 * written on or over its doors and gates). Hook at (X, Y, Top); R the hexagon's radius, H the paper's height.
	 * Its light: the paper panels would shadow a lamp inside them, so the lamp's light leaves as it does from a real
	 * lantern's open top and bottom: a warm pool thrown down from under its base and a softer one up onto the eave
	 * above it (both shadowed, so nothing leaks through a roof or a wall).
	 */
	void Lantern(FChMeshes& Out, TArray<FChLight>* Lights, const TCHAR* Name, double X, double Y, double Top, double Rod, double R = 0.17,
				 double H = 0.36)
	{
		const double Z1 = Top - Rod - 0.05, Z0 = Z1 - H;
		if (Lights)
		{
			const double S = 1.6 * R;
			FChLight Down = Lamp(FString::Printf(TEXT("%s lantern (down)"), Name), FVector(X, Y, Z0 - 0.05), FVector(0, 0, -1), FVector(1, 0, 0), S, S,
								 210.0 * (R / 0.17) * (H / 0.36));
			Down.bNight = true;
			Lights->Add(Down);
			FChLight Up = Lamp(FString::Printf(TEXT("%s lantern (up)"), Name), FVector(X, Y, Z1 + 0.06), FVector(0, 0, 1), FVector(1, 0, 0), 0.7 * S, 0.7 * S,
							   90.0 * (R / 0.17) * (H / 0.36));
			Up.bNight = true;
			Lights->Add(Up);
			return;
		}
		FPart& Wood = Out[CB::BlackLacquer];
		Kit::Rod(Out[CB::Brass], TEXT("Lantern rod"), FVector(X, Y, Top), FVector(X, Y, Z1 + 0.045), 0.005, 8);
		TArray<FVector2D> Hex, HexS;
		for (int32 k = 0; k < 6; ++k)
		{
			const double A = PI / 3.0 * k;
			Hex.Add(FVector2D(X + (R + 0.018) * FMath::Cos(A), Y + (R + 0.018) * FMath::Sin(A)));
			HexS.Add(FVector2D(X + (R - 0.05) * FMath::Cos(A), Y + (R - 0.05) * FMath::Sin(A)));
		}
		Kit::Slab(Wood, TEXT("Lantern top"), Hex, Z1, Z1 + 0.025);
		Kit::Slab(Wood, TEXT("Lantern crown"), HexS, Z1 + 0.025, Z1 + 0.045);
		Kit::Slab(Wood, TEXT("Lantern base"), Hex, Z0 - 0.025, Z0);
		Kit::Slab(Wood, TEXT("Lantern foot"), HexS, Z0 - 0.045, Z0 - 0.025);
		for (int32 k = 0; k < 6; ++k)
		{
			const double A = PI / 3.0 * k, B = PI / 3.0 * (k + 1);
			const FVector P(X + R * FMath::Cos(A), Y + R * FMath::Sin(A), 0.0);
			Kit::Box(Wood, TEXT("Lantern post"), FVector(P.X - 0.01, P.Y - 0.01, Z0), FVector(P.X + 0.01, P.Y + 0.01, Z1));
			// A paper panel between posts k and k + 1 (a thin plate, lit from within), a thin rail across its middle.
			const FVector2D PA(X + (R - 0.004) * FMath::Cos(A), Y + (R - 0.004) * FMath::Sin(A));
			const FVector2D PB(X + (R - 0.004) * FMath::Cos(B), Y + (R - 0.004) * FMath::Sin(B));
			const FVector2D Dir = (PB - PA).GetSafeNormal();
			const FVector2D Nrm(Dir.Y, -Dir.X);
			TArray<FVector2D> Panel = {PA - Nrm * 0.002, PB - Nrm * 0.002, PB + Nrm * 0.002, PA + Nrm * 0.002};
			Kit::Slab(Out[CB::LanternSilk], TEXT("Lantern paper"), Panel, Z0 + 0.005, Z1 - 0.005);
			const FVector2D QA(X + (R + 0.002) * FMath::Cos(A), Y + (R + 0.002) * FMath::Sin(A));
			const FVector2D QB(X + (R + 0.002) * FMath::Cos(B), Y + (R + 0.002) * FMath::Sin(B));
			const double Zm = 0.5 * (Z0 + Z1);
			Kit::Member(Wood, TEXT("Lantern rail"), FVector(QA.X, QA.Y, Zm), FVector(QB.X, QB.Y, Zm), FVector::UpVector, 0.012, 0.008);
		}
	}

	void Lanterns(FChMeshes& Out, TArray<FChLight>* Lights)
	{
		// Plain paper lanterns where a house hangs its evening light: under the main hall's veranda, along the galleries
		// and the side halls' verandas, at the doors of the front and rear rows, down the garden's covered walk, in the
		// water pavilion and before the flower hall. None at, on or over the gate or the festooned gate.
		const double A = CH::Axis;
		Lantern(Out, Lights, TEXT("Hall W"), A - 1.0, CH::HallEave - 0.15, CH::HallPlinth + CH::HallCol + 0.25, 0.86);
		Lantern(Out, Lights, TEXT("Hall E"), A + 1.0, CH::HallEave - 0.15, CH::HallPlinth + CH::HallCol + 0.25, 0.86);
		// The south gallery behind the court wall (small: a gallery is 2.6 m to its lintels).
		const double GY = 0.5 * (CH::GallerySouthY0 + CH::GallerySouthY1) - 0.2;
		const double GT = CH::GalleryFloor + CH::GalleryCol + 0.15;
		Lantern(Out, Lights, TEXT("Gallery SW"), A - 3.8, GY, GT, 0.3, 0.12, 0.26);
		Lantern(Out, Lights, TEXT("Gallery SE"), A + 3.8, GY, GT, 0.3, 0.12, 0.26);
		// The side halls' verandas, their middle bays.
		const double SX = 0.5 * (CH::SideHallFace + CH::SideHallEave), SY = 0.5 * (CH::SideHallCols[1] + CH::SideHallCols[2]);
		const double ST = CH::SideHallPlinth + CH::SideHallCol + 0.25;
		Lantern(Out, Lights, TEXT("Tianqing veranda"), SX, SY, ST, 0.47, 0.15, 0.32);
		Lantern(Out, Lights, TEXT("Changnan veranda"), 2.0 * A - SX, SY, ST, 0.47, 0.15, 0.32);
		// The front row's and the rear row's doors, just outside their heads.
		const double FB = 17.9 / 6.0, RB = 22.0 / 7.0;
		const double FT = CH::FrontRowPlinth + CH::FrontRowCol + 0.3, RT = CH::RearRowPlinth + CH::RearRowCol + 0.3;
		Lantern(Out, Lights, TEXT("Linchi W door"), CH::SiteW + 0.5 + 0.5 * FB, CH::FrontRowFace - 0.28, FT, 0.52, 0.13, 0.26);
		Lantern(Out, Lights, TEXT("Linchi E door"), CH::SiteW + 0.5 + 5.5 * FB, CH::FrontRowFace - 0.28, FT, 0.52, 0.13, 0.26);
		Lantern(Out, Lights, TEXT("Shujuan W door"), CH::SiteW + 0.5 + 0.5 * RB, CH::RearRowFace + 0.28, RT, 0.52, 0.13, 0.26);
		Lantern(Out, Lights, TEXT("Shujuan E door"), CH::SiteW + 0.5 + 6.5 * RB, CH::RearRowFace + 0.28, RT, 0.52, 0.13, 0.26);
		// The covered walk (2.1 m to its lintels: small, in the roof above its middle).
		for (const double Y : {-26.4, -34.5, -42.6, -50.7})
		{
			Lantern(Out, Lights, *FString::Printf(TEXT("Walk %.0f"), -Y), 0.5 * (CH::WalkX0 + CH::WalkColX), Y, 0.15 + 2.1 + 0.3, 0.12, 0.11, 0.24);
		}
		// The water pavilion; the flower hall's pair before its doors.
		Lantern(Out, Lights, TEXT("Zhiyu"), 0.5 * (CH::WaterPavX0 + CH::WaterPavX1), 0.5 * (CH::WaterPavY0 + CH::WaterPavY1), 0.35 + 2.6 + 0.25, 0.25);
		Lantern(Out, Lights, TEXT("Flower hall W"), 9.9, CH::FlowerHallS + 0.1, 0.45 + 3.0 - 0.14, 0.35);
		Lantern(Out, Lights, TEXT("Flower hall E"), 11.4, CH::FlowerHallS + 0.1, 0.45 + 3.0 - 0.14, 0.35);
	}

	// ------------------------------------------------------------------------------------------------ the rooms

	/** The ear rooms' side cases (on the gable wall opposite the hall's door, y −49.6 … −48.4). */
	constexpr double EarSideCaseY0 = -49.6, EarSideCaseY1 = -48.4;

	/**
	 * The rooms' museum light besides the cases: a slim black track fixed under a lintel (a wall washer, 3000 K, lensed
	 * and baffled), from A to B at height Z, throwing its light across the room onto the far wall and the floor (Facing:
	 * down and across). It keeps the walls, the floor and the corners of a room lit only through paper legible, day and
	 * night, the works still the brightest things (the cases carry their own 50-80 lux).
	 */
	void Washer(FChMeshes& Out, TArray<FChLight>* Lights, const TCHAR* Name, const FVector2D& A, const FVector2D& B, double Z, const FVector& Facing,
				double LumensPerM)
	{
		const FVector2D D = (B - A).GetSafeNormal();
		const double Len = FVector2D::Distance(A, B);
		const FVector2D M = 0.5 * (A + B);
		if (Lights)
		{
			FChLight L = Lamp(FString::Printf(TEXT("%s washer"), Name), FVector(M.X, M.Y, Z - 0.035), Facing, FVector(D.X, D.Y, 0.0), Len - 0.1, 0.03, LumensPerM * Len);
			Lights->Add(L);
			return;
		}
		Kit::Member(Out[CB::BlackLacquer], TEXT("Washer track"), FVector(A.X, A.Y, Z - 0.015), FVector(B.X, B.Y, Z - 0.015), FVector::UpVector, 0.03, 0.045, 0.004);
	}

	/** The washers of every room (plan metres; heights under each room's lintels). */
	void RoomLight(FChMeshes& Out, TArray<FChLight>* Lights)
	{
		const double A = CH::Axis;
		auto Dir = [](double X, double Y) { return FVector(X, Y, -1.0).GetSafeNormal(); };
		// 臨池 (the front row): along its lattice front, across onto the case and the south wall; a weaker one back.
		Washer(Out, Lights, TEXT("Linchi S"), FVector2D(-19.6, -21.95), FVector2D(-2.9, -21.95), CH::FrontRowPlinth + 2.72, Dir(0.0, 0.7), 90.0);
		Washer(Out, Lights, TEXT("Linchi N"), FVector2D(-19.6, -17.85), FVector2D(-2.9, -17.85), CH::FrontRowPlinth + 2.72, Dir(0.0, -0.8), 45.0);
		// 天青 and 昌南: along the lattice, across onto the wall case.
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double S = Side ? -1.0 : 1.0;
			const double X = Side ? 2.0 * A - (CH::SideHallFace - 0.35) : CH::SideHallFace - 0.35;
			Washer(Out, Lights, Side ? TEXT("Changnan") : TEXT("Tianqing"), FVector2D(X, CH::SideHallN + 0.6), FVector2D(X, CH::SideHallS - 0.6),
				   CH::SideHallPlinth + CH::SideHallCol - 0.18, Dir(-0.75 * S, 0.0), 80.0);
		}
		// 澄懷堂: behind the front lattice, across onto the three scroll cases' wall and the screens.
		Washer(Out, Lights, TEXT("Hall"), FVector2D(CH::HallX0 + 0.5, CH::HallFace - 0.4), FVector2D(CH::HallX1 - 0.5, CH::HallFace - 0.4),
			   CH::HallPlinth + CH::HallCol - 0.2, Dir(0.0, -0.8), 110.0);
		// 清閟 and 停雲 (lit from the hall and their paper doors): one along the doors, one along the hall's gable.
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double X0 = Side ? 2.0 * A - CH::EarX1 : CH::EarX0, X1 = Side ? 2.0 * A - CH::EarX0 : CH::EarX1;
			const double S = Side ? -1.0 : 1.0;
			const TCHAR* N = Side ? TEXT("Tingyun") : TEXT("Qingbi");
			Washer(Out, Lights, *FString::Printf(TEXT("%s S"), N), FVector2D(X0 + 0.3, CH::EarS - 0.35), FVector2D(X1 - 0.3, CH::EarS - 0.35),
				   CH::EarPlinth + CH::EarCol - 0.45, Dir(0.0, -0.8), 120.0);
			const double XG = Side ? X0 + 0.3 : X1 - 0.3;      // along the hall's gable wall, across onto the outer gable's case
			Washer(Out, Lights, *FString::Printf(TEXT("%s E"), N), FVector2D(XG, CH::EarS - 0.8), FVector2D(XG, CH::EarN + 0.6),
				   CH::EarPlinth + CH::EarCol - 0.2, Dir(-0.8 * S, 0.0), 120.0);
		}
		// 舒卷 (the rear row): along its lattice front, across onto the cases and the north wall.
		Washer(Out, Lights, TEXT("Shujuan"), FVector2D(CH::InW + 0.6, CH::RearRowFace - 0.4), FVector2D(CH::HouseInE - 0.6, CH::RearRowFace - 0.4),
			   CH::RearRowPlinth + 2.72, Dir(0.0, -0.8), 85.0);
		// 林泉 (the flower hall): behind its front, across onto the back wall and the table.
		Washer(Out, Lights, TEXT("Linquan"), FVector2D(CH::FlowerHallX0 + 0.5, CH::FlowerHallS - 0.4), FVector2D(CH::FlowerHallX1 - 0.5, CH::FlowerHallS - 0.4),
			   0.45 + 3.0 - 0.2, Dir(0.0, -0.8), 80.0);
	}

	void Rooms(FChMeshes& Out, TArray<FChLight>* Lights)
	{
		Lanterns(Out, Lights);
		RoomLight(Out, Lights);
		// 臨池: the long slanted case against the south wall (the reader on its north), the desk in the east bay.
		{
			const FLocal L = Frame(0.0, CH::LinchiCaseY0, 1, 0, 0, 1);
			SlantCase(Out, Lights, TEXT("Linchi case"), L, CH::LinchiCaseX0, CH::LinchiCaseX1, CH::InS - CH::LinchiCaseY0, CH::FrontRowPlinth);
			if (!Lights)
			{
				const FLocal D = Frame(0.0, 0.0, 1, 0, 0, 1);
				Table(Out, D, -4.25, -2.75, -19.65, -18.9, CH::FrontRowPlinth, 0.82, false);
				Chair(Out, Frame(0.0, 0.0, 1, 0, 0, 1), -3.5, -18.55, CH::FrontRowPlinth);   // south of the desk, facing north
				DeskThings(Out, D, -3.45, -19.27, CH::FrontRowPlinth + 0.82);
			}
		}
		// 天青 and 昌南: the wall cases on the side halls' back walls; the Ru basin's pedestal in the west; the table and
		// chairs in the east (the handling copy of the chicken cup).
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double S = Side ? -1.0 : 1.0;                      // mirror about the axis
			const double BackX = Side ? 2.0 * CH::Axis - CH::InW : CH::InW;
			const double FrontX = BackX + S * CH::SideCaseDepth;
			const FLocal L = Frame(FrontX, 0.0, 0, 1, -S, 0);       // u south, v toward the wall
			const double F = CH::SideHallPlinth, Deck = F + 0.82, Canopy = F + 2.32, Top = F + 2.58;
			UprightCase(Out, Lights, Side ? TEXT("Changnan case") : TEXT("Tianqing case"), L, CH::SideCaseY0, CH::SideCaseY1, CH::SideCaseDepth, F, Deck,
						Canopy, Top, {CH::SideHallCols[1] + 0.0, CH::SideHallCols[2]}, false, 160.0);
			const double CX = 0.5 * (CH::LoneCaseX0 + CH::LoneCaseX1), CY = 0.5 * (CH::LoneCaseY0 + CH::LoneCaseY1);
			if (Side == 0) { PedestalCase(Out, Lights, CX, CY, F, 0.4, 0.95, 0.5); }
			else if (!Lights)
			{
				const double X = 2.0 * CH::Axis - CX;
				const FLocal T = Frame(0.0, 0.0, 0, 1, 1, 0);         // u south, v east
				Table(Out, T, CY - 0.55, CY + 0.55, X - 0.3, X + 0.3, F, 0.8, false);
				Chair(Out, Frame(0.0, 0.0, 1, 0, 0, -1), X, -(CY - 0.95), F);    // north of the table, facing south
				Chair(Out, Frame(0.0, 0.0, -1, 0, 0, 1), -X, CY + 0.95, F);      // south of it, facing north
			}
		}
		// 澄懷堂: three full-height scroll cases in the back wall, the altar table before Fan Kuan, the two open screens.
		{
			const FLocal L = Frame(0.0, CH::HallBack + CH::HallCaseDepth, 1, 0, 0, -1);   // u east, v north to the wall
			const double F = CH::HallPlinth;
			for (int32 k = 0; k < 3; ++k)
			{
				UprightCase(Out, Lights, *FString::Printf(TEXT("Hall case %d"), k), L, CH::HallCaseX[k][0], CH::HallCaseX[k][1], CH::HallCaseDepth, F,
							F + 0.2, F + 3.56, F + 3.86, {}, true, 110.0);
			}
			if (!Lights)
			{
				Table(Out, Frame(0.0, 0.0, 1, 0, 0, 1), CH::Axis - 1.0, CH::Axis + 1.0, -49.96, -49.54, F, 0.82, true);
				OpenScreen(Out, CH::HallCols[1], CH::HallFace - 0.16, CH::HallBack + 1.05, F, F + CH::HallCol - 0.02);
				OpenScreen(Out, CH::HallCols[2], CH::HallFace - 0.16, CH::HallBack + 1.05, F, F + CH::HallCol - 0.02);
			}
		}
		// The ear rooms: one wall case on the back wall, one on the outer gable wall (清閟 west, 停雲 east).
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double S = Side ? -1.0 : 1.0;
			const double F = CH::EarPlinth, Deck = F + 0.2, Canopy = F + 3.08, Top = F + 3.36;
			const double X0 = Side ? 2.0 * CH::Axis - CH::EarX1 : CH::EarX0, X1 = Side ? 2.0 * CH::Axis - CH::EarX0 : CH::EarX1;
			const double D = 0.45;
			// Back wall (north), u east: Ni Zan (清閟), Shen Zhou (停雲, the wider).
			const double Mid = 0.5 * (X0 + X1), BackHalf = Side ? 0.75 : 0.55;
			UprightCase(Out, Lights, Side ? TEXT("Tingyun back case") : TEXT("Qingbi back case"), Frame(0.0, CH::EarN + D, 1, 0, 0, -1), Mid - BackHalf,
						Mid + BackHalf, D, F, Deck, Canopy, Top, {}, true, 110.0);
			// Gable wall (west for 清閟: Wang Meng; east for 停雲: Wen Zhengming), u south, facing the hall's door.
			const double WallX = Side ? X1 : X0;
			UprightCase(Out, Lights, Side ? TEXT("Tingyun side case") : TEXT("Qingbi side case"), Frame(WallX + S * D, 0.0, 0, 1, -S, 0),
						EarSideCaseY0, EarSideCaseY1, D, F, Deck, Canopy, Top, {}, true, 110.0);
		}
		// 舒卷: the two slanted cases on the rear row's north wall (the reader on their south).
		{
			const FLocal L = Frame(0.0, CH::RearCaseY1, 1, 0, 0, -1);
			const double D = CH::RearCaseY1 - CH::InN;
			SlantCase(Out, Lights, TEXT("Thousand Li case"), L, CH::ThousandLiCaseX[0], CH::ThousandLiCaseX[1], D, CH::RearRowPlinth);
			SlantCase(Out, Lights, TEXT("Qingming case"), L, CH::QingmingCaseX[0], CH::QingmingCaseX[1], D, CH::RearRowPlinth);
		}
		// 林泉: the painting table and two chairs in the flower hall.
		if (!Lights)
		{
			const double F = 0.45, CX = 0.5 * (CH::FlowerHallX0 + CH::FlowerHallX1), CY = -57.6;
			const FLocal T = Frame(0.0, 0.0, 1, 0, 0, 1);
			Table(Out, T, CX - 0.95, CX + 0.95, CY - 0.4, CY + 0.4, F, 0.8, false);
			Chair(Out, Frame(0.0, 0.0, -1, 0, 0, -1), -(CX - 0.5), -(CY - 0.75), F);   // north of the table, facing south
			Chair(Out, Frame(0.0, 0.0, -1, 0, 0, -1), -(CX + 0.5), -(CY - 0.75), F);
			DeskThings(Out, T, CX + 0.2, CY + 0.05, F + 0.8);
		}
	}
}

namespace ChenghuaiBuild
{
	void BuildDisplay(FChMeshes& Out) { ChenghuaiDisplayImpl::Rooms(Out, nullptr); }

	void DisplayLights(TArray<FChLight>& Out)
	{
		FChMeshes Unused;
		ChenghuaiDisplayImpl::Rooms(Unused, &Out);
	}
}
