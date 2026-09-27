#include "FormGlyph.h"

bool FFormGlyph::IsWellFormed(FString* OutWhy) const
{
	auto Fail = [OutWhy](const FString& Why)
	{
		if (OutWhy)
		{
			*OutWhy = Why;
		}
		return false;
	};
	if (Letter.Len() != 1 || Advance <= 0.0)
	{
		return Fail(TEXT("a glyph is one letter with an advance"));
	}
	if (Points.Num() < 3 || Smooth.Num() != Points.Num() || Mitres.Num() != Points.Num())
	{
		return Fail(FString::Printf(TEXT("%s: every point has a smooth flag and a mitre"), *Letter));
	}
	if (ContourStarts.IsEmpty() || ContourStarts[0] != 0)
	{
		return Fail(FString::Printf(TEXT("%s: its first contour starts at its first point"), *Letter));
	}
	for (int32 Index = 0; Index < ContourStarts.Num(); ++Index)
	{
		const int32 End = Index + 1 < ContourStarts.Num() ? ContourStarts[Index + 1] : Points.Num();
		if (End - ContourStarts[Index] < 3)
		{
			return Fail(FString::Printf(TEXT("%s: contour %d has fewer than three points, or the contours are out of order"), *Letter, Index));
		}
	}
	if (Triangles.IsEmpty() || Triangles.Num() % 3 != 0)
	{
		return Fail(FString::Printf(TEXT("%s: its triangles come in threes"), *Letter));
	}
	for (const int32 Corner : Triangles)
	{
		if (!Points.IsValidIndex(Corner))
		{
			return Fail(FString::Printf(TEXT("%s: a triangle names point %d of %d"), *Letter, Corner, Points.Num()));
		}
	}
	return true;
}
