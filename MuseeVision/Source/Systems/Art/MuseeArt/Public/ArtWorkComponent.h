#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "MuseeCaption.h"
#include "MuseeCaptioned.h"

#include "ArtWorkComponent.generated.h"

/**
 * A hung work's place in its stage: the component a work's pieces hang from (a canvas's face, linen and frame; a
 * panel's glass; what the stage built for it), carrying what the museum says about the work (its label) and the care
 * class its light is held to. The visitor's hands find the label by looking up from the piece they see; Light's checks
 * find the class, as each piece carries the tag `musee.care:<class>` too.
 */
UCLASS(ClassGroup = Musee)
class MUSEEART_API UArtWorkComponent : public USceneComponent, public IMuseeCaptioned
{
	GENERATED_BODY()

public:
	/** The tag a work's pieces carry with its care class: `musee.care:<class>`. */
	static FName CareTag(FName CareClass);

	/** The tag a work's pieces carry with its id: `musee.work:<id>`. */
	static FName WorkTag(const FString& WorkId);

	/** Sets the work's id, care class and label. Making time only. */
	void Describe(const FString& InWorkId, FName InCare, const FMuseeCaption& InCaption);

	[[nodiscard]] const FString& WorkId() const { return Work; }
	[[nodiscard]] FName CareClass() const { return Care; }

	virtual FMuseeCaption Caption() const override { return Label; }

private:
	UPROPERTY()
	FString Work;

	UPROPERTY()
	FName Care;

	UPROPERTY()
	FMuseeCaption Label;
};
