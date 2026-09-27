#include "CubeParts.h"

namespace Musee::Cube
{
	FMuseeKey Key(const FString& Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, *Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	double PlanAngle(double Bearing)
	{
		return Bearing - 90.0;
	}

	FVector2D Polar(double Radius, double Bearing)
	{
		return Radius * Outward(Bearing);
	}

	FVector2D Outward(double Bearing)
	{
		const double B = FMath::DegreesToRadians(Bearing);
		return FVector2D(FMath::Sin(B), -FMath::Cos(B));
	}

	FVector2D Along(double Bearing)
	{
		const double B = FMath::DegreesToRadians(Bearing);
		return FVector2D(FMath::Cos(B), FMath::Sin(B));
	}

	FMuseeFrame FrameOfAxes(const FVector& OriginMeters, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(OriginMeters, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	double HalfAngle(double HalfWidth, double Radius)
	{
		return FMath::RadiansToDegrees(FMath::Asin(HalfWidth / Radius));
	}

	FFormLathe Band(double R0, double R1, double Z0, double Z1, double FromBearing, double ToBearing)
	{
		FFormLathe Shape;
		Shape.Section = FFormProfile::Rectangle(FVector2D(R0, Z0), FVector2D(R1, Z1));
		Shape.FromDegrees = PlanAngle(FromBearing);
		Shape.ToDegrees = PlanAngle(ToBearing <= FromBearing ? ToBearing + 360.0 : ToBearing);
		return Shape;
	}

	FFormLathe Ring(double R0, double R1, double Z0, double Z1)
	{
		FFormLathe Shape;
		Shape.Section = FFormProfile::Rectangle(FVector2D(R0, Z0), FVector2D(R1, Z1));
		return Shape;
	}

	FFormExtrusion Prism(FFormProfile Outline, double Bottom, double Top, TArray<FFormProfile> Holes)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Top - Bottom;
		return Shape;
	}

	FFormProfile AtBearing(double Bearing, double U0, double U1, double T0, double T1)
	{
		auto At = [Bearing](double U, double T) { return U * Outward(Bearing) + T * Along(Bearing); };
		return FFormProfile::Polyline({At(U0, T0), At(U1, T0), At(U1, T1), At(U0, T1)}, true);
	}

	FFormDrawing& AsGlass(FFormDrawing& Drawing)
	{
		Drawing.bNanite = false;
		return Drawing;
	}

	FMuseeKey BladeBody(int32 Blade)
	{
		return Key(FString::Printf(TEXT("up.iris.blade.%d"), Blade + 1));
	}

	FMuseeFrame BladePin(int32 Blade)
	{
		// Pin k on plan angle 360k/7 (pin 0 due east), blade k lying 4k mm under the top blade's face.
		const double Angle = FMath::DegreesToRadians(360.0 * Blade / Plan::IrisBlades);
		return FMuseeFrame(FVector(Plan::PinRadius * FMath::Cos(Angle), Plan::PinRadius * FMath::Sin(Angle), Plan::TopBlade - Plan::BladePitch * Blade));
	}
}
