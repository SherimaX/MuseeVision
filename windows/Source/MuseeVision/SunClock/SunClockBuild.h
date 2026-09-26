#pragma once

#include "CoreMinimal.h"
#include "Plan/MuseePlan.h"
#include "Salon/SalonKit.h"

/**
 * The sun clock's geometry (ASunClock), in metres: x east, y south, z up, the Rotunda's centre at the
 * origin; plan angles φ from east towards south. The drum's parts are built in the drum's own frame
 * (its top, the dial, at z 0 when lowered); everything else in the actor's.
 *
 * Every circle is cut at the same 768 stations (Around), so rings, faces and floors that meet share
 * their vertices; where an opening crosses a circle (the drum's door, the north port), its samples are
 * added to the stations of every surface that meets there. Solids that only touch sink a centimetre
 * into each other instead (pilasters, mouldings, the bronze).
 *
 * A named namespace: the module builds in unity files.
 */
namespace SunClockBuild
{
	using SalonKit::FMeshData;
	using SalonKit::FProfile;
	using SalonKit::FFrame;
	namespace SC = MuseePlan::SunClock;

	constexpr double Turn = 2.0 * UE_DOUBLE_PI;
	constexpr double Deg = UE_DOUBLE_PI / 180.0;
	constexpr int32 Around = 768;
	/** Segments across every round arch (the door, the port). */
	constexpr int32 ArchSegments = 48;

	inline double GridAngle(int32 I) { return Turn * I / Around; }
	inline FVector Polar(double R, double Phi, double Z) { return FVector(R * FMath::Cos(Phi), R * FMath::Sin(Phi), Z); }
	inline FVector RadialDir(double Phi) { return FVector(FMath::Cos(Phi), FMath::Sin(Phi), 0.0); }
	/** The direction of increasing φ (east → south → west → north). */
	inline FVector TangentDir(double Phi) { return FVector(-FMath::Sin(Phi), FMath::Cos(Phi), 0.0); }
	/** φ in [0, 2π). */
	inline double Wrap(double Phi) { return FMath::Fmod(FMath::Fmod(Phi, Turn) + Turn, Turn); }

	// ---- The drum (its own frame: the dial at z 0, the door's sill at −Lift).

	constexpr double DrumBottom = -SC::Lift - 0.06;        // the ring under the sill
	constexpr double SoffitZ = -0.14;                       // the dial slab's underside (the bronze rim)
	/**
	 * Round the rim's outer edge (r 6.46 to the wall), a groove up to −0.06: lowered, the top of the stair's
	 * wall (the sleeve, −0.07) sits in it, so that raised, the drum's foot (−0.06) and the sleeve's top
	 * leave no slot into the pocket between them.
	 */
	constexpr double GrooveInner = 6.46;
	constexpr double GrooveZ = -0.06;
	constexpr double SleeveTop = -0.07;
	constexpr double RibZ = -0.13;                          // the soffit's ribs (the bronze rim hangs to −0.14)
	constexpr double ArchitraveZ = -0.45;                   // the wall face's top
	constexpr double BaseTopZ = -3.08;                      // … and its foot, on the base moulding

	/** The door's half-angle: its jambs are radial, 2.6 m apart at the wall face. */
	double DoorHalfAngle();
	/** The door's springing (the arch's centre height): 2.95 m door, 1.3 m arch radius at the face. */
	double DoorSpring();
	/** Arch sample J (0 = south jamb … ArchSegments = north jamb): its angle (the same at every radius). */
	double DoorPhi(int32 J);
	/** … and its height at radius R (the arch is conical: its half-width is R sin(half-angle)). */
	double DoorZ(double R, int32 J);

	/** A station round the drum: the 768 grid outside the door, the arch's samples across it. */
	struct FStation
	{
		double Phi = 0;
		int32 Arch = INDEX_NONE;
	};
	/** Ascending from 0. */
	const TArray<FStation>& DrumStations();
	/** The drum's stations as angles, going round the wall from the north jamb to the south jamb (door excluded, unwrapped). */
	TArray<double> DrumRunAngles();

	// ---- The north port (the actor's frame).

	/** Port sample J (0 = east jamb, x +1.8 … ArchSegments = west jamb, x −1.8): x = 1.8 cos(πJ/N). */
	double PortX(int32 J);
	/** The port's arch height at sample J (the springing at the jambs). */
	double PortArchZ(int32 J);
	/** The angle of the point with x = PortX(J) on the circle R, north side. */
	double PortPhi(double R, int32 J);
	/** Stations round the circle R: the grid, with the port's samples in place of the grid across the port. */
	TArray<double> PortCircle(double R);

	// ---- The stair.

	struct FStairLayout
	{
		static constexpr int32 MidLanding = 20;
		double Alpha = 0;                    // a tread's turn (rad)
		double Beta = 0;                     // the mid landing's
		double Front[SC::Risers] = {};       // [K]: tread K's nosing line (rad), K = 1 … 39
		double Top = 0;                      // the top landing's back edge (rad)
		double End = 0;                      // the last riser, onto the floor
		/** Where the retracting balustrade meets the fixed one (tread 8's nosing): below it, all stays under the lowered dial. */
		double Split = 0;
		/** Where the wall's handrail starts (tread 7's nosing), returned to the wall: its top 4 cm under the lowered dial. */
		double WallRailStart = 0;
		TArray<FVector2D> Ramp;              // the walking surface (φ, z), φ descending

		static const FStairLayout& Get();

		double TreadZ(int32 K) const { return -SC::Riser * K; }
		double Back(int32 K) const { return K == 1 ? Top : Front[K - 1]; }
		bool IsLanding(int32 K) const { return K == 1 || K == MidLanding; }
		/** The ramp (through the treads' middles, flat on the landings; the floor past the foot). */
		double RampZ(double Phi) const;
		/** The inner string's top: 120 mm over the ramp, never above −0.14 (under the lowered dial). */
		double CurbTop(double Phi) const;
		/** The stair's underside: 400 mm under the ramp, into the floor at the foot. */
		double SoffitZ(double Phi) const;
		/** The ramp's corners (φ), for sampling. */
		TArray<double> Kinks() const;
	};

	/** The stair's radii: the inner string (well side, top face) and the treads' span. */
	constexpr double CurbInner = SC::WellRadius;             // 2.2
	constexpr double CurbOuter = 2.35;
	constexpr double RailRadius = 2.15;                       // the balustrade's plane, in the well beside the string
	constexpr double WallRailRadius = 6.40;

	/**
	 * The retracting balustrade (the upper flight's well side and the top landing's back edge). Each panel stands
	 * in the slot of a hollow housing (a U channel with end blocks: marble sides with sunk fields where they are
	 * seen, bound in bronze: a cap round the slot, a shoe, end plates) hung on the stair: along the inner
	 * string (r 2.07–2.20, its top the string's) and under the landing's back edge (0–0.14 m behind it, its top
	 * the landing's). While the dial is down the panel sits PanelDrop lower, wholly inside: its handrail's top
	 * under the housing's top, its carrier bar over the housing's floor.
	 *
	 * It is driven from the lift through a cam with a dwell at each end (PanelOffset): it stays down, wholly in its
	 * housing, until the dial's underside is PanelRiseFrom up, rises with the drum for the next PanelDrop and then
	 * stands. Lowering, it is back in its housing before the drum comes within 1.07 m of it: the moving ceiling never
	 * rides down on the rail (a hand on it, a rail brushing the ribs), and the dial closes over bare housings.
	 */
	constexpr double PanelDrop = 0.94;
	constexpr double PanelRiseFrom = 1.20;                    // the lift at which the panels start to rise (m)
	/** The panels' height over their raised position (−PanelDrop … 0) at the drum's lift Height (m). */
	inline double PanelOffset(double Height)
	{
		return FMath::Clamp(Height - PanelRiseFrom, 0.0, PanelDrop) - PanelDrop;
	}
	constexpr double HousingDepth = 1.20;                     // under its top
	constexpr double HousingFloor = 0.03;                     // the channel's floor, thick
	constexpr double HousingEndBlock = 0.04;                  // the solid block closing each end
	constexpr double SlotHalf = 0.038;                        // the slot (the handrail, 60 mm, passes with 4 mm to spare)
	constexpr double WellHousingInner = 2.07;                 // … out to the string's face (CurbInner)
	constexpr double BackPlane = 0.07;                        // the back panel's plane, behind the landing's back edge
	constexpr double StemFoot = -0.22;                        // the panel's standards run down this far into the housing …
	constexpr double CarrierTop = -0.18;                      // … onto the carrier bar (StemFoot … CarrierTop)

	// ---- The lights (on the drum, its own frame): the fittings' places, and their bounds for the checks.

	constexpr int32 CoveCount = 12;
	constexpr double CoveRadius = 4.15;                       // the fourth ring of coffers, over the treads
	constexpr double CoveZ = -0.108;                          // just under the coffer's panel (−0.10)
	inline double CovePhi(int32 I) { return (7.5 + 30.0 * I) * UE_DOUBLE_PI / 180.0; }   // every other coffer
	constexpr double WellLightZ = -0.29;                      // under the pendant boss (−0.265)
	constexpr double FillLightZ = -2.4;                       // no fitting: on the well's axis

	// ---- The meshes.

	struct FSunClockMeshes
	{
		// The drum (its own frame).
		FMeshData DialA, DialB, DialRosso, DialNero;   // the dial's stone: pale slabs, the rosso and nero bands
		FMeshData DrumStone;                           // wall, cornice, base, pilasters, door, sill
		FMeshData DrumSoffit;                          // the coffered underside
		FMeshData DrumBronze, DrumGilt;                // the inlay, numerals, sunburst; the boss; the soffit's rim
		// The ring round it (fixed).
		FMeshData RingA, RingB;                        // slabs of two tones
		FMeshData RingBronze, RingGilt;                // the motto; its stops
		// The shaft (fixed).
		FMeshData Masonry;                             // the stair's wall, the pocket, the port
		FMeshData PortStone;                           // the port's dressings: its archivolt, transoms, plinths and keystone
		FMeshData FloorA, FloorRosso, FloorNero;       // the landing and its rosette
		// The stair (fixed).
		FMeshData Treads;                              // treads, risers, nosings
		FMeshData Curb;                                // the inner string
		FMeshData StairSoffit;                         // its underside and ends
		FMeshData Ramp;                                // hidden: what the visitor walks on
		FMeshData RailBronze, RailFence;               // the balustrade below the split and the wall rail; its collision
		FMeshData Housing, HousingStone;               // the retracting balustrade's housings (fixed): their bronze tops, their marble sides
		FMeshData TopRailBronze, TopRailFence;         // the retracting balustrade and its collision (PanelDrop down while the dial is down)
	};

	void BuildDial(FSunClockMeshes& M);
	void BuildDrum(FSunClockMeshes& M);
	void BuildRing(FSunClockMeshes& M);
	void BuildShaft(FSunClockMeshes& M);
	void BuildStair(FSunClockMeshes& M);
	void BuildRails(FSunClockMeshes& M);
	void BuildAll(FSunClockMeshes& M);

	// ---- Shared helpers (SunClockKit.cpp).

	/** A surface of revolution: Profile's (A, B) is (r, z), swept through Angles (ascending); closed joins the last to the first. */
	void Revolve(FMeshData& M, const FProfile& Profile, const TArray<double>& Angles, bool bClosed);
	/** The grid's angles, 0 … 767. */
	TArray<double> GridAngles();

	/** Triangles between two rows of vertices, ordered along the same parameter (the rows share their ends' parameters). */
	void Zip(FMeshData& M, const TArray<int32>& A, const TArray<double>& ParamA, const TArray<int32>& B, const TArray<double>& ParamB);
	/** A flat annulus from R0 to R1 at Z, facing Up (or down), with its own stations on each circle (closed). */
	void Annulus(FMeshData& M, double R0, const TArray<double>& Inner, double R1, const TArray<double>& Outer, double Z, bool bUp);
	/** An annular sector (closed = false: the arcs run from the first station to the last). */
	void Sector(FMeshData& M, double R0, const TArray<double>& Inner, double R1, const TArray<double>& Outer, double Z, bool bUp);

	/** A vertical face on the cylinder R: each column an angle and one or more spans of heights (ascending). */
	struct FColumn
	{
		double Phi = 0;
		TArray<TArray<double>> Spans;
	};
	void CylinderColumns(FMeshData& M, double R, const TArray<FColumn>& Columns, bool bOutward, bool bClosed);

	/** A simple polygon (either winding) triangulated by ear clipping: index triples into Poly. */
	TArray<int32> EarClip(const TArray<FVector2D>& Poly);
	/** A flat polygon: its 2D outline placed by Place, facing Normal. */
	void FlatPolygon(FMeshData& M, const TArray<FVector2D>& Outline, TFunctionRef<FVector(const FVector2D&)> Place, const FVector& Normal);

	/** The inlay's section: sides from Sink below the floor, a Bevel at the top edges, Height proud. */
	FProfile InlaySection(double HalfWidth, double Height, double Bevel, double Sink);
	/** A straight inlaid bar on the floor (Z0) from A to B (plan), with end caps. */
	void InlayBar(FMeshData& M, const FVector2D& A, const FVector2D& B, double Z0, double Width, double Height, double Bevel, double Sink);
	/** A ring of the same section, R0 … R1 (closed, on Angles). */
	void InlayRing(FMeshData& M, double R0, double R1, double Z0, double Height, double Bevel, double Sink, const TArray<double>& Angles);

	/**
	 * Text in the bronze capitals (SunClockGlyphs.h), extruded Height proud with a Bevel round the top,
	 * its sides from Sink below Z0. Place maps a glyph point (x along the line, y up the letter, in cap
	 * heights from the line's start) to plan metres. Spaces are skipped (the pen moves on); a '.' is a
	 * gap filled by Stop (called with the pen's x at the gap's middle).
	 */
	struct FTextStyle
	{
		double Tracking = 0.0;     // added to each advance (cap heights)
		double WordGap = 0.6;      // a space (cap heights)
		double StopGap = 1.0;      // a '.' (cap heights)
	};
	double TextWidth(const FString& Text, const FTextStyle& Style);
	void AddText(FMeshData& M, const FString& Text, const FTextStyle& Style, TFunctionRef<FVector2D(double, double)> Place,
				 double CapHeight, double Z0, double Height, double Bevel, double Sink, TFunctionRef<void(double)> Stop);

	/** A box in a local frame: Origin + a·AxisA + b·AxisB + c·AxisC for a, b, c in the ranges; Faces as FMeshData::Box. */
	void FrameBox(FMeshData& M, const FVector& Origin, const FVector& AxisA, const FVector& AxisB, const FVector& AxisC,
				  const FVector& Lo, const FVector& Hi, int32 Faces = FMeshData::AllFaces);
}
