#pragma once

#include "CoreMinimal.h"
#include "Nature/MuseeLawn.h"

/**
 * Clipped hedges of real leaves: box (Buxus sempervirens) and yew (Taxus baccata), for the grounds' parterre, its
 * topiary balls, the exedra and the Hall of Light's garden hedges (AMuseeLandscape plants them).
 *
 * A clipped hedge is a shell: the shears keep a dense skin of small leaves on a surface that is crisp but never
 * mathematically smooth (a few millimetres of wander, here and there a shallow dip), a few centimetres deep, over a dark
 * interior of bare twigs. Each module is that shell for one piece of hedge, built as real geometry (no alpha cards):
 * box leaves 1–2 cm, elliptic, cupped (the margins rolled under), a pale midrib, glossy dark green above and paler
 * beneath, the season's new growth lighter at the skin, older leaves duller inside; yew needles 1.6–2.6 cm. They are
 * sown in the shell with most at the skin and fewer deeper down, each turned out towards the light, and pushed back
 * inside the clipped surface wherever a tip would stand proud of it (the shears' work). Twigs run out to the skin.
 * The vertex alpha is the occlusion down in the shell.
 *
 * The editor bakes them (UMuseeLawnLibrary::BuildHedgeMeshes, Scripts/lawn.py hedges) into Nanite static meshes in
 * /Game/Museum/Nature/Hedge; the landscape instances them along its hedges, with a dark procedural core (collision)
 * Inset inside the clipped surface. Modules, in module space (metres, x along, y across, z up from the ground):
 * - Run: a straight piece Length long (x 0 … Length), both sides and the top; its two ends join any other module. (The
 *   exedra's yew runs are bent round its centre, 14 m to their left: FSpec::Bend.)
 * - End: an open end, the hedge on −x, its end face at x 0 (x −EndLength … 0).
 * - Corner: a convex right-angled corner at the origin, the hedge running on along +x and +y (x, y −W/2 … W/2).
 * - Ball: a clipped ball on the ground at the origin.
 * Where modules meet, the clipped surface's wander is shared (it is a function of the cross-section alone there), so
 * a hedge shows no joints.
 */
namespace MuseeHedge
{
	enum class EKind : uint8
	{
		BoxRun,
		BoxCorner,
		BoxEnd,
		BoxBall,
		YewRun,
		YewEnd,
		NarrowYewRun,
		NarrowYewEnd,
		Num
	};

	enum class EPart : uint8
	{
		Run,
		End,
		Corner,
		Ball
	};

	struct FSpec
	{
		const TCHAR* Name = TEXT("");
		EPart Part = EPart::Run;
		bool bYew = false;
		int32 Variants = 1;
		/** The clipped hedge's width at the foot and height (m); for a ball, its radius (Width) and diameter. */
		double Width = 0.42, Height = 0.52;
		/** Narrower at the top by this on each side (the shears' batter). */
		double Batter = 0.015;
		/** Radius of the clipped shoulders. */
		double Shoulder = 0.05;
		/** The dark core's surface lies this far inside the clipped one. */
		double Inset = 0.06;
		/** Leaves per square metre of clipped surface. */
		double Density = 30000.0;
		/** A run bent round a centre this far to its left (+y), m (0: straight): the exedra's curve, so it has no joints. */
		double Bend = 0.0;
	};

	/** Run modules' length; End modules' length. */
	constexpr double Length = 0.5;
	constexpr double EndLength = 0.3;
	/** How far a ball sinks into the ground. */
	constexpr double BallSink = 0.04;

	MUSEEVISION_API const FSpec& Spec(EKind Kind);
	MUSEEVISION_API int32 NumMeshes();
	MUSEEVISION_API int32 MeshIndex(EKind Kind, int32 Variant);
	MUSEEVISION_API void DecodeMeshIndex(int32 Index, EKind& OutKind, int32& OutVariant);
	MUSEEVISION_API FString MeshName(int32 Index);
	MUSEEVISION_API FString MeshPath(int32 Index);
	/** The leaves (and twigs) of one module, in centimetres. */
	MUSEEVISION_API void BuildModule(int32 Index, FMuseeLawnPatch& Out);

	inline const TCHAR* Folder() { return TEXT("/Game/Museum/Nature/Hedge"); }
	inline const TCHAR* BoxMaterialPath() { return TEXT("/Game/Museum/Nature/MI_Hedge_Box.MI_Hedge_Box"); }
	inline const TCHAR* YewMaterialPath() { return TEXT("/Game/Museum/Nature/MI_Hedge_Yew.MI_Hedge_Yew"); }
	/** The dark interior (bare twigs in shade) under the leaves: the landscape's core. */
	inline const TCHAR* CoreMaterialPath() { return TEXT("/Game/Museum/Nature/MI_Hedge_Core.MI_Hedge_Core"); }
}
