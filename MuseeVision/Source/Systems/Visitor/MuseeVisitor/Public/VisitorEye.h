#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Scene.h"
#include "MuseeUnits.h"

#include "VisitorEye.generated.h"

class UPostProcessComponent;

/**
 * The visitor's eye: the museum's one camera model. It owns the one unbound post-process volume that sets camera
 * fields, and only camera fields: exposure metered from the scene's luminance in EV100 (the physically based camera of
 * the engine's extended luminance range), adapting at the eye's speed between the dimmest mesopic scene and a sunlit
 * white, and the tone curve with a highlight shoulder, so skies and glass roll off instead of clipping. Nothing in a
 * stage sets exposure; walking from a dim bay into a bright room is an adaptation the eye makes. The body creates it.
 */
UCLASS(ClassGroup = (Musee))
class MUSEEVISITOR_API UVisitorEye final : public UActorComponent
{
	GENERATED_BODY()

public:
	/** The camera fields the Eye sets, and only those. */
	[[nodiscard]] static FPostProcessSettings CameraSettings();

	/** The Eye's unbound post-process volume, made when the Eye is registered. */
	[[nodiscard]] UPostProcessComponent* Volume() const { return EyeVolume; }

	/** The exposure value of a scene luminance at ISO 100, by the reflected-light meter equation of ISO 2720. */
	[[nodiscard]] static Musee::EV100 ExposureOf(Musee::Nits Luminance);

	/** The darkest and brightest scenes the eye meters: the lower bound of mesopic vision, and a white surface in full sun. */
	[[nodiscard]] static Musee::EV100 DarkestExposure();
	[[nodiscard]] static Musee::EV100 BrightestExposure();

protected:
	virtual void OnRegister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> EyeVolume;
};
