#pragma once

#include "CoreMinimal.h"
#include "Growth/NatureSurface.h"
#include "Math/RandomStream.h"

/**
 * The organ shapes plants are grown from, re-expressed from the as-built museum's generators
 * (windows/Source/MuseeVision/Nature/NatureMesh.cpp and MuseeWaterLilies.cpp): tubes for stalks, blades for strap and
 * sword leaves, curved petal sheets, cards for small leaves and florets, ellipsoids for buds and spikes, and lathes
 * for turned parts (a lily's stigma disc, a bud). All in metres, with UVs in metres along the organ.
 */
namespace Musee::NatureMaking
{
	inline constexpr double Tau = 2.0 * UE_DOUBLE_PI;

	/** A colour written as sRGB hex (`#3F6B2E` or `3F6B2E`), as linear; black if it isn't one. */
	[[nodiscard]] FLinearColor FromHex(const FString& Hex);

	/** A blend of two colours, T from 0 (A) to 1 (B). */
	[[nodiscard]] FLinearColor Mix(const FLinearColor& A, const FLinearColor& B, double T);

	/** A natural variation of a colour: hue by up to Hue degrees, saturation and value by up to the given fractions. */
	[[nodiscard]] FLinearColor Vary(const FLinearColor& Colour, FRandomStream& Random, double Hue, double Saturation, double Value);

	/** Rotates V about Axis by Angle radians (Rodrigues). */
	[[nodiscard]] FVector RotateAbout(const FVector& V, const FVector& Axis, double Angle);

	/** Two unit vectors perpendicular to Dir and to each other, U horizontal where it can be. */
	void Basis(const FVector& Dir, FVector& OutU, FVector& OutW);

	/** A ring of a tube: its centre, the tube's direction there, its radius and its tint. */
	struct FTubeRing
	{
		FVector Centre = FVector::ZeroVector;
		FVector Dir = FVector::UpVector;
		double Radius = 0.01;
		FLinearColor Tint = FLinearColor::White;
	};

	/** A tapered tube through its rings (a stalk, a culm), its frame carried without twist; capped at the top if asked. */
	void AddTube(FNatureSurface& Mesh, const TArray<FTubeRing>& Rings, int32 Sides, bool bCapEnd);

	/** A stalk from From to To, bowed sideways by Bow at its middle, tapering by a quarter, tinted from foot to top. */
	void AddStalk(FNatureSurface& Mesh, const FVector& From, const FVector& To, double Radius, const FVector& Bow, const FLinearColor& Foot,
		const FLinearColor& Top);

	/** A blade: a strap or sword leaf, a sedge's blade. Curl and twist in degrees over its length; droop in radians a metre. */
	struct FBlade
	{
		double Length = 0.1;
		double Width = 0.02;
		double Curl = 0.0;
		double Droop = 0.0;
		double Twist = 0.0;
		/** How far the edges rise towards the upper side, as a share of the width. */
		double Cup = 0.0;
		/** Where the blade is widest: smaller is nearer the base. */
		double TipShape = 0.8;
		/** Its width at the base, as a share. */
		double BaseWidth = 0.2;
		int32 Across = 2;
		int32 Along = 6;
		FLinearColor Root = FLinearColor::White;
		FLinearColor Tip = FLinearColor::White;
	};

	/** A blade from Base along Dir, its upper side facing Normal. */
	void AddBlade(FNatureSurface& Mesh, const FVector& Base, const FVector& Dir, const FVector& Normal, const FBlade& Blade);

	/** A petal (a fall, a standard, a sepal, a stamen): a curved sheet out from Base along a level Radial direction. */
	struct FPetal
	{
		double Length = 0.05;
		double Width = 0.02;
		/** Its rise above the level at the base, in degrees. */
		double Elevation = 30.0;
		/** How much more it rises (negative: falls) by its tip, in degrees. */
		double Curl = 0.0;
		/** How the curl gathers along it: 1 evenly, 2 towards the tip. */
		double CurlPower = 1.0;
		/** How far the edges rise towards the upper side, as a share of the width. */
		double Cup = 0.0;
		/** Where it is widest: smaller is nearer the base. */
		double TipShape = 1.0;
		/** Its width at the base, as a share. */
		double BaseWidth = 0.25;
		/** The waves along its edge, as a share of the width (an iris fall's ruffle). */
		double Ruffle = 0.0;
		int32 Across = 4;
		int32 Along = 7;
	};

	/** A petal, its tint a function of the place across it (X from -1 to 1) and along it (S from 0 to 1). */
	void AddPetal(FNatureSurface& Mesh, const FVector& Base, const FVector& Radial, const FPetal& Petal,
		TFunctionRef<FLinearColor(double X, double S)> Tint);

	/** A leaf or floret card from Base along Dir, facing Normal: its halves folded on the midrib by Fold, its tip dropped by Curl. */
	void AddCard(FNatureSurface& Mesh, const FVector& Base, const FVector& Dir, const FVector& Normal, double Length, double Width,
		double Fold, double Curl, int32 Rows, const FLinearColor& Tint, double Occlusion);

	/** A flat floret seen face on: a square card about Centre facing Normal, turned by Spin, its middle a little cupped. */
	void AddFlowerCard(FNatureSurface& Mesh, const FVector& Centre, const FVector& Normal, double Spin, double Size, double Cup,
		const FLinearColor& Tint, double Occlusion);

	/** An ellipsoid about Axis: a bud, a spikelet. */
	void AddEllipsoid(FNatureSurface& Mesh, const FVector& Centre, const FVector& Axis, double Radius, double HalfLength, int32 Segments,
		int32 Rings, const FLinearColor& Tint, double Occlusion);

	/** A profile of (radius, height) points turned about Axis from Origin, tinted per point (or one tint for all). */
	void AddLathe(FNatureSurface& Mesh, const FVector& Origin, const FVector& Axis, const TArray<FVector2D>& Profile, int32 Segments,
		const TArray<FLinearColor>& Tints, double Occlusion);
}
