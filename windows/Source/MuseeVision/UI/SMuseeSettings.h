#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class UMuseeGraphics;

/** A patch of the card that answers the mouse: a click steps on (+1), a right click back (-1). */
class SMuseeHitArea : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnStep, int32);

	SLATE_BEGIN_ARGS(SMuseeHitArea) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_EVENT(FOnStep, OnStep)
		SLATE_EVENT(FSimpleDelegate, OnHover)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	FOnStep OnStep;
	FSimpleDelegate OnHover;
};

/**
 * The graphics card (UMuseeGraphics): a wall label set out as a settings sheet. The five presets run
 * along the top as a gallery's rooms, I to V; under them, the lines of the sheet in two columns (Image
 * and Light, Detail and Display). Arrows or WASD choose and change, Enter steps on, 1-5 pick a preset,
 * Esc (or the key that opened it) closes; a gamepad's D-pad, A, B and shoulders do the same, and the
 * mouse points and clicks (right click steps back).
 */
class SMuseeSettingsCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMuseeSettingsCard) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UMuseeGraphics>, Graphics)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
	virtual FNavigationReply OnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& NavigationEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** Back to the presets line, as when the card opens. */
	void ResetSelection() { Selected = 0; }

private:
	TSharedRef<SWidget> MakeHeader();
	TSharedRef<SWidget> MakePresets();
	TSharedRef<SWidget> MakeColumn(int32 FirstOption, int32 EndOption);
	TSharedRef<SWidget> MakeRow(int32 Option);
	TSharedRef<SWidget> MakeFooter();

	/** Line 0 is the presets; line 1 + i is option i. */
	void Move(int32 Delta);
	void Change(int32 Delta, bool bWrap);
	void ChangeOption(int32 Option, int32 Delta, bool bWrap);
	void PickPreset(int32 Index);
	bool IsLocked(int32 Option) const;
	FText ValueText(int32 Option) const;

	TWeakObjectPtr<UMuseeGraphics> Graphics;
	int32 Selected = 0;
};

/**
 * Everything the graphics settings put over the view, added to the viewport by AMuseeHUD above the
 * gallery overlay: the card over a blurred, darkened museum; the toast, a small label top right that
 * confirms a change with the live frame rate and GPU time; and the readout, a discreet sign top left.
 */
class SMuseeSettingsLayer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMuseeSettingsLayer) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UMuseeGraphics>, Graphics)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Follow the settings: the card open or closed, the toast after a change, the readout. */
	void Update(float DeltaSeconds, bool bPhotoMode);

	TSharedPtr<SMuseeSettingsCard> Card() const { return CardWidget; }

private:
	TSharedRef<SWidget> MakeToast();
	TSharedRef<SWidget> MakeReadout();

	TWeakObjectPtr<UMuseeGraphics> Graphics;
	TSharedPtr<SWidget> CardLayer;
	TSharedPtr<SMuseeSettingsCard> CardWidget;
	TSharedPtr<SWidget> Toast;
	TSharedPtr<SWidget> Readout;
	float CardAlpha = 0.f;
	float ToastAlpha = 0.f;
	float ReadoutAlpha = 0.f;
	bool bWasOpen = false;
};
