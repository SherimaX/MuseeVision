#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "MuseeCaption.h"
#include "MuseeCaptioned.h"

#include "ArtWorkComponent.generated.h"

/**
 * A hung work's place in its stage: it rides on the work's painted face and carries what the museum says about the
 * work (its wall label) and the care class its light is held to. Visitor's hands find the label here; Light's checks
 * find the class, as the face carries the tag `musee.care:<class>` too.
 */
UCLASS(ClassGroup = Musee)
class MUSEEART_API UArtWorkComponent : public USceneComponent, public IMuseeCaptioned
{
	GENERATED_BODY()

public:
	/** The tag a work's painted face carries with its care class: `musee.care:<class>`. */
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
