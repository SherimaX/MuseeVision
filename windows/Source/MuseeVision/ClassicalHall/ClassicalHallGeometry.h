#pragma once

#include "CoreMinimal.h"
#include "ClassicalHall/ClassicalHallKit.h"

/**
 * The classical hall's geometry and fittings, in plan metres with the Rotunda's centre at the origin (X east, Y south,
 * Z up; heights include the hall's floor at MuseePlan::ClassicalHall::FloorZ). Pure CoreMinimal: AClassicalHallStructure
 * writes it into procedural meshes and lights, and the same code builds outside the engine for the geometry checks.
 */
namespace ClassicalHallGeometry
{
	using ClassicalHallKit::FMesh;

	/** One mesh per material. */
	enum class EPart : uint8
	{
		Wall,            // cream marble ashlar: walls, reveals, passage, lintel soffit
		Dado,            // rosso antico: the dado below the rail
		Niche,           // Pompeian stucco: the niches' insides
		FloorWhite,      // polished white marble: the plain floor and the pale inlays
		FloorGiallo,     // giallo antico
		FloorPorphyry,   // red porphyry: discs, the threshold
		FloorVerde,      // verde antico
		FloorRosso,      // rosso antico
		Carved,          // honed statuary marble: bases, capitals, entablatures, frames, skirtings, plinths, consoles
		ShaftGiallo,     // the gallery's column shafts
		ShaftPavonazzetto, // the tribune's ring of shafts
		Panel,           // pavonazzetto wall panels
		Vault,           // stucco: the vaults, the dome, the coffers
		Gilt,            // the coffers' rosettes
		Laylight,        // the luminous diffusers
		Shell,           // the closed masonry box outside (never seen)
		Collision,       // simple blockers for the walk (columns, plinths, niche mouths): collision only, never drawn
		Count
	};

	struct FHall
	{
		FMesh Parts[static_cast<int32>(EPart::Count)];

		FMesh& operator[](EPart P) { return Parts[static_cast<int32>(P)]; }
		const FMesh& operator[](EPart P) const { return Parts[static_cast<int32>(P)]; }
	};

	/** Builds everything (a few hundred thousand triangles). */
	FHall Build();

	/** A place for a work of art: where it stands (the top of its plinth, console or pedestal) and which way it faces. */
	struct FSpot
	{
		FString Name;       // e.g. Gallery.Niche.E1
		FString Kind;       // Niche, Bust, Herm, Reclining, Freestanding, Centrepiece
		FVector Location = FVector::ZeroVector;   // metres: the top of the plinth, console or pedestal, where the work stands
		FVector Facing = FVector::ForwardVector;  // the way the work looks (out of its niche, into the room)
		double Height = 0.0;                      // the tallest work that fits (suggested height), metres
		double MaxWidth = 0.0;                    // the widest (or, reclining, the longest) work that fits, metres
		FString Suggested;                        // the cast meant for it, if any
	};

	TArray<FSpot> StatueSpots();
	FSpot Centrepiece();

	/** A light: a rect light (laylight, cove) or a spot, in metres; intensities in candela, colour temperature in K. */
	struct FLight
	{
		enum class EType : uint8 { Rect, Spot };
		EType Type = EType::Rect;
		FString Name;
		FVector Location = FVector::ZeroVector;
		FVector Direction = -FVector::UpVector;   // where it shines
		FVector Across = FVector::ForwardVector;  // a rect light's width runs along this
		double Width = 1.0, Height = 1.0;         // a rect light's size, metres
		double BarnDoorAngle = 88.0, BarnDoorLength = 0.0;
		double Candela = 1000.0;
		double InnerCone = 10.0, OuterCone = 20.0; // a spot's cones, degrees
		double SourceRadius = 0.05;               // a spot's source, metres
		double Range = 20.0;                      // attenuation radius, metres
		double Kelvin = 3800.0;
		bool bShadows = true;
		bool bLaylight = false;                   // behind a laylight's diffuser (its luminance must match)
	};

	/** Every light of the hall, in a fixed order (the actor makes one component each). */
	TArray<FLight> Lights();

	/** The laylights' luminance (nits): their diffusers' emissive and the rect lights behind them agree. */
	double LaylightNits();
}
