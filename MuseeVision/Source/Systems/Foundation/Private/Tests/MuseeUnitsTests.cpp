#include "MuseeFoundationTests.h"
#include "MuseeUnits.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Tests
{
	template <typename T>
	concept CScalable = requires(T Quantity) { Quantity * 2.0; };

	template <typename T>
	concept CNegatable = requires(T Quantity) { -Quantity; };

	template <typename A, typename B>
	concept CAddable = requires(A Left, B Right) { Left + Right; };

	// Only the arithmetic the physics allows compiles.
	static_assert(CScalable<Meters> && CScalable<Lux> && CScalable<Kelvin>, "Ratio-scale quantities scale by pure numbers.");
	static_assert(!CScalable<EV100> && !CScalable<Decibels>, "A logarithm is shifted, never scaled.");
	static_assert(CNegatable<Meters> && CNegatable<Decibels> && CNegatable<EV100>, "Displacements, gains and stops have signs.");
	static_assert(!CNegatable<Lux> && !CNegatable<Kilograms> && !CNegatable<Kelvin>, "Illuminance, mass and absolute temperature have none.");
	static_assert(!CAddable<Meters, Lux> && !CAddable<Meters, Seconds>, "Different kinds never add.");
	static_assert(!CAddable<Meters, double>, "A bare number is not a length.");
	static_assert(!std::is_convertible_v<double, Meters>, "A quantity is made explicitly, with its unit.");
	static_assert(Meters(2.0) + Meters(3.0) == Meters(5.0), "Lengths add at compile time.");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationUnitsTest, "Musee.Foundation.Units", Musee::Tests::Flags)

bool FMuseeFoundationUnitsTest::RunTest(const FString& Parameters)
{
	using namespace Musee;
	constexpr double Tolerance = 1e-12;

	TestEqual(TEXT("Lengths add"), (Meters(1.2) + Meters(0.4)).Get(), 1.6, Tolerance);
	TestEqual(TEXT("Lengths subtract"), (Meters(1.2) - Meters(0.4)).Get(), 0.8, Tolerance);
	TestEqual(TEXT("A length scales"), (Meters(0.63) * 2.0).Get(), 1.26, Tolerance);
	TestEqual(TEXT("A pure number scales a length"), (2.0 * Meters(0.63)).Get(), 1.26, Tolerance);
	TestEqual(TEXT("Two illuminances make a ratio"), Lux(300.0) / Lux(150.0), 2.0, Tolerance);
	TestTrue(TEXT("Quantities compare"), Lux(150.0) < Lux(200.0) && Kelvin(3000.0) > Kelvin(2700.0));
	TestEqual(TEXT("A stop is added to an exposure"), (EV100(12.0) + EV100(1.0)).Get(), 13.0, Tolerance);
	TestEqual(TEXT("A gain is negated"), (-Decibels(6.0)).Get(), -6.0, Tolerance);

	Meters Walked(1.0);
	Walked += Meters(0.63);
	Walked *= 2.0;
	TestEqual(TEXT("Compound assignment"), Walked.Get(), 3.26, Tolerance);

	TestEqual(TEXT("A half turn in radians"), ToRadians(Degrees(180.0)).Get(), UE_DOUBLE_PI, Tolerance);
	TestEqual(TEXT("A right angle in degrees"), ToDegrees(Radians(UE_DOUBLE_HALF_PI)).Get(), 90.0, Tolerance);

	const MetersPerSecond Pace = Meters(0.63) / Seconds(0.5);
	TestEqual(TEXT("Speed is distance over time"), Pace.Get(), 1.26, Tolerance);
	TestEqual(TEXT("Distance is speed times time"), (Pace * Seconds(2.0)).Get(), 2.52, Tolerance);
	TestEqual(TEXT("Distance is time times speed"), (Seconds(2.0) * Pace).Get(), 2.52, Tolerance);
	TestEqual(TEXT("Time is distance over speed"), (Meters(2.52) / Pace).Get(), 2.0, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationEngineUnitsTest, "Musee.Foundation.EngineUnits", Musee::Tests::Flags)

bool FMuseeFoundationEngineUnitsTest::RunTest(const FString& Parameters)
{
	using namespace Musee;
	constexpr double Tolerance = 1e-9;

	TestEqual(TEXT("A metre is 100 engine units"), ToEngine(Meters(1.0)), 100.0, Tolerance);
	TestEqual(TEXT("The eye's height in engine units"), ToEngine(Meters(1.6)), 160.0, Tolerance);
	TestEqual(TEXT("A walking speed in engine units per second"), ToEngine(MetersPerSecond(1.4)), 140.0, Tolerance);
	TestEqual(TEXT("An engine length in metres"), FromEngine(160.0).Get(), 1.6, Tolerance);
	TestEqual(TEXT("An engine speed in metres per second"), FromEngine<MetersPerSecond>(140.0).Get(), 1.4, Tolerance);
	TestTrue(TEXT("A point keeps its axes and changes its unit"), ToEngine(FVector(1.0, -2.0, 3.0)).Equals(FVector(100.0, -200.0, 300.0)));
	TestTrue(TEXT("And back"), FromEngine(FVector(100.0, -200.0, 300.0)).Equals(FVector(1.0, -2.0, 3.0)));
	return true;
}

#endif
