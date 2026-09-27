#pragma once

#include "CoreMinimal.h"
#include "Growth/NatureSurface.h"
#include "Math/RandomStream.h"
#include "NatureClip.h"
#include "NatureSpecies.h"

namespace Musee::NatureMaking
{
	/** The organ kinds a form grows, each cast as its own piece with its species' tissue. */
	namespace Organ
	{
		inline const FName Leaf(TEXT("leaf"));
		inline const FName Sepal(TEXT("sepal"));
		inline const FName Flower(TEXT("flower"));
		inline const FName Eye(TEXT("eye"));
		inline const FName Stem(TEXT("stem"));
		inline const FName Seed(TEXT("seed"));
		/** A tree's trunk (or trunks), which the visitor can't pass through, and a hedge's core of bare twigs. */
		inline const FName Trunk(TEXT("trunk"));
	}

	/**
	 * Where one plant grows, in its bed's frame (metres; the bed anchor at the origin, on the soil): its crown, the
	 * level of the water over the soil (for a plant of the water or its margin), the plan direction to the water's
	 * middle (a marginal leans out over the water, away from the rim), and the test that keeps its low parts off the
	 * rim: a point near or under the water must lie inside the water's outline, less a margin. A tree takes the height
	 * and crown its bed draws, a hedge its line and section, a cover its region, a climber the paths it is trained along.
	 */
	struct FPlantSite
	{
		FVector Crown = FVector::ZeroVector;
		double WaterLevel = 0.0;
		/** How deep the water is, from its surface to its bed (a lotus's stalks rise from the mud). */
		double WaterDepth = 0.0;
		FVector2D Inward = FVector2D::ZeroVector;
		TFunction<bool(const FVector& Point, double MarginMeters)> IsClear;

		/** The plant's height, its crown's middle and radii [across, vertical], as the bed draws them; zero for the species' own. */
		double Height = 0.0;
		double CrownCentre = 0.0;
		FVector2D CrownRadii = FVector2D::ZeroVector;

		/**
		 * A hedge's line, from and to, on the soil, and its clipped section; whether the run's start and end are the
		 * hedge's own open ends, clipped across (a run between two others is closed at both).
		 */
		TArray<FVector> Line;
		FNatureClip Clip;
		bool bOpenStart = false;
		bool bOpenEnd = false;

		/** A cover's region on the soil: its outline and the holes in it (the water), as plan points. */
		TArray<FVector2D> Region;
		TArray<TArray<FVector2D>> Holes;

		/** A climber's paths, its stem first, then its canes. */
		TArray<TArray<FVector>> Trained;

		/** Whether every point of a surface is clear of the rim by a margin. */
		[[nodiscard]] bool IsClearOf(const FNatureSurface& Surface, double MarginMeters) const;

		/** Whether a plan point lies in the cover's region and out of its holes. */
		[[nodiscard]] bool InRegion(const FVector2D& Point) const;
	};

	/** The organs one planting grows, one surface per organ kind. */
	using FPlantParts = TMap<FName, FNatureSurface>;

	/** A form's grower: one plant of the species at a site, drawn from its seeded stream. */
	using FGrower = void (*)(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out);

	/**
	 * How a form is planted: one plant at each crown of its bed (a tree, an iris); once along the bed's line (a clipped
	 * hedge, whose plants grow into one); or once over the bed's region (a cover of moss or grass).
	 */
	enum class EPlanting : uint8
	{
		EachPlant,
		AlongLine,
		OverRegion,
	};

	/**
	 * A form: its name in the species records, the organs it grows, whether it must stand in water, how it is planted,
	 * and its grower. The organs that stop the visitor (a trunk, a hedge's core) are solid.
	 */
	struct FForm
	{
		FName Name;
		TArray<FName> Organs;
		bool bInWater = false;
		FGrower Grow = nullptr;
		/** What the species record lacks for this form, one sentence each. */
		TArray<FString> (*Faults)(const FNatureSpecies& Species) = nullptr;
		EPlanting Planting = EPlanting::EachPlant;
		/** Whether its leaves float on the water (a lily's pads). */
		bool bFloating = false;
	};

	/** The form a species record names, or null. */
	[[nodiscard]] const FForm* FindForm(FName Name);

	/** Every form's name, for the finding that names none. */
	[[nodiscard]] FString FormNames();

	/** A measure's range drawn uniformly. The form's Faults have checked it holds two numbers. */
	[[nodiscard]] double Draw(const TArray<float>& Range, FRandomStream& Random);

	/** An integer range drawn uniformly. */
	[[nodiscard]] int32 Draw(const TArray<int32>& Range, FRandomStream& Random);

	/** The middle of a range. */
	[[nodiscard]] double Middle(const TArray<float>& Range);

	/** A colour of a list, as linear. */
	[[nodiscard]] FLinearColor Colour(const TArray<FString>& List, int32 Index);

	/** A colour drawn from a list, as linear. */
	[[nodiscard]] FLinearColor AnyColour(const TArray<FString>& List, FRandomStream& Random);

	/** The faults of a range: it must hold two numbers, least first. */
	void NeedRange(const TArray<float>& Range, const TCHAR* Name, TArray<FString>& Out);
	void NeedRange(const TArray<int32>& Range, const TCHAR* Name, TArray<FString>& Out);

	/** The faults of a colour list: it must hold at least Count colours. */
	void NeedColours(const TArray<FString>& List, int32 Count, const TCHAR* Name, TArray<FString>& Out);

	/** The forms, each in its own source. */
	const FForm& WaterLilyForm();
	const FForm& IrisForm();
	const FForm& SedgeForm();
	const FForm& ForgetMeNotForm();
	const FForm& TreeForm();
	const FForm& BambooForm();
	const FForm& LotusForm();
	const FForm& MossForm();
	const FForm& GrassForm();
	const FForm& HerbForm();
	const FForm& HedgeForm();
	const FForm& VineForm();
}
