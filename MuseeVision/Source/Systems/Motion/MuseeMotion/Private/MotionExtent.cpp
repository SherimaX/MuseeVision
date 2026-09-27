#include "MotionExtent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "MotionConditions.h"
#include "MuseeUnits.h"

namespace
{
	/** A length within a hundredth of a millimetre of the last is the same length: nothing is set again. */
	constexpr double SameLengthMeters = 1.0e-5;
}

UMotionExtent::UMotionExtent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMotionExtent::ConfigureTelescope(const FMotionExtentSpec& InSpec, const TArray<USceneComponent*>& InCarriages, double DrawnLength)
{
	ExtentSpec = InSpec;
	Carriages.Reset();
	CarriageHomes.Reset();
	for (USceneComponent* Carriage : InCarriages)
	{
		Carriages.Add(Carriage);
		CarriageHomes.Add(Carriage ? Carriage->GetRelativeTransform() : FTransform::Identity);
	}
	bSet = false;
	SetLength(DrawnLength);
}

void UMotionExtent::ConfigureTurns(const FMotionExtentSpec& InSpec, UInstancedStaticMeshComponent* InColumn, const FTransform& InFirstTurn, double DrawnLength)
{
	ExtentSpec = InSpec;
	Column = InColumn;
	FirstTurn = InFirstTurn;
	bSet = false;
	SetLength(DrawnLength);
}

void UMotionExtent::Advance()
{
	SetLength(ExtentSpec.Length.At(Musee::Motion::ChannelValue(GetWorld(), ExtentSpec.Length.Channel)));
}

void UMotionExtent::SetLength(double InLength)
{
	if (bSet && FMath::IsNearlyEqual(InLength, Length, SameLengthMeters))
	{
		return;
	}
	bSet = true;
	Length = InLength;
	const FVector Along = ExtentSpec.Axis.GetSafeNormal();
	for (int32 Stage = 0; Stage < Carriages.Num(); ++Stage)
	{
		if (USceneComponent* Carriage = Carriages[Stage])
		{
			FTransform Pose = CarriageHomes[Stage];
			Pose.AddToTranslation(Along * Musee::ToEngine(Musee::Meters(ExtentSpec.StageOffset(Stage, Length))));
			Carriage->SetRelativeTransform(Pose);
		}
	}
	if (!Column)
	{
		return;
	}
	// Turn k stands k pitches back from the anchored end along the axis, the last one still forming in the housing.
	const int32 Count = ExtentSpec.TurnsAt(Length);
	TArray<FTransform> Turns;
	for (int32 Turn = 0; Turn < Count; ++Turn)
	{
		FTransform Pose = FirstTurn;
		Pose.AddToTranslation(Along * Musee::ToEngine(Musee::Meters(Turn * ExtentSpec.Turns.Pitch)));
		Turns.Add(Pose);
	}
	Column->ClearInstances();
	Column->AddInstances(Turns, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false);
}

void UMotionExtent::BeginPlay()
{
	Super::BeginPlay();
	bSet = false;
	Advance();
}

void UMotionExtent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance();
}
