#include "Salon/PondLift.h"

#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "Visitor/MuseeCharacter.h"
#include "Visitor/MuseeWorld.h"
#include "Salon/SalonInterior.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ShapeComponent.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "Pond"

namespace
{
	// PondPlan (Reserve.swift).
	const FVector2D PondCentre(-86.5, 0);
	constexpr double Lift = 2.6;
	constexpr double LiftSeconds = 5;
	constexpr double OpeningX0 = -90.4, OpeningX1 = -82.9, OpeningY0 = -1.2, OpeningY1 = 1.2;
	constexpr float ArmedSeconds = 5.f;
	constexpr double LeaveDistance = 16;   // the pond settles once you are this far from it
	constexpr double UseDistance = 6.5;    // the buttons show within this distance

	void Collect(AActor* Actor, TArray<TWeakObjectPtr<AActor>>& Out)
	{
		Out.Add(Actor);
		TArray<AActor*> Children;
		Actor->GetAttachedActors(Children);
		for (AActor* Child : Children) { Collect(Child, Out); }
	}

	void MakeMovable(AActor* Actor)
	{
		if (USceneComponent* Root = Actor ? Actor->GetRootComponent() : nullptr) { Root->SetMobility(EComponentMobility::Movable); }
	}

	void Show(AActor* Actor, bool bShow)
	{
		if (!Actor) { return; }
		Actor->SetActorHiddenInGame(!bShow);
		Actor->SetActorEnableCollision(bShow);
	}

	FString PrimOf(const AActor* Actor)
	{
		for (const FName& Tag : Actor->Tags)
		{
			FString S = Tag.ToString();
			if (S.RemoveFromStart(TEXT("prim:"))) { return S; }
		}
		return FString();
	}
}

APondLift::APondLift()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void APondLift::BeginPlay()
{
	Super::BeginPlay();
	FindParts();
	Apply();
}

void APondLift::FindParts()
{
	UWorld* World = GetWorld();
	TArray<AActor*> Found;
	MuseeWorld::FindParts(World, TEXT("pond"), Found);
	if (Found.Num() == 0)
	{
		UE_LOG(LogMusee, Warning, TEXT("Pond: no pond found; import the Reserve wing."));
		return;
	}
	Pond = Found[0];
	PondHome = Found[0]->GetActorLocation();
	Collect(Found[0], PondActors);
	for (const TWeakObjectPtr<AActor>& A : PondActors)
	{
		if (ASalonPond* P = Cast<ASalonPond>(A.Get())) { Edge = P; }
		MakeMovable(A.Get());
		// The golden lily is what you use (the whole pond answers too, for a forgiving aim).
		MuseeWorld::RegisterUse(A.Get(), this);
	}

	Found.Reset();
	MuseeWorld::FindParts(World, TEXT("pond_post"), Found);
	for (AActor* A : Found)
	{
		MakeMovable(A);
		Posts.Add({A, A->GetActorLocation()});
	}

	Found.Reset();
	MuseeWorld::FindParts(World, TEXT("pond_rim_light"), Found);
	// Rim_light_01 … 32: they light in order, starting nearest the lily.
	Found.Sort([](const AActor& A, const AActor& B) { return PrimOf(&A) < PrimOf(&B); });
	for (AActor* A : Found)
	{
		MakeMovable(A);
		// The pond's edge carries its own line of light in the lip: the imported lights stay hidden.
		if (Edge.IsValid()) { Show(A, false); continue; }
		RimLights.Add(A);
	}

	Found.Reset();
	MuseeWorld::FindParts(World, TEXT("pond_halo"), Found);
	if (Found.Num() > 0) { MakeMovable(Found[0]); Halo = Found[0]; }

	UE_LOG(LogMusee, Log, TEXT("Pond: %d parts, %d posts, %d rim lights."), PondActors.Num(), Posts.Num(), RimLights.Num());
}

bool APondLift::Near(const AMuseeCharacter* Visitor) const
{
	if (!Visitor) { return false; }
	const FVector Feet = Visitor->FeetLocation() / MuseePlan::Cm;
	return FMath::Abs(Feet.Z) < 0.5 && (FVector2D(Feet.X, Feet.Y) - PondCentre).Size() < UseDistance;
}

bool APondLift::OverOpening(const FVector& Feet) const
{
	return Feet.X > OpeningX0 && Feet.X < OpeningX1 && Feet.Y > OpeningY0 && Feet.Y < OpeningY1;
}

bool APondLift::CanUse(const AMuseeCharacter* Visitor) const
{
	if (!Visitor || !Pond.IsValid()) { return false; }
	switch (State)
	{
	case EState::Down:
	case EState::Armed:
		return true;
	case EState::Up:
	{
		// Lowered only from the oval floor, never from under it or on the stair.
		const FVector Feet = Visitor->FeetLocation() / MuseePlan::Cm;
		return Feet.Z > -0.05 && !OverOpening(Feet);
	}
	default:
		return false;
	}
}

FText APondLift::UseTitle() const
{
	switch (State)
	{
	case EState::Down: return LOCTEXT("Wake", "Wake the golden lily");
	case EState::Armed: return LOCTEXT("Lift", "Lift the pond");
	case EState::Up: return LOCTEXT("Lower", "Lower the pond");
	default: return FText::GetEmpty();
	}
}

bool APondLift::CanInteract(const AMuseeCharacter* Visitor, const FHitResult& Hit) const { return CanUse(Visitor); }
void APondLift::Interact(AMuseeCharacter* Visitor, const FHitResult& Hit) { Use(Visitor); }
FText APondLift::InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const { return UseTitle(); }

void APondLift::GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const
{
	if (Near(Visitor) && CanUse(Visitor)) { Out.Add({TEXT("pond.use"), UseTitle()}); }
}

void APondLift::RunPrompt(AMuseeCharacter* Visitor, FName Id)
{
	if (Id == TEXT("pond.use")) { Use(Visitor); }
}

void APondLift::Use(AMuseeCharacter* Visitor)
{
	if (!CanUse(Visitor)) { return; }
	switch (State)
	{
	case EState::Down:
		State = EState::Armed;
		ArmedTimer = ArmedSeconds;
		RimReveal = 0.f;
		Visitor->SetMessage(LOCTEXT("Armed", "A line of light runs round the rim. Use the golden lily again to lift the pond."));
		break;
	case EState::Armed:
		State = EState::Rising;
		Visitor->SetMessage(LOCTEXT("Rising", "The pond rises on four bronze posts…"));
		break;
	case EState::Up:
		State = EState::Lowering;
		Visitor->SetMessage(FText::GetEmpty());
		break;
	default:
		break;
	}
}

void APondLift::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Pond.IsValid()) { return; }
	AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const FVector Feet = Visitor ? Visitor->FeetLocation() / MuseePlan::Cm : FVector::ZeroVector;
	switch (State)
	{
	case EState::Armed:
		RimReveal = FMath::Min(1.f, RimReveal + DeltaSeconds);
		ArmedTimer -= DeltaSeconds;
		if (ArmedTimer <= 0.f)
		{
			State = EState::Down;
			if (Visitor) { Visitor->SetMessage(FText::GetEmpty()); }
		}
		break;
	case EState::Rising:
		Height = FMath::Min(Lift, Height + DeltaSeconds * Lift / LiftSeconds);
		if (Height >= Lift)
		{
			State = EState::Up;
			if (Visitor) { Visitor->SetMessage(LOCTEXT("Up", "Beneath the pond, a long stair leads down to the Reserve.")); }
		}
		break;
	case EState::Lowering:
	{
		Height = FMath::Max(0.0, Height - DeltaSeconds * Lift / LiftSeconds);
		if (Height <= 0) { State = EState::Down; RimReveal = 0.f; }
		// Never lower it onto anyone: a visitor on the oval's floor under its edge (off the opening) is moved out.
		const double T = Height / Lift, Bottom = T * T * (3 - 2 * T) * Lift;
		if (Visitor && Feet.Z > -0.05 && Feet.Z < Bottom - 0.05 && Bottom < Feet.Z + 2.0 && !OverOpening(Feet)
			&& ASalonPond::IsOver(FVector2D(Feet.X, Feet.Y), 0.32))
		{
			const FVector2D To = ASalonPond::PushOut(FVector2D(Feet.X, Feet.Y), 0.32);
			const FVector Delta = FVector(To.X - Feet.X, To.Y - Feet.Y, 0.0) * MuseePlan::Cm;
			Visitor->SetActorLocation(Visitor->GetActorLocation() + Delta, false, nullptr, ETeleportType::TeleportPhysics);
		}
		break;
	}
	case EState::Up:
		if (Visitor && Feet.Z < -3 && Visitor->Message().ToString().StartsWith(TEXT("Beneath"))) { Visitor->SetMessage(FText::GetEmpty()); }
		// It settles again once you have left the oval (never while you are below).
		if (Visitor && Feet.Z > -0.05 && (FVector2D(Feet.X, Feet.Y) - PondCentre).Size() > LeaveDistance) { State = EState::Lowering; }
		break;
	case EState::Down:
		RimReveal = 0.f;
		break;
	}
	Apply();
}

void APondLift::Apply()
{
	if (!Pond.IsValid()) { return; }
	// Ease the motion.
	const double T = Height / Lift;
	const double Eased = T * T * (3 - 2 * T) * Lift;
	Pond->SetActorLocation(PondHome + FVector(0, 0, Eased * MuseePlan::Cm));
	for (const FPost& Post : Posts)
	{
		AActor* A = Post.Actor.Get();
		if (!A) { continue; }
		Show(A, Eased > 0.02);
		// The post is a 1 m cylinder centred on its origin: scaled to the lift, standing on the floor.
		FVector Scale = A->GetActorScale3D();
		Scale.Z = FMath::Max(0.001, Eased);
		A->SetActorScale3D(Scale);
		A->SetActorLocation(FVector(Post.Home.X, Post.Home.Y, Eased * MuseePlan::Cm / 2));
	}
	const int32 Lit = State == EState::Down ? 0 : FMath::FloorToInt(RimReveal * RimLights.Num());
	for (int32 k = 0; k < RimLights.Num(); ++k) { Show(RimLights[k].Get(), k < Lit); }
	if (ASalonPond* P = Edge.Get()) { P->SetRimReveal(State == EState::Down ? 0.f : RimReveal); }
	Show(Halo.Get(), State != EState::Down);
}

#if !UE_BUILD_SHIPPING
void APondLift::CheckClearance() const
{
	UWorld* World = GetWorld();
	if (!World || !Pond.IsValid()) { UE_LOG(LogMusee, Warning, TEXT("PondCheck: no pond.")); return; }
	FCollisionQueryParams Params(TEXT("PondCheck"), true);
	for (const TWeakObjectPtr<AActor>& A : PondActors) { if (A.IsValid()) { Params.AddIgnoredActor(A.Get()); } }
	for (const FPost& P : Posts) { if (P.Actor.IsValid()) { Params.AddIgnoredActor(P.Actor.Get()); } }
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	// The swept volume: the pond's outline (to the coping's face, less 2 cm) from the floor to the tallest iris at the
	// top of the lift (0.38 + 0.9 + 2.6 m, and some).
	const double Top = 4.3;
	int32 Samples = 0, Hits = 0;
	TArray<FString> Report;
	for (double X = PondCentre.X - 4.5; X <= PondCentre.X + 4.5; X += 0.05)
	{
		for (double Y = -2.6; Y <= 2.6; Y += 0.05)
		{
			if (!ASalonPond::IsOver(FVector2D(X, Y), -0.06)) { continue; }
			++Samples;
			TArray<FHitResult> Found;
			World->LineTraceMultiByObjectType(Found, FVector(X, Y, Top) * MuseePlan::Cm, FVector(X, Y, 0.02) * MuseePlan::Cm, Objects, Params);
			for (const FHitResult& H : Found)
			{
				const UPrimitiveComponent* C = H.GetComponent();
				if (!C || !C->IsVisible() || C->bHiddenInGame || C->IsA<UShapeComponent>()) { continue; }
				++Hits;
				if (Report.Num() < 16)
				{
					Report.Add(FString::Printf(TEXT("(%.2f, %.2f) h %.2f: %s.%s"), X, Y, H.ImpactPoint.Z / MuseePlan::Cm,
						H.GetActor() ? *H.GetActor()->GetActorNameOrLabel() : TEXT("?"), *C->GetName()));
				}
			}
		}
	}
	UE_LOG(LogMusee, Log, TEXT("PondCheck: %d columns through the pond's swept volume (floor to %.1f m): %d visible obstacles%s%s"),
		Samples, Top, Hits, Report.Num() ? TEXT(": ") : TEXT("."), *FString::Join(Report, TEXT("; ")));
	// The posts stand under the pond's outline (their tops meet the tray at every height of the lift).
	for (const FPost& P : Posts)
	{
		const FVector At = P.Home / MuseePlan::Cm;
		UE_LOG(LogMusee, Log, TEXT("PondCheck: post at (%.2f, %.2f): under the pond %s."), At.X, At.Y,
			ASalonPond::IsOver(FVector2D(At.X, At.Y), -0.3) ? TEXT("yes") : TEXT("NO"));
	}
}

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GPondCheck(TEXT("musee.PondCheck"), TEXT("Logs anything in the lifting pond's swept volume."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			for (TActorIterator<APondLift> It(World); It; ++It) { It->CheckClearance(); }
		}));

	FAutoConsoleCommandWithWorldAndArgs GPondEdge(TEXT("musee.PondEdge"), TEXT("musee.PondEdge 0|1: hide or show the pond's edge and its planting (to measure their cost)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const bool bShow = Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0;
			int32 N = 0;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->IsA<ASalonPond>() || It->IsA<ASalonPondPlants>()) { It->SetActorHiddenInGame(!bShow); ++N; }
			}
			UE_LOG(LogMusee, Log, TEXT("PondEdge %s (%d actors)."), bShow ? TEXT("shown") : TEXT("hidden"), N);
		}));

	FAutoConsoleCommandWithWorldAndArgs GEyeHeight(TEXT("musee.EyeHeight"), TEXT("musee.EyeHeight <m>: the visitor's eyes above the feet (1.6)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AMuseeCharacter* V = World ? Cast<AMuseeCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
			if (!V || !V->Camera) { return; }
			const double H = Args.Num() ? FCString::Atod(*Args[0]) : MuseePlan::EyeHeight;
			const float Half = V->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
			V->Camera->SetRelativeLocation(FVector(0, 0, H * MuseePlan::Cm - Half));
			UE_LOG(LogMusee, Log, TEXT("EyeHeight %.2f m."), H);
		}));
}
#else
void APondLift::CheckClearance() const {}
#endif

#undef LOCTEXT_NAMESPACE
