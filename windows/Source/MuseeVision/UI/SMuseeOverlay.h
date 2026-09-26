#pragma once

#include "CoreMinimal.h"
#include "Visitor/MuseeInteractable.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

class AMuseeCharacter;
class SMuseePlacard;
class SOverlay;
class STextBlock;
class SVerticalBox;
struct FMuseeArtwork;

/**
 * The point of view: a small travertine dot with a soft dark halo, so it reads on stone and in
 * the dark; a thin gilt ring opens round it when what is looked at can be used.
 */
class SMuseeReticle : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMuseeReticle) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) {}

	/** 0: the dot alone; 1: the ring fully open. */
	void SetEngaged(float InEngaged);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(24.0, 24.0); }

private:
	float Engaged = 0.f;
};

/**
 * The visitor's overlay, added to the game viewport by AMuseeHUD (iOS/Overlays.swift):
 * the dot at the centre with the hint for what is looked at under it ([E] WAKE THE GOLDEN LILY),
 * the wall label lower left, the buttons for where you stand lower right ([1] CALL THE CAR), and
 * the museum's message top centre. Everything fades; in photo mode only the one line remains.
 */
class SMuseeOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMuseeOverlay) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Follow the visitor: what they look at, what they can do, what the museum says. */
	void Update(const AMuseeCharacter& Visitor, float DeltaSeconds);

private:
	void UpdatePlacard(const AMuseeCharacter& Visitor);
	void UpdatePrompts(const TArray<FMuseeActionPrompt>& Wanted, float DeltaSeconds);
	void RebuildPrompts(const TArray<FMuseeActionPrompt>& Wanted);

	/**
	 * Fade a line toward the wanted text: in when there is one, out when it is empty. A different
	 * text waits for the old one to fade out first, then comes in. Returns the new opacity.
	 */
	static float Crossfade(const FText& Wanted, FString& Shown, STextBlock& Line, float Opacity, float DeltaSeconds, float InSpeed, float OutSpeed);
	/** Show a widget at an opacity (0…1, eased), displaced by Offset when fully faded. */
	static void Present(SWidget& Widget, float Opacity, const FVector2f& Offset);
	static bool SamePrompts(const TArray<FMuseeActionPrompt>& A, const TArray<FMuseeActionPrompt>& B);

	TSharedPtr<SOverlay> GalleryLayer;
	TSharedPtr<SMuseeReticle> Reticle;
	TSharedPtr<SWidget> HintSign;
	TSharedPtr<STextBlock> HintLabel;
	TSharedPtr<SMuseePlacard> Placard;
	TSharedPtr<SVerticalBox> PromptStack;
	TSharedPtr<SWidget> MessageSign;
	TSharedPtr<STextBlock> MessageLine;
	TSharedPtr<STextBlock> PhotoLine;

	FString ShownHint;
	float HintAlpha = 0.f;
	/** The work on the label (compared only, never read: the catalogue owns it). */
	const FMuseeArtwork* ShownWork = nullptr;
	TArray<FMuseeActionPrompt> ShownPrompts;
	float PromptAlpha = 0.f;
	FString ShownMessage;
	float MessageAlpha = 0.f;
};
