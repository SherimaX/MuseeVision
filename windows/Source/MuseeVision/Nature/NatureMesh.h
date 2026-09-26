#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "ProceduralMeshComponent.h"

class UMaterialInterface;

/**
 * What the Nature materials (Scripts/nature.py) read per vertex, besides the surface UV (UV0):
 *
 *   UV1 = (Bend, Flutter)       how far the point sways with the wind; leaf and petal flutter
 *   UV2 = (Phase, FlutterPhase) sway phase, continuous along a branch; flutter phase, one per leaf
 *   UV3 = (Height, Twig)        metres above the plant's base; 0 on the trunk … 1 on the finest twigs
 *   Colour                      the tint in sRGB (the materials decode it), alpha = ambient occlusion
 */
struct FNatureWind
{
	double Bend = 0;
	double Flutter = 0;
	double Phase = 0;
	double FlutterPhase = 0;
	double Height = 0;
	double Twig = 0;

	static FNatureWind Lerp(const FNatureWind& A, const FNatureWind& B, double T)
	{
		FNatureWind W;
		W.Bend = FMath::Lerp(A.Bend, B.Bend, T);
		W.Flutter = FMath::Lerp(A.Flutter, B.Flutter, T);
		W.Phase = FMath::Lerp(A.Phase, B.Phase, T);
		W.FlutterPhase = FMath::Lerp(A.FlutterPhase, B.FlutterPhase, T);
		W.Height = FMath::Lerp(A.Height, B.Height, T);
		W.Twig = FMath::Lerp(A.Twig, B.Twig, T);
		return W;
	}
};

/**
 * One procedural mesh section with the channels above. Positions are given in metres in the
 * actor's frame (x east, y south, z up) and written in centimetres. Triangles are wound to face
 * along their vertices' normals (Unreal's front faces: cross(b − a, c − a) points away from the
 * side they face), so give normals that point towards the viewer of that face.
 */
struct FNatureMesh
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	TArray<FVector2D> UV0;
	TArray<FVector2D> UV1;
	TArray<FVector2D> UV2;
	TArray<FVector2D> UV3;
	TArray<FColor> Colors;

	int32 Vertex(const FVector& PositionM, const FVector& Normal, const FVector& TangentX, const FVector2D& UV,
				 const FNatureWind& Wind, const FLinearColor& Colour, double Occlusion = 1.0)
	{
		const int32 Index = Vertices.Num();
		Vertices.Add(PositionM * 100.0);
		Normals.Add(Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector));
		Tangents.Add(FProcMeshTangent(TangentX.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector), false));
		UV0.Add(UV);
		UV1.Add(FVector2D(Wind.Bend, Wind.Flutter));
		UV2.Add(FVector2D(Wind.Phase, Wind.FlutterPhase));
		UV3.Add(FVector2D(Wind.Height, Wind.Twig));
		FLinearColor C = Colour;
		C.A = static_cast<float>(FMath::Clamp(Occlusion, 0.0, 1.0));
		Colors.Add(C.ToFColor(true));
		return Index;
	}

	void Triangle(int32 A, int32 B, int32 C)
	{
		const FVector& PA = Vertices[A];
		const FVector& PB = Vertices[B];
		const FVector& PC = Vertices[C];
		const FVector Facing = Normals[A] + Normals[B] + Normals[C];
		if (FVector::DotProduct(FVector::CrossProduct(PB - PA, PC - PA), Facing) < 0) { Triangles.Append({A, B, C}); }
		else { Triangles.Append({A, C, B}); }
	}

	void Quad(int32 A, int32 B, int32 C, int32 D)
	{
		Triangle(A, B, C);
		Triangle(A, C, D);
	}

	int32 NumTriangles() const { return Triangles.Num() / 3; }
	bool IsEmpty() const { return Triangles.Num() == 0; }

	void Reserve(int32 InVertices, int32 InTriangles)
	{
		Vertices.Reserve(InVertices);
		Normals.Reserve(InVertices);
		Tangents.Reserve(InVertices);
		UV0.Reserve(InVertices);
		UV1.Reserve(InVertices);
		UV2.Reserve(InVertices);
		UV3.Reserve(InVertices);
		Colors.Reserve(InVertices);
		Triangles.Reserve(InTriangles * 3);
	}
};

/** A ring of a tube: where it is, which way the tube runs, its radius and what it carries. */
struct FNatureTubeRing
{
	FVector Centre = FVector::ZeroVector;
	FVector Dir = FVector::UpVector;
	double Radius = 0.01;
	FNatureWind Wind;
	FLinearColor Colour = FLinearColor::White;
	double Occlusion = 1.0;
};

/** A blade of solid geometry: a strap leaf, a grass blade, a petal. */
struct FNatureBlade
{
	double Length = 0.1;
	double Width = 0.02;
	double Curl = 0;          // degrees over the length, towards the upper side
	double Droop = 0;         // bends towards the ground along the length (radians per metre)
	double Twist = 0;         // degrees over the length
	double Cup = 0;           // the edges raised towards the upper side (fraction of the width)
	double TipShape = 0.8;    // where it is widest: smaller is nearer the base
	double BaseWidth = 0.2;   // width at the base (fraction)
	int32 NX = 2;             // quads across (2 gives a keel)
	int32 NY = 6;             // quads along
	FLinearColor Root = FLinearColor::White;
	FLinearColor Tip = FLinearColor::White;
	double BendGain = 0.2;    // wind bend added from base to tip
	double FlutterGain = 0.5;
};

/** Irregularity of a tube: a root flare with buttress lobes near the ground, and gnarled wood. */
struct FNatureTubeShape
{
	double Flare = 0;         // extra radius at the ground (fraction)
	double FlareHeight = 0.35;
	int32 Lobes = 0;          // buttress roots
	double Gnarl = 0;         // radial noise (fraction)
	int32 Seed = 0;
};

namespace MuseeNature
{
	constexpr double Tau = 2.0 * UE_DOUBLE_PI;

	/** Rotates V about Axis by Angle (radians), right-handed in the maths sense (Rodrigues). */
	inline FVector RotateAbout(const FVector& V, const FVector& Axis, double Angle)
	{
		const FVector K = Axis.GetSafeNormal();
		if (K.IsNearlyZero()) { return V; }
		const double C = FMath::Cos(Angle), S = FMath::Sin(Angle);
		return V * C + FVector::CrossProduct(K, V) * S + K * FVector::DotProduct(K, V) * (1.0 - C);
	}

	/** Two unit vectors perpendicular to Dir and to each other (U horizontal where it can be). */
	inline void Basis(const FVector& Dir, FVector& OutU, FVector& OutW)
	{
		OutU = FVector::CrossProduct(Dir, FVector::UpVector);
		if (OutU.SizeSquared() < 1e-8) { OutU = FVector::CrossProduct(Dir, FVector::ForwardVector); }
		OutU.Normalize();
		OutW = FVector::CrossProduct(Dir, OutU).GetSafeNormal();
	}

	inline double Hash(int32 X, int32 Y, int32 Z, int32 Seed)
	{
		uint32 H = uint32(X) * 0x8da6b343u ^ uint32(Y) * 0xd8163841u ^ uint32(Z) * 0xcb1ab31fu ^ uint32(Seed) * 0x165667b1u;
		H ^= H >> 13;
		H *= 0x5bd1e995u;
		H ^= H >> 15;
		return double(H & 0xFFFFFFu) / double(0xFFFFFFu);
	}

	/** Smooth value noise in [0, 1]. */
	inline double Noise(const FVector& P, int32 Seed = 0)
	{
		const double FX = FMath::FloorToDouble(P.X), FY = FMath::FloorToDouble(P.Y), FZ = FMath::FloorToDouble(P.Z);
		const int32 IX = int32(FX), IY = int32(FY), IZ = int32(FZ);
		auto Smooth = [](double T) { return T * T * (3.0 - 2.0 * T); };
		const double TX = Smooth(P.X - FX), TY = Smooth(P.Y - FY), TZ = Smooth(P.Z - FZ);
		auto Corner = [IX, IY, IZ, Seed](int32 DX, int32 DY, int32 DZ) { return Hash(IX + DX, IY + DY, IZ + DZ, Seed); };
		const double X00 = FMath::Lerp(Corner(0, 0, 0), Corner(1, 0, 0), TX);
		const double X10 = FMath::Lerp(Corner(0, 1, 0), Corner(1, 1, 0), TX);
		const double X01 = FMath::Lerp(Corner(0, 0, 1), Corner(1, 0, 1), TX);
		const double X11 = FMath::Lerp(Corner(0, 1, 1), Corner(1, 1, 1), TX);
		return FMath::Lerp(FMath::Lerp(X00, X10, TY), FMath::Lerp(X01, X11, TY), TZ);
	}

	/** Fractal value noise in about [0, 1]. */
	inline double Fbm(const FVector& P, int32 Octaves, int32 Seed = 0)
	{
		double Sum = 0, Amp = 0.5, Norm = 0;
		FVector Q = P;
		for (int32 O = 0; O < Octaves; ++O)
		{
			Sum += Amp * Noise(Q, Seed + O * 131);
			Norm += Amp;
			Q = Q * 2.03 + FVector(17.1, 3.7, 9.3);
			Amp *= 0.5;
		}
		return Sum / FMath::Max(Norm, 1e-6);
	}

	/** An sRGB colour 0xRRGGBB as linear. */
	inline FLinearColor Srgb(uint32 Hex)
	{
		return FLinearColor::FromSRGBColor(FColor(uint8((Hex >> 16) & 255u), uint8((Hex >> 8) & 255u), uint8(Hex & 255u), 255));
	}

	/** A natural variation of a colour: hue (degrees), saturation and value (fractions). */
	inline FLinearColor Vary(const FLinearColor& Colour, const FRandomStream& Rng, double Hue, double Sat, double Val)
	{
		FLinearColor HSV = Colour.LinearRGBToHSV();
		HSV.R = static_cast<float>(FMath::Fmod(double(HSV.R) + Rng.FRandRange(-Hue, Hue) + 360.0, 360.0));
		HSV.G = static_cast<float>(FMath::Clamp(double(HSV.G) * (1.0 + Rng.FRandRange(-Sat, Sat)), 0.0, 1.0));
		HSV.B = static_cast<float>(FMath::Max(0.0, double(HSV.B) * (1.0 + Rng.FRandRange(-Val, Val))));
		FLinearColor Out = HSV.HSVToLinearRGB();
		Out.A = 1.f;
		return Out;
	}

	inline FLinearColor Mix(const FLinearColor& A, const FLinearColor& B, double T)
	{
		const float F = static_cast<float>(FMath::Clamp(T, 0.0, 1.0));
		return A * (1.f - F) + B * F;
	}

	/** Distance along Dir from P to the far side of an axis-aligned ellipsoid, or -1 if the ray misses it. */
	inline double RayToEllipsoid(const FVector& P, const FVector& Dir, const FVector& Centre, const FVector& Radii)
	{
		const FVector Q = (P - Centre) / Radii;
		const FVector E = Dir / Radii;
		const double A = FVector::DotProduct(E, E);
		const double B = 2.0 * FVector::DotProduct(Q, E);
		const double C = FVector::DotProduct(Q, Q) - 1.0;
		const double Disc = B * B - 4.0 * A * C;
		if (Disc < 0 || A < 1e-12) { return -1.0; }
		const double S = (-B + FMath::Sqrt(Disc)) / (2.0 * A);
		return S > 0 ? S : -1.0;
	}

	/** Loads /Game/Museum/Nature/<Name> (made by Scripts/nature.py); warns once if it is missing. */
	UMaterialInterface* LoadMaterial(const TCHAR* Name);

	/** A tapered tube through the rings: smooth normals, u round the tube and v along it, both in metres. */
	void AddTube(FNatureMesh& Mesh, const TArray<FNatureTubeRing>& Rings, int32 Sides, double VStart, bool bCapEnd,
				 const FNatureTubeShape& Shape = FNatureTubeShape());

	/**
	 * A leaf or petal card from Base along Dir, Width across, facing Normal; u across (0 left … 1
	 * right), v along (0 at the stalk … 1 at the tip). Fold tilts the two halves' normals like a
	 * leaf folded on its midrib; Curl bends the tip down (fraction of the length); Rows 1 or 2.
	 * ShadeBias leans the shading normals (not the card) towards a direction: out of the crown, so
	 * a crown of cards shades like one soft volume.
	 */
	void AddCard(FNatureMesh& Mesh, const FVector& Base, const FVector& Dir, const FVector& Normal, double Length, double Width,
				 double Fold, double Curl, int32 Rows, const FNatureWind& WindBase, const FNatureWind& WindTip,
				 const FLinearColor& Colour, double Occlusion, const FVector& ShadeBias = FVector::ZeroVector);

	/** A square card centred on Centre, facing Normal, turned by Spin about it (flowers). */
	void AddFlowerCard(FNatureMesh& Mesh, const FVector& Centre, const FVector& Normal, double Spin, double Size, double Cup,
					   const FNatureWind& Wind, const FLinearColor& Colour, double Occlusion);

	/** A blade from Base along Dir, its upper side facing Normal; u across, v along; normals smooth. */
	void AddBlade(FNatureMesh& Mesh, const FVector& Base, const FVector& Dir, const FVector& Normal, const FNatureBlade& Blade, const FNatureWind& Wind0);

	/** An ellipsoid about Axis (fruit, buds, berries). */
	void AddEllipsoid(FNatureMesh& Mesh, const FVector& Centre, const FVector& Axis, double Radius, double HalfLength,
					  int32 Segments, int32 Rings, const FNatureWind& Wind, const FLinearColor& Colour, double Occlusion);

	/** A profile (radius, height) turned about Axis from Origin; colours per profile point (or one for all). */
	void AddLathe(FNatureMesh& Mesh, const FVector& Origin, const FVector& Axis, const TArray<FVector2D>& Profile, int32 Segments,
				  const FNatureWind& Wind, const TArray<FLinearColor>& Colours, double Occlusion, double USpan = 1.0);
}
