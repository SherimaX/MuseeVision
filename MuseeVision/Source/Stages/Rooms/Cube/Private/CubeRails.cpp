#include "CubeParts.h"

/**
 * The guide rails (§3.5), north and south in the class's notches, in the Atrium's T-section: a 16 mm blade from r 2.265
 * to 2.385 on a foot 140 wide to r 2.425. Each runs from under the Atrium's rails to 5 mm over the ceiling iris's top
 * blade, and goes on as a tail 5 mm under the stack, so the car's top ring holds on rack down to the clamp stop. Dark
 * bronze brackets fix each foot to the glass's frames and plates and to the sleeve; a gilt bracket hangs each tail from
 * the soffit ring. The brackets' backs are turned to the faces they bear on. The racks on the rails' backs wait for polish.
 */
namespace Musee::Cube
{
	namespace
	{
		const TCHAR* SideOf(double Bearing)
		{
			return Bearing < 90.0 ? TEXT("north") : TEXT("south");
		}

		/** The rail's T-section in plan at its bearing. */
		FFormProfile Tee(double Bearing)
		{
			using namespace Plan;
			auto At = [Bearing](double U, double T) { return U * Outward(Bearing) + T * Along(Bearing); };
			return FFormProfile::Polyline({At(RailBladeInner, -RailBladeHalf), At(RailFootInner, -RailBladeHalf), At(RailFootInner, -RailFootHalf),
				At(RailFootOuter, -RailFootHalf), At(RailFootOuter, RailFootHalf), At(RailFootInner, RailFootHalf), At(RailFootInner, RailBladeHalf),
				At(RailBladeInner, RailBladeHalf)}, true);
		}

		/** A bracket between two radii and heights at a bearing, 0.180 wide at its outer face. */
		FFormLathe Bracket(double Bearing, double R0, double R1, double Z0, double Z1)
		{
			const double Half = HalfAngle(Plan::BracketHalf, R1);
			return Band(R0, R1, Z0, Z1, Bearing - Half, Bearing + Half);
		}
	}

	void DrawRails(TArray<FFormDrawing>& Out)
	{
		using namespace Plan;
		for (const double Bearing : RailBearings)
		{
			const TCHAR* Side = SideOf(Bearing);
			Out.Add(Drawn(FString::Printf(TEXT("rail.%s"), Side), TEXT("rail"), Prism(Tee(Bearing), RailBottom, RailTop), Solid,
				FMuseeFrame(FVector(0.0, 0.0, RailBottom))));
			Out.Add(Drawn(FString::Printf(TEXT("rail.%s.tail"), Side), TEXT("rail"), Prism(Tee(Bearing), TailBottom, TailTop), Solid,
				FMuseeFrame(FVector(0.0, 0.0, TailBottom))));
			for (int32 Level = 0; Level < int32(UE_ARRAY_COUNT(BracketFeet)); ++Level)
			{
				const double Foot = BracketFeet[Level];
				Out.Add(Drawn(FString::Printf(TEXT("rail.%s.bracket.%d"), Side, Level + 1), TEXT("rail.bracket"),
					Bracket(Bearing, BracketInner, BracketOuter, Foot, Foot + BracketHeight), Solid));
			}
			Out.Add(Drawn(FString::Printf(TEXT("rail.%s.tail.bracket"), Side), TEXT("soffit"),
				Bracket(Bearing, BracketInner, TailBracketOuter, TailBracketBottom, Ceiling), Solid));
		}
	}
}
