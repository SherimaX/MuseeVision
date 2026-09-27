#pragma once

#include "CoreMinimal.h"
#include "Growth/NatureSurface.h"
#include "Math/RandomStream.h"
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
	}

	/**
	 * Where one plant grows, in its bed's frame (metres; the bed anchor at the origin, on the soil): its crown, the
	 * level of the water over the soil (for a plant of the water or its margin), the plan direction to the water's
	 * middle (a marginal leans out over the water, away from the rim), and the test that keeps its low parts off the
	 * rim: a point near or under the water must lie inside the water's outline, less a margin.
	 */
	struct FPlantSite
	{
		FVector Crown = FVector::ZeroVector;
		double WaterLevel = 0.0;
		FVector2D Inward = FVector2D::ZeroVector;
		TFunction<bool(const FVector& Point, double MarginMeters)> IsClear;

		/** Whether every point of a surface is clear of the rim by a margin. */
		[[nodiscard]] bool IsClearOf(const FNatureSurface& Surface, double MarginMeters) const;
	};

	/** The organs one planting grows, one surface per organ kind. */
	using FPlantParts = TMap<FName, FNatureSurface>;

	/** A form's grower: one plant of the species at a site, drawn from its seeded stream. */
	using FGrower = void (*)(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out);

	/** A form: its name in the species records, the organs it grows, whether it must stand in water, and its grower. */
	struct FForm
	{
		FName Name;
		TArray<FName> Organs;
		bool bInWater = false;
		FGrower Grow = nullptr;
		/** What the species record lacks for this form, one sentence each. */
		TArray<FString> (*Faults)(const FNatureSpecies& Species) = nullptr;
	};

	/** The form a species record names, or null. */
	[[nodiscard]] const FForm* FindForm(FName Name);

	/** A measure's range drawn uniformly. The form's Faults have checked it holds two numbers. */
	[[nodiscard]] double Draw(const TArray<float>& Range, FRandomStream& Random);

	/** An integer range drawn uniformly. */
	[[nodiscard]] int32 Draw(const TArray<int32>& Range, FRandomStream& Random);

	/** A colour of a list, as linear. */
	[[nodiscard]] FLinearColor Colour(const TArray<FString>& List, int32 Index);

	/** The faults of a range: it must hold two numbers, least first. */
	void NeedRange(const TArray<float>& Range, const TCHAR* Name, TArray<FString>& Out);
	void NeedRange(const TArray<int32>& Range, const TCHAR* Name, TArray<FString>& Out);

	/** The faults of a colour list: it must hold at least Count colours. */
	void NeedColours(const TArray<FString>& List, int32 Count, const TCHAR* Name, TArray<FString>& Out);

	/** The four forms, each in its own source. */
	const FForm& WaterLilyForm();
	const FForm& IrisForm();
	const FForm& SedgeForm();
	const FForm& ForgetMeNotForm();
}
