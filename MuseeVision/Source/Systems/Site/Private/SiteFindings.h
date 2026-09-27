#pragma once

#include "CoreMinimal.h"
#include "MuseeFinding.h"

namespace Musee::Site
{
	/**
	 * A finding of one of Site's truths (Site.Described, Site.PortsMate, Site.LoopsClose), with its reading: what was
	 * counted or measured, the value, and the range expected of it.
	 */
	inline FMuseeFinding SiteFinding(const TCHAR* Check, FName Drawer, const FString& Summary, const FString& Label, double Value,
		double Least, double Most, const FMuseeKey& Subject = FMuseeKey())
	{
		FMuseeMeasurement Reading;
		Reading.Label = Label;
		Reading.Value = Value;
		Reading.ExpectedMinimum = Least;
		Reading.ExpectedMaximum = Most;
		Reading.Subject = Subject;
		FMuseeFinding Finding = FMuseeFinding::Make(Drawer, Summary, Reading);
		Finding.Check = Check;
		Finding.Owner = TEXT("Site");
		return Finding;
	}
}
