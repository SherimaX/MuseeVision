#include "MuseeUse.h"

FMuseeUse FMuseeUse::Available(const FText& InVerb, const FText& InCaption)
{
	FMuseeUse Use;
	Use.Verb = InVerb;
	Use.Caption = InCaption;
	Use.bAvailable = true;
	return Use;
}

FMuseeUse FMuseeUse::Unavailable(const FText& InVerb, const FText& InCaption, const FText& InWhyNot)
{
	FMuseeUse Use;
	Use.Verb = InVerb;
	Use.Caption = InCaption;
	Use.bAvailable = false;
	Use.WhyNot = InWhyNot;
	return Use;
}

FMuseeUse FMuseeUse::Seating(const FMuseeFrame& InSeatFrame) const
{
	FMuseeUse Use = *this;
	Use.bSeats = true;
	Use.SeatFrame = InSeatFrame;
	return Use;
}
