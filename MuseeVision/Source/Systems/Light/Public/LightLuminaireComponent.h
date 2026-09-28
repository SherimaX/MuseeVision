#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LightOperation.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

#include "LightLuminaireComponent.generated.h"

class ULocalLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPrimitiveComponent;
class UTextureLightProfile;
struct FMuseeChannel;
struct FTimeMoment;

/**
 * One luminaire in the museum: a catalogue type (Data/Light/Luminaires) mounted where a stage's lighting schedule puts
 * it, with its light leaving along the component's x axis (a linear luminaire lies along its y axis, centred on the
 * component; the lamps of an electrolier stand on a ring about its x axis; an areal face lies square to x, its width
 * along y and its height along z, unless it encloses the component, a sphere's inside about it). It lights at its catalogue output, switched by its operation (always, at dusk, on
 * presence) and dimmed by the channel it follows (0 to 1); an image panel gives the light of the picture it shows, and
 * stands at its black level until one is shown. An image panel laid over a sphere's inside about it (the Sphere's) is
 * shown on a surface of that sphere, which gives the light itself, emissive in Light's image material, showing the
 * real sky of Time's moment (LightImage.h). A type lit by its maker's photometric file (a wallwasher) gives that file's
 * candela table through the light profile made from it: the file's nadir, its optical axis, along x, and the file's 0°
 * plane on the side of z, the side the luminaire tilts towards, which for a wallwasher aimed at its wall is the wall's.
 * Nothing else sets how much light it gives. Light's stage dresser places it.
 */
UCLASS(ClassGroup = (Musee))
class MUSEELIGHT_API ULightLuminaireComponent final : public USceneComponent
{
	GENERATED_BODY()

public:
	ULightLuminaireComponent();

	/**
	 * Sets what the luminaire is and how it is run: its catalogue type, its operation, the channel it follows (the none
	 * key for none), for a linear type the length laid, and for an areal type the face it lights, [width, height] in
	 * metres (zero for any other), and, when that face encloses it, the sphere's inside about it: [radius, the radius of
	 * the opening at its foot] in metres (zero for none).
	 */
	void Configure(const FMuseeKey& InType, ELightOperation InOperation, const FMuseeKey& InFollows, Musee::Meters InLength,
		const FVector2D& InFaceMeters = FVector2D::ZeroVector, const FVector2D& InSphereMeters = FVector2D::ZeroVector);

	[[nodiscard]] const FMuseeKey& Type() const { return LuminaireType; }
	[[nodiscard]] ELightOperation Operation() const { return LuminaireOperation; }
	[[nodiscard]] const FMuseeKey& Follows() const { return FollowedChannel; }
	[[nodiscard]] Musee::Meters Length() const { return Musee::Meters(LengthMeters); }
	/** The face an areal type lights, [width, height] in metres; zero for any other. */
	[[nodiscard]] const FVector2D& Face() const { return FaceMeters; }

	/**
	 * Whether its face encloses it: the inside of a sphere about it, its light converging from every side (the Sphere's
	 * image). No engine light stands for such a face, since each shines out from one place, so it drives none: its light
	 * leaves the surface it is laid over (ImageSurface), which shows the sky.
	 */
	[[nodiscard]] bool FaceEncloses() const { return SphereMeters.X > 0.0; }

	/** The sphere its face is the inside of, [radius, opening] in metres; zero when its face doesn't enclose it. */
	[[nodiscard]] const FVector2D& Sphere() const { return SphereMeters; }

	/**
	 * The surface an enclosing image is shown on: the sphere's inside less its opening, facing in, emissive in Light's
	 * image material, which Studio makes; null for any other luminaire, or while the material isn't made.
	 */
	[[nodiscard]] UPrimitiveComponent* ImageSurface() const { return Surface; }

	/** The light profile made from its type's photometric file, or null for a type lit otherwise (or not yet made). */
	[[nodiscard]] const UTextureLightProfile* LightProfile() const { return Profile; }

	/** The fraction of its catalogue output it gives now, 0 to 1: its switch times the channel it follows. */
	[[nodiscard]] double Level() const;

	/** The luminous flux it emits now: its engine lights', or its image surface's, pi times its picture's mean luminance times its area. */
	[[nodiscard]] Musee::Lumens EmittedFlux() const;

	/** The engine light it drives (its first lamp's, for a luminaire of several lamps); null for a type Light doesn't light. */
	[[nodiscard]] ULocalLightComponent* Emitter() const { return EmitterLights.IsEmpty() ? nullptr : EmitterLights[0].Get(); }

	/** Every engine light it drives: one, or one per lamp of a luminaire of several lamps. */
	[[nodiscard]] const TArray<TObjectPtr<ULocalLightComponent>>& Emitters() const { return EmitterLights; }

	/**
	 * Takes a moment: switches a dusk luminaire, looks for the visitor for a presence luminaire, and turns an enclosing
	 * image's sky to the moment's.
	 */
	void FollowMoment(const FTimeMoment& Moment);

	/** Takes a value of the channel it follows. */
	void FollowChannel(float Value);

	/**
	 * For an image panel, the picture it shows now, as its mean luminance over its peak white (0 to 1): the work whose
	 * image source feeds the panel sets it. 0, its black level, is the standby picture.
	 */
	void ShowImage(double MeanLuminanceShare);

protected:
	virtual void OnRegister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
	void MakeEmitters();
	void MakeImageSurface();
	void DestroyEmitters();
	void FollowSky(const FTimeMoment& Moment);
	void Apply();
	void HearChannel(const FMuseeChannel& Channel);
	bool SeesVisitor() const;

	/** The catalogue type. */
	UPROPERTY(EditAnywhere, Category = "Luminaire")
	FMuseeKey LuminaireType;

	UPROPERTY(EditAnywhere, Category = "Luminaire")
	ELightOperation LuminaireOperation = ELightOperation::Always;

	/** The channel whose value (0 to 1) dims it; the none key for none. */
	UPROPERTY(EditAnywhere, Category = "Luminaire")
	FMuseeKey FollowedChannel;

	/** The length laid, for a linear type. */
	UPROPERTY(EditAnywhere, Category = "Luminaire", meta = (Units = "Meters"))
	double LengthMeters = 0.0;

	/** The face an areal type lights, [width, height] in metres. */
	UPROPERTY(EditAnywhere, Category = "Luminaire")
	FVector2D FaceMeters = FVector2D::ZeroVector;

	/** The sphere whose inside its face is, [radius, opening] in metres; zero when its face doesn't enclose it. */
	UPROPERTY(EditAnywhere, Category = "Luminaire")
	FVector2D SphereMeters = FVector2D::ZeroVector;

	/**
	 * Light's image material, which Studio made, found when an enclosing image is configured and kept with the stage, so
	 * the level carries the material (and its star field) its image surface shows.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Luminaire")
	TObjectPtr<UMaterialInterface> ImageMaterial;

	/**
	 * The light profile Studio made from its type's photometric file, found when it is configured and kept with the stage,
	 * so the level carries the profile it is lit by.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Luminaire")
	TObjectPtr<UTextureLightProfile> Profile;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULocalLightComponent>> EmitterLights;

	/** An enclosing image's surface, and the instance of the image material it shows. */
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Surface;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Picture;

	bool bSwitchedOn = true;
	double ChannelLevel = 1.0;
	/** An image panel's picture: its mean luminance over its peak white. */
	double ImageLevel = 0.0;
	/** The world time, in seconds, at which the presence detector last saw the visitor. */
	TOptional<double> LastSeenSeconds;
	FDelegateHandle MomentHandle;
	FDelegateHandle ChannelHandle;
};
