#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClassicalHallStructure.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class URectLightComponent;
class USpotLightComponent;

/** A place for a work of art in the classical hall. */
USTRUCT(BlueprintType)
struct FClassicalStatueSpot
{
	GENERATED_BODY()

	/** e.g. Gallery.Niche.E1, Tribune.Niche.N, Gallery.Plinth.2, Tribune.Centrepiece. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FName Name;

	/** Niche, Bust (a console), Herm, Reclining (a long low plinth), Freestanding (a plinth seen in the round), Centrepiece. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FName Kind;

	/**
	 * World transform: the location is the top of the plinth, console or pedestal, at the middle of the place; X is the way
	 * the work should face (out of its niche, into the room), Z up. On a Reclining plinth the long side runs along the
	 * gallery (north-south) and X faces across it.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FTransform Transform;

	/** The tallest work that fits (cm). */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	float SuggestedHeight = 0.f;

	/** The widest work that fits (cm); on a Reclining plinth, the longest. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	float MaxWidth = 0.f;

	/** The cast the place was made for (the classical casts in assets/classical), or empty for a free place. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FString SuggestedWork;
};

/**
 * The classical hall under the Rotunda, reached from the spiral stair's landing (h −6) through the round-arched port in
 * the shaft's wall facing north (MuseePlan::ClassicalHall, MuseePlan::SunClock). Place it at the world origin with no
 * rotation: it is built in plan coordinates.
 *
 * - A short passage (a 1 cm porphyry threshold) from the port into a barrel-vaulted sculpture gallery, 6 m wide and 24 m
 *   long, in four bays: paired Ionic columns of giallo antico on the bay lines carry ressauts of a full Ionic entablature
 *   (fasciae, frieze, dentils, corona, sima); between them statue niches (Pompeian red) and pavonazzetto panels with bust
 *   consoles, over a rosso antico dado. A segmental coffered barrel vault (crown 5.15 m) with a laylight in each bay's crown.
 * - At its north end a screen of two columns under a lintel opens into a round tribune (Ø 13.2 m) after the Sala delle
 *   Muse and the Sala Rotonda: an ambulatory under a coffered annular vault round a ring of twelve pavonazzetto columns
 *   whose entablature carries a coffered dome with a laylit eye over the centrepiece; five statue niches and ten busts in
 *   the outer wall between Ionic pilasters.
 * - Opus sectile floors: porphyry discs in squares of giallo antico and rosso lozenges down the gallery (after the
 *   Pantheon's floor); a radial pattern of discs and bands in the tribune.
 * - Light: the laylights (luminous diffusers, and rect lights behind them at the same luminance), concealed coves on the
 *   cornices' ledges washing the vaults, and a concealed spot on every place for a work. 3800 K throughout.
 *
 * The masonry is one closed surface from the port's lip (5 cm inside the shaft's wall, 1 cm inside the port's faces) to the
 * box outside (its roof at −0.55 m); nothing inside rises above −0.62 m.
 */
UCLASS()
class MUSEEVISION_API AClassicalHallStructure : public AActor
{
	GENERATED_BODY()

public:
	AClassicalHallStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/** Every place for a work of art (niches, consoles, herms, plinths), world transforms; see FClassicalStatueSpot. */
	UFUNCTION(BlueprintPure, Category = "Musee|ClassicalHall")
	TArray<FClassicalStatueSpot> GetStatueSpots() const;

	/** The tribune's pedestal under the eye, for a porphyry basin or a major statue. */
	UFUNCTION(BlueprintPure, Category = "Musee|ClassicalHall")
	FClassicalStatueSpot GetCentrepieceSpot() const;

	/**
	 * With collision. 0: marble walls, reveals, the passage and the lintel's soffit. 1: the dado. 2: the niches. 3–7: the
	 * floors (white, giallo, porphyry, verde, rosso). 8: the closed box outside (no collision). 9: hidden blockers round
	 * columns, plinths and niche mouths (collision only).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Masonry;

	/** 0: the carved white marble (bases, capitals, entablatures, frames, plinths). 1: giallo shafts. 2: pavonazzetto shafts. 3: panels. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Order;

	/** 0: the vaults, the dome and their coffers. 1: the gilt rosettes and the eye's bead. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Vaults;

	/** The laylights' diffusers (self-lit; no shadows). Kept procedural: its material is made at run time. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Laylights;

	/** The laylights' and the coves' area lights, in ClassicalHallGeometry::Lights() order. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> AreaLights;

	/** The concealed accent spots, one or more per place (named Accent_<place>). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> AccentLights;

	/** Off: the laylights are electric daylight at a steady luminance (it is underground). On: they follow the sky (M_Daylit's day and its 60 % night floor), and so do all this actor's lights. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	bool bLaylightsFollowDaylight = false;

	/** Scales every light (and the diffusers' luminance). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light", meta = (ClampMin = "0.0"))
	float LightScale = 1.f;

	/** The accent spots on the places for works of art. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	bool bAccentLights = true;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> WallMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> DadoMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> NicheMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> FloorMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> GialloMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> PorphyryMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> VerdeMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> RossoMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> CarvedMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> PavonazzettoMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> VaultMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> GiltMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> LaylightMaterial;
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> ShellMaterial;

	/** The stone master: the coloured marbles fall back to instances of it (made at load) until materials.py has made them. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials") TSoftObjectPtr<UMaterialInterface> StoneMaster;

private:
	void Build();
	void ApplyMaterials(bool bRuntime);
	void PlaceLights();
	void AddTags();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LaylightGlow;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Fallbacks;
};
