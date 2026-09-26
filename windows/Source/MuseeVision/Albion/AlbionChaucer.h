#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visitor/MuseeInteractable.h"
#include "AlbionChaucer.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class UTexture2D;

/**
 * The Kelmscott Chaucer on its lectern in Morris's bay (Albion, the Details and Hang boards): The Works of Geoffrey
 * Chaucer, Kelmscott Press 1896, open on an oak lectern 1.2 m high; the visitor turns its pages.
 *
 * The book as it is: a folio 42.5 × 29 cm, its block about 7 cm thick in half holland over blue-grey paper boards; open,
 * each page runs down into the gutter and each stack is as thick as the leaves already read (or still to come). A page
 * turns as paper does: it lifts from its lower corner, its free edge trailing in a curl, rolls over the spine and settles
 * on the other stack (TurnSeconds). Its two sides are the leaf's recto and verso, from the scans (Scripts/albion_hang.py
 * imports assets/albion/chaucer/page_NNN.jpg as /Game/Museum/Textures/Albion/Chaucer/T_chaucer_NNN: even pages on the
 * left, odd on the right, page_000 + page_001 the first opening).
 *
 * Use: look at the book and click (or E): the right-hand page turns forward, the left-hand back; or near the lectern the
 * prompts "Turn the page" and "Turn back" (MuseeDo steps chaucer.next and chaucer.prev). Place it with its actor's origin
 * at the lectern's foot, its +X the way the reader faces (Scripts/native.py places it in Morris's bay facing east).
 * The lectern and the book's block may be baked; the pages stay procedural (musee.nobake on them).
 */
UCLASS()
class MUSEEVISION_API AAlbionChaucer : public AActor, public IMuseeInteractable, public IMuseePromptProvider
{
	GENERATED_BODY()

public:
	AAlbionChaucer();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// IMuseeInteractable
	virtual bool CanInteract(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override;
	virtual void Interact(AMuseeCharacter* Visitor, const FHitResult& Hit) override;
	virtual FText InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override;
	// IMuseePromptProvider
	virtual void GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const override;
	virtual void RunPrompt(AMuseeCharacter* Visitor, FName Id) override;

	/** Turn forward (+1) or back (−1), if a turn isn't under way and there is a page to turn. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Albion")
	bool TurnPage(int32 Direction);

	/** The lectern (oak). With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Lectern;

	/** The book's boards, spine and the two stacks' edges. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Block;

	/** The open pages (section 0 the left, 1 the right) and the turning leaf (2 its recto, 3 its verso). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Pages;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> OakMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BoardMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> ClothMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> EdgeMaterial;

	/** The page material (a "Page" texture parameter). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> PageMaterial;

	/** Where the page scans are: <Folder>/T_chaucer_000 … */
	UPROPERTY(EditAnywhere, Category = "Musee")
	FString PageFolder = TEXT("/Game/Museum/Textures/Albion/Chaucer");

	/** The opening shown: pages 2 × Opening (left) and 2 × Opening + 1 (right). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	int32 Opening = 0;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float TurnSeconds = 1.6f;

	/** How near the lectern (m) the prompts are offered. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float PromptDistance = 2.4f;

private:
	void BuildStatic();
	void BuildPages();
	void LoadPages();
	void ApplyPageTextures();
	UTexture2D* PageTexture(int32 Index) const;
	bool Near(const AMuseeCharacter* Visitor) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> PageTextures;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PageMIDs;

	int32 TurnDirection = 0;     // +1 forward, −1 back, 0 at rest
	float TurnT = 0.f;           // 0 … 1
};
