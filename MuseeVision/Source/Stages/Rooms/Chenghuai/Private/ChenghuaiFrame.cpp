#include "ChenghuaiFrame.h"

namespace Musee::Chenghuai
{
	FBuildingFrame FBuildingFrame::Of(const FVector2D& InOrigin, const FVector2D& InU, const FVector2D& InV)
	{
		FBuildingFrame Frame;
		Frame.Origin = InOrigin;
		Frame.U = InU.GetSafeNormal();
		Frame.V = InV.GetSafeNormal();
		return Frame;
	}

	FBuildingFrame FBuildingFrame::Within(double AtU, double AtV, const FVector2D& UInHere, const FVector2D& VInHere) const
	{
		return Of(Plan(AtU, AtV), U * UInHere.X + V * UInHere.Y, U * VInHere.X + V * VInHere.Y);
	}

	double FBuildingFrame::Hand() const
	{
		return FVector2D::CrossProduct(U, V) >= 0.0 ? 1.0 : -1.0;
	}

	FVector FBuildingFrame::At(double AtU, double AtV, double Z) const
	{
		const FVector2D Point = Plan(AtU, AtV);
		return FVector(Point.X, Point.Y, Z);
	}

	FMuseeFrame FBuildingFrame::AlongU(double UStart) const
	{
		return FrameOfAxes(At(UStart, 0.0, 0.0), V3(), FVector::UpVector);
	}

	FMuseeFrame FBuildingFrame::AlongV(double VStart) const
	{
		return FrameOfAxes(At(0.0, VStart, 0.0), U3(), FVector::UpVector);
	}

	FMuseeFrame FBuildingFrame::Flat(double Z) const
	{
		return FrameOfAxes(At(0.0, 0.0, Z), U3(), V3() * Hand());
	}

	FMuseeFrame FrameOfAxes(const FVector& OriginMeters, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(OriginMeters, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}
}
