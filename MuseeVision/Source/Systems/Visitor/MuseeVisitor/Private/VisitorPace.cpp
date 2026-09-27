#include "VisitorPace.h"

#include "MuseeSetting.h"

namespace Musee::Visitor
{
	namespace
	{
		/**
		 * The as-built museum's pace (MuseeCharacter: WalkSpeed 260 cm/s, BriskSpeed 520 cm/s). The owner walked the
		 * as-built galleries at it after finding 1.40 m/s slow; it is the default because it is the museum as he knows it.
		 */
		constexpr double AsBuiltWalkingMetersPerSecond = 2.6;
		constexpr double AsBuiltStridingMetersPerSecond = 5.2;

		/**
		 * The mean free walking speed of adult pedestrians, about 1.35 m/s (J. J. Fruin, Pedestrian Planning and Design,
		 * 1971), the pace at which a visitor walks a gallery.
		 */
		constexpr double PhysicalWalkingMetersPerSecond = 1.35;

		/**
		 * The fastest walk: people change from walking to running at about 2.0 m/s (A. Hreljac, "Preferred and
		 * energetically optimal gait transition speeds in human locomotion", Medicine and Science in Sports and
		 * Exercise 25, 1993: 2.06 m/s). Striding is walking at that speed.
		 */
		constexpr double PhysicalStridingMetersPerSecond = 2.0;
	}
}

FMuseeKey FVisitorPace::SettingKey()
{
	return FMuseeKey(TEXTVIEW("visitor.pace"));
}

FName FVisitorPace::AsBuilt()
{
	return TEXT("as-built");
}

FName FVisitorPace::Physical()
{
	return TEXT("physical");
}

FVisitorPace FVisitorPace::Of(FName Choice)
{
	using namespace Musee::Visitor;
	if (Choice == Physical())
	{
		return { Musee::MetersPerSecond(PhysicalWalkingMetersPerSecond), Musee::MetersPerSecond(PhysicalStridingMetersPerSecond) };
	}
	return { Musee::MetersPerSecond(AsBuiltWalkingMetersPerSecond), Musee::MetersPerSecond(AsBuiltStridingMetersPerSecond) };
}

FVisitorPace FVisitorPace::Chosen()
{
	const FMuseeSetting* Setting = FMuseeSetting::Find(SettingKey());
	return Of(Setting ? Setting->Choice() : AsBuilt());
}

void FVisitorPace::Register()
{
	FMuseeSetting::RegisterChoice(SettingKey(), AsBuilt(), { AsBuilt(), Physical() },
		TEXT("The visitor's pace: as-built, the as-built museum's brisk 2.6 m/s walk and 5.2 m/s stride, or physical, a real person's 1.35 m/s gallery walk and 2.0 m/s stride."));
}

void FVisitorPace::Unregister()
{
	FMuseeSetting::Unregister(SettingKey());
}
