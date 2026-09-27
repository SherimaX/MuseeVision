#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MuseeCollision.h"
#include "MuseeHumanMeasure.h"
#include "MuseeSetting.h"
#include "Tests/VisitorTestUsable.h"
#include "VisitorAdapter.h"
#include "VisitorBody.h"
#include "VisitorEye.h"
#include "VisitorOverlay.h"
#include "VisitorPace.h"

// Visitor's connectors: the body made from the human measure, its feet, the Eye's camera, and the hand that uses what it sees.
namespace Musee::Visitor::Tests
{
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisitorBodyTest, "Musee.Visitor.Body", Flags)
	bool FVisitorBodyTest::RunTest(const FString& Parameters)
	{
		const AVisitorBody* Body = GetDefault<AVisitorBody>();
		TestEqual(TEXT("The pawn's class path is the stage contract's"), AVisitorBody::StaticClass()->GetPathName(), FString(TEXT("/Script/MuseeVisitor.VisitorBody")));
		TestTrue(TEXT("The capsule's radius is the measure's"), FMath::IsNearlyEqual(Body->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), static_cast<float>(ToEngine(FMuseeHumanMeasure::CapsuleRadius))));
		TestTrue(TEXT("and it is as tall as the body"), FMath::IsNearlyEqual(Body->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), static_cast<float>(ToEngine(FMuseeHumanMeasure::CapsuleHalfHeight))));
		const double EyeAboveFeet = Body->Camera()->GetRelativeLocation().Z + ToEngine(FMuseeHumanMeasure::CapsuleHalfHeight);
		TestTrue(TEXT("The eye stands at eye height"), FMath::IsNearlyEqual(EyeAboveFeet, ToEngine(FMuseeHumanMeasure::EyeHeight), 1e-3));
		const UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		TestTrue(TEXT("It steps up the measure's rise, no more"), FMath::IsNearlyEqual(Movement->MaxStepHeight, 22.0f));
		TestTrue(TEXT("and walks the measure's slope"), FMath::IsNearlyEqual(Movement->GetWalkableFloorAngle(), 20.0f, 1e-3f));
		TestTrue(TEXT("It is made at the default pace, the as-built museum's"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 260.0f));
		TestTrue(TEXT("and strides faster"), AVisitorBody::StrideSpeed() > AVisitorBody::WalkSpeed());
		TestTrue(TEXT("A hop lifts the feet one step's rise: sqrt(2 g 0.22)"), FMath::IsNearlyEqual(AVisitorBody::HopSpeed().Get(), 2.0772, 1e-3));
		TestTrue(TEXT("It reaches a pace and an arm's length"), FMath::IsNearlyEqual(AVisitorBody::UseDistance().Get(), 2.2522, 1e-3));
		TestNotNull(TEXT("The body creates the Eye"), Body->Eye());
		TestNotNull(TEXT("and the overlay"), Body->Overlay());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisitorEyeTest, "Musee.Visitor.Eye", Flags)
	bool FVisitorEyeTest::RunTest(const FString& Parameters)
	{
		TestTrue(TEXT("K / S cd/m2 meters at EV100 0"), FMath::IsNearlyEqual(UVisitorEye::ExposureOf(Nits(0.125)).Get(), 0.0, 1e-9));
		TestTrue(TEXT("Twice the luminance is one stop more"), FMath::IsNearlyEqual(UVisitorEye::ExposureOf(Nits(0.25)).Get(), 1.0, 1e-9));
		TestTrue(TEXT("The darkest scene metered is the mesopic floor, EV100 -4.6"), FMath::IsNearlyEqual(UVisitorEye::DarkestExposure().Get(), -4.644, 1e-3));
		TestTrue(TEXT("The brightest is a sunlit white, EV100 18.4"), FMath::IsNearlyEqual(UVisitorEye::BrightestExposure().Get(), 18.378, 1e-2));
		const FPostProcessSettings Camera = UVisitorEye::CameraSettings();
		TestTrue(TEXT("It meters the scene"), Camera.bOverride_AutoExposureMethod && Camera.AutoExposureMethod == AEM_Histogram);
		TestTrue(TEXT("without compensation"), Camera.bOverride_AutoExposureBias && Camera.AutoExposureBias == 0.0f);
		TestTrue(TEXT("with a shoulder on its tone curve"), Camera.bOverride_FilmShoulder && Camera.FilmShoulder > 0.0f);
		TestFalse(TEXT("It sets no transport field (Light's)"), Camera.bOverride_DynamicGlobalIlluminationMethod || Camera.bOverride_ReflectionMethod
			|| Camera.bOverride_LumenFinalGatherQuality || Camera.bOverride_LumenSceneLightingQuality);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisitorUseTest, "Musee.Visitor.Use", Flags)
	bool FVisitorUseTest::RunTest(const FString& Parameters)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		AVisitorBody* Body = World->SpawnActor<AVisitorBody>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
		AActor* Thing = World->SpawnActor<AActor>();
		UVisitorTestUsable* Iris = NewObject<UVisitorTestUsable>(Thing, TEXT("Iris"));
		Thing->SetRootComponent(Iris);
		Iris->SetBoxExtent(FVector(10.0));
		Iris->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Iris->SetCollisionResponseToAllChannels(ECR_Ignore);
		Iris->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
		Iris->RegisterComponent();
		if (TestNotNull(TEXT("The body stands in the world"), Body))
		{
			TestTrue(TEXT("The Eye's volume is unbound"), Body->Eye()->Volume() && Body->Eye()->Volume()->bUnbound);
			Thing->SetActorLocation(Body->EyeLocation() + FVector(100.0, 0.0, 0.0));
			Body->Look();
			TestTrue(TEXT("A usable a metre ahead is looked at"), Body->LookedAt() == Iris);
			TestTrue(TEXT("and the overlay says what using it does"), Body->Overlay()->Text().ToString().Contains(TEXT("A line of light")));
			Body->UseLooked();
			TestEqual(TEXT("The use key uses it"), Iris->Uses, 1);
			Thing->SetActorLocation(Body->EyeLocation() + FVector(ToEngine(AVisitorBody::UseDistance()) + 50.0, 0.0, 0.0));
			Body->Look();
			TestNull(TEXT("Beyond reach it isn't"), Body->LookedAt());
			TestTrue(TEXT("and the overlay is clear"), Body->Overlay()->Text().IsEmpty());
		}
		World->DestroyWorld(false);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisitorPaceTest, "Musee.Visitor.Pace", Flags)
	bool FVisitorPaceTest::RunTest(const FString& Parameters)
	{
		const FVisitorPace AsBuilt = FVisitorPace::Of(FVisitorPace::AsBuilt());
		const FVisitorPace Physical = FVisitorPace::Of(FVisitorPace::Physical());
		TestTrue(TEXT("The as-built pace walks at 2.6 m/s and strides at 5.2"), AsBuilt.Walk.Get() == 2.6 && AsBuilt.Stride.Get() == 5.2);
		TestTrue(TEXT("The physical pace walks at 1.35 m/s and strides at 2.0"), Physical.Walk.Get() == 1.35 && Physical.Stride.Get() == 2.0);
		FMuseeSetting* Setting = FMuseeSetting::Find(FVisitorPace::SettingKey());
		if (!TestNotNull(TEXT("Visitor registers visitor.pace"), Setting))
		{
			return false;
		}
		TestTrue(TEXT("a choice of as-built and physical"), Setting->Choices() == TArray<FName>({ FVisitorPace::AsBuilt(), FVisitorPace::Physical() }));
		TestTrue(TEXT("with its command"), FMuseeSetting::CommandName(Setting->Key()) == FName(TEXT("musee.visitor.set-pace")));
		const FName Kept = Setting->Choice();

		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		AVisitorBody* Body = World->SpawnActor<AVisitorBody>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
		if (TestNotNull(TEXT("The body stands in the world"), Body))
		{
			const UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
			Setting->SetChoice(FVisitorPace::Physical());
			Body->DispatchBeginPlay();
			TestTrue(TEXT("At play it takes the pace chosen"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 135.0f));
			Body->StrideOn();
			TestTrue(TEXT("and strides at it"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 200.0f));
			Setting->SetChoice(FVisitorPace::AsBuilt());
			TestTrue(TEXT("A new pace takes effect at once, even mid-stride"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 520.0f));
			Body->StrideOff();
			TestTrue(TEXT("and walking again, it walks at it"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 260.0f));
		}
		World->DestroyWorld(false);
		Setting->SetChoice(Kept);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisitorFeetTest, "Musee.Visitor.Feet", Flags)
	bool FVisitorFeetTest::RunTest(const FString& Parameters)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		// A walkable floor whose top is the world's z 0, and a body whose capsule stands a centimetre over it.
		AActor* Slab = World->SpawnActor<AActor>();
		UBoxComponent* Floor = NewObject<UBoxComponent>(Slab, TEXT("Floor"));
		Slab->SetRootComponent(Floor);
		Floor->SetBoxExtent(FVector(200.0, 200.0, 10.0));
		Floor->SetRelativeLocation(FVector(0.0, 0.0, -10.0));
		Floor->SetCollisionProfileName(Collision::Walkable.Profile());
		Floor->RegisterComponent();
		const double HalfHeight = ToEngine(FMuseeHumanMeasure::CapsuleHalfHeight);
		AVisitorBody* Body = World->SpawnActor<AVisitorBody>(FVector(0.0, 0.0, HalfHeight + 1.0), FRotator::ZeroRotator);
		if (TestNotNull(TEXT("The body stands in the world"), Body))
		{
			UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
			Movement->SetMovementMode(MOVE_Falling);
			TestTrue(TEXT("Off the ground, the feet are the bottom of the capsule"), FMath::IsNearlyEqual(Body->FeetLocation().Z, 1.0, 1e-3));
			// Walking finds the floor and holds the capsule the movement's skin above it.
			Movement->SetMovementMode(MOVE_Walking);
			const double Capsule = Body->GetActorLocation().Z - HalfHeight;
			TestTrue(FString::Printf(TEXT("Standing, the capsule is held a skin of 1.9 to 2.4 cm over the floor (%.2f cm)"), Capsule),
				Capsule >= UCharacterMovementComponent::MIN_FLOOR_DIST - 1e-3 && Capsule <= UCharacterMovementComponent::MAX_FLOOR_DIST + 1e-3);
			TestTrue(FString::Printf(TEXT("and the feet are on the floor (z %.3f cm)"), Body->FeetLocation().Z), FMath::IsNearlyEqual(Body->FeetLocation().Z, 0.0, 1e-2));
		}
		World->DestroyWorld(false);
		return true;
	}

	/** An adapter for the test: active or not as it is told. */
	class FTestAdapter final : public IVisitorAdapter
	{
	public:
		explicit FTestAdapter(bool bInActive) : bActive(bInActive) {}
		virtual FName Name() const override { return TEXT("test"); }
		virtual bool IsActive() const override { return bActive; }
		virtual void Adopt(AVisitorBody& Body) override {}
		bool bActive = false;
	};

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisitorAdapterTest, "Musee.Visitor.Adapter", Flags)
	bool FVisitorAdapterTest::RunTest(const FString& Parameters)
	{
		const TSharedPtr<IVisitorAdapter> Before = ActiveAdapter();
		TestFalse(TEXT("In the editor no headset adapter is active"), Before.IsValid());
		const TSharedRef<FTestAdapter> Idle = MakeShared<FTestAdapter>(false);
		RegisterAdapter(Idle);
		TestFalse(TEXT("An adapter whose device isn't running isn't chosen"), ActiveAdapter().IsValid());
		Idle->bActive = true;
		TestTrue(TEXT("One whose device runs is"), ActiveAdapter().Get() == &Idle.Get());
		UnregisterAdapter(TEXT("test"));
		TestFalse(TEXT("and unregistered, it is gone"), ActiveAdapter().IsValid());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisitorOverlayTest, "Musee.Visitor.Overlay", Flags)
	bool FVisitorOverlayTest::RunTest(const FString& Parameters)
	{
		UVisitorOverlay* Overlay = NewObject<UVisitorOverlay>();
		FMuseeCaption Caption;
		Caption.Title = FText::FromString(TEXT("Water Lilies"));
		Caption.Credit = FText::FromString(TEXT("Musee de l'Orangerie"));
		Overlay->ShowCaption(Caption);
		TestEqual(TEXT("A caption's lines show, the empty one left out"), Overlay->Text().ToString(), FString(TEXT("Water Lilies\nMusee de l'Orangerie")));
		Overlay->ShowMessage(FText::FromString(TEXT("The museum's clock stands at 10:00.")));
		TestTrue(TEXT("A message shows under it"), Overlay->Text().ToString().EndsWith(TEXT("10:00.")));
		Overlay->Pass(Seconds(10.0));
		TestFalse(TEXT("and is gone once read twice"), Overlay->Text().ToString().Contains(TEXT("10:00")));
		Overlay->Clear();
		TestTrue(TEXT("Clear leaves nothing"), Overlay->Text().IsEmpty());
		return true;
	}
}

#endif
