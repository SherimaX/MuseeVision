#include "SalonParts.h"

#include "SalonPlan.h"

namespace Musee::Salon
{
	FMuseeKey Key(const TCHAR* Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FName Surface(const FString& Local)
	{
		return Surface(*Local);
	}

	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(Origin, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	FMuseeFrame FaceFrame(const FVector& Origin, const FVector& N)
	{
		return FrameOfAxes(Origin, FVector::CrossProduct(FVector::UpVector, N), FVector::UpVector);
	}

	FFormProfile Polygon(TConstArrayView<FVector2D> Points)
	{
		return FFormProfile::Polyline(Points, true);
	}

	FFormProfile Section(TConstArrayView<Plan::FStep> Steps, const FVector2D& Offset)
	{
		const Plan::FStep& First = Steps[0];
		FFormProfile Profile = FFormProfile::From(FVector2D(First.A, First.B) + Offset);
		for (int32 Index = 1; Index < Steps.Num(); ++Index)
		{
			const Plan::FStep& Step = Steps[Index];
			if (Step.bArc)
			{
				Profile.ArcAbout(FVector2D(Step.CenterA, Step.CenterB) + Offset, Step.RadiusA, Step.RadiusB, Step.From, Step.To);
			}
			else
			{
				Profile.LineTo(FVector2D(Step.A, Step.B) + Offset);
			}
		}
		return Profile.Close();
	}

	FFormDrawing Box(const FString& Name, const TCHAR* LocalSurface, const FVector& Min, const FVector& Max,
		const Musee::Collision::FClass* Collision)
	{
		FFormExtrusion Shape;
		Shape.Outline = FFormProfile::Rectangle(FVector2D(Min.X, Min.Y), FVector2D(Max.X, Max.Y));
		Shape.DepthMeters = Max.Z - Min.Z;
		return Drawn(Name, LocalSurface, MoveTemp(Shape), Collision, FMuseeFrame(FVector(0.0, 0.0, Min.Z)));
	}

	FFormDrawing PlanSlab(const FString& Name, const TCHAR* LocalSurface, FFormProfile Outline, TArray<FFormProfile> Holes, double Bottom,
		double Top, const Musee::Collision::FClass* Collision)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Top - Bottom;
		return Drawn(Name, LocalSurface, MoveTemp(Shape), Collision, FMuseeFrame(FVector(0.0, 0.0, Bottom)));
	}

	FFormProfile Circle(const FVector2D& Centre, double Radius)
	{
		return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
	}

	FFormProfile ArchedOutline(double Half, double Springing, double Bottom)
	{
		FFormProfile Outline = FFormProfile::From(FVector2D(-Half, Bottom));
		Outline.LineTo(FVector2D(Half, Bottom), EFormJoint::Crisp).LineTo(FVector2D(Half, Springing), EFormJoint::Crisp)
			.ArcAbout(FVector2D(0.0, Springing), Half, Half, 0.0, 180.0, EFormJoint::Smooth);
		return Outline.Close();
	}

	FFormProfile ArchedBand(double Inner, double Outer, double Springing, double Bottom)
	{
		FFormProfile Band = FFormProfile::From(FVector2D(-Outer, Bottom));
		Band.LineTo(FVector2D(-Inner, Bottom), EFormJoint::Crisp).LineTo(FVector2D(-Inner, Springing), EFormJoint::Crisp)
			.ArcAbout(FVector2D(0.0, Springing), Inner, Inner, 180.0, 0.0, EFormJoint::Smooth).LineTo(FVector2D(Inner, Bottom), EFormJoint::Smooth)
			.LineTo(FVector2D(Outer, Bottom), EFormJoint::Crisp).LineTo(FVector2D(Outer, Springing), EFormJoint::Crisp)
			.ArcAbout(FVector2D(0.0, Springing), Outer, Outer, 0.0, 180.0, EFormJoint::Smooth);
		return Band.Close();
	}

	FFormDrawing FaceSlab(const FString& Name, const TCHAR* LocalSurface, const FMuseeFrame& Face, FFormProfile Outline, TArray<FFormProfile> Holes,
		double Depth, const Musee::Collision::FClass* Collision)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Depth;
		return Drawn(Name, LocalSurface, MoveTemp(Shape), Collision, Face);
	}

	void ArchedTrim(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& FaceCentre, const FVector& N,
		double Half, double Springing, TConstArrayView<Plan::FStep> Steps)
	{
		const FVector Along = FVector::CrossProduct(FVector::UpVector, N);
		for (const double Side : {-1.0, 1.0})
		{
			// The jamb's section lies level, A away from the opening and B out of the face; it is pushed up (or down) its height.
			const FVector X = Along * Side;
			const FVector Z = FVector::CrossProduct(X, N);
			const FVector Foot = FaceCentre + X * Half + (Z.Z < 0.0 ? FVector::UpVector * Springing : FVector::ZeroVector);
			FFormExtrusion Jamb;
			Jamb.Outline = Section(Steps);
			Jamb.DepthMeters = Springing;
			Out.Add(Drawn(FString::Printf(TEXT("%s.jamb.%s"), *Name, Side < 0.0 ? TEXT("left") : TEXT("right")), LocalSurface, MoveTemp(Jamb),
				Solid, FrameOfAxes(Foot, X, N)));
		}
		FFormLathe Head;
		Head.Section = Section(Steps, FVector2D(Half, 0.0));
		Head.FromDegrees = 0.0;
		Head.ToDegrees = 180.0;
		Out.Add(Drawn(FString::Printf(TEXT("%s.head"), *Name), LocalSurface, MoveTemp(Head), Solid,
			FaceFrame(FaceCentre + FVector::UpVector * Springing, N)));
	}

	double ColumnX(int32 Bay, int32 C)
	{
		const Plan::FBay& B = Plan::Bays[Bay];
		const double Pitch = (B.East - B.West - 2.0 * Plan::Band) / Plan::Columns;
		return B.West + Plan::Band + Pitch * C;
	}

	double RowAngle(int32 K)
	{
		return UE_DOUBLE_PI * K / Plan::Rows;
	}

	FVector VaultPoint(double X, double Phi, double R)
	{
		return FVector(X, -R * FMath::Cos(Phi), Plan::Springing + R * FMath::Sin(Phi));
	}

	FBox2D Lantern(int32 Bay)
	{
		const double North = -Plan::VaultRadius * FMath::Cos(RowAngle(Plan::LanternRow));
		const double South = -Plan::VaultRadius * FMath::Cos(RowAngle(Plan::LanternRow + 1));
		return FBox2D(FVector2D(ColumnX(Bay, Plan::LanternColumn), North), FVector2D(ColumnX(Bay, Plan::LanternColumn + 2), South));
	}
}
