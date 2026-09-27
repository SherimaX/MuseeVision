#include "VisitorOverlay.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MuseeVisitor"

namespace Musee::Visitor
{
	namespace
	{
		/**
		 * Silent reading runs at about 238 words a minute for adults reading non-fiction (M. Brysbaert, "How many words
		 * do we read per minute?", Journal of Memory and Language 109, 2019); a message stays long enough to be read
		 * twice, once at a glance and once to be sure.
		 */
		constexpr double WordsPerMinute = 238.0;
		constexpr double Readings = 2.0;
		constexpr double SecondsPerMinute = 60.0;

		/** The overlay's text stands clear of the screen's lower edge by the height of a few lines of its type. */
		constexpr float LinesClearOfEdge = 4.0f;
		constexpr float LineHeightPixels = 24.0f;

		double ReadingSeconds(const FText& Text)
		{
			TArray<FString> Words;
			Text.ToString().ParseIntoArrayWS(Words);
			return Readings * Words.Num() * SecondsPerMinute / WordsPerMinute;
		}

		FText Lines(std::initializer_list<FText> Parts)
		{
			TArray<FString> Kept;
			for (const FText& Part : Parts)
			{
				if (!Part.IsEmpty()) { Kept.Add(Part.ToString()); }
			}
			return FText::FromString(FString::Join(Kept, TEXT("\n")));
		}
	}
}

UVisitorOverlay::UVisitorOverlay()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UVisitorOverlay::ShowCaption(const FMuseeCaption& Caption)
{
	Subject = Musee::Visitor::Lines({ Caption.Title, Caption.Body, Caption.Credit });
	Present();
}

void UVisitorOverlay::ShowUse(const FMuseeUse& Use)
{
	const FText Offer = Use.bAvailable
		? FText::Format(LOCTEXT("UseOffer", "{0}: {1}"), Use.Verb, Use.Caption)
		: Use.WhyNot;
	Subject = Musee::Visitor::Lines({ Offer });
	Present();
}

void UVisitorOverlay::Clear()
{
	if (Subject.IsEmpty()) { return; }
	Subject = FText::GetEmpty();
	Present();
}

void UVisitorOverlay::ShowMessage(const FText& InMessage)
{
	Message = InMessage;
	MessageSeconds = Musee::Visitor::ReadingSeconds(InMessage);
	Present();
}

FText UVisitorOverlay::Text() const
{
	return Musee::Visitor::Lines({ Subject, Message });
}

void UVisitorOverlay::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Pass(Musee::Seconds(DeltaTime));
}

void UVisitorOverlay::Pass(Musee::Seconds Time)
{
	if (Message.IsEmpty()) { return; }
	MessageSeconds -= Time.Get();
	if (MessageSeconds <= 0.0)
	{
		Message = FText::GetEmpty();
		Present();
	}
}

void UVisitorOverlay::BeginPlay()
{
	Super::BeginPlay();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || !Pawn || !Pawn->IsLocallyControlled()) { return; }
	Widget = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, Musee::Visitor::LinesClearOfEdge * Musee::Visitor::LineHeightPixels))
		[
			SAssignNew(TextBlock, STextBlock)
			.Justification(ETextJustify::Center)
			.ShadowOffset(FVector2D(1.0, 1.0))
		];
	Viewport->AddViewportWidgetContent(Widget.ToSharedRef());
	Present();
}

void UVisitorOverlay::EndPlay(EEndPlayReason::Type Reason)
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (Viewport && Widget.IsValid()) { Viewport->RemoveViewportWidgetContent(Widget.ToSharedRef()); }
	Widget.Reset();
	TextBlock.Reset();
	Super::EndPlay(Reason);
}

void UVisitorOverlay::Present()
{
	if (TextBlock.IsValid()) { TextBlock->SetText(Text()); }
}

#undef LOCTEXT_NAMESPACE
