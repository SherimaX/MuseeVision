#pragma once

#include "ChenghuaiPlan.h"
#include "CoreMinimal.h"

/**
 * The fittings' plan (§3.10; ChenghuaiDisplay.cpp:63-604, ChenghuaiHouse.cpp:658-772): the cases Art's works lie and hang
 * in, built by the stage; the main hall's open screens; the furniture, in phoebe and in huanghuali; the scholars' things on
 * the desks; the seventeen silk lanterns; the name boards and the couplet.
 */
namespace Musee::Chenghuai::Plan
{
	// ---- the cases (ChenghuaiDisplay.cpp:63-204, 510-597) ----------------------------------------------------------------
	/**
	 * A case in its own frame: u along it from (OriginX, OriginY) in the plan direction (UX, UY), v from the glass (or the
	 * reader's side) back to the wall in (VX, VY); U0 … U1 along it, Depth deep; its floor; for an upright case its deck,
	 * canopy and top heights (absolute), its mullions (u), whether it hangs scrolls, and whether its silk is the ceramics'.
	 */
	struct FCasePlan
	{
		const TCHAR* Local;
		double OriginX, OriginY;
		double UX, UY, VX, VY;
		double U0, U1, Depth, Floor;
		bool bSlanted;
		double Deck, Canopy, Top;
		double Mullions[2];
		int32 MullionCount;
		bool bScrolls;
		bool bCeramics;
	};
	inline constexpr FCasePlan Cases[] = {
		{TEXT("linchi.case"), -19.900, -1.300, 1.0, 0.0, 0.0, 1.0, 0.000, 15.200, 0.800, 0.300, true, 0, 0, 0, {0, 0}, 0, true, false},
		{TEXT("shujuan.case.thousand-li"), -19.700, -43.600, 1.0, 0.0, 0.0, -1.0, 0.000, 13.400, 0.800, 0.300, true, 0, 0, 0, {0, 0}, 0, true, false},
		{TEXT("shujuan.case.qingming"), -5.300, -43.600, 1.0, 0.0, 0.0, -1.0, 0.000, 6.200, 0.800, 0.300, true, 0, 0, 0, {0, 0}, 0, true, false},
		{TEXT("tianqing.case"), -19.600, -23.400, 0.0, 1.0, -1.0, 0.0, 0.000, 8.400, 0.600, 0.450, false, 1.270, 2.770, 3.030, {2.550, 5.850}, 2,
			false, true},
		{TEXT("changnan.case"), 1.200, -23.400, 0.0, 1.0, 1.0, 0.0, 0.000, 8.400, 0.600, 0.450, false, 1.270, 2.770, 3.030, {2.550, 5.850}, 2,
			false, true},
		{TEXT("chenghuaitang.case.west"), -13.550, -33.800, 1.0, 0.0, 0.0, -1.0, 0.000, 1.700, 0.600, 0.750, false, 0.950, 4.310, 4.610, {0, 0}, 0,
			true, false},
		{TEXT("chenghuaitang.case.centre"), -10.000, -33.800, 1.0, 0.0, 0.0, -1.0, 0.000, 1.600, 0.600, 0.750, false, 0.950, 4.310, 4.610, {0, 0}, 0,
			true, false},
		{TEXT("chenghuaitang.case.east"), -6.700, -33.800, 1.0, 0.0, 0.0, -1.0, 0.000, 2.000, 0.600, 0.750, false, 0.950, 4.310, 4.610, {0, 0}, 0,
			true, false},
		{TEXT("qingbi.case.back"), -16.500, -32.950, 1.0, 0.0, 0.0, -1.0, 0.000, 1.100, 0.450, 0.500, false, 0.700, 3.580, 3.860, {0, 0}, 0, true,
			false},
		{TEXT("qingbi.case.side"), -16.750, -31.400, 0.0, -1.0, -1.0, 0.0, 0.000, 1.200, 0.450, 0.500, false, 0.700, 3.580, 3.860, {0, 0}, 0, true,
			false},
		{TEXT("tingyun.case.back"), -3.200, -32.950, 1.0, 0.0, 0.0, -1.0, 0.000, 1.500, 0.450, 0.500, false, 0.700, 3.580, 3.860, {0, 0}, 0, true,
			false},
		{TEXT("tingyun.case.side"), -1.650, -32.600, 0.0, 1.0, 1.0, 0.0, 0.000, 1.200, 0.450, 0.500, false, 0.700, 3.580, 3.860, {0, 0}, 0, true,
			false},
	};

	/** A slanted case: a black toe 0.09; the phoebe body to floor + 0.84, its panels every ≈ 1.5; the base moulding and cornice
	 *  to + 0.88; the silk bed tilted 11° from floor + 0.905 at v 0.05 to v D - 0.06 (a 0.02 board, its top 0.01 over that line);
	 *  the glass front upright to floor + 1.10, its top sloping 11° to the back; ends 0.035; a bronze lamp rail at the back
	 *  under the glass (v D - 0.10 … D - 0.03). */
	inline constexpr double SlantDegrees = 11.000;
	inline constexpr double SlantToe = 0.090;
	inline constexpr double SlantBody = 0.840;
	inline constexpr double SlantCornice = 0.880;
	inline constexpr double SlantBed = 0.905;
	inline constexpr double SlantBedFront = 0.050;
	inline constexpr double SlantBedBack = 0.060;
	inline constexpr double SlantBedBoard = 0.020;
	inline constexpr double SlantFront = 1.100;
	inline constexpr double SlantEnd = 0.035;
	inline constexpr double LampRailFront = 0.100;
	inline constexpr double LampRailBack = 0.030;
	inline constexpr double CasePanel = 1.500;
	/** As built: the toe set in 0.04 and 0.05; the mouldings 8 and 12 mm proud, 0.04 high; the panels' beads 6 mm proud from
	 *  0.18 to 0.79 over the floor, 0.05 in from their ends; the back upstand 0.03 and 0.05 over the glass's back; the front
	 *  rail 0.034 × 0.024; the lamp rail from 0.075 to 0.04 under the glass's back; the glass 8 mm. */
	inline constexpr double ToeIn = 0.040;
	inline constexpr double ToeBack = 0.050;
	inline constexpr double MouldingProud = 0.008;
	inline constexpr double CorniceProud = 0.012;
	inline constexpr double MouldingHigh = 0.040;
	inline constexpr double BeadProud = 0.006;
	inline constexpr double BeadFrom = 0.180;
	inline constexpr double BeadTo = 0.790;
	inline constexpr double BeadIn = 0.050;
	inline constexpr double BeadWidth = 0.010;
	inline constexpr double UpstandThick = 0.030;
	inline constexpr double UpstandOver = 0.050;
	inline constexpr double FrontRailDeep = 0.034;
	inline constexpr double FrontRailHigh = 0.024;
	inline constexpr double LampRailTop = 0.040;
	inline constexpr double LampRailBottom = 0.075;
	inline constexpr double SlantGlass = 0.008;

	/** An upright case: a toe 0.08; the plinth to the deck with mouldings and a conditioning drawer; a silk deck 12 mm and back
	 *  25 mm on a 10 mm board (the silk's face at v D - 0.035); ends 0.045; mullions 0.05; a canopy with a cornice 0.02 proud
	 *  and a baffle 0.05 hiding the lamp run; glass 10 mm at v 0.012; a phoebe lip 0.048 at the deck's front in cases over 2 m
	 *  tall inside, hiding the low run; the scroll cases' bronze hanging rail Ø 0.016 at v D - 0.05, the canopy - 0.05. */
	inline constexpr double UprightToe = 0.080;
	inline constexpr double UprightDeck = 0.012;
	inline constexpr double UprightBackSilk = 0.025;
	inline constexpr double UprightBackBoard = 0.010;
	inline constexpr double UprightEnd = 0.045;
	inline constexpr double UprightMullion = 0.050;
	inline constexpr double UprightCornice = 0.020;
	inline constexpr double UprightCorniceHigh = 0.050;
	inline constexpr double UprightBaffle = 0.050;
	inline constexpr double UprightBaffleDeep = 0.030;
	inline constexpr double UprightGlass = 0.010;
	inline constexpr double UprightGlassAt = 0.012;
	inline constexpr double UprightLip = 0.048;
	inline constexpr double UprightLipDeep = 0.025;
	inline constexpr double UprightTall = 2.000;
	inline constexpr double HangingRail = 0.008;
	inline constexpr double HangingRailBack = 0.050;
	inline constexpr double HangingRailUnder = 0.050;
	/** As built: the toe set in 0.03 and 0.04; the deck 0.02 in from the glass and the back; the mouldings 0.04 high, 10 and 6 mm
	 *  proud; the drawer's front 5 mm proud from 0.16 over the floor to 0.07 under the deck, 0.08 in from the ends, where the
	 *  plinth is over 0.30 high; the mullions 6 mm before the glass. */
	inline constexpr double UprightToeIn = 0.030;
	inline constexpr double UprightDeckIn = 0.020;
	inline constexpr double DrawerFrom = 0.160;
	inline constexpr double DrawerUnder = 0.070;
	inline constexpr double DrawerIn = 0.080;
	inline constexpr double DrawerProud = 0.005;
	inline constexpr double DrawerLeast = 0.300;
	inline constexpr double MullionBefore = 0.006;

	/** The Ru basin's case: a pedestal 0.80 square at (-18.20, -19.20) to 0.95 over the floor, a glass hood 0.50 high. */
	inline constexpr double PedestalX = -18.200;
	inline constexpr double PedestalY = -19.200;
	inline constexpr double PedestalHalf = 0.400;
	inline constexpr double PedestalHigh = 0.950;
	inline constexpr double PedestalHood = 0.500;
	inline constexpr double PedestalFloor = 0.450;
	/** As built: the cap 15 mm proud, 0.04 high; the deck 0.04 in; the hood's glass 10 mm. */
	inline constexpr double PedestalCap = 0.040;
	inline constexpr double PedestalCapProud = 0.015;
	inline constexpr double PedestalDeckIn = 0.040;

	// ---- the hall's open screens (落地罩) --------------------------------------------------------------------------------
	/** Across the main hall on x -11.15 and -7.25, from y -28.46 to -33.35, floor 0.75 to 4.63: end posts 0.09; fretwork legs
	 *  0.55 on solid bases 0.42; a frieze 0.50 under a head 0.07; grid spandrels 0.42 × 0.30 in the upper corners. */
	inline constexpr double ScreensX[] = {-11.150, -7.250};
	inline constexpr double ScreenY0 = -28.460;
	inline constexpr double ScreenY1 = -33.350;
	inline constexpr double ScreenFloor = 0.750;
	inline constexpr double ScreenTop = 4.630;
	inline constexpr double ScreenPost = 0.090;
	inline constexpr double ScreenLeg = 0.550;
	inline constexpr double ScreenLegBase = 0.420;
	inline constexpr double ScreenFrieze = 0.500;
	inline constexpr double ScreenHead = 0.070;
	inline constexpr double SpandrelWide = 0.420;
	inline constexpr double SpandrelHigh = 0.300;
	/** As built: the posts 0.09 thick, the leg posts 0.06 × 0.07, the bases and rails 0.06 and 0.07 thick. */
	inline constexpr double ScreenThick = 0.090;
	inline constexpr double ScreenLegPost = 0.060;
	inline constexpr double ScreenLegThick = 0.070;
	inline constexpr double ScreenBaseThick = 0.060;
	inline constexpr double ScreenRail = 0.060;

	// ---- the furniture (ChenghuaiDisplay.cpp:209-348) ------------------------------------------------------------------
	/** A table: its extent on the plan, its floor and height; top 0.035, legs 0.045 set in 0.06, aprons, low stretchers. */
	struct FTablePlan
	{
		const TCHAR* Local;
		double X0, X1, Y0, Y1, Floor, Height;
		bool bEverted;
	};
	inline constexpr FTablePlan Tables[] = {
		{TEXT("chenghuaitang.altar-table"), -10.200, -8.200, -32.960, -32.540, 0.750, 0.820, true},
		{TEXT("linchi.desk"), -4.250, -2.750, -2.650, -1.900, 0.300, 0.820, false},
		{TEXT("changnan.table"), -0.500, 0.100, -19.750, -18.650, 0.450, 0.800, false},
	};
	inline constexpr double TableTop = 0.035;
	inline constexpr double TableLeg = 0.045;
	inline constexpr double TableLegIn = 0.060;
	/** The everted ends (翘头) rise 0.06 over the top; as built 0.09 long, their tips 0.05. */
	inline constexpr double EvertedRise = 0.060;
	inline constexpr double EvertedLong = 0.090;
	inline constexpr double EvertedTip = 0.050;
	/** As built: the legs 0.03 in from the long sides; aprons 0.07 deep, 0.02 thick; stretchers 0.14 … 0.17 over the floor. */
	inline constexpr double TableLegSide = 0.030;
	inline constexpr double ApronHigh = 0.070;
	inline constexpr double ApronThick = 0.020;
	inline constexpr double StretcherZ0 = 0.140;
	inline constexpr double StretcherZ1 = 0.170;

	/** A southern official's hat chair (南官帽椅): 0.58 × 0.46, seat 0.50, crest 1.03, an S-splat 0.16 wide, stepped stretchers. */
	struct FChairPlan
	{
		const TCHAR* Local;
		double X, Y, Floor;
		double FacingX, FacingY;
	};
	inline constexpr FChairPlan Chairs[] = {
		{TEXT("linchi.chair"), -3.500, -1.550, 0.300, 0.0, -1.0},
		{TEXT("changnan.chair.south"), -0.200, -20.150, 0.450, 0.0, 1.0},
		{TEXT("changnan.chair.north"), -0.200, -18.250, 0.450, 0.0, -1.0},
	};
	inline constexpr double ChairWide = 0.580;
	inline constexpr double ChairDeep = 0.460;
	inline constexpr double ChairSeat = 0.500;
	inline constexpr double ChairCrest = 1.030;
	inline constexpr double ChairSplat = 0.160;
	/** As built: round members r 0.017; the seat frame 0.05 wide, 0.036 deep; the arms 0.23 over the seat, r 0.015; the back
	 *  posts leaning back 0.045; the crest r 0.02; the splat 0.016 thick, bowed 0.03 back at the shoulders; stretchers r 0.011 at
	 *  0.14 (sides) and 0.20 (back), the footrest 0.06 … 0.085; the apron 0.034 deep. */
	inline constexpr double ChairMember = 0.017;
	inline constexpr double ChairFrame = 0.050;
	inline constexpr double ChairFrameDeep = 0.036;
	inline constexpr double ChairArm = 0.230;
	inline constexpr double ChairArmR = 0.015;
	inline constexpr double ChairLean = 0.045;
	inline constexpr double ChairCrestR = 0.020;
	inline constexpr double ChairSplatThick = 0.016;
	inline constexpr double ChairSplatBow = 0.030;
	inline constexpr double ChairStretcherR = 0.011;
	inline constexpr double ChairSideStretcher = 0.140;
	inline constexpr double ChairBackStretcher = 0.200;
	inline constexpr double FootrestZ0 = 0.060;
	inline constexpr double FootrestZ1 = 0.085;
	inline constexpr double ChairApron = 0.034;
	/** The splat's S, back from the back posts' line at five even heights from the seat to the crest (as built). */
	inline constexpr double SplatBow[] = {0.000, -0.012, 0.000, 0.030, 0.028};
	/** The seat's board sits 8 mm under the frame's top, 16 mm thick; the arms rise 15 mm to the posts; the footrest 12 mm
	 *  either side of the front legs' line; the apron 0.07 deep, 6 mm in from the seat's front; the legs 5 mm in (as built). */
	inline constexpr double ChairBoardUnder = 0.008;
	inline constexpr double ChairBoard = 0.016;
	inline constexpr double ChairArmRise = 0.015;
	inline constexpr double FootrestHalf = 0.012;
	inline constexpr double ChairApronHigh = 0.070;
	inline constexpr double ChairApronIn = 0.006;
	inline constexpr double ChairLegIn = 0.005;

	/** 林泉's painting table (画案, 夹头榫) centred (10.65, -40.60), 1.90 × 0.80, top 0.80 (0.035 thick); round legs Ø 0.05 set
	 *  0.10 in from the ends and 0.05 from the sides; a plain apron with spandrels; two stretchers between the legs at each end. */
	inline constexpr double PaintingX = 10.650;
	inline constexpr double PaintingY = -40.600;
	inline constexpr double PaintingLong = 1.900;
	inline constexpr double PaintingDeep = 0.800;
	inline constexpr double PaintingHigh = 0.800;
	inline constexpr double PaintingFloor = 0.450;
	inline constexpr double PaintingLegR = 0.025;
	inline constexpr double PaintingLegEnd = 0.100;
	inline constexpr double PaintingLegSide = 0.050;
	/** As built: the apron 0.07 deep; the spandrels 0.12 along it; the end stretchers at 0.20 and 0.55 over the floor, r 0.012. */
	inline constexpr double PaintingSpandrel = 0.120;
	inline constexpr double PaintingStretchers[] = {0.200, 0.550};
	inline constexpr double PaintingStretcherR = 0.012;

	/** A pair of rose chairs (玫瑰椅) at (10.15, -41.35) and (11.15, -41.35) facing south: seat 0.57 × 0.44 at 0.50, a low
	 *  straight back to 0.86, arms to 0.72, framed in round members Ø 0.025, an openwork rail 0.12 over the seat round the back
	 *  and sides, stepped stretchers, a board seat. */
	inline constexpr double RoseChairsX[] = {10.150, 11.150};
	inline constexpr double RoseChairY = -41.350;
	inline constexpr double RoseWide = 0.570;
	inline constexpr double RoseDeep = 0.440;
	inline constexpr double RoseSeat = 0.500;
	inline constexpr double RoseBack = 0.860;
	inline constexpr double RoseArm = 0.720;
	inline constexpr double RoseMember = 0.0125;
	inline constexpr double RoseRail = 0.120;
	/** As built: the seat board 0.02; the stretchers at 0.06 (front), 0.12 (sides) and 0.18 (back). */
	inline constexpr double RoseBoard = 0.020;
	inline constexpr double RoseStretchers[] = {0.060, 0.120, 0.180};

	/** The scholar's things (§3.10): a fired-clay inkstone 0.18 × 0.26 × 0.03 in a phoebe box 0.20 × 0.28 × 0.012, a brush pot
	 *  r 0.065 × 0.16 and a water dropper r 0.04 × 0.05, on the desk (top 1.12) and on the painting table (top 1.25). */
	struct FDeskThings
	{
		const TCHAR* Local;
		double Top;
		double InkX, InkY, PotX, PotY, DropperX, DropperY;
	};
	inline constexpr FDeskThings DeskThings[] = {
		{TEXT("linchi.desk"), 1.120, -3.450, -2.270, -3.100, -2.220, -3.900, -2.270},
		{TEXT("linquan.table"), 1.250, 10.850, -40.550, 10.450, -40.450, 10.300, -40.700},
	};
	inline constexpr double InkstoneWide = 0.180;
	inline constexpr double InkstoneLong = 0.260;
	inline constexpr double InkstoneHigh = 0.030;
	inline constexpr double InkBoxWide = 0.200;
	inline constexpr double InkBoxLong = 0.280;
	inline constexpr double InkBoxHigh = 0.012;
	inline constexpr double PotR = 0.065;
	inline constexpr double PotHigh = 0.160;
	/** The pot is thrown with a wall 7 mm thick and a bed 1 cm (a stoneware brush pot's). */
	inline constexpr double PotWall = 0.007;
	inline constexpr double PotBed = 0.010;
	inline constexpr double DropperR = 0.040;
	inline constexpr double DropperHigh = 0.050;
	/** The Lingbi rock's stand: phoebe, r 0.11 × 0.03 at (11.35, -40.45) on the painting table. */
	inline constexpr double RockStandX = 11.350;
	inline constexpr double RockStandY = -40.450;
	inline constexpr double RockStandR = 0.110;
	inline constexpr double RockStandHigh = 0.030;

	// ---- the lanterns (纱灯, ChenghuaiDisplay.cpp:360-443) -----------------------------------------------------------------
	/** A lantern: its hook, its rod, its hexagon's radius and height. From the rod down: the crown (R - 0.05, 0.02), the top
	 *  plate (R + 0.018, 0.025), six posts 0.02 square with silk panels 4 mm between them and a rail 0.012 × 0.008 round their
	 *  middle, the base plate and the foot the same as the top; the rod bronze, Ø 0.01 (as built). */
	struct FLanternPlan
	{
		double X, Y, Hook, Rod, Radius, Height;
	};
	inline constexpr FLanternPlan Lanterns[] = {
		{-10.200, -27.150, 4.900, 0.860, 0.170, 0.360}, {-8.200, -27.150, 4.900, 0.860, 0.170, 0.360},
		{-13.000, -12.450, 3.200, 0.300, 0.120, 0.260}, {-5.400, -12.450, 3.200, 0.300, 0.120, 0.260},
		{-15.750, -19.200, 3.600, 0.470, 0.150, 0.320}, {-2.650, -19.200, 3.600, 0.470, 0.150, 0.320},
		{-18.708, -5.580, 3.100, 0.400, 0.130, 0.260}, {-3.792, -5.580, 3.100, 0.400, 0.130, 0.260},
		{-18.629, -39.420, 3.100, 0.400, 0.130, 0.260}, {0.229, -39.420, 3.100, 0.400, 0.130, 0.260},
		{2.925, -9.400, 2.750, 0.120, 0.110, 0.240}, {2.925, -17.500, 2.750, 0.120, 0.110, 0.240},
		{2.925, -25.600, 2.750, 0.120, 0.110, 0.240}, {2.925, -33.700, 2.750, 0.120, 0.110, 0.240},
		{14.700, -31.350, 3.200, 0.250, 0.170, 0.360},
		{9.900, -38.400, 3.310, 0.350, 0.170, 0.360}, {11.400, -38.400, 3.310, 0.350, 0.170, 0.360},
	};
	inline constexpr double LanternRod = 0.005;
	inline constexpr double LanternCrownIn = 0.050;
	inline constexpr double LanternCrown = 0.020;
	inline constexpr double LanternPlateOut = 0.018;
	inline constexpr double LanternPlate = 0.025;
	inline constexpr double LanternPost = 0.020;
	inline constexpr double LanternSilk = 0.004;
	inline constexpr double LanternRailHigh = 0.012;
	inline constexpr double LanternRailDeep = 0.008;

	// ---- the name boards and the couplet (ChenghuaiHouse.cpp:658-772) -----------------------------------------------------
	/** A board 25 mm thick on its wall, its face's centre and the way it faces (a plan direction), its width and height. */
	struct FPlaquePlan
	{
		const TCHAR* Local;
		double X, Y, Z, FacingX, FacingY, Width, Height;
	};
	inline constexpr FPlaquePlan Plaques[] = {
		{TEXT("linchi"), -2.327, -2.950, 2.450, -1.0, 0.0, 1.000, 0.500},
		{TEXT("tianqing"), -18.100, -14.541, 2.900, 0.0, -1.0, 1.100, 0.550},
		{TEXT("changnan"), -0.300, -14.541, 2.900, 0.0, -1.0, 1.100, 0.550},
		{TEXT("qingbi"), -14.725, -30.050, 3.050, -1.0, 0.0, 0.800, 0.400},
		{TEXT("tingyun"), -3.675, -30.050, 3.050, 1.0, 0.0, 0.800, 0.400},
		{TEXT("shujuan"), -9.200, -44.359, 2.420, 0.0, 1.0, 1.100, 0.550},
		{TEXT("linquan"), 10.650, -42.059, 2.950, 0.0, 1.0, 1.200, 0.600},
		{TEXT("chenghuaitang"), -9.200, -33.234, 4.840, 0.0, 1.0, 2.000, 0.640},
	};
	inline constexpr double PlaqueBoard = 0.025;
	/** 澄懷堂's board hangs on a painted lintel between the rear 金柱: x -10.975 … -7.425, y -33.36 ± 0.10, z 4.67 … 4.97. */
	inline constexpr double HallLintelX0 = -10.975;
	inline constexpr double HallLintelX1 = -7.425;
	inline constexpr double HallLintelY = -33.360;
	inline constexpr double HallLintelHalf = 0.100;
	inline constexpr double HallLintelZ0 = 4.670;
	inline constexpr double HallLintelZ1 = 4.970;
	/** The couplet's shells (22 mm) wrap the hall's middle veranda columns (x -11.15, -7.25; y -27.00, r 0.16), 1.9 rad centred
	 *  facing south, z 2.05 … 3.45; 2 mm off the column. */
	inline constexpr double CoupletX[] = {-11.150, -7.250};
	inline constexpr double CoupletY = -27.000;
	inline constexpr double CoupletColumn = 0.160;
	inline constexpr double CoupletGap = 0.002;
	inline constexpr double CoupletShell = 0.022;
	inline constexpr double CoupletSpan = 1.900;
	inline constexpr double CoupletZ0 = 2.050;
	inline constexpr double CoupletZ1 = 3.450;
}
