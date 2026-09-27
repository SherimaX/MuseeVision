#include "CubeParts.h"

/**
 * The concrete box (§3.1): cast in situ, 1.000 thick and sealed, hidden behind the panel wall and its service void. Its
 * east and west walls run the full square, the north and south walls stand between them, the base slab lies inside the
 * walls, and the roof slab lies inside them too, pierced where the sleeve comes through.
 */
namespace Musee::Cube
{
	namespace
	{
		FFormProfile Square(double X0, double Y0, double X1, double Y1)
		{
			return FFormProfile::Rectangle(FVector2D(X0, Y0), FVector2D(X1, Y1));
		}

		FFormProfile Circle(double Radius)
		{
			return FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, 0.0, true);
		}
	}

	void DrawBox(TArray<FFormDrawing>& Out)
	{
		using namespace Plan;
		const FMuseeFrame AtBase(FVector(0.0, 0.0, BaseBottom));
		Out.Add(Drawn(TEXT("box.wall.east"), TEXT("box"), Prism(Square(BoxInner, -BoxOuter, BoxOuter, BoxOuter), BaseBottom, RoofTop), Solid, AtBase));
		Out.Add(Drawn(TEXT("box.wall.west"), TEXT("box"), Prism(Square(-BoxOuter, -BoxOuter, -BoxInner, BoxOuter), BaseBottom, RoofTop), Solid, AtBase));
		Out.Add(Drawn(TEXT("box.wall.north"), TEXT("box"), Prism(Square(-BoxInner, -BoxOuter, BoxInner, -BoxInner), BaseBottom, RoofTop), Solid, AtBase));
		Out.Add(Drawn(TEXT("box.wall.south"), TEXT("box"), Prism(Square(-BoxInner, BoxInner, BoxInner, BoxOuter), BaseBottom, RoofTop), Solid, AtBase));
		Out.Add(Drawn(TEXT("box.base"), TEXT("box"), Prism(Square(-BoxInner, -BoxInner, BoxInner, BoxInner), BaseBottom, BaseTop), Solid, AtBase));
		Out.Add(Drawn(TEXT("box.roof"), TEXT("box"), Prism(Square(-BoxInner, -BoxInner, BoxInner, BoxInner), RoofSoffit, RoofTop, {Circle(RoofHole)}), Solid,
			FMuseeFrame(FVector(0.0, 0.0, RoofSoffit))));
	}
}
