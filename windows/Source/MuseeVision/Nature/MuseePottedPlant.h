#pragma once

#include "CoreMinimal.h"
#include "Nature/MuseeNatureActor.h"
#include "MuseePottedPlant.generated.h"

class UCapsuleComponent;

/**
 * A plant in a glazed pot for the Chinese garden (Shared/Wings/ChineseWing.swift: "Orchids at Chunfen,
 * chrysanthemums at Shuangjiang", pots at the foot of the bamboo beds and the rails). The pot is the
 * Swift's (0.12 m at the foot, 0.17 m at the lip, 0.27 m tall) with a rolled lip, soil, moss and pebbles.
 *
 * Orchid: a Cymbidium, a fan of arching, keeled strap leaves that twist as they fall, and slender
 * flower spikes with pale lilac flowers (three sepals, two petals, a spotted lip) at terms 2-5.
 * Chrysanthemum: a mounded plant of many stems with lobed leaves and a dome of golden flower heads
 * (layered ray florets round a disc) at terms 16-19; green buds otherwise.
 */
UCLASS()
class MUSEEVISION_API AMuseePottedPlant : public AMuseeNatureActor
{
	GENERATED_BODY()

public:
	AMuseePottedPlant();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	EMuseePottedPlantKind Kind = EMuseePottedPlantKind::Orchid;

	/** The glaze (the Swift's 0x6B7A7E, a grey-blue celadon). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	FLinearColor PotColour = FLinearColor(0.147f, 0.195f, 0.205f, 1.f);

	/** The flowers' colour (0 = the Swift's: lilac orchids 0xE8D8EE, golden chrysanthemums 0xE6B84A), 0xRRGGBB. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	int32 FlowerColour = 0;

	UPROPERTY(VisibleAnywhere, Category = "Nature")
	TObjectPtr<UCapsuleComponent> PotCollision;

protected:
	virtual void BuildPlant() override;
};
