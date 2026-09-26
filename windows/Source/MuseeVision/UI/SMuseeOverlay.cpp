#include "UI/SMuseeOverlay.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Catalog/MuseeCatalog.h"
#include "Rendering/DrawElements.h"
#include "UI/MuseeStyle.h"
#include "UI/SMuseePlacard.h"
#include "Visitor/MuseeCharacter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void SMuseeReticle::SetEngaged(float InEngaged)
{
	if (Engaged != InEngaged)
	{
		Engaged = InEngaged;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

int32 SMuseeReticle::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FLinearColor& Tint = InWidgetStyle.GetColorAndOpacityTint();
	const FVector2f Middle = FVector2f(AllottedGeometry.GetLocalSize()) * 0.5f;

	// A disc centred on the view: filled, or a ring when the fill is transparent.
	auto Disc = [&](int32 Layer, float Diameter, const FLinearColor& Fill, const FLinearColor& Ring)
	{
		const FSlateRoundedBoxBrush DiscBrush(FLinearColor::White, Diameter * 0.5f, Ring * Tint, 1.25f);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(FVector2f(Diameter, Diameter), FSlateLayoutTransform(Middle - FVector2f(Diameter, Diameter) * 0.5f)),
			&DiscBrush, ESlateDrawEffect::None, Fill * Tint);
	};
	Disc(LayerId, 8.f, MuseeStyle::Alpha(MuseeStyle::Bronze, 0.3f), FLinearColor::Transparent);         // the halo
	Disc(LayerId + 1, 4.f, MuseeStyle::Alpha(MuseeStyle::Travertine, 0.95f), FLinearColor::Transparent); // the dot
	if (Engaged > 0.f)
	{
		Disc(LayerId + 1, FMath::Lerp(9.f, 18.f, Engaged), FLinearColor::Transparent, MuseeStyle::Alpha(MuseeStyle::Gilt, 0.95f * Engaged));
	}
	return LayerId + 2;
}

void SMuseeOverlay::Construct(const FArguments& InArgs)
{
	const float Safe = MuseeStyle::SafeMargin;

	HintLabel = MuseeStyle::MakeCaps(FText::GetEmpty());
	HintSign = MuseeStyle::MakeSign(NSLOCTEXT("Musee", "UseKey", "E"), HintLabel.ToSharedRef());

	// The museum's voice: a light caption between two short gilt rules.
	MessageLine = SNew(STextBlock)
		.Font(MuseeStyle::Font("Light", MuseeStyle::MessageSize, 20))
		.ColorAndOpacity(MuseeStyle::Travertine)
		.WrapTextAt(720.f)
		.Justification(ETextJustify::Center);
	MessageSign = MuseeStyle::MakeGlass(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			MuseeStyle::MakeRule(MuseeStyle::Alpha(MuseeStyle::Gilt, 0.85f), 22.f, 1.f)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(16.f, 0.f))
		[
			MessageLine.ToSharedRef()
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			MuseeStyle::MakeRule(MuseeStyle::Alpha(MuseeStyle::Gilt, 0.85f), 22.f, 1.f)
		],
		6.f, FMargin(24.f, 12.f));

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SAssignNew(GalleryLayer, SOverlay)
			// The dot, at the exact centre.
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SAssignNew(Reticle, SMuseeReticle)
			]
			// The hint, centred just under the dot.
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNullWidget::NullWidget
				]
				+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(0.f, 30.f, 0.f, 0.f))
				[
					HintSign.ToSharedRef()
				]
			]
			// The wall label, lower left.
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(FMargin(Safe, 0.f, 0.f, Safe))
			[
				SAssignNew(Placard, SMuseePlacard)
			]
			// The buttons, lower right, all as wide as the widest.
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0.f, 0.f, Safe, Safe))
			[
				SAssignNew(PromptStack, SVerticalBox)
			]
			// The message, top centre.
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(Safe, 56.f, Safe, 0.f))
			[
				MessageSign.ToSharedRef()
			]
		]
		// Photo mode: one discreet line under the picture.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(Safe, 0.f, Safe, 40.f))
		[
			SAssignNew(PhotoLine, STextBlock)
			.Font(MuseeStyle::Font("Light", MuseeStyle::PhotoSize, 150))
			.ColorAndOpacity(MuseeStyle::Alpha(MuseeStyle::Travertine, 0.72f))
			.ShadowOffset(FVector2D(0.0, 1.0))
			.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.45f))
			.Justification(ETextJustify::Center)
			.Visibility(EVisibility::Collapsed)
		]
	];

	HintSign->SetVisibility(EVisibility::Collapsed);
	Placard->SetVisibility(EVisibility::Collapsed);
	PromptStack->SetVisibility(EVisibility::Collapsed);
	MessageSign->SetVisibility(EVisibility::Collapsed);
}

void SMuseeOverlay::Update(const AMuseeCharacter& Visitor, float DeltaSeconds)
{
	const bool bPhotoMode = Visitor.InPhotoMode();
	GalleryLayer->SetVisibility(bPhotoMode ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	PhotoLine->SetVisibility(bPhotoMode && !Visitor.Message().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	if (bPhotoMode)
	{
		// Nothing over the picture but the one line; the gallery's message starts afresh after.
		PhotoLine->SetText(Visitor.Message());
		ShownMessage.Reset();
		MessageAlpha = 0.f;
		return;
	}

	HintAlpha = Crossfade(Visitor.LookHint(), ShownHint, *HintLabel, HintAlpha, DeltaSeconds, 8.f, 10.f);
	Present(*HintSign, HintAlpha, FVector2f(0.f, 4.f));
	Reticle->SetEngaged(FMath::SmoothStep(0.f, 1.f, HintAlpha));

	UpdatePlacard(Visitor);
	UpdatePrompts(Visitor.CurrentPrompts(), DeltaSeconds);

	MessageAlpha = Crossfade(Visitor.Message(), ShownMessage, *MessageLine, MessageAlpha, DeltaSeconds, 3.f, 5.f);
	Present(*MessageSign, MessageAlpha, FVector2f(0.f, -8.f));
}

void SMuseeOverlay::UpdatePlacard(const AMuseeCharacter& Visitor)
{
	// The character fades the label in after a moment's look and out when the eye moves on; it
	// keeps the work while fading out, so the label never empties before it is gone.
	const FMuseeArtwork* Work = Visitor.LookedAtWork();
	if (Work && Work != ShownWork)
	{
		Placard->SetWork(*Work);
		ShownWork = Work;
	}
	Present(*Placard, Work ? Visitor.PlacardAlpha() : 0.f, FVector2f(0.f, 14.f));
}

void SMuseeOverlay::UpdatePrompts(const TArray<FMuseeActionPrompt>& Wanted, float DeltaSeconds)
{
	// Like the lines: new buttons wait for the old ones to fade out.
	if (Wanted.Num() > 0 && PromptAlpha <= 0.f && !SamePrompts(Wanted, ShownPrompts))
	{
		RebuildPrompts(Wanted);
	}
	const bool bUp = Wanted.Num() > 0 && SamePrompts(Wanted, ShownPrompts);
	PromptAlpha = FMath::FInterpConstantTo(PromptAlpha, bUp ? 1.f : 0.f, DeltaSeconds, bUp ? 5.f : 7.f);
	Present(*PromptStack, PromptAlpha, FVector2f(0.f, 10.f));
}

void SMuseeOverlay::RebuildPrompts(const TArray<FMuseeActionPrompt>& Wanted)
{
	PromptStack->ClearChildren();
	for (int32 Row = 0; Row < Wanted.Num(); ++Row)
	{
		const FMuseeActionPrompt& Prompt = Wanted[Row];
		PromptStack->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.Padding(FMargin(0.f, Row == 0 ? 0.f : 10.f, 0.f, 0.f))
			[
				MuseeStyle::MakeSign(FText::AsNumber(Prompt.Slot), MuseeStyle::MakeCaps(Prompt.Title))
			];
	}
	ShownPrompts = Wanted;
}

float SMuseeOverlay::Crossfade(const FText& Wanted, FString& Shown, STextBlock& Line, float Opacity, float DeltaSeconds, float InSpeed, float OutSpeed)
{
	const FString WantedString = Wanted.ToString();
	if (!WantedString.IsEmpty() && Opacity <= 0.f && !WantedString.Equals(Shown, ESearchCase::CaseSensitive))
	{
		Shown = WantedString;
		Line.SetText(Wanted);
	}
	const bool bUp = !WantedString.IsEmpty() && WantedString.Equals(Shown, ESearchCase::CaseSensitive);
	return FMath::FInterpConstantTo(Opacity, bUp ? 1.f : 0.f, DeltaSeconds, bUp ? InSpeed : OutSpeed);
}

void SMuseeOverlay::Present(SWidget& Widget, float Opacity, const FVector2f& Offset)
{
	const float Eased = FMath::SmoothStep(0.f, 1.f, Opacity);
	Widget.SetVisibility(Opacity > 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	Widget.SetRenderOpacity(Eased);
	Widget.SetRenderTransform(FSlateRenderTransform(Offset * (1.f - Eased)));
}

bool SMuseeOverlay::SamePrompts(const TArray<FMuseeActionPrompt>& A, const TArray<FMuseeActionPrompt>& B)
{
	if (A.Num() != B.Num()) { return false; }
	for (int32 Row = 0; Row < A.Num(); ++Row)
	{
		if (A[Row].Id != B[Row].Id || A[Row].Slot != B[Row].Slot
			|| !A[Row].Title.ToString().Equals(B[Row].Title.ToString(), ESearchCase::CaseSensitive))
		{
			return false;
		}
	}
	return true;
}
