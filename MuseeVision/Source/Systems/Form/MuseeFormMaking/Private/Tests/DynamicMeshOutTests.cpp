#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMeshOverlay.h"
#include "Kernel/DynamicMeshOut.h"
#include "Kernel/Extrude.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/PlanRuns.h"
#include "Kernel/Profile.h"
#include "Kernel/ShellEdges.h"
#include "Kernel/Sweep.h"
#include "Kernel/Tangents.h"
#include "Kernel/Welding.h"
#include "Stations/Stations.h"
#include "VectorUtil.h"
#include "Tests/FormTestShapes.h"

namespace Musee::Form::Tests
{
	namespace
	{
		/** Engine units in one metre, as Foundation's Musee::ToEngine will give them. */
		constexpr double EngineUnitsPerMeter = 100.0;

		/** Two closed pieces in two slots: a capped moulding round a corner, and a bronze plinth block. */
		FMeshBuilder TwoPieces()
		{
			FMeshBuilder Mesh;
			const FProfileStations Section = CoveBlock().Resolve(0.001);
			const TArray<FVector2D> Walls = {FVector2D(0, 0), FVector2D(4, 0), FVector2D(4, 3)};
			const TArray<TArray<FSweepFrame>> Runs = PlanRuns(Walls, false, 2.5);
			for (const TArray<FSweepFrame>& Run : Runs)
			{
				Sweep(Mesh, Run, Section);
			}
			Cap(Mesh, Runs[0][0], Section, false);
			Cap(Mesh, Runs.Last().Last(), Section, true);
			Mesh.UseSlot(TEXT("bronze"));
			const FProfileStations Block = FProfile(FVector2D(6, 0)).LineTo(FVector2D(7, 0)).LineTo(FVector2D(7, 1))
				.LineTo(FVector2D(6, 1)).Close().Resolve(0.001);
			Extrude(Mesh, FPlaneFrame(), Block, {}, 0.9, true);
			Mesh.SetColor(0, FLinearColor(0.25f, 0.5f, 0.75f, 1.0f));
			return Mesh;
		}

		/** A 2 m by 1 m floor facing up, with U along x (or against it, mirrored) and V along y. */
		FMeshBuilder Floor(bool bMirrored)
		{
			FMeshBuilder Mesh;
			const double Across = bMirrored ? -1.0 : 1.0;
			const FVector Points[4] = {FVector(0, 0, 0), FVector(2, 0, 0), FVector(2, 1, 0), FVector(0, 1, 0)};
			for (const FVector& P : Points)
			{
				Mesh.AddVertex(P, FVector::UpVector, FVector2D(Across * P.X, P.Y));
			}
			Mesh.AddQuad(0, 1, 2, 3);
			return Mesh;
		}

		/** The unit directions of increasing U and V over a written triangle, from its positions and UV layer 0. */
		bool WrittenGradient(const UE::Geometry::FDynamicMesh3& Out, int32 Triangle, FVector& OutAlongU, FVector& OutAlongV)
		{
			const UE::Geometry::FIndex3i T = Out.GetTriangle(Triangle);
			FVector2f UV[3];
			Out.Attributes()->GetUVLayer(0)->GetTriElements(Triangle, UV[0], UV[1], UV[2]);
			const FVector E1 = Out.GetVertex(T.B) - Out.GetVertex(T.A);
			const FVector E2 = Out.GetVertex(T.C) - Out.GetVertex(T.A);
			const FVector2D D1 = FVector2D(UV[1] - UV[0]);
			const FVector2D D2 = FVector2D(UV[2] - UV[0]);
			const double Det = D1.X * D2.Y - D2.X * D1.Y;
			if (FMath::Abs(Det) < 1e-12)
			{
				return false;
			}
			OutAlongU = ((E1 * D2.Y - E2 * D1.Y) / Det).GetSafeNormal();
			OutAlongV = ((E2 * D1.X - E1 * D2.X) / Det).GetSafeNormal();
			return true;
		}

		/**
		 * Corners whose written tangent or bitangent leans from its triangle's UV gradient (made perpendicular to the
		 * corner's normal) by Limit or more, or whose handedness disagrees with the gradient's.
		 */
		int32 CountAstray(const UE::Geometry::FDynamicMesh3& Out, double LimitRadians)
		{
			const UE::Geometry::FDynamicMeshAttributeSet& Attributes = *Out.Attributes();
			int32 Astray = 0;
			for (const int32 Triangle : Out.TriangleIndicesItr())
			{
				FVector AlongU;
				FVector AlongV;
				if (!WrittenGradient(Out, Triangle, AlongU, AlongV))
				{
					continue;
				}
				FVector3f N[3];
				FVector3f T[3];
				FVector3f B[3];
				Attributes.PrimaryNormals()->GetTriElements(Triangle, N[0], N[1], N[2]);
				Attributes.PrimaryTangents()->GetTriElements(Triangle, T[0], T[1], T[2]);
				Attributes.PrimaryBiTangents()->GetTriElements(Triangle, B[0], B[1], B[2]);
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					const FVector Normal(N[Corner]);
					const FVector U = (AlongU - Normal * FVector::DotProduct(Normal, AlongU)).GetSafeNormal();
					const FVector V = (AlongV - Normal * FVector::DotProduct(Normal, AlongV)).GetSafeNormal();
					const FVector Tangent(T[Corner]);
					const FVector Bitangent(B[Corner]);
					const double Sign = UE::Geometry::VectorUtil::BitangentSign(Normal, Tangent, Bitangent);
					const double GradientSign = UE::Geometry::VectorUtil::BitangentSign(Normal, U, V);
					const bool bTangent = AngleBetween(Tangent, U) < LimitRadians;
					const bool bBitangent = AngleBetween(Bitangent, V) < LimitRadians;
					Astray += (bTangent && bBitangent && Sign == GradientSign) ? 0 : 1;
				}
			}
			return Astray;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormDynamicMeshOutTest, "Musee.Form.DynamicMeshOut",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormDynamicMeshOutTest::RunTest(const FString& Parameters)
	{
		const FMeshBuilder Mesh = TwoPieces();
		UE::Geometry::FDynamicMesh3 Out;
		const FDynamicMeshWriteReport Report = WriteDynamicMesh(Mesh, EngineUnitsPerMeter, Out);
		TestEqual(TEXT("Collapsed triangles"), Report.Collapsed, 0);
		TestEqual(TEXT("Separated triangles"), Report.Separated, 0);
		TestEqual(TEXT("Every triangle is written"), Out.TriangleCount(), Mesh.NumTriangles());
		TestEqual(TEXT("Vertices are the welded points"), Out.VertexCount(), WeldVertices(Mesh).Points.Num());

		// Written, the two pieces are still two closed shells.
		const FShellEdges Shell = MeasureShellEdges(Out);
		TestTrue(TEXT("The written mesh is closed"), Shell.IsClosed());
		TestEqual(TEXT("Two closed blocks: Euler characteristic 4"), Shell.EulerCharacteristic(), 4);

		// Corner by corner, the written mesh carries the builder's positions (in engine units), winding, normals,
		// UV-derived tangent frames, UVs, colours and slots.
		const UE::Geometry::FDynamicMeshAttributeSet& Attributes = *Out.Attributes();
		TestEqual(TEXT("UV layers"), Attributes.NumUVLayers(), Mesh.NumUVChannels());
		TestTrue(TEXT("At least the metre UVs and Matter's piece layout"), Attributes.NumUVLayers() >= 2);
		const UE::Geometry::FDynamicMeshUVOverlay* PieceLayout = Attributes.GetUVLayer(FMeshBuilder::PieceLayoutChannel);
		int32 Laid = 0;
		for (const int32 Element : PieceLayout->ElementIndicesItr())
		{
			Laid += PieceLayout->GetElement(Element).IsZero() ? 0 : 1;
		}
		TestEqual(TEXT("Piece layout UVs the kernel set (Matter writes them)"), Laid, 0);
		const TArray<FVertexTangent> Frames = DeriveTangents(Mesh);
		int32 Wrong = 0;
		for (int32 Triangle = 0; Triangle < Mesh.NumTriangles(); ++Triangle)
		{
			const FIntVector& Built = Mesh.Triangle(Triangle);
			const int32 Corners[3] = {Built.X, Built.Y, Built.Z};
			const UE::Geometry::FIndex3i Written = Out.GetTriangle(Triangle);
			const UE::Geometry::FIndex3i Normals = Attributes.PrimaryNormals()->GetTriangle(Triangle);
			const UE::Geometry::FIndex3i Tangents = Attributes.PrimaryTangents()->GetTriangle(Triangle);
			const UE::Geometry::FIndex3i Bitangents = Attributes.PrimaryBiTangents()->GetTriangle(Triangle);
			const UE::Geometry::FIndex3i UVs = Attributes.GetUVLayer(0)->GetTriangle(Triangle);
			const UE::Geometry::FIndex3i Colors = Attributes.PrimaryColors()->GetTriangle(Triangle);
			Wrong += Attributes.GetMaterialID()->GetValue(Triangle) == Mesh.TriangleSlot(Triangle) ? 0 : 1;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 Vertex = Corners[Corner];
				const FVertexTangent& Frame = Frames[Vertex];
				const FVector Bitangent = FVector::CrossProduct(Mesh.Normal(Vertex), Frame.TangentX) * Frame.BinormalSign;
				const FLinearColor Color = Mesh.HasColors() ? Mesh.Color(Vertex) : FLinearColor::White;
				const FVector WrittenNormal(Attributes.PrimaryNormals()->GetElement(Normals[Corner]));
				const FVector WrittenTangent(Attributes.PrimaryTangents()->GetElement(Tangents[Corner]));
				const FVector WrittenBitangent(Attributes.PrimaryBiTangents()->GetElement(Bitangents[Corner]));
				const FVector2D WrittenUV(Attributes.GetUVLayer(0)->GetElement(UVs[Corner]));
				const FLinearColor WrittenColor(Attributes.PrimaryColors()->GetElement(Colors[Corner]));
				const FVector Engine = Mesh.Position(Vertex) * EngineUnitsPerMeter;
				const bool bPosition = Out.GetVertex(Written[Corner]).Equals(Engine, 1e-9);
				const bool bNormal = WrittenNormal.Equals(Mesh.Normal(Vertex), 1e-6);
				const bool bTangent = WrittenTangent.Equals(Frame.TangentX, 1e-6);
				const bool bBitangent = WrittenBitangent.Equals(Bitangent, 1e-6);
				const bool bUV = WrittenUV.Equals(Mesh.UV(Vertex), 1e-5);
				const bool bColor = WrittenColor.Equals(Color, 1e-6f);
				Wrong += (bPosition && bNormal && bTangent && bBitangent && bUV && bColor) ? 0 : 1;
			}
		}
		TestEqual(TEXT("Corners or triangles written unlike the builder's"), Wrong, 0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormDynamicMeshTangentsTest, "Musee.Form.DynamicMeshTangents",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormDynamicMeshTangentsTest::RunTest(const FString& Parameters)
	{
		// A floor with plan UVs: tangent east, bitangent south, right-handed; with U mirrored, tangent west and
		// left-handed. These are the frames Studio converts as they are (VectorUtil::BitangentSign is the engine's
		// BinormalSign).
		for (const bool bMirrored : {false, true})
		{
			UE::Geometry::FDynamicMesh3 Out;
			WriteDynamicMesh(Floor(bMirrored), EngineUnitsPerMeter, Out);
			const UE::Geometry::FDynamicMeshAttributeSet& Attributes = *Out.Attributes();
			const FVector East = bMirrored ? -FVector::ForwardVector : FVector::ForwardVector;
			int32 Wrong = 0;
			for (const int32 Element : Attributes.PrimaryTangents()->ElementIndicesItr())
			{
				const FVector Normal(Attributes.PrimaryNormals()->GetElement(Element));
				const FVector Tangent(Attributes.PrimaryTangents()->GetElement(Element));
				const FVector Bitangent(Attributes.PrimaryBiTangents()->GetElement(Element));
				const double Sign = UE::Geometry::VectorUtil::BitangentSign(Normal, Tangent, Bitangent);
				const bool bFrame = Tangent.Equals(East, 1e-6) && Bitangent.Equals(FVector::RightVector, 1e-6);
				Wrong += (bFrame && Sign == (bMirrored ? -1.0 : 1.0)) ? 0 : 1;
			}
			const TCHAR* What = bMirrored ? TEXT("Mirrored floor corners with the wrong frame")
				: TEXT("Floor corners with the wrong frame");
			TestEqual(What, Wrong, 0);
			TestEqual(TEXT("Floor corners leaning from the UV gradient"), CountAstray(Out, 1e-6), 0);
		}

		// The cornice swept round a 3 m curve and a corner: every corner's frame follows its triangle's UV gradient to
		// within one station's turn (the ovolo turns 15 degrees a station at 1 mm).
		FMeshBuilder Cornice;
		const FProfileStations Section = OvoloCornice().Resolve(0.001);
		TArray<FVector2D> Path;
		for (const double Angle : ArcStations(3.0, 0.0, UE_DOUBLE_PI / 3.0, 0.001))
		{
			Path.Add(3.0 * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
		for (const TArray<FSweepFrame>& Run : PlanRuns(Path, false, 3.0))
		{
			Sweep(Cornice, Run, Section);
		}
		UE::Geometry::FDynamicMesh3 Out;
		WriteDynamicMesh(Cornice, EngineUnitsPerMeter, Out);
		TestEqual(TEXT("Swept corners leaning from the UV gradient by a station's turn"),
			CountAstray(Out, FMath::DegreesToRadians(16.0)), 0);
		return true;
	}
}

#endif
