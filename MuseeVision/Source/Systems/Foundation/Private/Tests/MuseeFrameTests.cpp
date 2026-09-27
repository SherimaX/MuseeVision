#include "MuseeConfiguration.h"
#include "MuseeFoundationTests.h"
#include "MuseeFrame.h"
#include "MuseeHumanMeasure.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Tests
{
	const Meters FrameTolerance(1e-9);
	const Radians AngleTolerance(1e-9);

	/** A turn about the up axis by a compass angle: clockwise seen from above, since y points south. */
	FQuat Bearing(Degrees Turn)
	{
		return FQuat(FMuseeFrame::Up(), ToRadians(Turn).Get());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationFrameTest, "Musee.Foundation.Frame", Musee::Tests::Flags)

bool FMuseeFoundationFrameTest::RunTest(const FString& Parameters)
{
	using namespace Musee;
	using namespace Musee::Tests;

	TestTrue(TEXT("North is -y"), FMuseeFrame::North().Equals(FVector(0.0, -1.0, 0.0)));
	TestTrue(TEXT("East crossed with south is up, as in the engine's axes"), FVector::CrossProduct(FMuseeFrame::East(), FMuseeFrame::South()).Equals(FMuseeFrame::Up()));

	const FMuseeFrame Stage(FVector(10.0, 0.0, 0.0), Bearing(Degrees(90.0)));
	TestTrue(TEXT("A quarter turn clockwise brings east to south"), Stage.DirectionToParent(FMuseeFrame::East()).Equals(FMuseeFrame::South()));
	TestTrue(TEXT("A point is turned and moved"), Stage.PointToParent(FVector(1.0, 0.0, 0.0)).Equals(FVector(10.0, 1.0, 0.0)));
	TestTrue(TEXT("And brought back"), Stage.PointFromParent(FVector(10.0, 1.0, 0.0)).Equals(FVector(1.0, 0.0, 0.0)));
	TestTrue(TEXT("A direction is never moved"), Stage.DirectionFromParent(FMuseeFrame::South()).Equals(FMuseeFrame::East()));

	const FMuseeFrame Body(FVector(0.0, 2.0, 1.2), Bearing(Degrees(30.0)));
	const FMuseeFrame BodyInMuseum = Stage.FrameToParent(Body);
	const FVector Corner(0.5, 0.25, 0.0);
	TestTrue(TEXT("Composition places a point as the two frames do in turn"),
		BodyInMuseum.PointToParent(Corner).Equals(Stage.PointToParent(Body.PointToParent(Corner)), FrameTolerance.Get()));
	TestTrue(TEXT("A frame taken back out of its parent is itself"), Stage.FrameFromParent(BodyInMuseum).Equals(Body, FrameTolerance, AngleTolerance));
	TestTrue(TEXT("A frame composed with its inverse is the parent's own"), Stage.FrameToParent(Stage.Inverse()).Equals(FMuseeFrame(), FrameTolerance, AngleTolerance));

	const FTransform Engine = ToEngine(Stage);
	TestTrue(TEXT("The engine sees the origin in its units"), Engine.GetLocation().Equals(FVector(1000.0, 0.0, 0.0)));
	TestTrue(TEXT("The engine sees the same rotation"), Engine.GetRotation().Equals(Stage.Rotation));
	TestTrue(TEXT("And the frame comes back whole"), FromEngine(Engine).Equals(Stage, FrameTolerance, AngleTolerance));
	TestTrue(TEXT("A rotation is kept unit length"), FMath::IsNearlyEqual(FMuseeFrame(FVector::ZeroVector, FQuat(0.0, 0.0, 2.0, 2.0)).Rotation.Size(), 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationConfigurationTest, "Musee.Foundation.Configuration", Musee::Tests::Flags)

bool FMuseeFoundationConfigurationTest::RunTest(const FString& Parameters)
{
	const FMuseeKey Cylinder(TEXTVIEW("rotunda.cylinder"));
	const FMuseeKey Door(TEXTVIEW("rotunda.west.door"));
	FMuseeConfiguration Configuration;
	Configuration.Name = FMuseeKey(TEXTVIEW("rotunda.cylinder-raised"));
	TestNull(TEXT("A body without a pose stands in its drawn place"), Configuration.Find(Cylinder));

	FMuseePose Raised;
	Raised.Key = Cylinder;
	Raised.Frame = FMuseeFrame(FVector(0.0, 0.0, 1.2));
	Configuration.Set(Raised);
	FMuseePose Open;
	Open.Key = Door;
	Configuration.Set(Open);
	Raised.Frame = FMuseeFrame(FVector(0.0, 0.0, 1.5));
	Configuration.Set(Raised);

	TestEqual(TEXT("Each body is placed once"), Configuration.Poses.Num(), 2);
	const FMuseePose* Found = Configuration.Find(Cylinder);
	TestTrue(TEXT("Setting a body again replaces its pose"), Found && FMath::IsNearlyEqual(Found->Frame.OriginMeters.Z, 1.5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationHumanMeasureTest, "Musee.Foundation.HumanMeasure", Musee::Tests::Flags)

bool FMuseeFoundationHumanMeasureTest::RunTest(const FString& Parameters)
{
	constexpr double Tolerance = 1e-12;
	TestEqual(TEXT("The standard eye"), FMuseeHumanMeasure::EyeHeight.Get(), 1.60, Tolerance);
	TestEqual(TEXT("The body is as tall as its eye and crown"), FMuseeHumanMeasure::Stature.Get(), 1.71, Tolerance);
	TestEqual(TEXT("The capsule is as tall as the body"), FMuseeHumanMeasure::CapsuleHalfHeight.Get() * 2.0, FMuseeHumanMeasure::Stature.Get(), Tolerance);
	TestEqual(TEXT("One pace"), FMuseeHumanMeasure::Pace.Get(), 0.63, Tolerance);
	TestEqual(TEXT("The capsule's radius reaches the engine in its units"), Musee::ToEngine(FMuseeHumanMeasure::CapsuleRadius), 30.5, 1e-9);
	TestTrue(TEXT("The eye stands below the least headroom"), FMuseeHumanMeasure::EyeHeight < FMuseeHumanMeasure::MinHeadroom);
	return true;
}

#endif
