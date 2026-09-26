#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MuseeGameMode.generated.h"

/** The visitor walks in on the gilt sun at the centre of the Rotunda, facing the west door. */
UCLASS()
class MUSEEVISION_API AMuseeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMuseeGameMode();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	/** The VR build (MuseeVisionVR.exe) links in the visitor in a headset: it is used there. */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

private:
	/**
	 * Development checks, like the iPhone's -pose: -MuseePose=x,y,feet,yaw,pitch (plan metres and
	 * degrees; yaw 0 faces east, 90 south) places the visitor; -MuseeShot=<file.png> waits for the
	 * shaders to finish, saves a screenshot there and quits; -MuseePhoto starts in photo mode;
	 * -MuseeQuit=<seconds> quits after that long; -MuseeFov=<degrees> sets the lens.
	 */
	/** The Chinese Wing's stele: today's solar term. */
	void ShowTodaysSolarTerm();
	void ApplyDebugPose(AController* Player);

	FString ShotPath;
	float ShotTimer = 0.f;
	int32 ShotStage = 0;
	/** -MuseeQuit=<seconds>: run normally and quit, for checking the log. */
	float QuitAfter = 0.f;
	float RunTime = 0.f;
	float CompileWait = 0.f;
	/** Frame and GPU times (ms) since the last perf step, for perf:<name> (MuseeDo). */
	TArray<float> PerfFrameMs, PerfGpuMs;
	/** -MuseePhoto: start in path-traced photo mode; a -MuseeShot then waits for it to converge. */
	bool bStartInPhoto = false;
	bool bPhotoStarted = false;
	/**
	 * -MuseeDo="step@seconds;…": a scripted visit for testing, like the iPhone's -do. Steps:
	 * a prompt id ("elevator.call", "pond.use"), "walk:x,y" (walk to a plan point), "tp:x,y,feet"
	 * (teleport), "look:yaw,pitch", "jump" (a hop), "stop" (end a walk), "log" (log the visitor's position).
	 */
	struct FStep { float At; FString What; bool bDone = false; };
	TArray<FStep> Steps;
	TOptional<FVector2D> WalkTarget;
	void RunStep(const FString& What);
	void TickSteps();

	/** Frame and GPU time while settling for a screenshot, for the log. */
	double FrameMs = 0, GpuMs = 0;
	int32 Frames = 0;
};
