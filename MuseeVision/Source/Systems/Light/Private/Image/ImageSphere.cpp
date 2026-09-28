#include "Image/ImageSphere.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

namespace Musee::Light
{
	namespace
	{
		/**
		 * The rings of latitude and the steps round each: on the Sphere's image (13.7 m, its opening's edge 167.5 degrees from
		 * its crown) chords 1.75 degrees long in latitude and 2.8 round, which stand at most 4 mm inside the true sphere.
		 */
		constexpr int32 Rings = 96;
		constexpr int32 Around = 128;
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
		// The opening's edge stands below the centre by the sphere's half-chord there.
		const double EdgeBelow = FMath::Sqrt(RadiusCentimeters * RadiusCentimeters - OpeningCentimeters * OpeningCentimeters);
		const double Lowest = FMath::Acos(-EdgeBelow / RadiusCentimeters);
		FDynamicMeshNormalOverlay* Normals = Out.Attributes()->PrimaryNormals();
		FDynamicMeshUVOverlay* UVs = Out.Attributes()->PrimaryUV();
		TArray<int32> Vertices;
		TArray<int32> NormalElements;
		TArray<int32> UVElements;
		for (int32 Ring = 0; Ring <= Rings; ++Ring)
		{
			const double Polar = Lowest * Ring / Rings;
			for (int32 Step = 0; Step <= Around; ++Step)
			{
				const double Azimuth = 2.0 * UE_DOUBLE_PI * Step / Around;
				const FVector3d Outward(FMath::Sin(Polar) * FMath::Cos(Azimuth), FMath::Sin(Polar) * FMath::Sin(Azimuth), FMath::Cos(Polar));
				Vertices.Add(Out.AppendVertex(Outward * RadiusCentimeters));
				NormalElements.Add(Normals->AppendElement(FVector3f(-Outward)));
				UVElements.Add(UVs->AppendElement(FVector2f(static_cast<float>(Step) / Around, static_cast<float>(Ring) / Rings)));
			}
		}
		const auto At = [](int32 Ring, int32 Step) { return Ring * (Around + 1) + Step; };
		const auto Triangle = [&](int32 A, int32 B, int32 C)
		{
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
		};
		for (int32 Ring = 0; Ring < Rings; ++Ring)
		{
			for (int32 Step = 0; Step < Around; ++Step)
			{
				// The crown's ring is one point: only one triangle of its quads carries a surface.
				if (Ring > 0)
				{
					Triangle(At(Ring, Step), At(Ring, Step + 1), At(Ring + 1, Step));
				}
				Triangle(At(Ring, Step + 1), At(Ring + 1, Step + 1), At(Ring + 1, Step));
			}
		}
	}
}
