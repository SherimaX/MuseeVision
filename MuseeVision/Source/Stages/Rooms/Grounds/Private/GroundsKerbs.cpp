#include "GroundsParts.h"
#include "GroundsSettingOut.h"

/**
 * The kerbs (§3.5): sawn travertine, 0.12 wide and 0.205 deep with 15 mm chamfers, in stones about 1.20 long, swept
 * along their centre lines. Each kerb end meets the next exactly: where two of the sheet's kerbs continue one another
 * round a corner they are one run, mitred there; where one stops against another (the parvis's kerbs at the cour's), it
 * stops on the other's face; where a kerb reaches the strip round the Salon it butts the strip's edge at y 9.16.
 */
namespace Musee::Grounds
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;
	}

	void DrawKerbs(TArray<FFormDrawing>& Out)
	{
		const FFormProfile Stone = Section(Plan::KerbSection);
		for (const TPair<FString, FRun>& Run : KerbRuns())
		{
			const TArray<FRunStone> Stones = Run.Value.Stones(Plan::KerbStone);
			for (int32 Index = 0; Index < Stones.Num(); ++Index)
			{
				FFormSweep Shape;
				Shape.Section = Stone;
				Shape.Path = Stones[Index].Path;
				Shape.StartCut = Stones[Index].StartCut;
				Shape.EndCut = Stones[Index].EndCut;
				Out.Add(Drawn(FString::Printf(TEXT("kerb.%s.%03d"), *Run.Key, Index + 1), TEXT("kerb"), MoveTemp(Shape), Walkable));
			}
		}
	}
}
