#pragma once

#include "CoreMinimal.h"
#include "FormAxes.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The hall's parts by trade, each drawn in Form's language into the description's pieces: the masonry (sec. 3.1), the
 * linings with the lunette (sec. 3.1, 3.3), the glass walls' steel and panes (sec. 3.2, 3.3), the vault's steel and panes
 * (sec. 3.2, 3.3), and the bronze fittings (sec. 3.4). The site's declarations (footprint, ports, openings, anchors,
 * enclosures) are HallOfLightSite's, made after the pieces so the enclosures name what was drawn.
 */
namespace Musee::HallOfLight
{
	/** Frames set out by their axes are Form's (FormAxes.h). */
	using Musee::Form::FrameOfAxes;

	/** A key under the stage: hall-of-light.<Local>. */
	[[nodiscard]] FMuseeKey Key(const FString& Local);

	/** A surface of the stage's finish schedule: hall-of-light.<Local>. */
	[[nodiscard]] FName Surface(const TCHAR* Local);

	/** A drawing of the stage, finished in one of its surfaces, colliding as given, standing in a frame. */
	template <typename TShape>
	[[nodiscard]] FFormDrawing Drawn(const FString& Name, FName InSurface, TShape&& Shape, const Musee::Collision::FClass* Collision,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing Drawing = FFormDrawing::Of(FName(*Name), InSurface, Forward<TShape>(Shape));
		Drawing.Collision = Collision ? Collision->Profile() : NAME_None;
		Drawing.Frame = Frame;
		return Drawing;
	}

	/** The glass walls: -1 the north wall, +1 the south. */
	inline constexpr int32 Sides[] = {-1, 1};
	[[nodiscard]] const TCHAR* SideName(int32 Side);

	/** A wall member's y at depth D in from the glass line (sec. 1). */
	[[nodiscard]] double WallY(int32 Side, double D);

	/** The rhythms along the hall (sec. 1): post k (0-9), arch k (0-17), bay b (1-10). */
	[[nodiscard]] double PostX(int32 Post);
	[[nodiscard]] double ArchX(int32 Arch);
	[[nodiscard]] double BayX(int32 Bay);

	/** Stereo stone i (0-3) on the floor: north of the axis first, then alternating (sec. 1). */
	[[nodiscard]] FVector StereoStone(int32 Index);

	/**
	 * Its reader plate's centre and frame (the stage contract's Mount): x to the viewer's right, y the plate's normal
	 * towards the axis tilted up 0.5 rad, z up the plate.
	 */
	[[nodiscard]] FMuseeFrame StereoReader(int32 Index);

	/** The vault's angle where the arches and panes bear on the eaves' top (z 5.10): 56.83 deg, in radians. */
	[[nodiscard]] double EavesAngle();

	/** The vault's nine lines across, north to south, in radians: the north eaves, the seven purlins, the south eaves. */
	[[nodiscard]] TArray<double> VaultLines();

	/** The point at a vault angle (radians, from the crown, positive south) and radius about the vault's axis, at x. */
	[[nodiscard]] FVector VaultPoint(double X, double Theta, double Radius);

	/** The frame of the vault's axis: x up, y north, z east, so a lathe about it turns from the crown towards the north. */
	[[nodiscard]] FMuseeFrame VaultAxis();

	void DrawMasonry(TArray<FFormDrawing>& Out);
	void DrawLinings(TArray<FFormDrawing>& Out);
	void DrawWalls(TArray<FFormDrawing>& Out);
	void DrawVault(TArray<FFormDrawing>& Out);
	void DrawBronze(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);
}
