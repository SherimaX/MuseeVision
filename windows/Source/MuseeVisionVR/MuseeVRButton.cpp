#include "MuseeVRButton.h"

#include "MuseeVRStyle.h"
#include "Components/WidgetComponent.h"
#include "Visitor/MuseeCharacter.h"

AMuseeVRButton::AMuseeVRButton()
{
	Sign = CreateDefaultSubobject<UWidgetComponent>(TEXT("Sign"));
	RootComponent = Sign;
	MuseeVRStyle::SetUpPanel(*Sign);
	// The look trace (ECC_Visibility) finds it; nothing walks into it.
	Sign->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sign->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sign->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AMuseeVRButton::SetPrompt(const FMuseeActionPrompt& InPrompt, AActor* InProvider)
{
	Prompt = InPrompt;
	Provider = InProvider;
	Sign->SetSlateWidget(MuseeVRStyle::Scaled(MuseeVRStyle::Sign(MuseeVRStyle::PinchKey(), Prompt.Title)));
	Sign->SetRelativeScale3D(FVector(MuseeVRStyle::UnitCm(90.f)));
}

void AMuseeVRButton::Interact(AMuseeCharacter* Visitor, const FHitResult& Hit)
{
	if (IMuseePromptProvider* PromptOwner = Cast<IMuseePromptProvider>(Provider.Get()))
	{
		PromptOwner->RunPrompt(Visitor, Prompt.Id);
	}
}
