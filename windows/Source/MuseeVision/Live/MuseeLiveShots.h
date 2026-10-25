#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MuseeLiveShots.generated.h"

/**
 * -Live: the running game takes screenshot requests, so a round of views is one request to a game that is already up
 * instead of a cold launch per view (tour.ps1, -MuseeShot). Scripts/live.py shots writes the request:
 *
 *   Saved/Live/shots.json   {"id": "round3", "settle": 6, "photo": false, "fov": 84,
 *                            "shots": [{"name": "ch-inner", "pose": [x, y, feet, yaw, pitch], "hour": 14,
 *                                       "fov": 84, "settle": 6, "photo": false}, …]}
 *                           pose as -MuseePose (plan metres and degrees; yaw 0 faces east, 90 south); hour as
 *                           musee.Hour; photo: the path tracer (settle defaults to 480 s then); preview (with photo):
 *                           a quick path-traced look with "spp" samples (64 by default), 8 bounces, denoised
 *                           (settle defaults to 40 s); a shot's own settle, fov, photo, preview and spp win over
 *                           the request's
 *                           "perf": seconds of frame and GPU times at each view after its settle; "fast_exposure":
 *                           false keeps the museum's eye adaptation speed (by default it adapts fast during a request)
 *   Saved/Live/shots/<id>/<name>.png, then done.json   {"id", "files", "failed", "perf": {name: {frame_ms, fps,
 *                           p99_ms, gpu_ms}}, "seconds"}, written after every PNG is on disk
 *
 * Each shot waits for shaders to finish compiling before its settle time starts; a path-traced one first stands 8 s in
 * raster (30 s when its hour differs from the last), so the room's own blends settle before the path tracer starts
 * accumulating. The HUD and the visitor's input are off while a request runs, and the photo mode goes back to what it
 * was afterwards; the hour stays at the last shot's, so the next request at that hour starts settled. A new id starts a new
 * request; the same id again is ignored.
 */
UCLASS()
class UMuseeLiveShots : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMuseeLiveShots, STATGROUP_Tickables); }

private:
	struct FShot
	{
		FString Name;
		double X = 0, Y = 0, Feet = 0, Yaw = 0, Pitch = 0;
		float Hour = -2.f;   // -2: leave the hour as it is
		float Fov = 0.f;     // 0: leave the lens as it is
		float Settle = 6.f;
		bool bPhoto = false;
		bool bPreview = false;
		int32 Spp = 64;
		float Perf = 0.f;   // seconds of frame and GPU times at the view, after the settle
	};

	FString LiveDir, RequestKey, DoneId, Id, OutDir;
	bool bActive = false;
	double NextPoll = 0.0;
	TArray<FShot> Shots;
	TArray<FString> Files;
	int32 Index = -1;   // the shot being taken, -1 when idle
	int32 Stage = 0;    // 0: pose it, 1: settling, 2: waiting for the PNG
	float Timer = 0.f;
	bool bPhotoPending = false;   // a path-traced shot, still in its raster lead
	static constexpr float RasterLead = 8.f;
	/** After an hour change the sky and the daylit rooms take longer to settle: with an 8 s lead a path-traced
	 *  shot taken right after the hour changed was still noisy at the end of its settle, the same view at the same
	 *  hour was clean. */
	static constexpr float HourLead = 30.f;
	bool bHourChanged = false;
	static constexpr float PhotoWarmUp = 90.f;   // extra seconds for the game's first path-traced shot
	bool bPhotoWarm = false;
	double Started = 0.0;
	// what the request changes, put back after it
	bool bWasPhoto = false, bHudWas = true;
	float FovWas = 0.f;
	/** A path-traced preview: fewer samples and bounces through r.PathTracing.SamplesPerPixel and MaxBounces, put
	 *  back after each preview shot. */
	bool bPreviewOn = false;
	int32 SppWas = -1, BouncesWas = -1;
	void SetPreview(const FShot* S);

	UPROPERTY(Transient) TObjectPtr<class APostProcessVolume> ExposureVolume;   // fast eye adaptation during a request
	TArray<float> PerfFrame, PerfGpu;
	TSharedPtr<class FJsonObject> PerfResults;
	void RecordPerf(const FString& Name);

	void ReadRequest();
	void Pose(const FShot& S);
	void SetPhoto(bool bOn);
	void Finish();
};
