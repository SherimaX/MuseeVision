#include "Sheet/SheetMesh.h"

#include "FormMesh.h"

namespace Musee::ArtMaking
{
	void BuildSheet(const FSheetSpec& Sheet, UE::Geometry::FDynamicMesh3& Out)
	{
		const double W = Sheet.SizeMeters.X;
		const double H = Sheet.SizeMeters.Y;
		const double T = Sheet.ThicknessMeters;
		const double Front = Sheet.FrontMeters;
		const double Back = Front - T;
		const double U0 = Sheet.ImageLeftMeters;
		const double V0 = Sheet.ImageTopMeters;
		FFormMesh Mesh;
		// The faces: the image's top-left corner, as seen from the front, is UV0's origin.
		for (const double Side : { Front, Back })
		{
			const FVector Normal(0.0, Side == Front ? 1.0 : -1.0, 0.0);
			const int32 A = Mesh.AddVertex(FVector(0.0, Side, 0.0), Normal, FVector2D(U0, V0 + H));
			const int32 B = Mesh.AddVertex(FVector(W, Side, 0.0), Normal, FVector2D(U0 + W, V0 + H));
			const int32 C = Mesh.AddVertex(FVector(W, Side, H), Normal, FVector2D(U0 + W, V0));
			const int32 D = Mesh.AddVertex(FVector(0.0, Side, H), Normal, FVector2D(U0, V0));
			Mesh.AddQuad(A, B, C, D);
		}
		// The edges: the foot and head, then the two sides.
		for (const double Z : { 0.0, H })
		{
			const FVector Normal(0.0, 0.0, Z > 0.0 ? 1.0 : -1.0);
			const int32 A = Mesh.AddVertex(FVector(0.0, Back, Z), Normal, FVector2D(0.0, 0.0));
			const int32 B = Mesh.AddVertex(FVector(W, Back, Z), Normal, FVector2D(W, 0.0));
			const int32 C = Mesh.AddVertex(FVector(W, Front, Z), Normal, FVector2D(W, T));
			const int32 D = Mesh.AddVertex(FVector(0.0, Front, Z), Normal, FVector2D(0.0, T));
			Mesh.AddQuad(A, B, C, D);
		}
		for (const double X : { 0.0, W })
		{
			const FVector Normal(X > 0.0 ? 1.0 : -1.0, 0.0, 0.0);
			const int32 A = Mesh.AddVertex(FVector(X, Back, 0.0), Normal, FVector2D(0.0, 0.0));
			const int32 B = Mesh.AddVertex(FVector(X, Front, 0.0), Normal, FVector2D(T, 0.0));
			const int32 C = Mesh.AddVertex(FVector(X, Front, H), Normal, FVector2D(T, H));
			const int32 D = Mesh.AddVertex(FVector(X, Back, H), Normal, FVector2D(0.0, H));
			Mesh.AddQuad(A, B, C, D);
		}
		Mesh.Write(Out);
	}
}
