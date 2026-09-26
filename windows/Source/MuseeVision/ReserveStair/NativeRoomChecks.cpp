// Development checks for the native Atrium base and long stair (not in shipping builds). From the game's
// console or -ExecCmds:
//
//   musee.NativeSpawn              places AAtriumBaseStructure and AReserveStairStructure at the origin if the
//                                  map has none yet (as Scripts/native.py will), so they can be seen and walked
//   musee.NativeDump <file.obj>    writes every procedural mesh's triangles near the Atrium and the stair (world
//                                  metres; one object per actor|component|section|material|visible|collision)
//   musee.StairTrace               traces the visitor's channel down the stair on seven lines and logs the steps
//   musee.AtriumTrace              traces the visitor's channel over the Atrium's floor and logs holes and heights

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Atrium/AtriumBaseStructure.h"
#include "MuseeVision.h"
#include "ReserveStair/ReserveStairStructure.h"
#include "Visitor/MuseeWorld.h"
#include "CollisionQueryParams.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"

namespace NativeRoomChecks
{
	void Spawn(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		for (UClass* Class : {AAtriumBaseStructure::StaticClass(), AReserveStairStructure::StaticClass()})
		{
			bool bHave = false;
			for (TActorIterator<AActor> It(World, Class); It; ++It) { bHave = true; }
			if (bHave) { continue; }
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Actor = World->SpawnActor<AActor>(Class, FTransform::Identity, Params);
			UE_LOG(LogMusee, Log, TEXT("NativeSpawn: %s %s."), *Class->GetName(), Actor ? TEXT("placed at the origin") : TEXT("could not be placed"));
		}
	}

	bool InRegion(const FVector& P)
	{
		// Metres. The Atrium (to its outer face and a little over the coping), and the stair's shaft.
		const double DX = P.X - MuseePlan::Elan::CentreX, DY = P.Y - MuseePlan::Elan::CentreY;
		if (DX * DX + DY * DY <= 15.5 * 15.5 && P.Z > -1.6 && P.Z < 7.0) { return true; }
		return P.X > -92.0 && P.X < -72.0 && FMath::Abs(P.Y) < 2.5 && P.Z > -6.6 && P.Z < 0.6;
	}

	void Dump(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		const FString Path = Args.Num() > 0 ? Args[0] : FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("NativeDump.obj"));
		TArray<FString> Lines;
		int32 Base = 1, Objects = 0, Tris = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TArray<UProceduralMeshComponent*> Components;
			It->GetComponents(Components);
			for (UProceduralMeshComponent* C : Components)
			{
				const FTransform Xf = C->GetComponentTransform();
				for (int32 S = 0; S < C->GetNumSections(); ++S)
				{
					const FProcMeshSection* Sec = C->GetProcMeshSection(S);
					if (!Sec || Sec->ProcIndexBuffer.Num() < 3) { continue; }
					TArray<FVector> Pos, Nrm;
					TArray<FVector2D> UVs;
					for (const FProcMeshVertex& V : Sec->ProcVertexBuffer)
					{
						Pos.Add(Xf.TransformPosition(V.Position) / MuseePlan::Cm);
						Nrm.Add(Xf.TransformVectorNoScale(V.Normal));
						UVs.Add(V.UV0);
					}
					TArray<int32> Keep;
					for (int32 T = 0; T + 2 < Sec->ProcIndexBuffer.Num(); T += 3)
					{
						const FVector Mid = (Pos[Sec->ProcIndexBuffer[T]] + Pos[Sec->ProcIndexBuffer[T + 1]] + Pos[Sec->ProcIndexBuffer[T + 2]]) / 3;
						if (InRegion(Mid)) { Keep.Add(T); }
					}
					if (Keep.Num() == 0) { continue; }
					const UMaterialInterface* Mat = C->GetMaterial(S);
					const bool bVisible = C->IsVisible() && !C->bHiddenInGame && Sec->bSectionVisible;
					const bool bCollide = C->IsCollisionEnabled() && Sec->bEnableCollision;
					Lines.Add(FString::Printf(TEXT("o %s|%s|%d|%s|%d|%d"), *It->GetClass()->GetName(), *C->GetName(), S,
											  Mat ? *Mat->GetName() : TEXT("none"), bVisible ? 1 : 0, bCollide ? 1 : 0));
					TMap<int32, int32> Remap;
					TArray<int32> Order;
					for (const int32 T : Keep)
					{
						for (int32 K = 0; K < 3; ++K)
						{
							const int32 Index = Sec->ProcIndexBuffer[T + K];
							if (!Remap.Contains(Index)) { Remap.Add(Index, Order.Num()); Order.Add(Index); }
						}
					}
					for (const int32 Index : Order) { Lines.Add(FString::Printf(TEXT("v %.6f %.6f %.6f"), Pos[Index].X, Pos[Index].Y, Pos[Index].Z)); }
					for (const int32 Index : Order) { Lines.Add(FString::Printf(TEXT("vn %.5f %.5f %.5f"), Nrm[Index].X, Nrm[Index].Y, Nrm[Index].Z)); }
					for (const int32 Index : Order) { Lines.Add(FString::Printf(TEXT("vt %.5f %.5f"), UVs[Index].X, UVs[Index].Y)); }
					for (const int32 T : Keep)
					{
						const int32 A = Base + Remap[Sec->ProcIndexBuffer[T]], B = Base + Remap[Sec->ProcIndexBuffer[T + 1]], D = Base + Remap[Sec->ProcIndexBuffer[T + 2]];
						Lines.Add(FString::Printf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d"), A, A, A, B, B, B, D, D, D));
					}
					Base += Order.Num();
					++Objects;
					Tris += Keep.Num();
				}
			}
		}
		FFileHelper::SaveStringArrayToFile(Lines, *Path);
		UE_LOG(LogMusee, Log, TEXT("NativeDump: %d sections, %d triangles to %s."), Objects, Tris, *Path);
	}

	constexpr double Miss = -1e9;   // no hit

	/** Trace the visitor's channel straight down from Start (m) over Depth (m): the hit height, or Miss. */
	double Down(UWorld* World, const FVector& Start, double Depth, const FCollisionQueryParams& Params)
	{
		FHitResult Hit;
		const FVector From = Start * MuseePlan::Cm, To = (Start - FVector(0, 0, Depth)) * MuseePlan::Cm;
		if (World->LineTraceSingleByChannel(Hit, From, To, ECC_Pawn, Params)) { return Hit.ImpactPoint.Z / MuseePlan::Cm; }
		return Miss;
	}

	FCollisionQueryParams IgnoringPond(UWorld* World)
	{
		FCollisionQueryParams Params(TEXT("NativeRoomChecks"), false);
		TArray<AActor*> Pond;
		MuseeWorld::FindParts(World, TEXT("pond"), Pond);
		TArray<AActor*> All = Pond;
		for (AActor* A : Pond)
		{
			TArray<AActor*> Attached;
			A->GetAttachedActors(Attached, true, true);
			All.Append(Attached);
		}
		Params.AddIgnoredActors(All);
		return Params;
	}

	void StairTrace(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		namespace L = MuseePlan::LongStair;
		const FCollisionQueryParams Params = IgnoringPond(World);
		for (const double Y : {-0.9, -0.6, -0.3, 0.0, 0.3, 0.6, 0.9})
		{
			double Prev = Miss, MaxStep = 0, StepAt = 0, MaxOff = 0, OffAt = 0, Lowest = 0;
			int32 Misses = 0, Samples = 0;
			for (double X = L::OpeningX0 - 1.0; X <= L::LandingEnd + 0.4; X += 0.01)
			{
				const double Walk = AReserveStairStructure::GetWalkHeightAt(X);
				double Top = X < L::OpeningX1 ? FMath::Min(0.5, Walk + 1.2) : FMath::Min(Walk + 1.2, L::SoffitHeight - 0.05);
				if (X < L::OpeningX0 || X > L::LandingEnd) { Top = X < L::OpeningX0 ? 0.5 : -4.0; }
				const double Z = Down(World, FVector(X, Y, Top), 3.0, Params);
				++Samples;
				if (Z <= Miss) { ++Misses; continue; }
				if (Prev > Miss && FMath::Abs(Z - Prev) > MaxStep) { MaxStep = FMath::Abs(Z - Prev); StepAt = X; }
				if (X >= L::OpeningX0 && X <= L::LandingEnd && FMath::Abs(Z - Walk) > MaxOff) { MaxOff = FMath::Abs(Z - Walk); OffAt = X; }
				Lowest = FMath::Min(Lowest, Z);
				Prev = Z;
			}
			UE_LOG(LogMusee, Log, TEXT("StairTrace y %+.1f: %d samples, %d misses; largest step %.1f cm at x %.2f; off the ramp by %.1f cm at most (x %.2f); lowest %.3f m."),
				   Y, Samples, Misses, MaxStep * 100.0, StepAt, MaxOff * 100.0, OffAt, Lowest);
		}
	}

	void AtriumTrace(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		namespace E = MuseePlan::Elan;
		const FCollisionQueryParams Params(TEXT("NativeRoomChecks"), false);
		int32 Samples = 0, Misses = 0, Raised = 0, Floor = 0;
		double MaxOff = 0;
		FVector OffAt = FVector::ZeroVector;
		TArray<FString> MissAt;
		for (double Rad = 2.6; Rad <= 14.25; Rad += 0.05)
		{
			for (int32 K = 0; K < 720; ++K)
			{
				const double T = 2 * UE_DOUBLE_PI * K / 720;
				const FVector P(E::CentreX + Rad * FMath::Cos(T), E::CentreY + Rad * FMath::Sin(T), 0.5);
				// Beyond the wall's face only in the west door.
				if (Rad > E::Radius - 0.01 && !(FMath::Abs(P.Y - E::CentreY) < 1.95 && P.X < E::CentreX)) { continue; }
				++Samples;
				const double Z = Down(World, P, 1.0, Params);
				if (Z <= Miss) { ++Misses; if (MissAt.Num() < 12) { MissAt.Add(FString::Printf(TEXT("(%.2f, %.2f)"), P.X, P.Y)); } continue; }
				if (Z > 0.01) { ++Raised; continue; }
				++Floor;
				if (FMath::Abs(Z) > MaxOff) { MaxOff = FMath::Abs(Z); OffAt = FVector(P.X, P.Y, Z); }
			}
		}
		UE_LOG(LogMusee, Log, TEXT("AtriumTrace: %d samples: %d on the floor (off level by %.2f mm at most, at %s), %d raised (plinths, the stele), %d misses %s."),
			   Samples, Floor, MaxOff * 1000.0, *OffAt.ToString(), Raised, Misses, *FString::Join(MissAt, TEXT(" ")));
	}

	/** musee.NativeLater <seconds> <command …>: run a console command later (after the collision has cooked). */
	void Later(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 2 || !World) { return; }
		const float Delay = FCString::Atof(*Args[0]);
		TArray<FString> Rest = Args;
		Rest.RemoveAt(0);
		const FString Command = FString::Join(Rest, TEXT(" "));
		TWeakObjectPtr<UWorld> WeakWorld(World);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld, Command](float)
		{
			if (UWorld* W = WeakWorld.Get())
			{
				if (GEngine) { GEngine->Exec(W, *Command); }
			}
			return false;
		}), Delay);
	}

	FAutoConsoleCommandWithWorldAndArgs LaterCommand(TEXT("musee.NativeLater"), TEXT("musee.NativeLater <seconds> <command>: run a console command later."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Later));
	FAutoConsoleCommandWithWorldAndArgs SpawnCommand(TEXT("musee.NativeSpawn"), TEXT("Place the native Atrium base and long stair at the origin if missing."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Spawn));
	FAutoConsoleCommandWithWorldAndArgs DumpCommand(TEXT("musee.NativeDump"), TEXT("Write the procedural meshes near the Atrium and the stair to an OBJ."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Dump));
	FAutoConsoleCommandWithWorldAndArgs StairCommand(TEXT("musee.StairTrace"), TEXT("Trace the walking surface down the long stair."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StairTrace));
	FAutoConsoleCommandWithWorldAndArgs AtriumCommand(TEXT("musee.AtriumTrace"), TEXT("Trace the walking surface over the Atrium's floor."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AtriumTrace));
}

#endif
