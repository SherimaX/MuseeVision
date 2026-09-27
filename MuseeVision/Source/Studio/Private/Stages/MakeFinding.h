#pragma once

#include "CoreMinimal.h"
#include "MuseeFinding.h"

namespace Musee::Studio
{
	/**
	 * A finding the make reports for a truth it found broken while making a stage (Form.Drawn, Matter.Resolved,
	 * Site.PlanResolves): the owner is the check's first name, the drawer where the fix belongs, and the reading one
	 * thing that should have been none.
	 */
	inline FMuseeFinding MakeFinding(const TCHAR* Check, FName Drawer, const FString& Summary, const FString& Subject = FString())
	{
		FMuseeMeasurement Reading;
		Reading.Label = TEXT("things that could not be made");
		Reading.Value = 1.0;
		Reading.ExpectedMaximum = 0.0;
		if (const TOptional<FMuseeKey> Key = FMuseeKey::Parse(Subject))
		{
			Reading.Subject = *Key;
		}
		FMuseeFinding Finding = FMuseeFinding::Make(Drawer, Summary, Reading);
		const FString Name(Check);
		int32 Dot = INDEX_NONE;
		Finding.Check = Check;
		Finding.Owner = Name.FindChar(TEXT('.'), Dot) ? FName(*Name.Left(Dot)) : FName(*Name);
		return Finding;
	}
}
