#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MuseeCaption.h"
#include "MuseeUnits.h"
#include "MuseeUse.h"

#include "VisitorOverlay.generated.h"

class STextBlock;
class SWidget;

/**
 * What the museum says to the visitor on the screen, outside the scene: the caption of the thing they look at, what
 * using it will do, and a short message after a key (the hour, the city). Nothing else is drawn over the museum.
 */
UCLASS(ClassGroup = (Musee))
class MUSEEVISITOR_API UVisitorOverlay final : public UActorComponent
{
	GENERATED_BODY()

public:
	UVisitorOverlay();

	/** Shows a thing's caption: its title, body and credit. */
	void ShowCaption(const FMuseeCaption& Caption);

	/** Shows what using a thing will do, or why it can't be used now. */
	void ShowUse(const FMuseeUse& Use);

	/** Shows nothing about a thing. */
	void Clear();

	/** Shows a message for as long as it takes to read it twice. */
	void ShowMessage(const FText& Message);

	/** Lets time pass for the message on the screen, which goes once it has been read twice. */
	void Pass(Musee::Seconds Time);

	/** The text on the screen now, lines joined by line breaks. */
	[[nodiscard]] FText Text() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
	void Present();

	FText Subject;
	FText Message;
	double MessageSeconds = 0.0;
	TSharedPtr<STextBlock> TextBlock;
	TSharedPtr<SWidget> Widget;
};
