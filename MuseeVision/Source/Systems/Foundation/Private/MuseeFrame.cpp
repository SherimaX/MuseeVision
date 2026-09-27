#include "MuseeFrame.h"

FMuseeFrame::FMuseeFrame(const FVector& InOriginMeters, const FQuat& InRotation)
	: OriginMeters(InOriginMeters)
	, Rotation(InRotation.GetNormalized())
{
}

FMuseeFrame::FMuseeFrame(const FVector& InOriginMeters)
	: OriginMeters(InOriginMeters)
{
}

FVector FMuseeFrame::PointToParent(const FVector& PointMeters) const
{
	return OriginMeters + Rotation.RotateVector(PointMeters);
}

FVector FMuseeFrame::PointFromParent(const FVector& PointMeters) const
{
	return Rotation.UnrotateVector(PointMeters - OriginMeters);
}

FVector FMuseeFrame::DirectionToParent(const FVector& Direction) const
{
	return Rotation.RotateVector(Direction);
}

FVector FMuseeFrame::DirectionFromParent(const FVector& Direction) const
{
	return Rotation.UnrotateVector(Direction);
}

FMuseeFrame FMuseeFrame::FrameToParent(const FMuseeFrame& Child) const
{
	return FMuseeFrame(PointToParent(Child.OriginMeters), Rotation * Child.Rotation);
}

FMuseeFrame FMuseeFrame::FrameFromParent(const FMuseeFrame& Other) const
{
	return FMuseeFrame(PointFromParent(Other.OriginMeters), Rotation.Inverse() * Other.Rotation);
}

FMuseeFrame FMuseeFrame::Inverse() const
{
	const FQuat Inverted = Rotation.Inverse();
	return FMuseeFrame(-Inverted.RotateVector(OriginMeters), Inverted);
}

bool FMuseeFrame::Equals(const FMuseeFrame& Other, Musee::Meters Tolerance, Musee::Radians AngleTolerance) const
{
	return FVector::Dist(OriginMeters, Other.OriginMeters) <= Tolerance.Get()
		&& Rotation.AngularDistance(Other.Rotation) <= AngleTolerance.Get();
}

FTransform Musee::ToEngine(const FMuseeFrame& Frame)
{
	return FTransform(Frame.Rotation, ToEngine(Frame.OriginMeters), FVector::OneVector);
}

FMuseeFrame Musee::FromEngine(const FTransform& Transform)
{
	ensureMsgf(Transform.GetScale3D().Equals(FVector::OneVector), TEXT("A frame has no scale; this transform scales by %s."),
		*Transform.GetScale3D().ToString());
	return FMuseeFrame(FromEngine(Transform.GetLocation()), Transform.GetRotation());
}
