#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"
#include "NympheasCurves.h"

struct FSiteDescription;

/**
 * The oval's parts by trade, each drawn in Form's language into the description's pieces: the shell (§3.1-3.3), the
 * floors (§3.4), the well and its stair (§3.7), the pond (§3.5), and the fittings (the guard rails, §3.6, and the
 * benches, §3.8). The site's declarations (footprint, ports, openings, anchors, enclosures) are NympheasSite's.
 */
namespace Musee::Nympheas
{
	/** A key under the stage: nympheas.<Local>. */
	FMuseeKey Key(const TCHAR* Local);

	/** A surface of the stage's finish schedule: nympheas.<Local>. */
	FName Surface(const TCHAR* Local);

	/**
	 * The moving body that carries one stage of the four posts (0 the lowest, Ø 0.210, to 2 the ram under the tray):
	 * nympheas.post.stage-<n>. Each rises as its mechanism follows the pond's height (Mechanisms.json).
	 */
	FMuseeKey PostStageBody(int32 Stage);

	/** The oval's inner face, and the pond's water edge: the two ellipses everything is set out from. */
	const FOvalCurve& Oval();
	const FOvalCurve& Pond();

	/** A drawing of the stage, finished in one of its surfaces, colliding as given, standing in a frame. */
	template <typename TShape>
	FFormDrawing Drawn(const TCHAR* Name, const TCHAR* LocalSurface, TShape&& Shape, const Musee::Collision::FClass* Collision,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing Drawing = FFormDrawing::Of(FName(Name), Surface(LocalSurface), Forward<TShape>(Shape));
		Drawing.Collision = Collision ? Collision->Profile() : NAME_None;
		Drawing.Frame = Frame;
		return Drawing;
	}

	/** A box between two corners, as a plan extrusion. */
	FFormExtrusion Block(const FVector& Min, const FVector& Max);

	/** A plan extrusion of an outline and its holes between two heights, in a frame raised to the lower. */
	FFormExtrusion Slab(FFormProfile Outline, TArray<FFormProfile> Holes, double Bottom, double Top);

	/** The parameter of the door's south jamb on the inner ellipse (y = half the door's width); the north one is its negative. */
	double JambT();

	/**
	 * The footprint's outline (§1): the passage block's port face and the oval's outer face (the wall's 0.60 parallel)
	 * at the oval's stations, clockwise from the block's south-east corner.
	 */
	TArray<FVector2D> FootprintOutline();

	/** A circle as a closed profile. */
	FFormProfile Circle(const FVector2D& Centre, double Radius);

	/** The points mirrored across the long axis (y to -y), in reverse order so the outline keeps its sense. */
	TArray<FVector2D> MirroredInY(TConstArrayView<FVector2D> Points);

	/** A bench's centreline (0 to 3: B1 south-west, B2 south-east, B3 north-east, B4 north-west), as its stations. */
	TArray<FVector2D> BenchLine(int32 Bench);

	/** The names of the pieces that bound the enclosures (the site's list reads them). */
	namespace Pieces
	{
		inline const TCHAR* const OvalWall = TEXT("oval.wall");
		inline const TCHAR* const Portal = TEXT("passage.portal");
		inline const TCHAR* const CheekNorth = TEXT("passage.cheek.north");
		inline const TCHAR* const CheekSouth = TEXT("passage.cheek.south");
		inline const TCHAR* const Upstand = TEXT("attic.upstand");
		inline const TCHAR* const RoofRing = TEXT("roof.ring");
		inline const TCHAR* const RoofGlass = TEXT("roof.glass");
		inline const TCHAR* const Floor = TEXT("floor");
		inline const TCHAR* const CurbNorth = TEXT("well.curb.north");
		inline const TCHAR* const CurbSouth = TEXT("well.curb.south");
		inline const TCHAR* const WellSlab = TEXT("well.slab");
		inline const TCHAR* const WellBase = TEXT("well.base");
		inline const TCHAR* const EndWall = TEXT("well.wall.west");
		inline const TCHAR* const HeadWall = TEXT("well.wall.head");
		inline const TCHAR* const Stair = TEXT("stair");
		inline const TCHAR* const TopLanding = TEXT("stair.landing.top");
		inline const TCHAR* const BottomLanding = TEXT("stair.landing.bottom");

		/** The side walls of the shaft, north and south, each in four stretches (below the chases, the chases' bands, and the ends). */
		TArray<FName> ShaftWalls();
	}

	void DrawShell(TArray<FFormDrawing>& Out);
	void DrawFloors(TArray<FFormDrawing>& Out);
	void DrawWell(TArray<FFormDrawing>& Out);
	void DrawPond(TArray<FFormDrawing>& Out);
	void DrawFittings(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);
}
