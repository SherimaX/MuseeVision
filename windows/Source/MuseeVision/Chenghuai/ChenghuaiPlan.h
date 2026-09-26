#pragma once

#include "CoreMinimal.h"

/**
 * 澄懷 Chenghuai: the three-court Beijing house (三进四合院) on the Rotunda's north door, with a Suzhou garden (臥遊)
 * beside it to the east (plan/proposals/chenghuai: the boards Main, Plan, Rooms, Section, Garden, Hang, Materials,
 * Names, Rules). Every number here is read from the boards' SVG (Plan.dc.html at 18.6 px/m about the Rotunda's centre;
 * Section.dc.html at 33 px/m): plan metres, x east, plan y south (so north is −y), heights from the ground (0, the
 * Rotunda's floor). MuseePlan.h's convention.
 *
 * The house faces south (坎宅巽门: a house north of its lane, gate at the south-east); the Rotunda stands where the lane
 * would be. Three courts on one axis (x −9.2): the front court (前院) behind the front row (倒座房), the inner court (内院)
 * behind the festooned gate (垂花门) with the side halls (厢房) and the main hall (正房) and its ear rooms (耳房), the rear
 * court (后院) and the rear row (后罩房). The gate (广亮大门) is on the Rotunda's axis (x 0), at the south-east corner.
 */
namespace Chenghuai
{
	/**
	 * Whether the Rotunda's north door is Chenghuai's: its open porch, no Sculpture Hall dress or passage, the ground's
	 * opening for the pond, the lawn kept off the site (MuseeFacadeStructure, RotundaStructure, MuseeFurniture,
	 * MuseeGround, MuseeLawn read it). False until the wing is placed in the map (native.py ROOMS): the Sculpture Hall
	 * keeps the door meanwhile, exactly as before.
	 */
	constexpr bool bNorthDoorOpen = true;

	// ------------------------------------------------------------------------------------------------ the site
	/** The site's outer faces: house x −20.7 … 2.3, garden 2.3 … 16.9; y −61.9 (north) … −17.0 (south). */
	constexpr double SiteW = -20.7, SiteE = 16.9, SiteN = -61.9, SiteS = -17.0;
	constexpr double HouseE = 2.3;              // the house's east outer face = the garden's west
	constexpr double Wall = 0.5;                // the enclosing walls
	constexpr double InW = SiteW + Wall, InN = SiteN + Wall, InS = SiteS - Wall;   // −20.2, −61.4, −17.5
	constexpr double HouseInE = HouseE - Wall;  // 1.8: the house's east wall's inner face
	constexpr double GardenInE = SiteE - Wall;  // 16.4

	// ------------------------------------------------------------------------------------------------ the porch and the gate
	/** The Rotunda's north door (the drum, r 10 … 11.2) opens onto an open porch: grey brick walls 0.6 thick. */
	constexpr double PorchHalf = 1.9, PorchWallOut = 2.5, PorchNorth = -15.9;
	constexpr double DrumOuter = 11.2;
	/** Five steps up to the gate's platform (0.9 m): risers of 0.18, treads of 0.275, x ±1.2. */
	constexpr double GateStepsHalf = 1.2;
	constexpr int32 GateRisers = 5;
	/** The gate hall (广亮大门): x ±1.8 inside, y −22.5 … −17.0; its door on the ridge line (y −19.75). */
	constexpr double GateHalf = 1.8, GateN = -22.5, GateS = -17.0, GateDoorY = -19.75, GatePlinth = 0.9;
	constexpr double GateColX = 1.5, GateColS = -17.3, GateColN = -22.2;
	/** The entry yard behind the gate: x −1.8 … 1.8 (its west wall x −2.3 … −1.9), y −28.1 … −22.5; the screen wall
	 * (影壁) against its north wall; the green screen door (屏门) west into the front court, y −26.3 … −24.5; opposite, the
	 * garden's way back in (the same size, in the house's east wall). */
	constexpr double YardN = -28.1, YardWallX0 = -2.3, YardWallX1 = -1.9;
	constexpr double YardDoorY0 = -26.3, YardDoorY1 = -24.5;
	constexpr double ScreenHalf = 1.4, ScreenY0 = -28.1, ScreenY1 = -27.55;

	// ------------------------------------------------------------------------------------------------ the front row 倒座房 · 臨池
	/** 17.9 × 4.8 inside, six bays; faces north (the court), its back is the south wall. Columns on the front (y −22.3). */
	constexpr double FrontRowX0 = -20.2, FrontRowX1 = -2.3, FrontRowFace = -22.3, FrontRowPlinth = 0.30;
	constexpr double FrontRowPlatformN = -22.9;
	constexpr double FrontRowCol = 2.5;          // eave columns (floor to top)
	/** Its long slanted case along the south wall (15.2 m), and the scholar's desk in the east bay. */
	constexpr double LinchiCaseX0 = -19.9, LinchiCaseX1 = -4.7, LinchiCaseY0 = -18.3;

	// ------------------------------------------------------------------------------------------------ the court walls
	/** Between the front and inner courts: y −28.5 … −28.1, 3.2 m high (white lime over a grey brick base), the
	 * festooned gate in it at x −10.9 … −7.5. */
	constexpr double DivideS = -28.1, DivideN = -28.5, DivideTop = 3.2, DivideBase = 0.93;

	// ------------------------------------------------------------------------------------------------ the festooned gate 垂花门
	constexpr double ChuihuaX0 = -10.9, ChuihuaX1 = -7.5, ChuihuaY0 = -29.6, ChuihuaY1 = -26.9, ChuihuaPlinth = 0.45;
	constexpr double ChuihuaColX0 = -10.6, ChuihuaColX1 = -7.8;
	constexpr double ChuihuaFrontY = -28.3, ChuihuaBackY = -29.4, ChuihuaPostY = -27.2, ChuihuaScreenY = -29.3;

	// ------------------------------------------------------------------------------------------------ the inner court
	constexpr double Axis = -9.2;                // the inner court's axis (the festooned gate, the main hall)
	/** The west side hall 西厢房 · 天青: inside x −20.2 … −16.3 (its lattice front), veranda to the eave columns at
	 * x −15.2, y −40.9 … −31.5; gables y −41.4 … −40.9 and −31.5 … −31.0 to the veranda's edge. The east hall mirrors it
	 * about the axis (x' = 2·Axis − x). */
	constexpr double SideHallFace = -16.3, SideHallEave = -15.2, SideHallN = -40.9, SideHallS = -31.5, SideHallPlinth = 0.45;
	constexpr double SideHallPlatformE = -14.6;
	constexpr double SideHallCols[4] = {-40.9, -37.85, -34.55, -31.5};
	constexpr double SideHallCol = 2.9;
	/** The ceramics: an 8.4 m wall case on the back wall, and the one piece in its own case opposite the door. */
	constexpr double SideCaseY0 = -40.4, SideCaseY1 = -32.0, SideCaseDepth = 0.6;
	constexpr double LoneCaseX0 = -18.6, LoneCaseX1 = -17.8, LoneCaseY0 = -36.6, LoneCaseY1 = -35.8;

	/** The main hall 正房 · 澄懷堂: 10.0 × 6.1 inside (x −14.2 … −4.2, y −51.4 … −45.3), a veranda 1.3 deep to the eave
	 * columns (y −44.0), on a platform x −14.7 … −3.7, y −51.9 … −43.3, 0.75 high; three bays (3.05, 3.9, 3.05). */
	constexpr double HallX0 = -14.2, HallX1 = -4.2, HallFace = -45.3, HallEave = -44.0, HallBack = -51.4;
	constexpr double HallPlatformS = -43.3, HallPlinth = 0.75;
	constexpr double HallCols[4] = {-14.2, -11.15, -7.25, -4.2};
	constexpr double HallCol = 3.9;              // raised so Fan Kuan hangs in his full mount
	constexpr double HallPurlinStep = 1.275;     // seven purlins, 举架 5, 7, 9 tenths
	/** The three full-height scroll cases in the back wall, and the Ming altar table before Fan Kuan. */
	constexpr double HallCaseX[3][2] = {{-13.55, -11.85}, {-10.0, -8.4}, {-6.7, -4.7}};
	constexpr double HallCaseDepth = 0.6;

	/** The ear rooms 耳房 (清閟 west, 停雲 east): 2.5 × 4.5 inside (x −17.2 … −14.7, y −50.4 … −45.9), lit from the hall; columns
	 * raised to 3.4 m (Rooms board) so Shen Zhou's Lofty Mount Lu hangs in its full mount. */
	constexpr double EarX0 = -17.2, EarX1 = -14.7, EarN = -50.4, EarS = -45.9, EarPlinth = 0.5, EarCol = 3.4;

	/** The galleries (抄手游廊): 1.5 m against the court wall, 1.3 m along the side halls' fronts. */
	constexpr double GalleryFloor = 0.45;
	constexpr double GallerySouthY0 = -30.0, GallerySouthY1 = -28.5;
	constexpr double GalleryColY = -29.9;
	constexpr double GalleryWestX0 = -16.3, GalleryWestX1 = -15.0;
	constexpr double GalleryStepY0 = -43.6, GalleryStepY1 = -43.0;
	constexpr double GalleryCol = 2.6;

	// ------------------------------------------------------------------------------------------------ the rear court and row
	/** The cross walls from the house's side walls to the ear rooms (y −48.1 … −47.7), a door in each. */
	constexpr double CrossWallY0 = -48.1, CrossWallY1 = -47.7;
	constexpr double CornerDoorW[2] = {-19.5, -18.4}, CornerDoorE[2] = {0.0, 1.1};
	/** The rear row 后罩房 · 舒卷: 22 × 4.7 inside (x −20.2 … 1.8, y −61.4 … −56.7), seven bays of 3.143, faces south. */
	constexpr double RearRowFace = -56.7, RearRowPlatformS = -56.1, RearRowPlinth = 0.30, RearRowCol = 2.5;
	constexpr double RearCaseY0 = -61.3, RearCaseY1 = -60.6;
	constexpr double ThousandLiCaseX[2] = {-19.7, -6.3}, QingmingCaseX[2] = {-5.3, 0.9};
	/** The moon gate (月洞门 · 臥遊) in the house's east wall at the rear court: y −55.5 … −53.1. */
	constexpr double MoonGateY0 = -55.5, MoonGateY1 = -53.1;

	// ------------------------------------------------------------------------------------------------ the garden 臥遊
	/** The covered walk (复廊) along the house's wall, x 2.3 … 3.7, y −56.3 … −22.5, columns every 2.7 m at x 3.55. */
	constexpr double WalkX0 = 2.3, WalkX1 = 3.7, WalkColX = 3.55, WalkY0 = -56.3, WalkY1 = -22.5;
	/** The flower hall 花厅 · 林泉: x 7.0 … 14.3, y −59.1 … −55.5 inside (walls 0.4), open south onto its terrace. */
	constexpr double FlowerHallX0 = 7.0, FlowerHallX1 = 14.3, FlowerHallN = -59.1, FlowerHallS = -55.5;
	constexpr double FlowerHallCols[2] = {9.0, 12.3};
	constexpr double TerraceX0 = 8.1, TerraceX1 = 13.2, TerraceY0 = -55.2, TerraceY1 = -53.7;
	/** The water pavilion 水榭 · 知魚 on the pond's east edge; the hexagonal pavilion 亭 · 見山 on the rockery. */
	constexpr double WaterPavX0 = 13.0, WaterPavX1 = 16.4, WaterPavY0 = -50.6, WaterPavY1 = -46.1;
	constexpr double HexCX = 10.6, HexCY = -28.3, HexR = 1.7, HexColR = 1.45;
	/** The zigzag bridge (plan points) across the pond's waist. */
	constexpr double Bridge[6][2] = {{5.2, -44.9}, {7.2, -44.9}, {8.2, -43.5}, {10.4, -43.5}, {11.4, -44.5}, {13.0, -44.5}};
	/** The pond (its edge: ChenghuaiGardenPlan.h): the water 0.28 m under the garden's ground. */
	constexpr double WaterLevel = -0.28;
	/** The rockery (假山): its foot and its upper level (outlines), heights to 2.6 m. */
	constexpr double RockeryTop = 2.6;

	/** The Classical Hall's tribune lies under the pond's west lobe: its masonry reaches r 7.9 about (0, −39.3) up to
	 * −0.55, so the pond's bed stays above −0.5 there. */
	constexpr double TribuneCX = 0.0, TribuneCY = -39.3, TribuneShell = 7.9, TribuneRoofZ = -0.55;
}
