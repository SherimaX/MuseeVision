#include "Live/MuseeLiveGeometry.h"
#include "Live/MuseeLiveBake.h"
#include "Live/MuseeLiveLook.h"
#include "Chenghuai/ChenghuaiStructure.h"
#include "Chenghuai/ChenghuaiTune.h"
#include "MuseeVision.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool UMuseeLiveGeometry::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("Live"))) { return false; }
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE);
}

void UMuseeLiveGeometry::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bActive = true;
	Key = TEXT("missing");   // no file at the start: the wing stays as it was built (and baked)
	const FFileStatData B = IFileManager::Get().GetStatData(*FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Live/geo_bake.txt")));
	BakeKey = B.bIsValid ? FString::Printf(TEXT("%lld:%lld"), B.ModificationTime.GetTicks(), B.FileSize) : TEXT("missing");   // an old request isn't taken
}

void UMuseeLiveGeometry::Tick(float DeltaTime)
{
	if (!bActive) { return; }
	const double Now = FPlatformTime::Seconds();
	if (Now < NextPoll) { return; }
	NextPoll = Now + 0.25;
	auto StatKey = [](const FString& P)
	{
		const FFileStatData S = IFileManager::Get().GetStatData(*P);
		return S.bIsValid ? FString::Printf(TEXT("%lld:%lld"), S.ModificationTime.GetTicks(), S.FileSize) : FString(TEXT("missing"));
	};
	const FString K = StatKey(ChenghuaiTune::Path());
	if (K != Key)
	{
		Key = K;
		ChenghuaiTune::Load();
		LastMs = 0.0;
		int32 Built = 0;
		for (TActorIterator<AChenghuaiStructure> It(GetWorld()); It; ++It)
		{
			MuseeLiveBake::Unbake(*It);   // the last live bake's meshes are of the old shapes
			LastMs += It->RebuildLive();
			++Built;
		}
		Baked = 0;
		Unused = ChenghuaiTune::WriteKeys();
		if (UMuseeLiveLook* Look = GetWorld()->GetSubsystem<UMuseeLiveLook>()) { Look->Reapply(); }   // its copies on the new sections
		++Applied;
		UE_LOG(LogMusee, Display, TEXT("live: Chenghuai rebuilt in %.0f ms (%d actor%s)%s"), LastMs, Built, Built == 1 ? TEXT("") : TEXT("s"),
			Unused.Num() ? *FString::Printf(TEXT("; not used: %s"), *FString::Join(Unused, TEXT(", "))) : TEXT(""));
		WriteState();
		return;
	}
	const FString BK = StatKey(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Live/geo_bake.txt")));
	if (BK != BakeKey)
	{
		BakeKey = BK;
		const double T0 = FPlatformTime::Seconds();
		Baked = 0;
		for (TActorIterator<AChenghuaiStructure> It(GetWorld()); It; ++It) { Baked += MuseeLiveBake::Bake(*It); }
		BakeMs = (FPlatformTime::Seconds() - T0) * 1000.0;
		if (UMuseeLiveLook* Look = GetWorld()->GetSubsystem<UMuseeLiveLook>()) { Look->Reapply(); }   // its copies on the baked meshes
		++Applied;
		UE_LOG(LogMusee, Display, TEXT("live: Chenghuai baked live in %.0f ms (%d meshes, distance fields and cards)"), BakeMs, Baked);
		WriteState();
	}
}

void UMuseeLiveGeometry::WriteState()
{
	TSharedRef<FJsonObject> State = MakeShared<FJsonObject>();
	State->SetNumberField(TEXT("applied"), Applied);
	State->SetNumberField(TEXT("ms"), FMath::RoundToDouble(LastMs));
	State->SetNumberField(TEXT("baked"), Baked);
	State->SetNumberField(TEXT("bake_ms"), FMath::RoundToDouble(BakeMs));
	TArray<TSharedPtr<FJsonValue>> U;
	for (const FString& N : Unused) { U.Add(MakeShared<FJsonValueString>(N)); }
	State->SetArrayField(TEXT("unused"), U);
	FString Text;
	FJsonSerializer::Serialize(State, TJsonWriterFactory<>::Create(&Text));
	FFileHelper::SaveStringToFile(Text, *FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Live/geometry_state.json")),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
