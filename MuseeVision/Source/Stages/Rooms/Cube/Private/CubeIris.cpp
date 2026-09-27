#include "CubeParts.h"

/**
 * The ceiling iris (§3.7), the closure of `cube.up` under class iris.4600: seven blades of 2 mm pearl steel, oil gilt,
 * stacked at a 4 mm pitch from the top blade's face 7.442 under the mating plane. Open, each blade is a 110 degree arc of
 * the middle circle r 3.095 about the axis, 1.250 wide with round ends, its inner edge at r 2.470 over the seat; its pin
 * stands at r 3.745 beyond its near end. Shut, each is turned 52 degrees on its pin towards increasing plan angle, and the
 * seven close the bore in a spiral. Each blade rides its own body, whose frame stands at the pin on the blade's top face
 * with the stage's axes, so Motion turns it about -z through the pin as it follows `cube.up.closure`.
 *
 * A blade's gilt top and gilt underside are one extrusion's faces, which Form can't yet finish apart: the blade takes the
 * underside's surface, the face the room sees (both are the same gilt). Its hairlines, lug and pin bearing wait for
 * polish.
 */
namespace Musee::Cube
{
	namespace
	{
		/**
		 * One blade in its open place, in a frame at its pin whose x points out from the axis: the axis lies PinRadius
		 * behind the pin, and the blade runs round the middle circle from the pin's radial line towards +y.
		 */
		FFormProfile OpenBlade()
		{
			using namespace Plan;
			const FVector2D Axis(-PinRadius, 0.0);
			const double Half = 0.5 * BladeWidth;
			const double Inner = BladeMiddle - Half;
			const double Outer = BladeMiddle + Half;
			const double Far = FMath::DegreesToRadians(BladeArcDeg);
			const FVector2D NearEnd = Axis + FVector2D(BladeMiddle, 0.0);
			const FVector2D FarEnd = Axis + BladeMiddle * FVector2D(FMath::Cos(Far), FMath::Sin(Far));
			// The inner edge out to the far end, round its cap ahead of it, the outer edge back, and round the near cap behind.
			return FFormProfile::From(Axis + FVector2D(Inner, 0.0))
				.ArcAbout(Axis, Inner, Inner, 0.0, BladeArcDeg)
				.ArcAbout(FarEnd, Half, Half, BladeArcDeg + 180.0, BladeArcDeg)
				.ArcAbout(Axis, Outer, Outer, BladeArcDeg, 0.0)
				.ArcAbout(NearEnd, Half, Half, 0.0, -180.0)
				.Close();
		}
	}

	void DrawIris(TArray<FFormDrawing>& Out)
	{
		using namespace Plan;
		const FFormProfile Blade = OpenBlade();
		for (int32 Index = 0; Index < IrisBlades; ++Index)
		{
			// In the body's frame (the stage's axes at the pin): turned to its pin's plan angle and on by the swing, shut,
			// and lying from the body's origin down through the blade's thickness.
			const double Turn = FMath::DegreesToRadians(360.0 * Index / IrisBlades + SwingDeg);
			const FMuseeFrame Shut(FVector(0.0, 0.0, -BladeThickness), FQuat(FVector::UpVector, Turn));
			FFormDrawing& Drawing = Out.Add_GetRef(Drawn(FString::Printf(TEXT("%s%d"), Pieces::Blade, Index + 1), TEXT("up.iris.under"),
				Prism(Blade, 0.0, BladeThickness), Solid, Shut));
			Drawing.Body = BladeBody(Index);
		}
	}
}
