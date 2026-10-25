#include "Live/MuseeLiveShots.h"
#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "Visitor/MuseeCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "RHI.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"

namespace
{
double Field(const FJsonObject& O, const TCHAR* Key, double Default)
{
	double V;
	return O.TryGetNumberField(Key, V) ? V : Default;
}

bool Flag(const FJsonObject& O, const TCHAR* Key, bool bDefault)
{
	bool B;
	return O.TryGetBoolField(Key, B) ? B : bDefault;
}

IConsoleVariable* HourVar() { return IConsoleManager::Get().FindConsoleVariable(TEXT("musee.Hour")); }
}

bool UMuseeLiveShots::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("Live"))) { return false; }
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE);
}

void UMuseeLiveShots::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	LiveDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Live"));
	IFileManager::Get().MakeDirectory(*LiveDir, true);
	// a request left over from an earlier run is not taken again
	FString Text;
	TSharedPtr<FJsonObject> J;
	if (FFileHelper::LoadFileToString(Text, *(LiveDir / TEXT("shots.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) && J)
	{
		J->TryGetStringField(TEXT("id"), DoneId);
	}
	bActive = true;
}

void UMuseeLiveShots::Tick(float DeltaTime)
{
	if (!bActive) { return; }
	if (Index < 0)
	{
		const double Now = FPlatformTime::Seconds();
		if (Now < NextPoll) { return; }
		NextPoll = Now + 0.25;
		ReadRequest();
		return;
	}
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) { return; }   // the visitor isn't in yet
	const FShot& S = Shots[Index];
	if (Stage == 0)
	{
		Pose(S);
		Stage = 1;
		Timer = 0.f;
		return;
	}
	if (Stage == 1)
	{
		// the settle starts once the shaders are done (the first look at a room, or after a material change)
		if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) { Timer = FMath::Min(Timer, 0.f); return; }   // keeps a warm-up
		Timer += DeltaTime;
		// a path-traced shot starts in raster, so what the sky and the room blend in on arrival has settled before the
		// path tracer starts accumulating
		if (S.bPhoto && bPhotoPending)
		{
			if (Timer < (bHourChanged ? HourLead : RasterLead)) { return; }
			SetPhoto(true);
			bPhotoPending = false;
			// the first path-traced shot of a game never converged in its settle (the progress bar near its start);
			// later ones did, so the first gets a warm-up on top
			Timer = bPhotoWarm ? 0.f : -PhotoWarmUp;
			bPhotoWarm = true;
			return;
		}
		if (Timer < S.Settle) { return; }
		// perf: frame and GPU times at the view for a few seconds before the picture
		if (S.Perf > 0.f && Timer < S.Settle + S.Perf)
		{
			PerfFrame.Add(DeltaTime * 1000.f);
			PerfGpu.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0)));
			return;
		}
		if (S.Perf > 0.f) { RecordPerf(S.Name); }
		const FString Path = OutDir / S.Name + TEXT(".png");
		IFileManager::Get().Delete(*Path, false, true, true);
		FScreenshotRequest::RequestScreenshot(Path, false, false);
		Stage = 2;
		Timer = 0.f;
		return;
	}
	// Stage 2: the PNG is on disk (or it never came)
	Timer += DeltaTime;
	const FString Path = OutDir / S.Name + TEXT(".png");
	const bool bThere = IFileManager::Get().FileSize(*Path) > 0;
	if (!bThere && Timer < 15.f) { return; }
	if (bThere) { Files.Add(Path); }
	UE_LOG(LogMusee, Display, TEXT("live: shot %s %s"), *S.Name, bThere ? TEXT("saved") : TEXT("failed (no PNG after 15 s)"));
	if (++Index < Shots.Num()) { Stage = 0; }
	else { Finish(); }
}

void UMuseeLiveShots::ReadRequest()
{
	const FString Path = LiveDir / TEXT("shots.json");
	const FFileStatData Stat = IFileManager::Get().GetStatData(*Path);
	const FString K = Stat.bIsValid ? FString::Printf(TEXT("%lld:%lld"), Stat.ModificationTime.GetTicks(), Stat.FileSize) : FString();
	if (K == RequestKey) { return; }
	FString Text;
	TSharedPtr<FJsonObject> J;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J) { return; }   // mid-save: next poll
	RequestKey = K;
	FString NewId;
	if (!J->TryGetStringField(TEXT("id"), NewId) || NewId.IsEmpty() || NewId == DoneId) { return; }
	const bool bPhoto = Flag(*J, TEXT("photo"), false);
	const bool bPreview = Flag(*J, TEXT("preview"), false);
	const int32 Spp = (int32)Field(*J, TEXT("spp"), 64.0);
	const float Fov = Field(*J, TEXT("fov"), 0.0);
	const float Perf = Field(*J, TEXT("perf"), 0.0);
	auto DefaultSettle = [](bool bP, bool bQuick) { return bP ? (bQuick ? 40.0 : 480.0) : 6.0; };
	const bool bSettleGiven = J->HasField(TEXT("settle"));
	const float Settle = Field(*J, TEXT("settle"), DefaultSettle(bPhoto, bPreview));
	Shots.Reset();
	const TArray<TSharedPtr<FJsonValue>>* List;
	if (J->TryGetArrayField(TEXT("shots"), List))
	{
		for (const TSharedPtr<FJsonValue>& V : *List)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			const TArray<TSharedPtr<FJsonValue>>* P;
			if (!O || !O->TryGetArrayField(TEXT("pose"), P) || P->Num() < 4) { continue; }
			FShot S;
			O->TryGetStringField(TEXT("name"), S.Name);
			if (S.Name.IsEmpty()) { S.Name = FString::Printf(TEXT("shot_%02d"), Shots.Num()); }
			S.X = (*P)[0]->AsNumber();
			S.Y = (*P)[1]->AsNumber();
			S.Feet = (*P)[2]->AsNumber();
			S.Yaw = (*P)[3]->AsNumber();
			S.Pitch = P->Num() > 4 ? (*P)[4]->AsNumber() : 0.0;
			S.Hour = Field(*O, TEXT("hour"), -2.0);
			S.Fov = Field(*O, TEXT("fov"), Fov);
			S.bPhoto = Flag(*O, TEXT("photo"), bPhoto);
			S.bPreview = S.bPhoto && Flag(*O, TEXT("preview"), bPreview);
			S.Spp = FMath::Max(1, (int32)Field(*O, TEXT("spp"), Spp));
			S.Settle = Field(*O, TEXT("settle"), bSettleGiven && S.bPhoto == bPhoto && S.bPreview == bPreview ? Settle : DefaultSettle(S.bPhoto, S.bPreview));
			S.Perf = Field(*O, TEXT("perf"), Perf);
			Shots.Add(S);
		}
	}
	DoneId = NewId;
	if (Shots.IsEmpty()) { UE_LOG(LogMusee, Warning, TEXT("live: shots request %s has no shots with a pose"), *NewId); return; }
	Id = NewId;
	OutDir = LiveDir / TEXT("shots") / Id;
	IFileManager::Get().MakeDirectory(*OutDir, true);
	IFileManager::Get().Delete(*(OutDir / TEXT("done.json")), false, true, true);
	Files.Reset();
	PerfResults = MakeShared<FJsonObject>();
	Index = 0;
	Stage = 0;
	Started = FPlatformTime::Seconds();
	// what the request changes, to put back afterwards
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AMuseeCharacter* Visitor = PC ? Cast<AMuseeCharacter>(PC->GetPawn()) : nullptr;
	bWasPhoto = Visitor && Visitor->InPhotoMode();
	FovWas = Visitor && Visitor->Camera ? Visitor->Camera->FieldOfView : 0.f;
	bHudWas = !(PC && PC->GetHUD()) || PC->GetHUD()->bShowHUD;
	if (PC)
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		if (AHUD* HUD = PC->GetHUD()) { HUD->bShowHUD = false; }
	}
	// fast eye adaptation while the request runs, so the exposure has settled within a shot's settle (at the museum's
	// own speed an A/B of a look change was still adapting); "fast_exposure": false keeps the museum's speed
	if (Flag(*J, TEXT("fast_exposure"), true))
	{
		if (!ExposureVolume)
		{
			ExposureVolume = GetWorld()->SpawnActorDeferred<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity);
			ExposureVolume->bUnbound = true;
			ExposureVolume->Priority = 1001.f;   // over the live look's volume (1000)
			ExposureVolume->BlendWeight = 1.f;
			ExposureVolume->Settings.bOverride_AutoExposureSpeedUp = true;
			ExposureVolume->Settings.AutoExposureSpeedUp = 30.f;
			ExposureVolume->Settings.bOverride_AutoExposureSpeedDown = true;
			ExposureVolume->Settings.AutoExposureSpeedDown = 30.f;
			ExposureVolume->FinishSpawning(FTransform::Identity);
		}
		ExposureVolume->bEnabled = true;
	}
	UE_LOG(LogMusee, Display, TEXT("live: shots request %s, %d shots"), *Id, Shots.Num());
}

void UMuseeLiveShots::Pose(const FShot& S)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(PC->GetPawn());
	bHourChanged = false;
	if (S.Hour > -2.f)
	{
		if (IConsoleVariable* H = HourVar(); H && H->GetFloat() != S.Hour) { H->Set(S.Hour, ECVF_SetByConsole); bHourChanged = true; }
	}
	SetPhoto(false);   // a path-traced shot switches it on after RasterLead (Tick)
	bPhotoPending = S.bPhoto;
	SetPreview(S.bPreview ? &S : nullptr);
	// as -MuseePose places the visitor (AMuseeGameMode::ApplyDebugPose)
	const float HalfHeight = Visitor ? Visitor->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() : 0.f;
	PC->GetPawn()->TeleportTo(MuseePlan::At(S.X, S.Y, S.Feet) + FVector(0, 0, HalfHeight + 2), FRotator(0, S.Yaw, 0), false, true);
	PC->SetControlRotation(FRotator(S.Pitch, S.Yaw, 0));
	if (Visitor && Visitor->Camera) { Visitor->Camera->SetFieldOfView(S.Fov > 10.f ? S.Fov : FovWas); }
}

void UMuseeLiveShots::SetPhoto(bool bOn)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AMuseeCharacter* Visitor = PC ? Cast<AMuseeCharacter>(PC->GetPawn()) : nullptr;
	if (!Visitor) { return; }
	if (bOn) { Visitor->StartPhotoMode(); }
	else { Visitor->StopPhotoMode(); }
}

void UMuseeLiveShots::SetPreview(const FShot* S)
{
	// console variables, not a volume: a preview volume spawned for the first shot of a request still left that shot
	// noisy after its settle, while the second shot converged. The museum's volume keeps the denoiser on
	IConsoleVariable* Spp = IConsoleManager::Get().FindConsoleVariable(TEXT("r.PathTracing.SamplesPerPixel"));
	IConsoleVariable* Bounces = IConsoleManager::Get().FindConsoleVariable(TEXT("r.PathTracing.MaxBounces"));
	if (!Spp || !Bounces) { return; }
	if (S)
	{
		if (!bPreviewOn) { SppWas = Spp->GetInt(); BouncesWas = Bounces->GetInt(); }
		Spp->Set(S->Spp, ECVF_SetByConsole);
		Bounces->Set(8, ECVF_SetByConsole);
		bPreviewOn = true;
	}
	else if (bPreviewOn)
	{
		Spp->Set(SppWas, ECVF_SetByConsole);
		Bounces->Set(BouncesWas, ECVF_SetByConsole);
		bPreviewOn = false;
	}
}

void UMuseeLiveShots::RecordPerf(const FString& Name)
{
	if (PerfFrame.Num() > 0)
	{
		TArray<float> Sorted = PerfFrame;
		Sorted.Sort();
		double Sum = 0, Gpu = 0;
		for (const float F : PerfFrame) { Sum += F; }
		for (const float G : PerfGpu) { Gpu += G; }
		const double Mean = Sum / PerfFrame.Num();
		TSharedRef<FJsonObject> R = MakeShared<FJsonObject>();
		R->SetNumberField(TEXT("frames"), PerfFrame.Num());
		R->SetNumberField(TEXT("frame_ms"), FMath::RoundToDouble(Mean * 100.0) / 100.0);
		R->SetNumberField(TEXT("fps"), FMath::RoundToDouble(10000.0 / Mean) / 10.0);
		R->SetNumberField(TEXT("p99_ms"), FMath::RoundToDouble(Sorted[FMath::Min(Sorted.Num() - 1, FMath::FloorToInt(Sorted.Num() * 0.99))] * 100.0) / 100.0);
		R->SetNumberField(TEXT("gpu_ms"), FMath::RoundToDouble(Gpu / PerfGpu.Num() * 100.0) / 100.0);
		PerfResults->SetObjectField(Name, R);
		UE_LOG(LogMusee, Display, TEXT("live: perf %s: %.2f ms (%.1f fps), GPU %.2f ms"), *Name, Mean, 1000.0 / Mean, Gpu / PerfGpu.Num());
	}
	PerfFrame.Reset();
	PerfGpu.Reset();
}

void UMuseeLiveShots::Finish()
{
	if (ExposureVolume) { ExposureVolume->bEnabled = false; }
	SetPreview(nullptr);
	SetPhoto(bWasPhoto);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		if (AHUD* HUD = PC->GetHUD()) { HUD->bShowHUD = bHudWas; }
		if (AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(PC->GetPawn()); Visitor && Visitor->Camera && FovWas > 0.f) { Visitor->Camera->SetFieldOfView(FovWas); }
	}
	TSharedRef<FJsonObject> Done = MakeShared<FJsonObject>();
	Done->SetStringField(TEXT("id"), Id);
	TArray<TSharedPtr<FJsonValue>> F;
	for (const FString& P : Files) { F.Add(MakeShared<FJsonValueString>(P)); }
	Done->SetArrayField(TEXT("files"), F);
	Done->SetNumberField(TEXT("failed"), Shots.Num() - Files.Num());
	if (PerfResults->Values.Num()) { Done->SetObjectField(TEXT("perf"), PerfResults.ToSharedRef()); }
	Done->SetNumberField(TEXT("seconds"), FMath::RoundToDouble((FPlatformTime::Seconds() - Started) * 10.0) / 10.0);
	FString Out;
	FJsonSerializer::Serialize(Done, TJsonWriterFactory<>::Create(&Out));
	FFileHelper::SaveStringToFile(Out, *(OutDir / TEXT("done.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogMusee, Display, TEXT("live: shots %s done, %d of %d in %.1f s"), *Id, Files.Num(), Shots.Num(), FPlatformTime::Seconds() - Started);
	Index = -1;
}
