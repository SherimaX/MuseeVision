#include "RotundaParts.h"

namespace Musee::Rotunda
{
	FMuseeKey Key(const TCHAR* Local)
	{
		return FMuseeKey(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FName Surface(const TCHAR* Local)
	{
		return FName(FString::Printf(TEXT("%s.%s"), Plan::Stage, Local));
	}

	FMuseeKey DrumBody()
	{
		return Key(TEXT("sun-clock.drum"));
	}

	FFormProfile Profile(TConstArrayView<Plan::FStep> Steps, bool bClosed)
	{
		auto ArcStart = [](const Plan::FStep& Step)
		{
			const double From = FMath::DegreesToRadians(Step.FromDeg);
			return FVector2D(Step.A + Step.RadiusA * FMath::Cos(From), Step.B + Step.RadiusB * FMath::Sin(From));
		};
		const Plan::FStep& First = Steps[0];
		FFormProfile Out = FFormProfile::From(First.bArc ? ArcStart(First) : FVector2D(First.A, First.B));
		for (int32 Index = First.bArc ? 0 : 1; Index < Steps.Num(); ++Index)
		{
			const Plan::FStep& Step = Steps[Index];
			if (Step.bArc)
			{
				Out.ArcAbout(FVector2D(Step.A, Step.B), Step.RadiusA, Step.RadiusB, Step.FromDeg, Step.ToDeg);
			}
			else
			{
				Out.LineTo(FVector2D(Step.A, Step.B));
			}
		}
		if (bClosed)
		{
			Out.Close();
		}
		return Out;
	}

	FVector2D Polar(double Radius, double Degrees)
	{
		const double Angle = FMath::DegreesToRadians(Degrees);
		return FVector2D(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
	}

	FMuseeFrame Turned(double Degrees, const FVector& Origin)
	{
		return FMuseeFrame(Origin, FQuat(FVector::UpVector, FMath::DegreesToRadians(Degrees)));
	}

	FMuseeFrame Raised(double Z)
	{
		return FMuseeFrame(FVector(0.0, 0.0, Z));
	}

	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(Origin, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	FFormLathe Ring(double Inner, double Outer, double Bottom, double Top, double FromDeg, double ToDeg)
	{
		FFormLathe Shape;
		if (Inner <= 0.0)
		{
			// A disc: its section ends on the axis at both ends, which a full turn closes.
			Shape.Section = FFormProfile::Polyline({FVector2D(0.0, Bottom), FVector2D(Outer, Bottom), FVector2D(Outer, Top), FVector2D(0.0, Top)}, false);
		}
		else
		{
			Shape.Section = FFormProfile::Rectangle(FVector2D(Inner, Bottom), FVector2D(Outer, Top));
		}
		Shape.FromDegrees = FromDeg;
		Shape.ToDegrees = ToDeg;
		return Shape;
	}

	FFormExtrusion Slab(FFormProfile Outline, TArray<FFormProfile> Holes, double Depth)
	{
		FFormExtrusion Shape;
		Shape.Outline = MoveTemp(Outline);
		Shape.Holes = MoveTemp(Holes);
		Shape.DepthMeters = Depth;
		return Shape;
	}

	FFormProfile Circle(double Radius, const FVector2D& Centre)
	{
		return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
	}

	FFormProfile AnnularSector(double Inner, double Outer, double FromDeg, double ToDeg)
	{
		FFormProfile Out = FFormProfile::From(Polar(Outer, FromDeg));
		Out.ArcAbout(FVector2D::ZeroVector, Outer, Outer, FromDeg, ToDeg, EFormJoint::Crisp);
		Out.LineTo(Polar(Inner, ToDeg), EFormJoint::Crisp);
		Out.ArcAbout(FVector2D::ZeroVector, Inner, Inner, ToDeg, FromDeg, EFormJoint::Crisp);
		return Out.Close();
	}

	double HalfAngleDeg(double HalfWidth, double Radius)
	{
		return FMath::RadiansToDegrees(FMath::Asin(HalfWidth / Radius));
	}

	FVector Radial(double Degrees)
	{
		const FVector2D P = Polar(1.0, Degrees);
		return FVector(P.X, P.Y, 0.0);
	}

	FVector Tangent(double Degrees)
	{
		return Radial(Degrees + 90.0);
	}

	FVector2D OnDome(double Radius, double ElevationDeg)
	{
		const double E = FMath::DegreesToRadians(ElevationDeg);
		return FVector2D(Radius * FMath::Cos(E), Plan::DomeCentreZ + Radius * FMath::Sin(E));
	}

	double ElevationAt(double Radius, double Z)
	{
		return FMath::RadiansToDegrees(FMath::Asin((Z - Plan::DomeCentreZ) / Radius));
	}

	double CorniceDeg()
	{
		return ElevationAt(Plan::DomeRadius, Plan::Entablature[0].B);
	}

	double EyeCurbDeg()
	{
		return FMath::RadiansToDegrees(FMath::Acos(Plan::EyeCurbRadius / Plan::DomeRadius));
	}

	namespace
	{
		/**
		 * The frame of the plane tangent to a cylindrical face at a distance U along it from an opening's axis: x along
		 * the face (the way U runs), y up, z out of the face towards whoever sees it. Seen from inside (the drum's inner
		 * face), U runs the way φ falls; seen from outside, the way φ rises; either way z = x × y faces the viewer.
		 */
		FMuseeFrame TangentAt(const FFaceArch& Arch, double U)
		{
			const double Sense = Arch.bConvex ? 1.0 : -1.0;
			const double Phi = FMath::DegreesToRadians(Arch.AxisDeg) + Sense * U / Arch.FaceRadius;
			const FVector Origin(Arch.FaceRadius * FMath::Cos(Phi), Arch.FaceRadius * FMath::Sin(Phi), 0.0);
			const FVector Along = Sense * FVector(-FMath::Sin(Phi), FMath::Cos(Phi), 0.0);
			return FrameOfAxes(Origin, Along, FVector::UpVector);
		}

		/** One voussoir: the section swept along a path drawn in the tangent plane at U. */
		void Voussoir(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FFormProfile& Section,
			FFormProfile Path, const FFaceArch& Arch, double U, const FMuseeKey& Body)
		{
			FFormSweep Shape;
			Shape.Section = Section;
			Shape.Path = MoveTemp(Path);
			Out.Add_GetRef(Drawn(Name, LocalSurface, MoveTemp(Shape), nullptr, TangentAt(Arch, U))).Body = Body;
		}
	}

	void RunRoundArch(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FFormProfile& Section,
		const FFaceArch& Arch, const FMuseeKey& Body)
	{
		// The path is drawn in each tangent plane's (x, y) = (U, z), so that the section's A (the walker's right) points
		// away from the opening: up the left jamb, over the head from left to right, down the right jamb.
		const double Radius = Arch.ArchRadius;
		const bool bJambs = Arch.JambFoot < Arch.Springing;
		int32 Index = 0;
		if (bJambs)
		{
			Voussoir(Out, FString::Printf(TEXT("%s.%02d"), *Name, Index++), LocalSurface, Section,
				FFormProfile::Polyline({FVector2D(0.0, Arch.JambFoot), FVector2D(0.0, Arch.Springing)}, false), Arch, -Radius, Body);
		}
		const int32 Count = FMath::Max(2, FMath::CeilToInt32(UE_DOUBLE_PI * Radius / Plan::VoussoirMeters));
		for (int32 Stone = 0; Stone < Count; ++Stone)
		{
			const double From = 180.0 - 180.0 * Stone / Count;
			const double To = 180.0 - 180.0 * (Stone + 1) / Count;
			const double Middle = Radius * FMath::Cos(FMath::DegreesToRadians(0.5 * (From + To)));
			const FVector2D Centre(-Middle, Arch.Springing);
			FFormProfile Path = FFormProfile::From(Centre + Polar(Radius, From));
			Path.ArcAbout(Centre, Radius, Radius, From, To);
			Voussoir(Out, FString::Printf(TEXT("%s.%02d"), *Name, Index++), LocalSurface, Section, MoveTemp(Path), Arch, Middle, Body);
		}
		if (bJambs)
		{
			Voussoir(Out, FString::Printf(TEXT("%s.%02d"), *Name, Index++), LocalSurface, Section,
				FFormProfile::Polyline({FVector2D(0.0, Arch.Springing), FVector2D(0.0, Arch.JambFoot)}, false), Arch, Radius, Body);
		}
	}
}
