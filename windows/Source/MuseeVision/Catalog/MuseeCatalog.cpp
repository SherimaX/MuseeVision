#include "Catalog/MuseeCatalog.h"

#include "MuseeVision.h"
#include "Catalog/MuseeFiles.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UMuseeCatalog::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadFile(TEXT("data/artworks.json"), TEXT("artworks"));
	LoadFile(TEXT("data/sculptures.json"), TEXT("sculptures"));
	LoadFile(TEXT("assets/collection.json"), TEXT("works"));
	LoadFile(TEXT("data/classical.json"), TEXT("works"));   // the Classical Hall's casts
	LoadFile(TEXT("data/albion.json"), TEXT("works"));      // Albion (the south door): the hang, the book, the glass
	UE_LOG(LogMusee, Log, TEXT("Catalogue: %d works."), Works.Num());
}

FString UMuseeCatalog::Key(const FString& Id)
{
	return Id.Replace(TEXT("-"), TEXT("_")).ToLower();
}

const FMuseeArtwork* UMuseeCatalog::Find(const FString& IdOrPrimName) const
{
	if (const FMuseeArtwork* Work = Works.Find(Key(IdOrPrimName))) { return Work; }
	// Duplicated prims are numbered in build order (name, name_2, …): try without the suffix.
	FString Left, Right;
	if (IdOrPrimName.Split(TEXT("_"), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd) && Right.IsNumeric())
	{
		return Works.Find(Key(Left));
	}
	return nullptr;
}

void UMuseeCatalog::LoadFile(const FString& RepoRelative, const FString& ArrayField)
{
	FString Text;
	if (!MuseeFiles::Load(RepoRelative, Text))
	{
		UE_LOG(LogMusee, Warning, TEXT("Catalogue: %s not found."), *RepoRelative);
		return;
	}
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogMusee, Warning, TEXT("Catalogue: could not read %s."), *RepoRelative);
		return;
	}
	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (!Root->TryGetArrayField(ArrayField, Items)) { return; }
	for (const TSharedPtr<FJsonValue>& Value : *Items)
	{
		const TSharedPtr<FJsonObject> O = Value->AsObject();
		if (!O.IsValid()) { continue; }
		FMuseeArtwork W;
		O->TryGetStringField(TEXT("id"), W.Id);
		O->TryGetStringField(TEXT("artist"), W.Artist);
		O->TryGetStringField(TEXT("title"), W.Title);
		O->TryGetStringField(TEXT("originalTitle"), W.OriginalTitle);
		O->TryGetStringField(TEXT("year"), W.Year);
		O->TryGetStringField(TEXT("medium"), W.Medium);
		O->TryGetStringField(TEXT("collection"), W.Collection);
		O->TryGetStringField(TEXT("city"), W.City);
		O->TryGetStringField(TEXT("notes"), W.Notes);
		if (!W.Id.IsEmpty() && !Works.Contains(Key(W.Id))) { Works.Add(Key(W.Id), W); }
	}
}
