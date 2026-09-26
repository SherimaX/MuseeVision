#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visitor/MuseeInteractable.h"
#include "ReserveRacks.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class URectLightComponent;

/**
 * The Reserve's sliding picture screens (MuseePlan::Reserve; the plan's "racks"), in place of the export's wire racks:
 * 36 screens in the aisles, three to a bay in bays 1–6 of each aisle, edge-on to the nave, as in the picture stores
 * and study galleries of the great museums (top-hung sliding screens, the old masters' "picture screens").
 *
 * Each screen is a panel 5.0 m long, 0.10 m thick and 3.33 m tall: a fumed-oak frame (a top rail, a kick rail, and at
 * each end an oak post 0.16 m thick) round two fields of natural linen stretched on boards. It hangs from two bronze
 * trolleys whose wheels run inside its own bronze track overhead (underside 3.58 m); the track is hung from the vault
 * by bronze rods (in the aisle, at the arcade and near the axis) and bolted to the side wall. Two bronze guide fins
 * under the screen run in a bronze-lined slot in the floor (AReserveStructure), so it cannot swing. The nave end post
 * carries a bronze pull and, on screens A–F, the export's letter plate.
 *
 * On both faces, under the top rail, a bronze picture rail; the works hang from it by pairs of slim bronze rods with
 * hooks into the frames' backs. Over each face that carries works, a bronze picture light on two arms, lit only while
 * its screen is out on its stop (its lamp a soft rect light, its lens a strip of lit opal).
 *
 * Pull a screen (look at it and use it, or the prompt when you stand by it) and it glides 5.65 m along its track into
 * the nave, to 1 m off the axis; use it again and it glides home. One screen is out at a time: pulling one sends the
 * others home. A screen never moves into the visitor: it waits while anyone stands in its way. The visit begins with
 * screen C out, its three Renoirs in the nave (rendering 04).
 *
 * The works are the export's (part:reserve_rack_work, framed by Scripts/frames.py): at BeginPlay the lettered screens'
 * imported roots (Rack_A … Rack_F, with their works and letter plates) are attached to the native screens' carriages
 * and move with them. Use a painting on a screen that is out (or the prompt) and it is carried to the viewing easel at
 * the east end; use it there and it goes back to its screen. The visit begins with Le Pont de l'Europe on the easel.
 *
 * ArrangeImported() (Scripts/native.py, in the editor) puts the export's pieces where this layout wants them: the
 * lettered roots at their screens' homes, the works on their faces (MuseePlan::Reserve::RackWorks: west or east,
 * centred 1.55 m up, 35 cm apart; an east face's works turned to face east), the letter plates on the nave end posts,
 * Le Pont de l'Europe on the easel and the Degas pastels on the south plan chests. GetReplacedImportPrims() lists what
 * it retires: the export's rack frames and meshes and the unlettered racks.
 *
 * The screens and tracks are baked (musee.bakeable): the baked meshes hang from the carriages (plain scene components)
 * and glide with them. The picture lights' lenses stay procedural (musee.nobake: shown only while lit). UV0 in metres,
 * U along each timber and each bronze member.
 */
UCLASS()
class MUSEEVISION_API AReserveRacks : public AActor, public IMuseeInteractable, public IMuseePromptProvider
{
	GENERATED_BODY()

public:
	AReserveRacks();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** USD paths of the imported prims this actor replaces: every rack's frame and mesh, and the unlettered racks whole. */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	static TArray<FString> GetReplacedImportPrims();

	/**
	 * Place the export's Reserve pieces for this layout (lettered screens, works, letter plates, the easel's painting,
	 * the pastels). Idempotent; for Scripts/native.py in the editor. Returns how many actors it placed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Musee|Reserve")
	int32 ArrangeImported();

	/** Where a painting's origin (its back, at its centre) goes on the easel (cm, world), by its catalogue id. */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	static FVector EaselPlacement(const FString& WorkId);

	// IMuseeInteractable: a screen (pull it out, push it home) or a painting on one (to the easel and back).
	virtual bool CanInteract(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override;
	virtual void Interact(AMuseeCharacter* Visitor, const FHitResult& Hit) override;
	virtual FText InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override;

	// IMuseePromptProvider: "rack.near" (the screen by you), "rack.<name>" (rack.C, rack.N07, rack.S12; "screen.<name>"
	// too), "easel.<work id>", "easel.return".
	virtual void GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const override;
	virtual void RunPrompt(AMuseeCharacter* Visitor, FName Id) override;

	/** One per screen (MuseePlan::Reserve::Rack(K)): the carriage that glides along its track, at its floor centre. Everything of the screen hangs from it. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USceneComponent>> Carriages;

	/** One per screen, on its carriage. Sections: 0 fumed oak, 1 linen, 2 brushed bronze (rails, pull, trolleys' hangers, fins, picture lights), 3 patinated bronze (rods and hooks). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<UProceduralMeshComponent>> Screens;

	/** The overhead tracks, fixed: 0 brushed bronze (tracks, stops, wall plates), 1 patinated bronze (hanger rods and their canopies). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Tracks;

	/** One per face that carries works (MuseePlan::Reserve::RackWorks), on its screen's carriage: the picture light's lamp. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> PictureLights;

	/** The same faces' picture lights' lenses (lit opal), shown while their lamp is on. Never baked. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<UProceduralMeshComponent>> Lenses;

	/** The screen out when the visit begins ("" for none). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	FString InitialRack;

	/** Seconds for a screen's whole glide. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float GlideSeconds = 4.5f;

	/** A picture light's output (lumens per metre of its length). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float PictureLightLumensPerMetre = 190.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> OakMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> LinenMaterial;

	/** Used for the linen while LinenMaterial doesn't exist yet. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> LinenFallbackMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> PatinaMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> LensMaterial;

	/** A screen's name: A … F, then N07 … N18 and S01 … S18 (west to east). */
	static FString RackName(int32 K);

private:
	void Build();
	void BuildLenses();
	void FindParts();
	void ApplyRack(int32 K);
	void UpdateLights();
	bool Blocked(int32 K, double Offset) const;
	int32 NearestRack(const AMuseeCharacter* Visitor, double MaxDistance) const;
	int32 RackOf(const FHitResult& Hit) const;
	int32 WorkOf(const FHitResult& Hit) const;
	void Toggle(int32 K);
	bool IsOut(int32 K) const;
	void Send(int32 Work);
	void Return();
	void UpdateCarry(float DeltaSeconds);
	FText RackTitle(int32 K) const;
	bool InReserve(const AMuseeCharacter* Visitor) const;

	struct FRackState { double Offset = 0, Target = 0; bool bWaiting = false; TWeakObjectPtr<AActor> Imported; };
	TArray<FRackState> State;

	struct FWork
	{
		TWeakObjectPtr<AActor> Actor;
		int32 Rack = INDEX_NONE;
		FVector HomeLocal = FVector::ZeroVector;   // cm, in its screen's carriage frame
		FQuat HomeRotation = FQuat::Identity;      // world, on its screen (the screens only glide)
		bool bEast = false;
		FString Id;
		FText Title;
		double Height = 1;                         // m, unframed
	};
	TArray<FWork> Works;
	int32 EaselWork = INDEX_NONE;
	int32 PendingEasel = INDEX_NONE;
	/** The world rotation of a painting facing west (on the easel, or on a west face). */
	FQuat WestFacing = FQuat::Identity;

	struct FCarry { int32 Work = INDEX_NONE; bool bToEasel = true; float T = 0.f; FTransform From; };
	TOptional<FCarry> Carry;
	TWeakObjectPtr<AMuseeCharacter> CarryVisitor;

	/** Per picture light: its screen and its current level (0 off … 1 on). */
	TArray<int32> LightRack;
	TArray<float> LightLevel;
};
