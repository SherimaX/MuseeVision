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

	FMuseeKey PanelsBody()
	{
		return Key(TEXT("stair.balustrade.panels"));
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
		 * The face's line in plan, run the way U runs so the face looks to its left, towards whoever sees it: seen from inside
		 * (the drum's inner face) the way φ falls, from outside the way φ rises. It spans a half turn centred on the opening's
		 * axis, so every run round an opening lies on it; U is measured from its start, a quarter turn before the axis.
		 */
		FFormProfile FaceLine(const FFaceArch& Arch)
		{
			const double Sense = Arch.bConvex ? 1.0 : -1.0;
			const double From = Arch.AxisDeg - Sense * 90.0;
			const double To = Arch.AxisDeg + Sense * 90.0;
			FFormProfile Line = FFormProfile::From(Polar(Arch.FaceRadius, From));
			Line.ArcAbout(FVector2D::ZeroVector, Arch.FaceRadius, Arch.FaceRadius, From, To, EFormJoint::Smooth);
			return Line;
		}

		/**
		 * Where a point Across the opening's axis lies on the face's development: through a straight passage the jambs run
		 * parallel to the axis, so a point across it meets the face round the arc; a square-cut opening is set out along
		 * the face itself.
		 */
		double AlongFace(const FFaceArch& Arch, double Across)
		{
			const double Along = Arch.bStraight ? Arch.FaceRadius * FMath::Asin(FMath::Clamp(Across / Arch.FaceRadius, -1.0, 1.0)) : Across;
			return 0.5 * UE_DOUBLE_PI * Arch.FaceRadius + Along;
		}

		/** One voussoir: the section swept on the face along a path in its development (U along the face, Z up). */
		void Voussoir(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FFormProfile& Section,
			FFormProfile Path, const FFaceArch& Arch, const FMuseeKey& Body)
		{
			FFormSweep Shape;
			Shape.Section = Section;
			Shape.Path = MoveTemp(Path);
			Shape.Face = FaceLine(Arch);
			Out.Add_GetRef(Drawn(Name, LocalSurface, MoveTemp(Shape), nullptr)).Body = Body;
		}
	}

	void RunRoundArch(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FFormProfile& Section,
		const FFaceArch& Arch, const FMuseeKey& Body)
	{
		// The path is drawn on the face's development, (U, z), so that the section's A (the walker's right) points away from
		// the opening: up the left jamb, over the head from left to right, down the right jamb. Each stone of the head runs
		// on the arch's stations, and lies on the face, so its back meets the face and whatever stands in the opening.
		const double Radius = Arch.ArchRadius;
		const bool bJambs = Arch.JambFoot < Arch.Springing;
		auto OnFace = [&Arch](double Across, double Z) { return FVector2D(AlongFace(Arch, Across), Z); };
		int32 Index = 0;
		if (bJambs)
		{
			Voussoir(Out, FString::Printf(TEXT("%s.%02d"), *Name, Index++), LocalSurface, Section,
				FFormProfile::Polyline({OnFace(-Radius, Arch.JambFoot), OnFace(-Radius, Arch.Springing)}, false), Arch, Body);
		}
		const int32 Count = FMath::Max(2, FMath::CeilToInt32(UE_DOUBLE_PI * Radius / Plan::VoussoirMeters));
		for (int32 Stone = 0; Stone < Count; ++Stone)
		{
			const double From = 180.0 - 180.0 * Stone / Count;
			const double To = 180.0 - 180.0 * (Stone + 1) / Count;
			const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Plan::ArchStations * (From - To) / 180.0));
			FFormProfile Path = FFormProfile::From(OnFace(Radius * FMath::Cos(FMath::DegreesToRadians(From)), Arch.Springing + Radius * FMath::Sin(FMath::DegreesToRadians(From))));
			for (int32 Step = 1; Step <= Steps; ++Step)
			{
				const double Angle = FMath::DegreesToRadians(FMath::Lerp(From, To, double(Step) / Steps));
				Path.LineTo(OnFace(Radius * FMath::Cos(Angle), Arch.Springing + Radius * FMath::Sin(Angle)), EFormJoint::Smooth);
			}
			Voussoir(Out, FString::Printf(TEXT("%s.%02d"), *Name, Index++), LocalSurface, Section, MoveTemp(Path), Arch, Body);
		}
		if (bJambs)
		{
			Voussoir(Out, FString::Printf(TEXT("%s.%02d"), *Name, Index++), LocalSurface, Section,
				FFormProfile::Polyline({OnFace(Radius, Arch.Springing), OnFace(Radius, Arch.JambFoot)}, false), Arch, Body);
		}
	}
}
