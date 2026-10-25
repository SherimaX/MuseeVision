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
 *                           musee.Hour; photo: the path tracer (settle defaults to 480 s then); a shot's own
 *                           settle, fov and photo win over the request's
 *   Saved/Live/shots/<id>/<name>.png, then done.json   {"id", "files", "seconds"}, written after every PNG is on disk
 *
 * Each shot waits for shaders to finish compiling before its settle time starts. The HUD and the visitor's input are
 * off while a request runs, and the photo mode and hour go back to what they were afterwards. A new id starts a new
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
	};

	FString LiveDir, RequestKey, DoneId, Id, OutDir;
	bool bActive = false;
	double NextPoll = 0.0;
	TArray<FShot> Shots;
	TArray<FString> Files;
	int32 Index = -1;   // the shot being taken, -1 when idle
	int32 Stage = 0;    // 0: pose it, 1: settling, 2: waiting for the PNG
	float Timer = 0.f;
	double Started = 0.0;
	// what the request changes, put back after it
	bool bWasPhoto = false, bHudWas = true;
	float HourWas = -1.f, FovWas = 0.f;

	void ReadRequest();
	void Pose(const FShot& S);
	void SetPhoto(bool bOn);
	void Finish();
};
