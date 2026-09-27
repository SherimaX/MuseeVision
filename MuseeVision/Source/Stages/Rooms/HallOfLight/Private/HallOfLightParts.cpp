#include "HallOfLightParts.h"

#include "HallOfLightPlan.h"

namespace Musee::HallOfLight
{
	FMuseeKey Key(const FString& Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, *Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FMuseeFrame FrameOfAxes(const FVector& OriginMeters, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(OriginMeters, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	FFormProfile Polygon(TConstArrayView<FVector2D> Points)
	{
		return FFormProfile::Polyline(Points, true);
	}

	const TCHAR* SideName(int32 Side)
	{
		return Side < 0 ? TEXT("north") : TEXT("south");
	}

	double WallY(int32 Side, double D)
	{
		return Side * (Plan::GlassLine - D);
	}

	double PostX(int32 Post)
	{
		return Plan::FirstPostX + Plan::PostStep * Post;
	}

	double ArchX(int32 Arch)
	{
		return Plan::FirstArchX + Plan::ArchStep * Arch;
	}

	double BayX(int32 Bay)
	{
		return Bay > Plan::PrintBays + 1 ? Plan::LastBayX : Plan::FirstBayX + Plan::BayStep * (Bay - 1);
	}

	FVector StereoStone(int32 Index)
	{
		const double Y = (Index % 2 == 0 ? -1.0 : 1.0) * Plan::StereoOffAxis;
		return FVector(Plan::FirstStereoX + Plan::StereoStep * Index, Y, 0.0);
	}

	FMuseeFrame StereoReader(int32 Index)
	{
		const FVector Stone = StereoStone(Index);
		// A plate north of the axis faces south, and its viewer's right is east.
		const double Facing = Stone.Y < 0.0 ? 1.0 : -1.0;
		const double Tilt = Plan::ReaderTiltRadians;
		const FVector Normal(0.0, Facing * FMath::Cos(Tilt), FMath::Sin(Tilt));
		return FrameOfAxes(FVector(Stone.X, Stone.Y, Plan::ReaderCentreZ), FVector(Facing, 0.0, 0.0), Normal);
	}

	double EavesAngle()
	{
		return FMath::Acos((Plan::EavesTop - Plan::VaultAxisZ) / Plan::VaultRadius);
	}

	TArray<double> VaultLines()
	{
		const double Coping = FMath::Acos((Plan::CopingLineZ - Plan::VaultAxisZ) / Plan::VaultRadius);
		TArray<double> Lines = {-EavesAngle()};
		for (int32 Third = -Plan::PurlinThirds; Third <= Plan::PurlinThirds; ++Third)
		{
			Lines.Add(Coping * Third / Plan::PurlinThirds);
		}
		Lines.Add(EavesAngle());
		return Lines;
	}

	FVector VaultPoint(double X, double Theta, double Radius)
	{
		return FVector(X, Radius * FMath::Sin(Theta), Plan::VaultAxisZ + Radius * FMath::Cos(Theta));
	}

	FMuseeFrame VaultAxis()
	{
		return FrameOfAxes(FVector(0.0, 0.0, Plan::VaultAxisZ), FVector::UpVector, FVector(0.0, -1.0, 0.0));
	}
}
