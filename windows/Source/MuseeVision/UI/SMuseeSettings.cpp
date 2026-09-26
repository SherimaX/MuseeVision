#include "UI/SMuseeSettings.h"

#include "Framework/Application/SlateApplication.h"
#include "Settings/MuseeGraphics.h"
#include "UI/MuseeStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MuseeSettings"

using namespace MuseeStyle;

namespace
{
	// The label's inks (SMuseePlacard): dark bronze, softer for what is secondary.
	const FLinearColor Ink = Alpha(Bronze, 0.94f);
	const FLinearColor Secondary = Alpha(Bronze, 0.70f);
	const FLinearColor Quiet = Alpha(Bronze, 0.46f);
	const FLinearColor Faint = Alpha(Bronze, 0.22f);
	/** The gilt, deepened to read as ink on the paper. */
	const FLinearColor GiltInk = FLinearColor(FColor(0x9A, 0x74, 0x3C));

	constexpr float CardWidth = 940.f;
	constexpr float CardPadX = 46.f;
	constexpr float ColumnGap = 56.f;
	constexpr float RowHeight = 29.f;
	constexpr float ValueWidth = 172.f;
	constexpr float ToastWidth = 344.f;

	const TCHAR* const Roman[] = {TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V")};

	TSharedRef<STextBlock> Caps(const FText& Text, float Size, const FLinearColor& Colour, int32 Tracking = 200)
	{
		return SNew(STextBlock)
			.Text(Text)
			.Font(Font("Medium", Size, Tracking))
			.TransformPolicy(ETextTransformPolicy::ToUpper)
			.ColorAndOpacity(Colour);
	}

	/** A keycap printed on the paper: bronze hairline, bronze letter. */
	TSharedRef<SWidget> PaperKey(const FText& Key)
	{
		return SNew(SMuseePanel)
			.Fill(Alpha(Bronze, 0.035f))
			.Outline(Alpha(Bronze, 0.34f))
			.OutlineWidth(1.f)
			.Radius(3.f)
			[
				SNew(SBox)
				.MinDesiredWidth(19.f)
				.HeightOverride(19.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FMargin(5.f, 1.f, 5.f, 0.f))
				[
					SNew(STextBlock).Text(Key).Font(Font("Medium", 8.f)).ColorAndOpacity(Alpha(Bronze, 0.78f))
				]
			];
	}

	/** Keys and what they do, for the card's foot: [↑][↓] CHOOSE. */
	TSharedRef<SWidget> KeyHint(std::initializer_list<FText> Keys, const FText& What)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		for (const FText& Key : Keys)
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 4.f, 0.f)) [ PaperKey(Key) ];
		}
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(5.f, 1.f, 0.f, 0.f)) [ Caps(What, 8.f, Quiet, 180) ];
		return Row;
	}

	FText FpsText(const UMuseeGraphics* G)
	{
		return G && G->Fps() > 0.f ? FText::AsNumber(FMath::RoundToInt(G->Fps())) : FText::FromString(TEXT("–"));
	}

	FText GpuText(const UMuseeGraphics* G)
	{
		FNumberFormattingOptions One;
		One.MinimumFractionalDigits = 1;
		One.MaximumFractionalDigits = 1;
		return G && G->GpuMs() > 0.f ? FText::AsNumber(G->GpuMs(), &One) : FText::FromString(TEXT("–"));
	}

	/** A number over its unit, like the year on a label: 62 / FPS. */
	TSharedRef<SWidget> Stat(TAttribute<FText> Number, TAttribute<FText> Unit, float Size, TAttribute<EVisibility> Shown = EVisibility::SelfHitTestInvisible)
	{
		return SNew(SVerticalBox)
			.Visibility(Shown)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Number).Font(Font("Light", Size, 10)).ColorAndOpacity(Ink)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(1.f, 1.f, 0.f, 0.f))
			[
				SNew(STextBlock).Text(Unit).Font(Font("Medium", 7.5f, 180)).TransformPolicy(ETextTransformPolicy::ToUpper).ColorAndOpacity(Quiet)
			];
	}

	/** Show a widget at an opacity (0…1, eased), displaced by Offset when faded (SMuseeOverlay's). */
	void Present(SWidget& Widget, float Opacity, const FVector2f& Offset, EVisibility WhenShown = EVisibility::HitTestInvisible)
	{
		const float Eased = FMath::SmoothStep(0.f, 1.f, Opacity);
		Widget.SetVisibility(Opacity > 0.f ? WhenShown : EVisibility::Collapsed);
		Widget.SetRenderOpacity(Eased);
		Widget.SetRenderTransform(FSlateRenderTransform(Offset * (1.f - Eased)));
	}
}

// ---------------------------------------------------------------------------------------------------------------

void SMuseeHitArea::Construct(const FArguments& InArgs)
{
	OnStep = InArgs._OnStep;
	OnHover = InArgs._OnHover;
	ChildSlot [ InArgs._Content.Widget ];
}

FReply SMuseeHitArea::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FKey Button = MouseEvent.GetEffectingButton();
	if (Button == EKeys::LeftMouseButton || Button == EKeys::RightMouseButton)
	{
		OnHover.ExecuteIfBound();
		OnStep.ExecuteIfBound(Button == EKeys::LeftMouseButton ? 1 : -1);
	}
	return FReply::Handled();
}

void SMuseeHitArea::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	OnHover.ExecuteIfBound();
}

FCursorReply SMuseeHitArea::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::Hand);
}

// ---------------------------------------------------------------------------------------------------------------

void SMuseeSettingsCard::Construct(const FArguments& InArgs)
{
	Graphics = InArgs._Graphics;
	const int32 NumOptions = UMuseeGraphics::Options().Num();
	// Image and Light on the left; Detail and Display on the right.
	int32 Split = 0;
	while (Split < NumOptions && !UMuseeGraphics::Options()[Split].Section.ToString().Equals(TEXT("Detail"))) { ++Split; }
	if (Split == NumOptions) { Split = (NumOptions + 1) / 2; }

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(CardWidth)
		[
			SNew(SMuseePanel)
			.Fill(Alpha(Paper, 0.985f))
			.Outline(Alpha(Bronze, 0.10f))
			.OutlineWidth(1.f)
			.Radius(2.f)
			.Shadow(0.22f)
			.Padding(FMargin(CardPadX, 36.f, CardPadX, 26.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight() [ MakeHeader() ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 26.f, 0.f, 0.f)) [ MakePresets() ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 22.f, 0.f, 0.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f) [ MakeColumn(0, Split) ]
					+ SHorizontalBox::Slot().AutoWidth() [ SNew(SBox).WidthOverride(ColumnGap) ]
					+ SHorizontalBox::Slot().FillWidth(1.f) [ MakeColumn(Split, NumOptions) ]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 24.f, 0.f, 0.f)) [ MakeRule(Faint, 0.f, 1.f) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 14.f, 0.f, 0.f)) [ MakeFooter() ]
			]
		]
	];
}

TSharedRef<SWidget> SMuseeSettingsCard::MakeHeader()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			// The gilt rule: the logo's slit of light, as on every label.
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left) [ MakeRule(Gilt, 36.f, 2.f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 18.f, 0.f, 0.f))
			[
				Caps(LOCTEXT("Kicker", "Musée Vision · Settings"), ArtistSize, Alpha(Bronze, 0.80f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 6.f, 0.f, 0.f))
			[
				SNew(STextBlock).Text(LOCTEXT("Title", "Graphics")).Font(Font("Italic", 27.f)).ColorAndOpacity(Ink)
			]
		]
		// The frame rate, live, set like a label's date.
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(FMargin(24.f, 0.f, 0.f, 2.f))
		[
			Stat(TAttribute<FText>::CreateLambda([this]() { return FpsText(Graphics.Get()); }),
				TAttribute<FText>::CreateLambda([this]()
				{
					const UMuseeGraphics* G = Graphics.Get();
					return G && G->IsGenerating() ? LOCTEXT("FpsGenerated", "fps, generated") : LOCTEXT("FpsUnit", "fps");
				}), 24.f)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(FMargin(30.f, 0.f, 0.f, 2.f))
		[
			Stat(TAttribute<FText>::CreateLambda([this]() { return GpuText(Graphics.Get()); }), LOCTEXT("GpuUnit", "ms GPU"), 24.f)
		];
}

TSharedRef<SWidget> SMuseeSettingsCard::MakePresets()
{
	TSharedRef<SHorizontalBox> Strip = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < UMuseeGraphics::NumPresets; ++Index)
	{
		auto IsCurrent = [this, Index]() { const UMuseeGraphics* G = Graphics.Get(); return G && G->PresetIndex() == Index; };
		Strip->AddSlot()
			.FillWidth(1.f)
			[
				SNew(SMuseeHitArea)
				.OnHover_Lambda([this]() { Selected = 0; })
				.OnStep_Lambda([this, Index](int32) { PickPreset(Index); })
				[
					SNew(SVerticalBox)
					// The room number, I to V.
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(Roman[Index]))
						.Font(Font("Light", 11.f, 60))
						.ColorAndOpacity_Lambda([IsCurrent]() { return FSlateColor(IsCurrent() ? GiltInk : Quiet); })
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 4.f, 0.f, 12.f))
					[
						SNew(STextBlock)
						.Text(UMuseeGraphics::PresetName(Index))
						.Font(Font("Medium", SignSize, 200))
						.TransformPolicy(ETextTransformPolicy::ToUpper)
						.ColorAndOpacity_Lambda([IsCurrent]() { return FSlateColor(IsCurrent() ? Ink : Secondary); })
					]
					// The gilt under the preset in use, on the strip's hairline.
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(64.f)
						.HeightOverride(2.f)
						[
							SNew(SImage)
							.Image(WhiteBrush())
							.ColorAndOpacity_Lambda([IsCurrent]() { return FSlateColor(IsCurrent() ? Gilt : FLinearColor::Transparent); })
						]
					]
				]
			];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SOverlay)
			// The line is chosen: a faint band behind it, and a gilt mark at its edge.
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(WhiteBrush())
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(Selected == 0 ? Alpha(Bronze, 0.045f) : FLinearColor::Transparent); })
			]
			+ SOverlay::Slot().HAlign(HAlign_Left)
			[
				SNew(SBox).WidthOverride(2.f)
				[
					SNew(SImage)
					.Image(WhiteBrush())
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(Selected == 0 ? Gilt : FLinearColor::Transparent); })
				]
			]
			+ SOverlay::Slot() [ Strip ]
		]
		+ SVerticalBox::Slot().AutoHeight() [ MakeRule(Faint, 0.f, 1.f) ]
		// What the preset does, in the label's italic.
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 14.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				const UMuseeGraphics* G = Graphics.Get();
				return UMuseeGraphics::PresetNote(G ? G->PresetIndex() : UMuseeGraphics::DefaultPreset);
			})
			.Font(Font("Italic", 12.5f))
			.ColorAndOpacity(Secondary)
		];
}

TSharedRef<SWidget> SMuseeSettingsCard::MakeColumn(int32 FirstOption, int32 EndOption)
{
	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);
	const TArray<FMuseeGraphicsOption>& Options = UMuseeGraphics::Options();
	for (int32 Option = FirstOption; Option < EndOption; ++Option)
	{
		if (Option == FirstOption || !Options[Option].Section.EqualTo(Options[Option - 1].Section))
		{
			// A section: a short gilt rule and letter-spaced caps, as the museum's room signs.
			Column->AddSlot()
				.AutoHeight()
				.Padding(FMargin(0.f, Option == FirstOption ? 0.f : 18.f, 0.f, 6.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ MakeRule(Alpha(Gilt, 0.9f), 14.f, 1.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(10.f, 1.f, 0.f, 0.f))
					[
						Caps(Options[Option].Section, 8.5f, Alpha(Bronze, 0.62f), 220)
					]
				];
		}
		Column->AddSlot().AutoHeight() [ MakeRow(Option) ];
	}
	return Column;
}

TSharedRef<SWidget> SMuseeSettingsCard::MakeRow(int32 Option)
{
	const int32 Line = Option + 1;
	auto IsSelected = [this, Line]() { return Selected == Line; };
	auto Chevron = [this, Option, IsSelected](const TCHAR* Glyph, int32 Delta)
	{
		return SNew(SMuseeHitArea)
			.OnHover_Lambda([this, Option]() { Selected = Option + 1; })
			.OnStep_Lambda([this, Option, Delta](int32) { ChangeOption(Option, Delta, false); })
			.Visibility_Lambda([this, Option]() { return IsLocked(Option) ? EVisibility::Hidden : EVisibility::Visible; })
			[
				SNew(SBox)
				.WidthOverride(18.f)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Glyph))
					.Font(Font("Light", 15.f))
					.ColorAndOpacity_Lambda([IsSelected]() { return FSlateColor(IsSelected() ? GiltInk : Faint); })
				]
			];
	};

	return SNew(SMuseeHitArea)
		.OnHover_Lambda([this, Line]() { Selected = Line; })
		.OnStep_Lambda([this, Option](int32 Delta) { ChangeOption(Option, Delta, true); })
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(WhiteBrush())
				.ColorAndOpacity_Lambda([IsSelected]() { return FSlateColor(IsSelected() ? Alpha(Bronze, 0.05f) : FLinearColor::Transparent); })
			]
			+ SOverlay::Slot().HAlign(HAlign_Left)
			[
				SNew(SBox).WidthOverride(2.f)
				[
					SNew(SImage)
					.Image(WhiteBrush())
					.ColorAndOpacity_Lambda([IsSelected]() { return FSlateColor(IsSelected() ? Gilt : FLinearColor::Transparent); })
				]
			]
			+ SOverlay::Slot().Padding(FMargin(12.f, 0.f, 2.f, 0.f))
			[
				SNew(SBox)
				.HeightOverride(RowHeight)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(UMuseeGraphics::Options()[Option].Label)
						.Font(Font("Light", 12.f))
						.ColorAndOpacity_Lambda([this, Option, IsSelected]() { return FSlateColor(IsSelected() ? Ink : Alpha(Bronze, 0.80f)); })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Chevron(TEXT("‹"), -1) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(ValueWidth)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text_Lambda([this, Option]() { return ValueText(Option); })
							.Font_Lambda([this, Option]() { return IsLocked(Option) ? Font("Italic", 11.f) : Font("Regular", 12.f); })
							.ColorAndOpacity_Lambda([this, Option]() { return FSlateColor(IsLocked(Option) ? Quiet : Ink); })
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Chevron(TEXT("›"), 1) ]
				]
			]
		];
}

TSharedRef<SWidget> SMuseeSettingsCard::MakeFooter()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			KeyHint({FText::FromString(TEXT("↑")), FText::FromString(TEXT("↓"))}, LOCTEXT("Choose", "Choose"))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(22.f, 0.f, 0.f, 0.f))
		[
			KeyHint({FText::FromString(TEXT("←")), FText::FromString(TEXT("→"))}, LOCTEXT("Change", "Change"))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(22.f, 0.f, 0.f, 0.f))
		[
			KeyHint({FText::FromString(TEXT("1–5"))}, LOCTEXT("PresetKeys", "Preset"))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(22.f, 0.f, 0.f, 0.f))
		[
			KeyHint({LOCTEXT("EscKey", "Esc")}, LOCTEXT("Close", "Close"))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f) [ SNullWidget::NullWidget ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Kept", "Kept for your next visit."))
			.Font(Font("Italic", 10.5f))
			.ColorAndOpacity(Quiet)
		];
}

FReply SMuseeSettingsCard::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	UMuseeGraphics* G = Graphics.Get();
	const FKey Key = KeyEvent.GetKey();
	// The snapshot, the console and the window's own keys go on to the game.
	if (!G || KeyEvent.IsAltDown() || Key == EKeys::F9 || Key == EKeys::Tilde || Key == EKeys::F11) { return FReply::Unhandled(); }
	auto Is = [&Key](std::initializer_list<FKey> Keys) { for (const FKey& K : Keys) { if (K == Key) { return true; } } return false; };

	if (Is({EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_LeftStick_Up})) { Move(-1); }
	else if (Is({EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_LeftStick_Down})) { Move(1); }
	else if (Key == EKeys::Tab) { Move(KeyEvent.IsShiftDown() ? -1 : 1); }
	else if (Is({EKeys::Left, EKeys::A, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftStick_Left})) { Change(-1, false); }
	else if (Is({EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Right})) { Change(1, false); }
	else if (KeyEvent.IsRepeat()) { }
	else if (Is({EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom})) { Change(1, true); }
	else if (Is({EKeys::Escape, EKeys::F10, EKeys::O, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_Special_Right})) { G->SetPanelOpen(false); }
	else if (Is({EKeys::F6, EKeys::Gamepad_LeftShoulder})) { G->StepPreset(-1); Selected = 0; }
	else if (Is({EKeys::F7, EKeys::Gamepad_RightShoulder})) { G->StepPreset(1); Selected = 0; }
	else if (Key == EKeys::F8) { G->ToggleReadout(); }
	else
	{
		const FKey Digits[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
		for (int32 i = 0; i < UE_ARRAY_COUNT(Digits); ++i)
		{
			if (Key == Digits[i]) { PickPreset(i); }
		}
	}
	// Nothing else reaches the visitor while the card is up (no walking, no using things behind it).
	return FReply::Handled();
}

FNavigationReply SMuseeSettingsCard::OnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& NavigationEvent)
{
	// The keys and the stick's directions are handled in OnKeyDown; the focus stays on the card.
	return FNavigationReply::Stop();
}

FReply SMuseeSettingsCard::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

void SMuseeSettingsCard::Move(int32 Delta)
{
	const int32 Lines = UMuseeGraphics::Options().Num() + 1;
	Selected = (Selected + Delta + Lines) % Lines;
}

void SMuseeSettingsCard::Change(int32 Delta, bool bWrap)
{
	UMuseeGraphics* G = Graphics.Get();
	if (!G) { return; }
	if (Selected == 0)
	{
		const int32 Index = G->PresetIndex();
		PickPreset(Index == INDEX_NONE ? UMuseeGraphics::DefaultPreset
			: (bWrap ? (Index + Delta + UMuseeGraphics::NumPresets) % UMuseeGraphics::NumPresets : FMath::Clamp(Index + Delta, 0, UMuseeGraphics::NumPresets - 1)));
		return;
	}
	ChangeOption(Selected - 1, Delta, bWrap);
}

void SMuseeSettingsCard::ChangeOption(int32 Option, int32 Delta, bool bWrap)
{
	UMuseeGraphics* G = Graphics.Get();
	const TArray<FMuseeGraphicsOption>& Options = UMuseeGraphics::Options();
	if (!G || !Options.IsValidIndex(Option) || IsLocked(Option)) { return; }
	const FMuseeGraphicsOption& O = Options[Option];
	FMuseeGraphicsState State = G->State();
	const int32 Count = O.Values.Num();
	const int32 Now = O.Get(State);
	const int32 Next = bWrap ? (Now + Delta + Count) % Count : FMath::Clamp(Now + Delta, 0, Count - 1);
	if (Next == Now) { return; }
	O.Set(State, Next);
	G->SetState(State, false);
}

void SMuseeSettingsCard::PickPreset(int32 Index)
{
	if (UMuseeGraphics* G = Graphics.Get())
	{
		if (G->PresetIndex() != Index) { G->ApplyPreset(Index, false); }
	}
	Selected = 0;
}

bool SMuseeSettingsCard::IsLocked(int32 Option) const
{
	const UMuseeGraphics* G = Graphics.Get();
	const FMuseeGraphicsOption& O = UMuseeGraphics::Options()[Option];
	return G && O.LockedBy && !O.LockedBy(G->State()).IsEmpty();
}

FText SMuseeSettingsCard::ValueText(int32 Option) const
{
	const UMuseeGraphics* G = Graphics.Get();
	if (!G) { return FText::GetEmpty(); }
	const FMuseeGraphicsOption& O = UMuseeGraphics::Options()[Option];
	if (O.LockedBy)
	{
		const FText Locked = O.LockedBy(G->State());
		if (!Locked.IsEmpty()) { return Locked; }
	}
	const int32 Value = O.Get(G->State());
	return O.Values.IsValidIndex(Value) ? O.Values[Value] : FText::GetEmpty();
}

// ---------------------------------------------------------------------------------------------------------------

void SMuseeSettingsLayer::Construct(const FArguments& InArgs)
{
	Graphics = InArgs._Graphics;
	const float Safe = SafeMargin;

	ChildSlot
	[
		SNew(SOverlay)
		// The card, over the museum blurred and dimmed like a darkened gallery.
		+ SOverlay::Slot()
		[
			SAssignNew(CardLayer, SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBackgroundBlur)
				.BlurStrength(9.f)
				.bApplyAlphaToBlur(true)
				.Padding(FMargin(0.f))
				[
					SNew(SImage)
					.Image(WhiteBrush())
					.ColorAndOpacity(Alpha(Bronze, 0.46f))
					// A click beside the card is for the card, not the museum behind it.
					.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&)
					{
						return CardWidget.IsValid() ? FReply::Handled().SetUserFocus(CardWidget.ToSharedRef(), EFocusCause::Mouse) : FReply::Handled();
					})
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(Safe, 40.f))
			[
				SAssignNew(CardWidget, SMuseeSettingsCard).Graphics(Graphics)
			]
		]
		// The toast, top right, where nothing else of the museum's lives.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(FMargin(0.f, 56.f, Safe, 0.f))
		[
			MakeToast()
		]
		// The readout, top left.
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(Safe, 56.f, 0.f, 0.f))
		[
			MakeReadout()
		]
	];

	CardLayer->SetVisibility(EVisibility::Collapsed);
	Toast->SetVisibility(EVisibility::Collapsed);
	Readout->SetVisibility(EVisibility::Collapsed);
}

TSharedRef<SWidget> SMuseeSettingsLayer::MakeToast()
{
	TSharedRef<SHorizontalBox> Ladder = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < UMuseeGraphics::NumPresets; ++Index)
	{
		// Five marks from light to heavy; the preset in use is gilt.
		Ladder->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(Index == 0 ? 0.f : 5.f, 0.f, 0.f, 0.f))
		[
			SNew(SBox)
			.WidthOverride_Lambda([this, Index]() { const UMuseeGraphics* G = Graphics.Get(); return FOptionalSize(G && G->PresetIndex() == Index ? 20.f : 10.f); })
			.HeightOverride(2.f)
			[
				SNew(SImage)
				.Image(WhiteBrush())
				.ColorAndOpacity_Lambda([this, Index]()
				{
					const UMuseeGraphics* G = Graphics.Get();
					return FSlateColor(G && G->PresetIndex() == Index ? Gilt : Faint);
				})
			]
		];
	}

	return SAssignNew(Toast, SBox)
		.WidthOverride(ToastWidth)
		[
			SNew(SMuseePanel)
			.Fill(Alpha(Paper, 0.97f))
			.Outline(Alpha(Bronze, 0.08f))
			.OutlineWidth(1.f)
			.Radius(2.f)
			.Shadow(0.3f)
			.Padding(FMargin(28.f, 24.f, 28.f, 22.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ MakeRule(Gilt, 28.f, 2.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f) [ SNullWidget::NullWidget ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Ladder ]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 16.f, 0.f, 0.f))
				[
					Caps(LOCTEXT("ToastKicker", "Graphics"), 9.f, Alpha(Bronze, 0.72f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 5.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { const UMuseeGraphics* G = Graphics.Get(); return UMuseeGraphics::PresetName(G ? G->PresetIndex() : INDEX_NONE); })
					.Font(Font("Italic", 22.f))
					.ColorAndOpacity(Ink)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 5.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { const UMuseeGraphics* G = Graphics.Get(); return G ? UMuseeGraphics::Summary(G->State(), true) : FText::GetEmpty(); })
					.Font(Font("Light", 11.f))
					.ColorAndOpacity(Secondary)
					.WrapTextAt(ToastWidth - 56.f)
					.LineHeightPercentage(1.2f)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 16.f, 0.f, 0.f)) [ MakeRule(Faint, 0.f, 1.f) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 13.f, 0.f, 0.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						Stat(TAttribute<FText>::CreateLambda([this]() { return FpsText(Graphics.Get()); }), LOCTEXT("ToastFps", "fps"), 20.f)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(28.f, 0.f, 0.f, 0.f))
					[
						Stat(TAttribute<FText>::CreateLambda([this]()
							{
								const UMuseeGraphics* G = Graphics.Get();
								return G ? FText::AsNumber(FMath::RoundToInt(G->RenderedFps())) : FText::GetEmpty();
							}),
							LOCTEXT("ToastRendered", "rendered"), 20.f,
							TAttribute<EVisibility>::CreateLambda([this]()
							{
								const UMuseeGraphics* G = Graphics.Get();
								return G && G->IsGenerating() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
							}))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(28.f, 0.f, 0.f, 0.f))
					[
						Stat(TAttribute<FText>::CreateLambda([this]() { return GpuText(Graphics.Get()); }), LOCTEXT("ToastGpu", "ms GPU"), 20.f)
					]
				]
			]
		];
}

TSharedRef<SWidget> SMuseeSettingsLayer::MakeReadout()
{
	// The museum's signage: letter-spaced caps on smoked bronze glass.
	TSharedRef<SWidget> Sign = MakeGlass(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).MinDesiredWidth(58.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::Format(LOCTEXT("ReadFps", "{0} fps"), FpsText(Graphics.Get())); })
				.Font(Font("Medium", SignSize, 160))
				.TransformPolicy(ETextTransformPolicy::ToUpper)
				.ColorAndOpacity(Travertine)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.f, 0.f))
		[
			MakeRule(Alpha(Gilt, 0.8f), 1.f, 10.f)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).MinDesiredWidth(84.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::Format(LOCTEXT("ReadGpu", "{0} ms GPU"), GpuText(Graphics.Get())); })
				.Font(Font("Medium", SignSize, 160))
				.TransformPolicy(ETextTransformPolicy::ToUpper)
				.ColorAndOpacity(Travertine)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.f, 0.f))
		[
			MakeRule(Alpha(Gilt, 0.8f), 1.f, 10.f)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { const UMuseeGraphics* G = Graphics.Get(); return UMuseeGraphics::PresetName(G ? G->PresetIndex() : INDEX_NONE); })
			.Font(Font("Medium", SignSize, 160))
			.TransformPolicy(ETextTransformPolicy::ToUpper)
			.ColorAndOpacity(Alpha(Travertine, 0.72f))
		],
		15.f, FMargin(16.f, 9.f, 18.f, 9.f));
	Readout = Sign;
	return Sign;
}

void SMuseeSettingsLayer::Update(float DeltaSeconds, bool bPhotoMode)
{
	const UMuseeGraphics* G = Graphics.Get();
	if (!G) { return; }
	const bool bOpen = G->IsPanelOpen();
	if (bOpen && !bWasOpen && CardWidget.IsValid()) { CardWidget->ResetSelection(); }
	bWasOpen = bOpen;

	CardAlpha = FMath::FInterpConstantTo(CardAlpha, bOpen ? 1.f : 0.f, DeltaSeconds, bOpen ? 6.f : 8.f);
	Present(*CardLayer, CardAlpha, FVector2f(0.f, 0.f), bOpen ? EVisibility::Visible : EVisibility::HitTestInvisible);
	if (CardWidget.IsValid()) { CardWidget->SetRenderTransform(FSlateRenderTransform(FVector2f(0.f, 14.f) * (1.f - FMath::SmoothStep(0.f, 1.f, CardAlpha)))); }

	// The toast: a change made outside the card, for a few seconds, never over a photograph.
	const double Age = FPlatformTime::Seconds() - G->ChangedAt();
	const bool bToast = !bOpen && !bPhotoMode && Age < 3.2;
	ToastAlpha = FMath::FInterpConstantTo(ToastAlpha, bToast ? 1.f : 0.f, DeltaSeconds, bToast ? 5.f : 2.5f);
	Present(*Toast, ToastAlpha, FVector2f(0.f, -10.f));

	const bool bReadout = G->State().bReadout && !bOpen && !bPhotoMode;
	ReadoutAlpha = FMath::FInterpConstantTo(ReadoutAlpha, bReadout ? 1.f : 0.f, DeltaSeconds, 5.f);
	Present(*Readout, ReadoutAlpha, FVector2f(0.f, -6.f));
}

#undef LOCTEXT_NAMESPACE
