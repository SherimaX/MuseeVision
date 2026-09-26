#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Nature/NatureTypes.h"
#include "MuseeNatureActor.generated.h"

class UProceduralMeshComponent;
struct FNatureMesh;

/**
 * Base of the native plants (Scripts/nature.py places them). The geometry is grown from a seed
 * whenever the actor is registered or one of its properties changes, into a transient procedural
 * mesh: it is never saved with the map (it would be tens of megabytes), and the same seed always
 * grows the same plant. In a game world it grows at once (when the level loads, before play);
 * in the editor on the next frame, or now with Regrow (which also sizes the collision, which is
 * saved).
 *
 * Season and SolarTerm: what the plant looks like. The Swift build followed the visitor's date;
 * the export (and nature.py) fixes 21 June 2026 at noon UTC, 40° N: summer, Xiazhi (term 9).
 * bFollowToday makes the plant follow today's solar term when play begins instead.
 */
UCLASS(Abstract)
class MUSEEVISION_API AMuseeNatureActor : public AActor
{
	GENERATED_BODY()

public:
	AMuseeNatureActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostRegisterAllComponents() override;
	virtual void BeginPlay() override;

	/** Grow the plant again now (it regrows by itself when a property changes). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Nature")
	void Regrow();

	/** Season (Hall of Light gardens): spring, summer, autumn, winter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	EMuseeNatureSeason Season = EMuseeNatureSeason::Summer;

	/** Solar term 0 (Lichun) … 23 (Dahan) for the Chinese garden; -1 = the middle of Season. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "-1", ClampMax = "23"))
	int32 SolarTerm = -1;

	/** The same seed grows the same plant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	int32 Seed = 1;

	/** Follow today's solar term (hemisphere from the time zone) when play begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	bool bFollowToday = false;

	/** Triangles of the last growth (all sections). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Nature")
	int32 TriangleCount = 0;

	/** The grown geometry (transient, rebuilt from the properties). */
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UProceduralMeshComponent> Plant;

	/** The solar term in effect (SolarTerm, else the middle of Season). */
	int32 EffectiveTerm() const;

	/** The season in effect (from SolarTerm when it is set). */
	EMuseeNatureSeason EffectiveSeason() const;

protected:
	/** Fill the plant's sections (use WriteSection). */
	virtual void BuildPlant() {}

	/** Movable plants (the lilies ride on the lifting pond). */
	virtual bool IsMovablePlant() const { return false; }

	/** Writes one section with its material from /Game/Museum/Nature (or clears it when empty). */
	void WriteSection(int32 Index, const FNatureMesh& Mesh, const TCHAR* MaterialName);

private:
	/** Grow now in a game world; in the editor, once on the next frame (edits and scripts come in bursts). */
	void RequestGrow();
	void EnsureGrown(bool bForce);
	uint32 PropertyKey() const;

	uint32 GrownKey = 0;
	bool bGrowPending = false;
};
