#include "MatterMomentCollection.h"

#include "Containers/StringConv.h"
#include "Hash/Blake3.h"
#include "Materials/MaterialParameterCollection.h"

namespace Musee::Matter
{
namespace
{
	/** The builder's version: part of the collection's identity, raised whenever it would make a different collection. */
	constexpr int32 MomentBuilderVersion = 1;

	/** Every value starts here until Time sets it. */
	constexpr float Unset = 0.0f;

	FGuid GuidOf(FStringView Role)
	{
		const FString Text = FString::Printf(TEXT("musee.matter.moment builder=%d %.*s"), MomentBuilderVersion, Role.Len(), Role.GetData());
		const FTCHARToUTF8 Utf8(*Text, Text.Len());
		return FGuid::NewGuidFromHash(FBlake3::HashBuffer(Utf8.Get(), Utf8.Length()));
	}
}

TArray<FName> MomentParameterNames()
{
	using namespace MomentParameter;
	return {
		SunElevationDeg, SunAzimuthDeg, Daylight, RainRateMmH, Wetness, SnowCover, TemperatureC, WindSpeedMS, SolarTerm,
		GivernyWetness, LondonWetness,
	};
}

FGuid MomentStateId(const TArray<FName>& Names)
{
	FString Joined;
	for (const FName Name : Names)
	{
		Joined += Name.ToString() + TEXT(";");
	}
	return GuidOf(TEXT("StateId ") + Joined);
}

void BuildMomentCollection(UMaterialParameterCollection& Collection)
{
	const TArray<FName> Names = MomentParameterNames();
	// The change is announced so the engine lays out the collection's uniform buffer for its parameters; the StateId the
	// engine then draws at random is replaced by the names' own.
	Collection.PreEditChange(nullptr);
	Collection.ScalarParameters.Reset();
	Collection.VectorParameters.Reset();
	for (const FName Name : Names)
	{
		FCollectionScalarParameter& Parameter = Collection.ScalarParameters.AddDefaulted_GetRef();
		Parameter.ParameterName = Name;
		Parameter.DefaultValue = Unset;
		Parameter.Id = GuidOf(TEXT("Parameter ") + Name.ToString());
	}
	Collection.PostEditChange();
	Collection.StateId = MomentStateId(Names);
}
}
