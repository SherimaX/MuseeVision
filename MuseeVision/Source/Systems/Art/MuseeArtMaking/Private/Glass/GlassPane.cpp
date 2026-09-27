#include "Glass/GlassPane.h"

#include "FormMesh.h"

namespace Musee::ArtMaking
{
	void BuildGlassPane(const FGlassPaneSpec& Pane, UE::Geometry::FDynamicMesh3& Out)
	{
		const double W = Pane.ImageMeters.X;
		const double H = Pane.ImageMeters.Y;
		const double Half = PaneThicknessMeters * 0.5;
		FFormMesh Glass;
		// The faces: the image's top-left, as seen from inside, is UV0's origin.
		for (const double Side : { Half, -Half })
		{
			const FVector Normal(0.0, Side > 0.0 ? 1.0 : -1.0, 0.0);
			const int32 A = Glass.AddVertex(FVector(0.0, Side, 0.0), Normal, FVector2D(0.0, H));
			const int32 B = Glass.AddVertex(FVector(W, Side, 0.0), Normal, FVector2D(W, H));
			const int32 C = Glass.AddVertex(FVector(W, Side, H), Normal, FVector2D(W, 0.0));
			const int32 D = Glass.AddVertex(FVector(0.0, Side, H), Normal, FVector2D(0.0, 0.0));
			Glass.AddQuad(A, B, C, D);
		}
		// The edges, where the came's heart grips the glass: the sill and head, then the two sides.
		for (const double Z : { 0.0, H })
		{
			const FVector Normal(0.0, 0.0, Z > 0.0 ? 1.0 : -1.0);
			const int32 A = Glass.AddVertex(FVector(0.0, -Half, Z), Normal, FVector2D(0.0, 0.0));
			const int32 B = Glass.AddVertex(FVector(W, -Half, Z), Normal, FVector2D(W, 0.0));
			const int32 C = Glass.AddVertex(FVector(W, Half, Z), Normal, FVector2D(W, PaneThicknessMeters));
			const int32 D = Glass.AddVertex(FVector(0.0, Half, Z), Normal, FVector2D(0.0, PaneThicknessMeters));
			Glass.AddQuad(A, B, C, D);
		}
		for (const double X : { 0.0, W })
		{
			const FVector Normal(X > 0.0 ? 1.0 : -1.0, 0.0, 0.0);
			const int32 A = Glass.AddVertex(FVector(X, -Half, 0.0), Normal, FVector2D(0.0, 0.0));
			const int32 B = Glass.AddVertex(FVector(X, Half, 0.0), Normal, FVector2D(PaneThicknessMeters, 0.0));
			const int32 C = Glass.AddVertex(FVector(X, Half, H), Normal, FVector2D(PaneThicknessMeters, H));
			const int32 D = Glass.AddVertex(FVector(X, -Half, H), Normal, FVector2D(0.0, H));
			Glass.AddQuad(A, B, C, D);
		}
		Glass.Write(Out);
	}
}
