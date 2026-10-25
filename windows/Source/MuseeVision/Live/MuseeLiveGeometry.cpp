#include "Live/MuseeLiveGeometry.h"
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
}

void UMuseeLiveGeometry::Tick(float DeltaTime)
{
	if (!bActive) { return; }
	const double Now = FPlatformTime::Seconds();
	if (Now < NextPoll) { return; }
	NextPoll = Now + 0.25;
	const FString Path = ChenghuaiTune::Path();
	const FFileStatData S = IFileManager::Get().GetStatData(*Path);
	const FString K = S.bIsValid ? FString::Printf(TEXT("%lld:%lld"), S.ModificationTime.GetTicks(), S.FileSize) : TEXT("missing");
	if (K == Key) { return; }
	Key = K;
	ChenghuaiTune::Load();
	double Ms = 0.0;
	int32 Built = 0;
	for (TActorIterator<AChenghuaiStructure> It(GetWorld()); It; ++It)
	{
		Ms += It->RebuildLive();
		++Built;
	}
	const TArray<FString> Unused = ChenghuaiTune::WriteKeys();
	if (UMuseeLiveLook* Look = GetWorld()->GetSubsystem<UMuseeLiveLook>()) { Look->Reapply(); }   // its copies on the new sections
	++Applied;
	UE_LOG(LogMusee, Display, TEXT("live: Chenghuai rebuilt in %.0f ms (%d actor%s)%s"), Ms, Built, Built == 1 ? TEXT("") : TEXT("s"),
		Unused.Num() ? *FString::Printf(TEXT("; not used: %s"), *FString::Join(Unused, TEXT(", "))) : TEXT(""));
	TSharedRef<FJsonObject> State = MakeShared<FJsonObject>();
	State->SetNumberField(TEXT("applied"), Applied);
	State->SetNumberField(TEXT("ms"), FMath::RoundToDouble(Ms));
	State->SetNumberField(TEXT("actors"), Built);
	TArray<TSharedPtr<FJsonValue>> U;
	for (const FString& N : Unused) { U.Add(MakeShared<FJsonValueString>(N)); }
	State->SetArrayField(TEXT("unused"), U);
	FString Text;
	FJsonSerializer::Serialize(State, TJsonWriterFactory<>::Create(&Text));
	FFileHelper::SaveStringToFile(Text, *FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Live/geometry_state.json")),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
