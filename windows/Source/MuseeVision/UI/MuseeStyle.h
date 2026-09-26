#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;
struct FSlateBrush;

/**
 * The overlay's design language. Two voices: the wall label, printed on travertine paper in dark
 * bronze ink with a gilt rule, and the museum's signage (the hint, the buttons, the message), set
 * in letter-spaced small caps on smoked bronze glass with a gilt hairline.
 *
 * All sizes are Slate units at the 1080p reference: the game viewport's DPI scale doubles them at
 * 4K, so a 20 pt title is 56 px tall on a 3840 × 2160 screen. Type is Roboto from the engine's
 * default font, which falls back to Droid Sans for Chinese titles.
 */
namespace MuseeStyle
{
	// The logo's colours (README, "The logo").
	inline const FLinearColor Gilt = FLinearColor(FColor(0xC9, 0xA2, 0x66));
	inline const FLinearColor Travertine = FLinearColor(FColor(0xF1, 0xEB, 0xDF));
	inline const FLinearColor Bronze = FLinearColor(FColor(0x17, 0x15, 0x0F));
	/** The label's card: travertine a shade lighter, like a printed label on the stone. */
	inline const FLinearColor Paper = FLinearColor(FColor(0xF7, 0xF3, 0xEA));

	// Type scale (points; letter-spacing in 1/1000 em).
	inline constexpr float ArtistSize = 10.f;      // ARTIST, Medium, tracked 200
	inline constexpr float TitleSize = 20.f;       // Title, Italic
	inline constexpr float OriginalSize = 12.f;    // Titre original, Italic
	inline constexpr float YearSize = 13.f;        // 1872, Light
	inline constexpr float DetailSize = 11.5f;     // medium, collection and city, Light
	inline constexpr float NotesSize = 12.f;       // the note, Light, 1.3 leading
	inline constexpr float SignSize = 9.5f;        // CALL THE CAR, Medium, tracked 180
	inline constexpr float KeySize = 8.5f;         // the keycap's E, Medium
	inline constexpr float MessageSize = 13.5f;    // the museum's voice, Light, tracked 20
	inline constexpr float PhotoSize = 10.f;       // photo mode's one line, Light, tracked 150

	/** The label's text column: about 50 characters of the note. */
	inline constexpr float LabelMeasure = 380.f;
	/** Distance from the screen's edges. */
	inline constexpr float SafeMargin = 64.f;

	inline FLinearColor Alpha(const FLinearColor& Colour, float Opacity) { return Colour.CopyWithNewOpacity(Opacity); }

	/** A face of the engine's Roboto ("Light", "Regular", "Medium", "Italic"…), tracked. */
	FSlateFontInfo Font(const FName Face, float Size, int32 Tracking = 0);

	/** A plain white brush, for rules and tinted fills. */
	const FSlateBrush* WhiteBrush();

	/** A rule: a bar of colour, Width wide (0: as wide as its slot) and Height tall. */
	TSharedRef<SWidget> MakeRule(const FLinearColor& Colour, float Width, float Height);

	/** A line of signage: letter-spaced small caps in travertine. */
	TSharedRef<STextBlock> MakeCaps(const FText& Text);

	/** A small outlined keycap: "E", "1". */
	TSharedRef<SWidget> MakeKeycap(const FText& Key);

	/** Smoked bronze glass over the blurred view, with a gilt hairline. */
	TSharedRef<SWidget> MakeGlass(const TSharedRef<SWidget>& Content, float Radius, const FMargin& Padding);

	/** A sign: a glass pill with a keycap and its label. The hint and the buttons. */
	TSharedRef<SWidget> MakeSign(const FText& Key, const TSharedRef<SWidget>& Label);
}

/**
 * A rounded panel with an optional hairline and soft shadow. Unlike an SBorder, every colour here
 * (the outline and the shadow too) follows the widget's opacity, so panels fade as one piece.
 */
class SMuseePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMuseePanel)
		: _Fill(FLinearColor::Transparent)
		, _Outline(FLinearColor::Transparent)
		, _OutlineWidth(0.f)
		, _Radius(0.f)
		, _Shadow(0.f)
		, _Padding(FMargin(0.f))
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(FLinearColor, Fill)
		SLATE_ARGUMENT(FLinearColor, Outline)
		SLATE_ARGUMENT(float, OutlineWidth)
		SLATE_ARGUMENT(float, Radius)
		/** Opacity of the shadow at the panel's edge (0: none). */
		SLATE_ARGUMENT(float, Shadow)
		SLATE_ARGUMENT(FMargin, Padding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	FLinearColor FillColour = FLinearColor::Transparent;
	FLinearColor OutlineColour = FLinearColor::Transparent;
	float OutlineThickness = 0.f;
	float CornerRadius = 0.f;
	float ShadowStrength = 0.f;
};
