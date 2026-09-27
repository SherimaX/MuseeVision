#include "Scroll/ScrollLayout.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The share of a photograph's height that is the work. */
		double WindowHeight(const FScrollImage& Image)
		{
			return Image.Bottom - Image.Top;
		}

		/** The work's window of a photograph, as its length at unit height. */
		double LengthPerHeight(const FScrollImage& Image)
		{
			return (Image.To - Image.From) * Image.Pixels.X / (WindowHeight(Image) * Image.Pixels.Y);
		}
	}

	FScrollLayout LayScroll(TConstArrayView<FScrollImage> LeftToRight, const FVector2D& Sheet)
	{
		FScrollLayout Out;
		double Joined = 0.0;
		for (const FScrollImage& Image : LeftToRight)
		{
			if (Image.Pixels.X <= 0 || Image.Pixels.Y <= 0 || Image.To <= Image.From || Image.Bottom <= Image.Top)
			{
				return Out;
			}
			Joined += LengthPerHeight(Image);
		}
		if (LeftToRight.IsEmpty() || Sheet.X <= 0.0 || Sheet.Y <= 0.0)
		{
			return Out;
		}
		const double SheetLength = Sheet.X / Sheet.Y;
		Out.Stray = Joined / SheetLength - 1.0;
		// Covering the sheet: the windows at its height when they are the longer, else at its length.
		const double Height = Joined >= SheetLength ? Sheet.Y : Sheet.X / Joined;
		const double EndCut = 0.5 * (Joined * Height - Sheet.X);
		const double HeadCut = 0.5 * (Height - Sheet.Y);
		Out.CutMeters = FMath::Max(EndCut, HeadCut);
		double Start = 0.0;
		for (int32 Index = 0; Index < LeftToRight.Num(); ++Index)
		{
			const FScrollImage& Image = LeftToRight[Index];
			const double Aspect = double(Image.Pixels.X) / Image.Pixels.Y;
			// The whole photograph's height, when its window stands at the windows' one height.
			const double ImageHeight = Height / WindowHeight(Image);
			const double Length = LengthPerHeight(Image) * Height;
			// The panel is the part of this photograph's window that falls on the sheet.
			const double Left = FMath::Max(Start, EndCut);
			const double Right = FMath::Min(Start + Length, EndCut + Sheet.X);
			if (Right > Left)
			{
				FScrollPanel& Panel = Out.Panels.AddDefaulted_GetRef();
				Panel.Image = Index;
				Panel.AlongMeters = Left - EndCut;
				Panel.LengthMeters = Right - Left;
				Panel.ImageLeftMeters = Image.From * Aspect * ImageHeight + (Left - Start);
				Panel.ImageTopMeters = Image.Top * ImageHeight + HeadCut;
				Panel.ImageMeters = FVector2D(Aspect * ImageHeight, ImageHeight);
			}
			Start += Length;
		}
		return Out;
	}
}
