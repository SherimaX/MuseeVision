#include "UI/MuseeStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

FSlateFontInfo MuseeStyle::Font(const FName Face, float Size, int32 Tracking)
{
	FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Face, Size);
	Info.LetterSpacing = Tracking;
	return Info;
}

const FSlateBrush* MuseeStyle::WhiteBrush()
{
	static const FSlateColorBrush Brush(FLinearColor::White);
	return &Brush;
}

TSharedRef<SWidget> MuseeStyle::MakeRule(const FLinearColor& Colour, float Width, float Height)
{
	return SNew(SBox)
		.WidthOverride(Width > 0.f ? FOptionalSize(Width) : FOptionalSize())
		.HeightOverride(Height)
		[
			SNew(SImage).Image(WhiteBrush()).ColorAndOpacity(Colour)
		];
}

TSharedRef<STextBlock> MuseeStyle::MakeCaps(const FText& Text)
{
	return SNew(STextBlock)
		.Text(Text)
		.Font(Font("Medium", SignSize, 180))
		.TransformPolicy(ETextTransformPolicy::ToUpper)
		.ColorAndOpacity(Travertine);
}

TSharedRef<SWidget> MuseeStyle::MakeKeycap(const FText& Key)
{
	return SNew(SMuseePanel)
		.Fill(Alpha(Travertine, 0.08f))
		.Outline(Alpha(Travertine, 0.72f))
		.OutlineWidth(1.f)
		.Radius(3.f)
		[
			SNew(SBox)
			.MinDesiredWidth(20.f)
			.HeightOverride(20.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FMargin(5.f, 1.f, 5.f, 0.f))
			[
				SNew(STextBlock).Text(Key).Font(Font("Medium", KeySize)).ColorAndOpacity(Travertine)
			]
		];
}

TSharedRef<SWidget> MuseeStyle::MakeGlass(const TSharedRef<SWidget>& Content, float Radius, const FMargin& Padding)
{
	return SNew(SBackgroundBlur)
		.BlurStrength(12.f)
		.CornerRadius(FVector4(Radius, Radius, Radius, Radius))
		.Padding(FMargin(0.f))
		[
			SNew(SMuseePanel)
			.Fill(Alpha(Bronze, 0.55f))
			.Outline(Alpha(Gilt, 0.42f))
			.OutlineWidth(1.f)
			.Radius(Radius)
			.Padding(Padding)
			[
				Content
			]
		];
}

TSharedRef<SWidget> MuseeStyle::MakeSign(const FText& Key, const TSharedRef<SWidget>& Label)
{
	// A pill: 20 keycap + 2 × 7 padding = 34 tall, so a 17 radius.
	return MakeGlass(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			MakeKeycap(Key)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.f, 0.f, 0.f, 0.f))
		[
			Label
		],
		17.f, FMargin(7.f, 7.f, 18.f, 7.f));
}

void SMuseePanel::Construct(const FArguments& InArgs)
{
	FillColour = InArgs._Fill;
	OutlineColour = InArgs._Outline;
	OutlineThickness = InArgs._OutlineWidth;
	CornerRadius = InArgs._Radius;
	ShadowStrength = InArgs._Shadow;
	ChildSlot.Padding(InArgs._Padding)
	[
		InArgs._Content.Widget
	];
}

int32 SMuseePanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FLinearColor& Tint = InWidgetStyle.GetColorAndOpacityTint();
	const FVector2f PanelSize(AllottedGeometry.GetLocalSize());

	// The soft shadow: rings growing outward from the panel, a little below it, each faint; where
	// they overlap near the edge they add up, so the shadow fades out over ShadowRings × ShadowStep.
	if (ShadowStrength > 0.f)
	{
		constexpr int32 ShadowRings = 8;
		constexpr float ShadowStep = 1.75f;
		constexpr float ShadowDrop = 3.f;
		const FLinearColor RingColour(0.f, 0.f, 0.f, ShadowStrength / ShadowRings * Tint.A);
		for (int32 Ring = 1; Ring <= ShadowRings; ++Ring)
		{
			const float Grow = Ring * ShadowStep;
			const FSlateRoundedBoxBrush RingBrush(FLinearColor::White, CornerRadius + Grow);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(PanelSize + FVector2f(2.f * Grow, 2.f * Grow), FSlateLayoutTransform(FVector2f(-Grow, ShadowDrop - Grow))),
				&RingBrush, ESlateDrawEffect::None, RingColour);
		}
	}

	// The panel. Draw elements copy what they need from a brush, so a brush made here is safe, and
	// it carries the outline already faded with the widget.
	const FSlateRoundedBoxBrush PanelBrush(FLinearColor::White, CornerRadius, MuseeStyle::Alpha(OutlineColour,OutlineColour.A * Tint.A), OutlineThickness);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), &PanelBrush, ESlateDrawEffect::None, FillColour * Tint);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 2, InWidgetStyle, bParentEnabled);
}
