#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"
#include "SitePort.h"

namespace Musee::Site
{
	/**
	 * A face a joint meets, in the joint's frame (x from A towards B, y across the opening, z up): a vertical plane
	 * x = constant, or a vertical cylinder whose axis lies on the opening's centreline (y = 0), taken on its side towards
	 * +x (Sign +1) or towards -x (Sign -1). A port's outer face and the wall's inner face behind it are concentric.
	 */
	struct FJointFace
	{
		bool bFlat = true;
		/** A plane's x. */
		double X = 0.0;
		/** A cylinder's axis in plan, its radius, and the side of it the face is. */
		FVector2D Centre = FVector2D::ZeroVector;
		double Radius = 0.0;
		double Sign = 1.0;

		/** Two faces closer than a micrometre are one. */
		static constexpr double Same = 1.0e-6;

		static FJointFace Plane(double InX)
		{
			FJointFace Face;
			Face.X = InX;
			return Face;
		}

		static FJointFace Cylinder(const FVector2D& InCentre, double InRadius, double InSign)
		{
			FJointFace Face;
			Face.bFlat = false;
			Face.Centre = InCentre;
			Face.Radius = InRadius;
			Face.Sign = InSign;
			return Face;
		}

		/**
		 * A port's face, Depth behind its outer face (0 for the outer face itself, the wall's depth for its inner face, which
		 * may be flat). A's port frame is the joint's origin; B's (bFar) stands Passage along x, facing back.
		 */
		static FJointFace OfPort(const FSitePort& Port, double Depth, double Passage, bool bFar)
		{
			const double R = Port.FaceRadiusMeters;
			const double Back = bFar ? 1.0 : -1.0;
			const double Face = bFar ? Passage : 0.0;
			if (R == 0.0 || (Depth > 0.0 && Port.bFlatInnerFace))
			{
				return Plane(Face + Back * Depth);
			}
			// A drum's axis stands behind its face, a wrapping face's ahead of it; its inner face is the concentric one behind.
			const double Behind = R > 0.0 ? 1.0 : -1.0;
			return Cylinder(FVector2D(Face + Back * Behind * FMath::Abs(R), 0.0), FMath::Abs(R) - Behind * Depth, -Back * Behind);
		}

		double XAt(double Y) const
		{
			return bFlat ? X : Centre.X + Sign * FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Y * Y));
		}

		/** The face's unit normal in plan at y, turned towards +x. */
		FVector2D NormalAt(double Y) const
		{
			return bFlat ? FVector2D(1.0, 0.0) : FVector2D(FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Y * Y)), Sign * Y) / Radius;
		}

		/** The face moved by Distance along its normal towards +x: the concentric cylinder, or the parallel plane. */
		FJointFace Offset(double Distance) const
		{
			return bFlat ? Plane(X + Distance) : Cylinder(Centre, Radius + Sign * Distance, Sign);
		}

		/** Its curvature in plan, per metre: zero for a plane. */
		double Tightness() const { return bFlat ? 0.0 : 1.0 / Radius; }

		/** Whether the face reaches Across either side of the centreline. */
		bool Reaches(double Across) const { return bFlat || Radius > Across; }

		bool SameAs(const FJointFace& Other) const
		{
			if (bFlat || Other.bFlat)
			{
				return bFlat && Other.bFlat && FMath::Abs(X - Other.X) < Same;
			}
			return Centre.Equals(Other.Centre, Same) && FMath::Abs(Radius - Other.Radius) < Same && Sign == Other.Sign;
		}

		/** Whether this face stands at or before Other along x everywhere within Across of the centreline. */
		bool LiesBefore(const FJointFace& Other, double Across) const
		{
			constexpr int32 Samples = 64;
			for (int32 Sample = 0; Sample <= Samples; ++Sample)
			{
				const double Y = FMath::Lerp(-Across, Across, double(Sample) / Samples);
				if (XAt(Y) > Other.XAt(Y) + Same)
				{
					return false;
				}
			}
			return true;
		}

		/** The distance along the face in plan from where it stands at y = From to where it stands at y. */
		double AlongFrom(double From, double Y) const
		{
			return bFlat ? FMath::Abs(Y - From) : Radius * FMath::Abs(FMath::Asin(Y / Radius) - FMath::Asin(From / Radius));
		}

		/** The face's line in plan from y = From to y = To, exact on a cylinder. */
		FFormProfile Line(double From, double To) const
		{
			FFormProfile Profile = FFormProfile::From(FVector2D(XAt(From), From));
			AppendTo(Profile, From, To);
			return Profile;
		}

		/** Continues a drawing along the face from y = From to y = To (a line joins it first where it starts elsewhere). */
		void AppendTo(FFormProfile& Profile, double From, double To) const
		{
			if (bFlat)
			{
				Profile.LineTo(FVector2D(X, From)).LineTo(FVector2D(X, To));
				return;
			}
			auto Angle = [this](double Y)
			{
				const double Along = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Y / Radius, -1.0, 1.0)));
				return Sign > 0.0 ? Along : 180.0 - Along;
			};
			Profile.ArcAbout(Centre, Radius, Radius, Angle(From), Angle(To));
		}
	};

	/** The closed outline between two faces over y in [Low, High]: Near from Low to High, then Far back. */
	inline FFormProfile Between(const FJointFace& Near, const FJointFace& Far, double Low, double High)
	{
		FFormProfile Outline = Near.Line(Low, High);
		Far.AppendTo(Outline, High, Low);
		return Outline.Close();
	}
}
