#include "MotionLiftStage.h"

#if WITH_DEV_AUTOMATION_TESTS

// The ports the site plan mates reach a mechanism through its stage's context, so a ride that needs a well below is
// offered only when the stage below is built.

namespace Musee::Motion::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionMatedFromContextTest, "Musee.Motion.MatedFromContext", Flags)
	bool FMotionMatedFromContextTest::RunTest(const FString& Parameters)
	{
		for (const bool bMated : {false, true})
		{
			FTestStage Stage;
			TArray<FMuseeFinding> Findings;
			DressAtrium(Stage, Findings, bMated ? TArray<FMuseeKey>({LiftKey(TEXT("atrium.down")), LiftKey(TEXT("atrium.up"))}) : TArray<FMuseeKey>());
			UMotionMechanism* Car = Machine(Stage, TEXT("atrium.car.lift"));
			if (!TestNotNull(TEXT("The car is placed"), Car) || !TestTrue(TEXT("It has rides"), !Car->Spec().Rides.IsEmpty()))
			{
				return false;
			}
			// The first ride, down to the Cube, needs only the well below mated (Mechanisms.json).
			TestEqual(bMated ? TEXT("With both wells mated in the context, the ride down holds") : TEXT("With none mated, it doesn't"),
				Car->Holds(Car->Spec().Rides[0].Requires), bMated);
		}
		return true;
	}
}

#endif
