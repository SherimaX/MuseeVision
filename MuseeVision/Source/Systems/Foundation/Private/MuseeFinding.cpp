#include "MuseeFinding.h"

FMuseeFinding FMuseeFinding::Make(FName InDrawer, const FString& InSummary, const FMuseeMeasurement& InMeasurement)
{
	FMuseeFinding Finding;
	Finding.Drawer = InDrawer;
	Finding.Summary = InSummary;
	Finding.Measurement = InMeasurement;
	return Finding;
}

FString FMuseeFinding::ToString() const
{
	FString Text = FString::Printf(TEXT("%s (owner %s, drawn by %s): %s %s."), *Check.ToString(), *Owner.ToString(), *Drawer.ToString(),
		*Summary, *Measurement.ToString());
	if (!Context.IsEmpty())
	{
		Text += FString::Printf(TEXT(" Under %s."), *Context);
	}
	return Text;
}
