#include "Visitor/MuseeGameMode.h"

#include "Plan/MuseePlan.h"
#include "UI/MuseeHUD.h"
#include "Elan/ElanElevator.h"
#include "Visitor/MuseeInteractable.h"
#include "Salon/PondLift.h"
#include "EngineUtils.h"
#include "Visitor/MuseeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "MuseeVision.h"
#include "HAL/PlatformMisc.h"
#include "RHI.h"
#include "GameFramework/HUD.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Engine/Texture.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sky/Ephemeris.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"
#include "TimerManager.h"

AMuseeGameMode::AMuseeGameMode()
{
	DefaultPawnClass = AMuseeCharacter::StaticClass();
	HUDClass = AMuseeHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	FParse::Value(FCommandLine::Get(), TEXT("-MuseeShot="), ShotPath);
	FParse::Value(FCommandLine::Get(), TEXT("-MuseeQuit="), QuitAfter);
	bStartInPhoto = FParse::Param(FCommandLine::Get(), TEXT("MuseePhoto"));
	FString Do;
	if (FParse::Value(FCommandLine::Get(), TEXT("-MuseeDo="), Do, false))
	{
		TArray<FString> Items;
		Do.TrimQuotes().ParseIntoArray(Items, TEXT(";"));
		for (const FString& Item : Items)
		{
			FString What, When;
			if (Item.Split(TEXT("@"), &What, &When)) { Steps.Add({FCString::Atof(*When), What.TrimStartAndEnd()}); }
		}
	}
}

AActor* AMuseeGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// A PlayerStart in the level wins (e.g. one moved for testing); else the plan's spawn.
	return Super::ChoosePlayerStart_Implementation(Player);
}

UClass* AMuseeGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// Only the VR target links MuseeVisionVR; the visitor there is the desktop one without a headset.
	if (UClass* VRVisitor = FindObject<UClass>(nullptr, TEXT("/Script/MuseeVisionVR.MuseeVRCharacter"))) { return VRVisitor; }
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AMuseeGameMode::BeginPlay()
{
	Super::BeginPlay();
	// The display: unless a run asks for a window of its own (-windowed, -ResX: the test harness), the whole screen
	// at its native resolution (borderless, so the compositor paces the frames). Vertical sync is on unless the
	// visitor turns it off in the graphics settings (UMuseeGraphics, which GameUserSettings mirrors).
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		bool bChanged = false;
		int32 ResX = 0;
		if (!FParse::Param(FCommandLine::Get(), TEXT("windowed")) && !FParse::Value(FCommandLine::Get(), TEXT("ResX="), ResX))
		{
			const FIntPoint Desktop = Settings->GetDesktopResolution();
			if (Settings->GetScreenResolution() != Desktop || Settings->GetFullscreenMode() != EWindowMode::WindowedFullscreen)
			{
				Settings->SetScreenResolution(Desktop);
				Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
				bChanged = true;
			}
		}
		if (bChanged) { Settings->ApplySettings(false); }
	}
	// The moving parts' controllers: spawned here if the level doesn't place them.
	auto Ensure = [this](UClass* Class)
	{
		for (TActorIterator<AActor> It(GetWorld(), Class); It; ++It) { return; }
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AActor>(Class, FTransform::Identity, Params);
	};
	Ensure(APondLift::StaticClass());
	Ensure(AElanElevator::StaticClass());
	ShowTodaysSolarTerm();
}

void AMuseeGameMode::ShowTodaysSolarTerm()
{
	// The Chinese Wing's stele carries today's solar term (Scripts/stele_inscriptions.py: one texture per term,
	// cooked from /Game/Museum/ChineseWing/Stele).
	const int32 Term = MuseeEphemeris::SolarTerm(FDateTime::UtcNow());
	UTexture* Inscription = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Museum/ChineseWing/Stele/T_stele_%02d.T_stele_%02d"), Term, Term));
	if (!Inscription) { return; }
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->Tags.Contains(FName(TEXT("prim:/Museum/ChineseWing/Stele_inscription")))) { continue; }
		for (UActorComponent* C : It->GetComponents())
		{
			UMeshComponent* Mesh = Cast<UMeshComponent>(C);
			if (!Mesh) { continue; }
			for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
			{
				if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(i))
				{
					MID->SetTextureParameterValue(TEXT("BaseColorTexture"), Inscription);
					MID->SetTextureParameterValue(TEXT("EmissiveColorTexture"), Inscription);
				}
			}
		}
	}
}

void AMuseeGameMode::RestartPlayer(AController* NewPlayer)
{
	if (FindPlayerStart(NewPlayer))
	{
		Super::RestartPlayer(NewPlayer);
	}
	else
	{
		const AMuseeCharacter* Defaults = GetDefault<AMuseeCharacter>();
		const float HalfHeight = Defaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		const FRotator Facing(0, MuseePlan::SpawnYawDegrees, 0);
		RestartPlayerAtTransform(NewPlayer, FTransform(Facing, MuseePlan::SpawnFeet() + FVector(0, 0, HalfHeight + 2)));
	}
	ApplyDebugPose(NewPlayer);
	// A screenshot run takes no input: the mouse and keys on the desk must not move the view.
	if (!ShotPath.IsEmpty())
	{
		if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
		{
			PC->SetIgnoreMoveInput(true);
			PC->SetIgnoreLookInput(true);
		}
	}
}

void AMuseeGameMode::ApplyDebugPose(AController* Player)
{
	FString Pose;
	if (!Player || !FParse::Value(FCommandLine::Get(), TEXT("-MuseePose="), Pose, false)) { return; }
	TArray<FString> Parts;
	Pose.ParseIntoArray(Parts, TEXT(","));
	if (Parts.Num() < 4) { return; }
	const double X = FCString::Atod(*Parts[0]), Y = FCString::Atod(*Parts[1]), Feet = FCString::Atod(*Parts[2]);
	const double Yaw = FCString::Atod(*Parts[3]), Pitch = Parts.Num() > 4 ? FCString::Atod(*Parts[4]) : 0.0;
	if (APawn* Pawn = Player->GetPawn())
	{
		const float HalfHeight = GetDefault<AMuseeCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		const bool bMoved = Pawn->TeleportTo(MuseePlan::At(X, Y, Feet) + FVector(0, 0, HalfHeight + 2), FRotator(0, Yaw, 0));
		UE_LOG(LogMusee, Log, TEXT("Pose: %s to (%.2f, %.2f, %.2f) yaw %.0f pitch %.0f."), bMoved ? TEXT("moved") : TEXT("could not move"), X, Y, Feet, Yaw, Pitch);
	}
	Player->SetControlRotation(FRotator(Pitch, Yaw, 0));
	// -MuseeFov=<horizontal degrees>: match a rendering's lens (18 mm ≈ 90°, 24 mm ≈ 74°).
	float Fov = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("-MuseeFov="), Fov) && Fov > 10.f)
	{
		if (AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(Player->GetPawn())) { Visitor->Camera->SetFieldOfView(Fov); }
	}
}

void AMuseeGameMode::RunStep(const FString& What)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AMuseeCharacter* Visitor = PC ? Cast<AMuseeCharacter>(PC->GetPawn()) : nullptr;
	if (!Visitor) { return; }
	TArray<FString> Args;
	FString Name = What, Rest;
	if (What.Split(TEXT(":"), &Name, &Rest)) { Rest.ParseIntoArray(Args, TEXT(",")); }
	auto Arg = [&Args](int32 i) { return Args.IsValidIndex(i) ? FCString::Atod(*Args[i]) : 0.0; };
	const float HalfHeight = Visitor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (Name == TEXT("walk")) { WalkTarget = FVector2D(Arg(0), Arg(1)); }
	else if (Name == TEXT("tp")) { Visitor->TeleportTo(MuseePlan::At(Arg(0), Arg(1), Arg(2)) + FVector(0, 0, HalfHeight + 2), Visitor->GetActorRotation()); }
	else if (Name == TEXT("look")) { PC->SetControlRotation(FRotator(Arg(1), Arg(0), 0)); }
	else if (Name == TEXT("stop")) { WalkTarget.Reset(); }
	else if (Name == TEXT("jump"))
	{
		// jump: Space pressed and released a moment later (the hop; a walk: step keeps going through it).
		Visitor->Jump();
		FTimerHandle Release;
		TWeakObjectPtr<AMuseeCharacter> Weak(Visitor);
		GetWorldTimerManager().SetTimer(Release, [Weak]() { if (Weak.IsValid()) { Weak->StopJumping(); } }, 0.15f, false);
	}
	else if (Name == TEXT("shot"))
	{
		// shot:<name>: a screenshot now, to Saved/MuseeDo/<name>.png (the HUD included, prompts and all).
		const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MuseeDo"), (Args.Num() ? Args[0] : FString(TEXT("shot"))) + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		UE_LOG(LogMusee, Log, TEXT("Do: shot %s"), *Path);
	}
	else if (Name == TEXT("perf"))
	{
		// perf:<name>: the frames since the last perf step (the settle after a teleport should be skipped with an
		// earlier perf:-), their mean and 99th-percentile frame time and mean GPU time, appended to
		// Saved/MuseeDo/perf.txt (a Shipping build logs nothing).
		const FString Label = Args.Num() ? Args[0] : FString(TEXT("perf"));
		if (Label != TEXT("-") && PerfFrameMs.Num() > 10)
		{
			TArray<float> Sorted = PerfFrameMs;
			Sorted.Sort();
			double Sum = 0, Gpu = 0;
			for (const float F : PerfFrameMs) { Sum += F; }
			for (const float G : PerfGpuMs) { Gpu += G; }
			const FString Line = FString::Printf(TEXT("%s\t%d frames\tmean %.2f ms (%.0f fps)\tp99 %.2f ms\tGPU %.2f ms\n"), *Label, PerfFrameMs.Num(),
				Sum / PerfFrameMs.Num(), 1000.0 / (Sum / PerfFrameMs.Num()), Sorted[FMath::Min(Sorted.Num() - 1, FMath::FloorToInt(Sorted.Num() * 0.99))],
				Gpu / FMath::Max(1, PerfGpuMs.Num()));
			FFileHelper::SaveStringToFile(Line, *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MuseeDo"), TEXT("perf.txt")),
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
			UE_LOG(LogMusee, Log, TEXT("Perf %s"), *Line);
		}
		PerfFrameMs.Reset();
		PerfGpuMs.Reset();
	}
	else if (Name == TEXT("cmd"))
	{
		// cmd:<console command>, e.g. cmd:musee.Hour 21 (commas and all, taken whole).
		PC->ConsoleCommand(Rest);
	}
	else if (Name != TEXT("log"))
	{
		// A prompt: every provider gets it; each ignores what isn't its own.
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (IMuseePromptProvider* Provider = Cast<IMuseePromptProvider>(*It)) { Provider->RunPrompt(Visitor, FName(*Name)); }
		}
	}
	const FVector Feet = Visitor->FeetLocation() / MuseePlan::Cm;
	TArray<FString> Prompts;
	for (const FMuseeActionPrompt& P : Visitor->CurrentPrompts()) { Prompts.Add(P.Title.ToString()); }
	UE_LOG(LogMusee, Log, TEXT("Do %s at %.1f s: feet (%.2f, %.2f, %.2f); prompts [%s]; message \"%s\""), *What, RunTime,
		Feet.X, Feet.Y, Feet.Z, *FString::Join(Prompts, TEXT(" | ")), *Visitor->Message().ToString());
}

void AMuseeGameMode::TickSteps()
{
	for (FStep& Step : Steps)
	{
		if (!Step.bDone && RunTime >= Step.At) { Step.bDone = true; RunStep(Step.What); }
	}
	if (!WalkTarget.IsSet()) { return; }
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AMuseeCharacter* Visitor = PC ? Cast<AMuseeCharacter>(PC->GetPawn()) : nullptr;
	if (!Visitor) { return; }
	const FVector Feet = Visitor->FeetLocation() / MuseePlan::Cm;
	const FVector2D To = WalkTarget.GetValue() - FVector2D(Feet.X, Feet.Y);
	if (To.Size() < 0.15) { WalkTarget.Reset(); return; }
	Visitor->AddMovementInput(FVector(To.X, To.Y, 0).GetSafeNormal(), 1.f, true);
}

void AMuseeGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// A scripted run's clock stops while shaders compile (the first launch after a material or renderer change),
	// so its steps and shots see the finished materials, not the grey stand-ins.
	if (!Steps.IsEmpty() && GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
	{
		CompileWait += DeltaSeconds;
		if (CompileWait > 30.f)
		{
			CompileWait = 0.f;
			UE_LOG(LogMusee, Log, TEXT("Do: waiting for %d shader jobs at %.1f s."), GShaderCompilingManager->GetNumRemainingJobs(), RunTime);
		}
		return;
	}
	RunTime += DeltaSeconds;
	PerfFrameMs.Add(DeltaSeconds * 1000.f);
	PerfGpuMs.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0)));
	TickSteps();
	if (QuitAfter > 0.f && RunTime > QuitAfter && ShotPath.IsEmpty())
	{
		QuitAfter = 0.f;
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
	}
	if (ShotPath.IsEmpty() && !bStartInPhoto) { return; }
	// Wait for the shaders, then give Lumen and the exposure a few seconds to settle.
	if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) { ShotTimer = 0.f; return; }
	ShotTimer += DeltaSeconds;
	// Nothing over the picture in a screenshot run.
	if (!ShotPath.IsEmpty())
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AHUD* HUD = PC->GetHUD()) { HUD->bShowHUD = false; }
		}
	}
	if (ShotTimer > 2.f)
	{
		FrameMs += DeltaSeconds * 1000.0;
		GpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		++Frames;
	}
	if (bStartInPhoto && !bPhotoStarted && ShotTimer > 4.f)
	{
		bPhotoStarted = true;
		ShotTimer = 0.f;
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(PC->GetPawn())) { Visitor->StartPhotoMode(); }
		}
		return;
	}
	if (ShotPath.IsEmpty()) { return; }
	// The path tracer needs time to converge; Lumen a few seconds.
	const float Settle = bStartInPhoto ? 100.f : 8.f;
	if (bStartInPhoto && !bPhotoStarted) { return; }
	if (ShotStage == 0 && ShotTimer > Settle)
	{
		if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			const FVector At = PC->GetPawn() ? PC->GetPawn()->GetActorLocation() / MuseePlan::Cm : FVector::ZeroVector;
			FVector2D Size(0, 0);
			if (GEngine && GEngine->GameViewport) { GEngine->GameViewport->GetViewportSize(Size); }
			UE_LOG(LogMusee, Log, TEXT("Shot: visitor at (%.2f, %.2f, %.2f), looking yaw %.0f pitch %.0f; %d x %d, frame %.1f ms, GPU %.1f ms."),
				At.X, At.Y, At.Z, PC->GetControlRotation().Yaw, PC->GetControlRotation().Pitch, int32(Size.X), int32(Size.Y),
				Frames ? FrameMs / Frames : 0.0, Frames ? GpuMs / Frames : 0.0);
		}
		FScreenshotRequest::RequestScreenshot(ShotPath, false, false);
		ShotStage = 1;
		ShotTimer = 0.f;
	}
	else if (ShotStage == 1 && ShotTimer > 2.f)
	{
		ShotStage = 2;
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
	}
}
