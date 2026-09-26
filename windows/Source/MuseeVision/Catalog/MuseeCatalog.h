#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MuseeCatalog.generated.h"

/** A work for the placards (Shared/Core/Artworks.swift). */
USTRUCT(BlueprintType)
struct FMuseeArtwork
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Id;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Artist;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Title;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString OriginalTitle;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Year;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Medium;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Collection;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString City;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Notes;
};

/**
 * The catalogues, bundled as-is: data/artworks.json (paintings), data/sculptures.json and
 * assets/collection.json (the Chinese Wing, the Hall of Light, The Starry Night).
 */
UCLASS()
class MUSEEVISION_API UMuseeCatalog : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** A work by id. USD prim names ("monet_impression_sunrise") match their ids with - for _. */
	const FMuseeArtwork* Find(const FString& IdOrPrimName) const;

	int32 Num() const { return Works.Num(); }

private:
	void LoadFile(const FString& RepoRelative, const FString& ArrayField);
	static FString Key(const FString& Id);

	TMap<FString, FMuseeArtwork> Works;
};
