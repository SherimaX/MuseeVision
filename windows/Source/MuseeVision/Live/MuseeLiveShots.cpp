#include "Live/MuseeLiveShots.h"
#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "Visitor/MuseeCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Dom/JsonObject.h"
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
		if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) { Timer = 0.f; return; }
		Timer += DeltaTime;
		if (Timer < S.Settle) { return; }
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
	const float Fov = Field(*J, TEXT("fov"), 0.0);
	const float Settle = Field(*J, TEXT("settle"), bPhoto ? 480.0 : 6.0);
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
			S.Settle = Field(*O, TEXT("settle"), S.bPhoto && !bPhoto ? 480.0 : Settle);
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
	Index = 0;
	Stage = 0;
	Started = FPlatformTime::Seconds();
	// what the request changes, to put back afterwards
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AMuseeCharacter* Visitor = PC ? Cast<AMuseeCharacter>(PC->GetPawn()) : nullptr;
	bWasPhoto = Visitor && Visitor->InPhotoMode();
	HourWas = HourVar() ? HourVar()->GetFloat() : -1.f;
	FovWas = Visitor && Visitor->Camera ? Visitor->Camera->FieldOfView : 0.f;
	bHudWas = !(PC && PC->GetHUD()) || PC->GetHUD()->bShowHUD;
	if (PC)
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		if (AHUD* HUD = PC->GetHUD()) { HUD->bShowHUD = false; }
	}
	UE_LOG(LogMusee, Display, TEXT("live: shots request %s, %d shots"), *Id, Shots.Num());
}

void UMuseeLiveShots::Pose(const FShot& S)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(PC->GetPawn());
	if (S.Hour > -2.f) { if (IConsoleVariable* H = HourVar()) { H->Set(S.Hour, ECVF_SetByConsole); } }
	SetPhoto(S.bPhoto);
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

void UMuseeLiveShots::Finish()
{
	SetPhoto(bWasPhoto);
	if (IConsoleVariable* H = HourVar()) { H->Set(HourWas, ECVF_SetByConsole); }
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
	Done->SetNumberField(TEXT("seconds"), FMath::RoundToDouble((FPlatformTime::Seconds() - Started) * 10.0) / 10.0);
	FString Out;
	FJsonSerializer::Serialize(Done, TJsonWriterFactory<>::Create(&Out));
	FFileHelper::SaveStringToFile(Out, *(OutDir / TEXT("done.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogMusee, Display, TEXT("live: shots %s done, %d of %d in %.1f s"), *Id, Files.Num(), Shots.Num(), FPlatformTime::Seconds() - Started);
	Index = -1;
}
