#include "Glass/GlassPane.h"

#include "Sheet/SheetMesh.h"

namespace Musee::ArtMaking
{
	void BuildGlassPane(const FGlassPaneSpec& Pane, UE::Geometry::FDynamicMesh3& Out)
	{
		// The sheet is centred on the light's glass line.
		BuildSheet(FSheetSpec{ Pane.ImageMeters, PaneThicknessMeters, 0.5 * PaneThicknessMeters }, Out);
	}
}
