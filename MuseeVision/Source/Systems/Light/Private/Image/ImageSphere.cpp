#include "Image/ImageSphere.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

namespace Musee::Light
{
	namespace
	{
		/**
		 * The rings of latitude and the steps round each: on the Sphere's image (13.7 m, its opening's edge 167.5 degrees from
		 * its crown) chords 1.75 degrees long in latitude and 2.8 round, whose faces' planes come within 5.7 mm of the true
		 * sphere at the equator, so the points stand that far outside it.
		 */
		constexpr int32 Rings = 96;
		constexpr int32 Around = 128;

		/** The angle from the crown of the opening's edge, on a sphere of a radius: the edge stands below the centre by the half-chord there. */
		double LowestOf(double Radius, double Opening)
		{
			return FMath::Acos(-FMath::Sqrt(Radius * Radius - Opening * Opening) / Radius);
		}

		/** The direction out from the centre of a ring's step, the rings running from the crown down to the lowest angle. */
		FVector3d Outward(double Lowest, int32 Ring, int32 Step)
		{
			const double Polar = Lowest * Ring / Rings;
			const double Azimuth = 2.0 * UE_DOUBLE_PI * Step / Around;
			return FVector3d(FMath::Sin(Polar) * FMath::Cos(Azimuth), FMath::Sin(Polar) * FMath::Sin(Azimuth), FMath::Cos(Polar));
		}

		/**
		 * Each face as its three corners (ring, step): two to a quad, but only one where a quad meets the crown's ring, which
		 * is one point.
		 */
		template <typename FFace>
		void ForEachFace(FFace&& Face)
		{
			for (int32 Ring = 0; Ring < Rings; ++Ring)
			{
				for (int32 Step = 0; Step < Around; ++Step)
				{
					if (Ring > 0)
					{
						Face(FIntPoint(Ring, Step), FIntPoint(Ring, Step + 1), FIntPoint(Ring + 1, Step));
					}
					Face(FIntPoint(Ring, Step + 1), FIntPoint(Ring + 1, Step + 1), FIntPoint(Ring + 1, Step));
				}
			}
		}

		/**
		 * The least cosine, over the faces between points of the unit sphere, of the angle from a face's axis (the normal
		 * through the centre) to its corners: how near the centre a face's plane comes, and so the nearest any point of the
		 * face may.
		 */
		double NearestPlane(double Lowest)
		{
			double Nearest = 1.0;
			ForEachFace([Lowest, &Nearest](const FIntPoint& A, const FIntPoint& B, const FIntPoint& C)
			{
				const FVector3d PA = Outward(Lowest, A.X, A.Y);
				const FVector3d Across = (Outward(Lowest, B.X, B.Y) - PA).Cross(Outward(Lowest, C.X, C.Y) - PA);
				// The faces by the crown are small on the unit sphere: their axis is taken by its length, with no tolerance.
				const double TwiceArea = Across.Length();
				if (TwiceArea > 0.0)
				{
					Nearest = FMath::Min(Nearest, FMath::Abs(PA.Dot(Across / TwiceArea)));
				}
			});
			return Nearest;
		}
	}

	double NearestToCentre(const FVector3d& A, const FVector3d& B, const FVector3d& C)
	{
		// The nearest point of a triangle to a point (Ericson, Real-Time Collision Detection, 5.1.5), the point the origin.
		const FVector3d P = FVector3d::ZeroVector;
		const FVector3d AB = B - A;
		const FVector3d AC = C - A;
		const FVector3d AP = P - A;
		const double D1 = AB.Dot(AP);
		const double D2 = AC.Dot(AP);
		if (D1 <= 0.0 && D2 <= 0.0) { return A.Length(); }
		const FVector3d BP = P - B;
		const double D3 = AB.Dot(BP);
		const double D4 = AC.Dot(BP);
		if (D3 >= 0.0 && D4 <= D3) { return B.Length(); }
		const double VC = D1 * D4 - D3 * D2;
		if (VC <= 0.0 && D1 >= 0.0 && D3 <= 0.0) { return (A + AB * (D1 / (D1 - D3))).Length(); }
		const FVector3d CP = P - C;
		const double D5 = AB.Dot(CP);
		const double D6 = AC.Dot(CP);
		if (D6 >= 0.0 && D5 <= D6) { return C.Length(); }
		const double VB = D5 * D2 - D1 * D6;
		if (VB <= 0.0 && D2 >= 0.0 && D6 <= 0.0) { return (A + AC * (D2 / (D2 - D6))).Length(); }
		const double VA = D3 * D6 - D5 * D4;
		if (VA <= 0.0 && (D4 - D3) >= 0.0 && (D5 - D6) >= 0.0) { return (B + (C - B) * ((D4 - D3) / ((D4 - D3) + (D5 - D6)))).Length(); }
		const double Denominator = 1.0 / (VA + VB + VC);
		return (A + AB * (VB * Denominator) + AC * (VC * Denominator)).Length();
	}

	void BuildImageSphere(double RadiusCentimeters, double OpeningCentimeters, UE::Geometry::FDynamicMesh3& Out)
	{
		using namespace UE::Geometry;
		Out = FDynamicMesh3();
		Out.EnableAttributes();
		if (RadiusCentimeters <= 0.0 || OpeningCentimeters < 0.0 || OpeningCentimeters >= RadiusCentimeters)
		{
			return;
		}
		// The points stand out from the sphere so their faces come no nearer the centre than it, the opening's edge drawn at
		// the points' own radius. The steps in latitude, and so the faces, follow the edge, which follows the points'
		// radius: it is found by turns (the third moves the edge by under a nanometre), and the radius is taken last from the
		// faces as drawn, so none comes nearer the centre than the sphere.
		double Lowest = LowestOf(RadiusCentimeters, OpeningCentimeters);
		for (int32 Turn = 0; Turn < 3; ++Turn)
		{
			Lowest = LowestOf(RadiusCentimeters / NearestPlane(Lowest), OpeningCentimeters);
		}
		const double PointRadius = RadiusCentimeters / NearestPlane(Lowest);
		FDynamicMeshNormalOverlay* Normals = Out.Attributes()->PrimaryNormals();
		FDynamicMeshUVOverlay* UVs = Out.Attributes()->PrimaryUV();
		TArray<int32> Vertices;
		TArray<int32> NormalElements;
		TArray<int32> UVElements;
		for (int32 Ring = 0; Ring <= Rings; ++Ring)
		{
			for (int32 Step = 0; Step <= Around; ++Step)
			{
				const FVector3d Direction = Outward(Lowest, Ring, Step);
				Vertices.Add(Out.AppendVertex(Direction * PointRadius));
				NormalElements.Add(Normals->AppendElement(FVector3f(-Direction)));
				UVElements.Add(UVs->AppendElement(FVector2f(static_cast<float>(Step) / Around, static_cast<float>(Ring) / Rings)));
			}
		}
		const auto At = [](const FIntPoint& Corner) { return Corner.X * (Around + 1) + Corner.Y; };
		ForEachFace([&](const FIntPoint& CornerA, const FIntPoint& CornerB, const FIntPoint& CornerC)
		{
			int32 A = At(CornerA);
			int32 B = At(CornerB);
			int32 C = At(CornerC);
			// Every face looks at the centre: wound so its normal, (B - A) x (C - A), points inwards.
			const FVector3d PA = Out.GetVertex(Vertices[A]);
			const FVector3d Across = (Out.GetVertex(Vertices[B]) - PA).Cross(Out.GetVertex(Vertices[C]) - PA);
			if (Across.SquaredLength() <= 0.0)
			{
				return;
			}
			if (Across.Dot(PA) > 0.0)
			{
				Swap(B, C);
			}
			const int32 Face = Out.AppendTriangle(Vertices[A], Vertices[B], Vertices[C]);
			if (Face >= 0)
			{
				Normals->SetTriangle(Face, FIndex3i(NormalElements[A], NormalElements[B], NormalElements[C]));
				UVs->SetTriangle(Face, FIndex3i(UVElements[A], UVElements[B], UVElements[C]));
			}
		});
	}
}
