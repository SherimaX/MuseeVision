// Development commands for the classical hall (not in shipping builds). From the game's console or -ExecCmds:
//
//   musee.ClassicalSpawn     places AClassicalHallStructure at the origin if the map has none yet (as Scripts/native.py
//                            will), so the hall can be seen and walked before it is in the map
//   musee.ClassicalSpots     logs every place for a work of art (name, kind, world location in cm, facing, sizes, cast)
//   musee.ClassicalTrace     traces the visitor's channel along the gallery's axis and round the ambulatory, and logs
//                            the floor's height and any gap

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "ClassicalHall/ClassicalHallStructure.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "MuseeVision.h"
#include "Plan/MuseePlan.h"

namespace ClassicalHallChecks
{
	AClassicalHallStructure* Find(UWorld* World)
	{
		for (TActorIterator<AClassicalHallStructure> It(World); It; ++It) { return *It; }
		return nullptr;
	}

	void Spawn(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Find(World)) { return; }
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World->SpawnActor<AClassicalHallStructure>(AClassicalHallStructure::StaticClass(), FTransform::Identity, Params);
		UE_LOG(LogMusee, Log, TEXT("ClassicalSpawn: %s."), Actor ? TEXT("the classical hall placed at the origin") : TEXT("could not place the classical hall"));
	}

	void Spots(const TArray<FString>& Args, UWorld* World)
	{
		const AClassicalHallStructure* Hall = World ? Find(World) : nullptr;
		if (!Hall) { UE_LOG(LogMusee, Warning, TEXT("ClassicalSpots: no classical hall in the map.")); return; }
		TArray<FClassicalStatueSpot> All = Hall->GetStatueSpots();
		All.Add(Hall->GetCentrepieceSpot());
		for (const FClassicalStatueSpot& S : All)
		{
			const FVector L = S.Transform.GetLocation();
			const FVector F = S.Transform.GetRotation().GetForwardVector();
			UE_LOG(LogMusee, Log, TEXT("ClassicalSpots: %-22s %-12s at (%.0f, %.0f, %.0f) facing (%.2f, %.2f), up to %.0f cm tall, %.0f wide; %s"),
				   *S.Name.ToString(), *S.Kind.ToString(), L.X, L.Y, L.Z, F.X, F.Y, S.SuggestedHeight, S.MaxWidth, *S.SuggestedWork);
		}
	}

	void Trace(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		namespace CH = MuseePlan::ClassicalHall;
		TArray<FVector> Points;   // plan metres
		for (double Y = CH::GallerySouthY + 0.5; Y > CH::TribuneCentreY - CH::TribuneRadius + 0.3; Y -= 0.5) { Points.Add(FVector(0.0, Y, 0.0)); }
		for (int32 k = 0; k < 72; ++k)
		{
			const double A = 2.0 * UE_DOUBLE_PI * k / 72;
			Points.Add(FVector(5.1 * FMath::Cos(A), CH::TribuneCentreY + 5.1 * FMath::Sin(A), 0.0));
		}
		FCollisionQueryParams Query(FName(TEXT("ClassicalTrace")), true);
		int32 Gaps = 0;
		for (const FVector& P : Points)
		{
			const FVector From = MuseePlan::At(P.X, P.Y, CH::FloorZ + 1.0), To = MuseePlan::At(P.X, P.Y, CH::FloorZ - 1.0);
			FHitResult Hit;
			if (!World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Query))
			{
				++Gaps;
				UE_LOG(LogMusee, Warning, TEXT("ClassicalTrace: no floor at (%.2f, %.2f)."), P.X, P.Y);
				continue;
			}
			const double H = Hit.ImpactPoint.Z / MuseePlan::Cm - CH::FloorZ;
			if (FMath::Abs(H) > 0.02) { UE_LOG(LogMusee, Log, TEXT("ClassicalTrace: (%.2f, %.2f) stands %.3f m over the floor (%s)."), P.X, P.Y, H, *GetNameSafe(Hit.GetComponent())); }
		}
		UE_LOG(LogMusee, Log, TEXT("ClassicalTrace: %d points, %d without a floor."), Points.Num(), Gaps);
	}

	FAutoConsoleCommandWithWorldAndArgs SpawnCommand(TEXT("musee.ClassicalSpawn"), TEXT("Place the classical hall at the origin if the map has none."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Spawn));
	FAutoConsoleCommandWithWorldAndArgs SpotsCommand(TEXT("musee.ClassicalSpots"), TEXT("Log the classical hall's places for works of art."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Spots));
	FAutoConsoleCommandWithWorldAndArgs TraceCommand(TEXT("musee.ClassicalTrace"), TEXT("Trace the classical hall's floor for the visitor."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Trace));
}

#endif
