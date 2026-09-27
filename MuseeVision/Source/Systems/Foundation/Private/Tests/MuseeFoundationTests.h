#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Tests
{
	/** Foundation's tests run in every application context and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** A fixture under Private/Tests/Data, as a full path. */
	inline FString Fixture(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Foundation/Private/Tests/Data") / Name);
	}
}

#endif
