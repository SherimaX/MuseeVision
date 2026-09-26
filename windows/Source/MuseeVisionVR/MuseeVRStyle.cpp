#include "MuseeVRStyle.h"

#include "Components/WidgetComponent.h"
#include "UI/MuseeStyle.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void MuseeVRStyle::SetUpPanel(UWidgetComponent& Panel)
{
	Panel.SetWidgetSpace(EWidgetSpace::World);
	Panel.SetDrawAtDesiredSize(true);
	Panel.SetBlendMode(EWidgetBlendMode::Transparent);
	Panel.SetPivot(FVector2D(0.5, 0.5));
	Panel.SetTwoSided(false);
	Panel.SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Panel.SetGenerateOverlapEvents(false);
	Panel.SetCastShadow(false);
	Panel.SetWindowFocusable(false);
	Panel.bReceivesDecals = false;
}

TSharedRef<SWidget> MuseeVRStyle::Scaled(const TSharedRef<SWidget>& Content, float Scale)
{
	return SNew(SDPIScaler).DPIScale(Scale)[Content];
}

FText MuseeVRStyle::PinchKey()
{
	return NSLOCTEXT("Musee", "PinchKey", "PINCH");
}

TSharedRef<SWidget> MuseeVRStyle::Glass(const TSharedRef<SWidget>& Content, float Radius, const FMargin& Padding)
{
	return SNew(SMuseePanel)
		.Fill(MuseeStyle::Alpha(MuseeStyle::Bronze, 0.82f))
		.Outline(MuseeStyle::Alpha(MuseeStyle::Gilt, 0.55f))
		.OutlineWidth(1.f)
		.Radius(Radius)
		.Padding(Padding)
		[
			Content
		];
}

TSharedRef<SWidget> MuseeVRStyle::Sign(const FText& Key, const FText& Title)
{
	// The desktop's sign (MuseeStyle::MakeSign) without the blur.
	return Glass(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			MuseeStyle::MakeKeycap(Key)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.f, 0.f, 0.f, 0.f))
		[
			MuseeStyle::MakeCaps(Title)
		],
		17.f, FMargin(7.f, 7.f, 18.f, 7.f));
}
