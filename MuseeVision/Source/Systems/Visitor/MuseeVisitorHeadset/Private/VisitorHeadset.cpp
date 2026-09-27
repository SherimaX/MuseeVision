#include "VisitorHeadset.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "IXRTrackingSystem.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "MotionControllerComponent.h"
#include "MuseeHumanMeasure.h"
#include "MuseeUsable.h"
#include "VisitorBody.h"
#include "VisitorOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MuseeVisitorHeadset"

namespace Musee::Visitor::Headset
{
	namespace
	{
		/** The comfort turn: the as-built headset museum's 30 degrees (MuseeVRCharacter, TurnDegrees). */
		constexpr double ComfortTurnDegrees = 30.0;

		/** How far a step reaches: the as-built headset museum's 15 m (MuseeVRCharacter, StepRange). */
		constexpr double StepReachMeters = 15.0;

		/**
		 * A step is a blink: the view fades in from black over the length of a blink of the eye, 100 to 400 ms
		 * (H. Caffier, W. Erdmann and P. Ullsperger, "Experimental evaluation of eye-blink parameters", European
		 * Journal of Applied Physiology 89, 2003), taken at its shorter end.
		 */
		constexpr float BlinkSeconds = 0.15f;

		/**
		 * The overlay's panel hangs a metre ahead of the eyes and a quarter of a metre below their line, as a label is
		 * read held a little low; one of its pixels is a millimetre, so its 24 px type stands 24 mm tall at a metre,
		 * the size of a wall label's title read from arm's length.
		 */
		constexpr double PanelAheadMeters = 1.0;
		constexpr double PanelBelowEyeMeters = 0.25;
		constexpr double MetersPerPanelPixel = 0.001;
		const FVector2D PanelPixels(1000.0, 250.0);
		constexpr float PanelFontPixels = 24.0f;

		/** A body follows the head once it has drifted this far from the body's centre: a tenth of the body's radius. */
		constexpr double HeadDriftFraction = 0.1;

		/** The OpenXR aim poses of the Touch controllers. */
		const FName RightAim(TEXT("RightAim"));
		const FName LeftAim(TEXT("LeftAim"));

		/** The headset's controls take precedence over the screen's, which stay mapped beneath them. */
		constexpr int32 HeadsetControlsPriority = 1;

		UInputAction* MakeAction(UObject& Outer, TMap<FName, TObjectPtr<UInputAction>>& Actions, const TCHAR* Name, EInputActionValueType Type)
		{
			UInputAction* Action = NewObject<UInputAction>(&Outer, FName(Name));
			Action->ValueType = Type;
			Actions.Add(Name, Action);
			return Action;
		}

		/** The height of the eyes over the capsule's centre. */
		double EyeOverCentre()
		{
			return Musee::ToEngine(FMuseeHumanMeasure::EyeHeight - FMuseeHumanMeasure::CapsuleHalfHeight);
		}
	}
}

UVisitorHeadset::UVisitorHeadset()
{
	PrimaryComponentTick.bCanEverTick = true;
}

AVisitorBody* UVisitorHeadset::Body() const
{
	return Cast<AVisitorBody>(GetOwner());
}

void UVisitorHeadset::BeginPlay()
{
	Super::BeginPlay();
	AVisitorBody* Visitor = Body();
	if (!Visitor) { return; }
	TakeTheEyes(*Visitor);
	MakePanel(*Visitor);
	BuildControls();
	ControlsHandle = Visitor->OnControlsReady().AddUObject(this, &UVisitorHeadset::AddControls);
	if (Visitor->InputComponent && Visitor->GetController()) { AddControls(*Visitor); }
}

void UVisitorHeadset::EndPlay(EEndPlayReason::Type Reason)
{
	if (AVisitorBody* Visitor = Body()) { Visitor->OnControlsReady().Remove(ControlsHandle); }
	Super::EndPlay(Reason);
}

void UVisitorHeadset::TakeTheEyes(AVisitorBody& Visitor)
{
	using namespace Musee::Visitor::Headset;
	TrackingOrigin = NewObject<USceneComponent>(&Visitor, TEXT("TrackingOrigin"), RF_Transient);
	TrackingOrigin->SetupAttachment(Visitor.GetCapsuleComponent());
	TrackingOrigin->SetRelativeLocation(FVector(0.0, 0.0, EyeOverCentre()));
	TrackingOrigin->RegisterComponent();
	UCameraComponent* Eyes = Visitor.Camera();
	Eyes->AttachToComponent(TrackingOrigin, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Eyes->bUsePawnControlRotation = false;
	Eyes->bLockToHmd = true;
	Visitor.bUseControllerRotationYaw = false;
	for (const TPair<TObjectPtr<UMotionControllerComponent>*, FName> Hand : { TPair<TObjectPtr<UMotionControllerComponent>*, FName>(&RightHand, RightAim), TPair<TObjectPtr<UMotionControllerComponent>*, FName>(&LeftHand, LeftAim) })
	{
		UMotionControllerComponent* Controller = NewObject<UMotionControllerComponent>(&Visitor, NAME_None, RF_Transient);
		Controller->SetTrackingMotionSource(Hand.Value);
		Controller->SetupAttachment(TrackingOrigin);
		Controller->RegisterComponent();
		*Hand.Key = Controller;
	}
	if (GEngine->XRSystem.IsValid())
	{
		GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::Local);
		GEngine->XRSystem->ResetOrientationAndPosition(0.0f);
	}
}

void UVisitorHeadset::MakePanel(AVisitorBody& Visitor)
{
	using namespace Musee::Visitor::Headset;
	Panel = NewObject<UWidgetComponent>(&Visitor, TEXT("OverlayPanel"), RF_Transient);
	Panel->SetupAttachment(Visitor.Camera());
	Panel->SetWidgetSpace(EWidgetSpace::World);
	Panel->SetDrawSize(PanelPixels);
	// A widget shows its face along its +x: turned to face the eyes.
	Panel->SetRelativeLocationAndRotation(FVector(Musee::ToEngine(Musee::Meters(PanelAheadMeters)), 0.0, -Musee::ToEngine(Musee::Meters(PanelBelowEyeMeters))), FRotator(0.0, 180.0, 0.0));
	Panel->SetRelativeScale3D(FVector(Musee::ToEngine(Musee::Meters(MetersPerPanelPixel))));
	Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Panel->SetSlateWidget(SNew(SBorder).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SAssignNew(PanelText, STextBlock).Justification(ETextJustify::Center).AutoWrapText(true)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", static_cast<int32>(PanelFontPixels)))
		]);
	Panel->RegisterComponent();
}

void UVisitorHeadset::BuildControls()
{
	using namespace Musee::Visitor::Headset;
	Controls = NewObject<UInputMappingContext>(this, TEXT("VisitorHeadset"));
	const auto Map = [this](const TCHAR* Name, EInputActionValueType Type, std::initializer_list<FKey> Keys)
	{
		UInputAction* Action = MakeAction(*this, Actions, Name, Type);
		for (const FKey& Key : Keys)
		{
			FEnhancedActionKeyMapping& Mapping = Controls->MapKey(Action, Key);
			if (Type != EInputActionValueType::Boolean) { Mapping.Modifiers.Add(NewObject<UInputModifierDeadZone>(this)); }
		}
	};
	Map(TEXT("Walk"), EInputActionValueType::Axis2D, { EKeys::OculusTouch_Left_Thumbstick_2D });
	Map(TEXT("Turn"), EInputActionValueType::Axis1D, { EKeys::OculusTouch_Right_Thumbstick_X });
	Map(TEXT("Point"), EInputActionValueType::Boolean, { EKeys::OculusTouch_Right_Trigger_Click });
	Map(TEXT("Use"), EInputActionValueType::Boolean, { EKeys::OculusTouch_Right_A_Click });
	Map(TEXT("Hour"), EInputActionValueType::Boolean, { EKeys::OculusTouch_Right_B_Click });
	Map(TEXT("City"), EInputActionValueType::Boolean, { EKeys::OculusTouch_Left_X_Click });
	Map(TEXT("Recentre"), EInputActionValueType::Boolean, { EKeys::OculusTouch_Left_Y_Click, EKeys::OculusTouch_Left_Menu_Click });
	Map(TEXT("Stride"), EInputActionValueType::Boolean, { EKeys::OculusTouch_Left_Thumbstick_Click });
}

void UVisitorHeadset::AddControls(AVisitorBody& Visitor)
{
	using namespace Musee::Visitor::Headset;
	const APlayerController* Player = Cast<APlayerController>(Visitor.GetController());
	UEnhancedInputLocalPlayerSubsystem* Input = Player ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer()) : nullptr;
	UEnhancedInputComponent* Bindings = Cast<UEnhancedInputComponent>(Visitor.InputComponent);
	if (!Input || !Bindings) { return; }
	Input->AddMappingContext(Controls, HeadsetControlsPriority);
	Bindings->BindAction(Actions[TEXT("Walk")], ETriggerEvent::Triggered, this, &UVisitorHeadset::Walk);
	Bindings->BindAction(Actions[TEXT("Turn")], ETriggerEvent::Triggered, this, &UVisitorHeadset::Turn);
	Bindings->BindAction(Actions[TEXT("Turn")], ETriggerEvent::Completed, this, &UVisitorHeadset::TurnReleased);
	Bindings->BindAction(Actions[TEXT("Point")], ETriggerEvent::Started, this, &UVisitorHeadset::PointStarted);
	Bindings->BindAction(Actions[TEXT("Point")], ETriggerEvent::Completed, this, &UVisitorHeadset::PointEnded);
	Bindings->BindAction(Actions[TEXT("Use")], ETriggerEvent::Started, this, &UVisitorHeadset::UseGazed);
	Bindings->BindAction(Actions[TEXT("Hour")], ETriggerEvent::Started, this, &UVisitorHeadset::AnotherHour);
	Bindings->BindAction(Actions[TEXT("City")], ETriggerEvent::Started, this, &UVisitorHeadset::OtherCity);
	Bindings->BindAction(Actions[TEXT("Recentre")], ETriggerEvent::Started, this, &UVisitorHeadset::Recentre);
	Bindings->BindAction(Actions[TEXT("Stride")], ETriggerEvent::Started, this, &UVisitorHeadset::StrideOn);
	Bindings->BindAction(Actions[TEXT("Stride")], ETriggerEvent::Completed, this, &UVisitorHeadset::StrideOff);
}

void UVisitorHeadset::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AVisitorBody* Visitor = Body();
	if (!Visitor) { return; }
	FollowHead();
	if (PanelText.IsValid()) { PanelText->SetText(Visitor->Overlay()->Text()); }
}

void UVisitorHeadset::Walk(const FInputActionValue& Value)
{
	AVisitorBody* Visitor = Body();
	const FVector2D Axis = Value.Get<FVector2D>();
	FVector Forward = Visitor->Camera()->GetForwardVector();
	Forward.Z = 0.0;
	Forward = Forward.GetSafeNormal();
	Visitor->AddMovementInput(Forward, Axis.Y);
	Visitor->AddMovementInput(FVector(-Forward.Y, Forward.X, 0.0), Axis.X);
}

void UVisitorHeadset::Turn(const FInputActionValue& Value)
{
	if (bTurned) { return; }
	bTurned = true;
	// Turn about the head, not the body's centre, so the view doesn't swing sideways.
	AVisitorBody* Visitor = Body();
	const FVector Head = Visitor->Camera()->GetComponentLocation();
	Visitor->AddActorWorldRotation(FRotator(0.0, FMath::Sign(Value.Get<float>()) * Musee::Visitor::Headset::ComfortTurnDegrees, 0.0));
	FVector Drift = Visitor->Camera()->GetComponentLocation() - Head;
	Drift.Z = 0.0;
	TrackingOrigin->AddWorldOffset(-Drift);
}

void UVisitorHeadset::TurnReleased()
{
	bTurned = false;
}

void UVisitorHeadset::PointStarted()
{
	UObject* Pointed = nullptr;
	UObject* Captioned = nullptr;
	Body()->FindAlong(RightHand->GetComponentLocation(), RightHand->GetForwardVector(), Pointed, Captioned);
	IMuseeUsable* Usable = Cast<IMuseeUsable>(Pointed);
	if (Usable && Usable->Usage().bAvailable)
	{
		Usable->Use();
		return;
	}
	bAimingStep = true;
}

void UVisitorHeadset::PointEnded()
{
	if (bAimingStep)
	{
		if (const TOptional<FVector> Floor = StepTarget()) { StepTo(Floor.GetValue()); }
	}
	bAimingStep = false;
}

TOptional<FVector> UVisitorHeadset::StepTarget() const
{
	using namespace Musee::Visitor::Headset;
	const AVisitorBody* Visitor = Body();
	const FVector From = RightHand->GetComponentLocation();
	const FVector To = From + RightHand->GetForwardVector() * Musee::ToEngine(Musee::Meters(StepReachMeters));
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(MuseeHeadsetStep), true, Visitor);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Query)) { return {}; }
	// Only ground the body could walk on, and never up onto a ledge above the eyes.
	const double WalkableNormalZ = FMath::Cos(FMath::DegreesToRadians(FMuseeHumanMeasure::MaxWalkableSlope.Get()));
	if (Hit.ImpactNormal.Z < WalkableNormalZ || Hit.ImpactPoint.Z >= Visitor->EyeLocation().Z) { return {}; }
	return Hit.ImpactPoint;
}

void UVisitorHeadset::StepTo(const FVector& Floor)
{
	AVisitorBody* Visitor = Body();
	const float HalfHeight = Visitor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (!Visitor->TeleportTo(Floor + FVector(0.0, 0.0, HalfHeight), Visitor->GetActorRotation())) { return; }
	Visitor->GetCharacterMovement()->StopMovementImmediately();
	FVector Offset = Visitor->Camera()->GetComponentLocation() - Visitor->GetActorLocation();
	Offset.Z = 0.0;
	TrackingOrigin->AddWorldOffset(-Offset);
	const APlayerController* Player = Cast<APlayerController>(Visitor->GetController());
	if (Player && Player->PlayerCameraManager)
	{
		Player->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, Musee::Visitor::Headset::BlinkSeconds, FLinearColor::Black, false, false);
	}
}

void UVisitorHeadset::UseGazed()
{
	Body()->UseLooked();
}

void UVisitorHeadset::AnotherHour()
{
	Body()->AnotherHour();
}

void UVisitorHeadset::OtherCity()
{
	Body()->OtherCity();
}

void UVisitorHeadset::StrideOn()
{
	Body()->StrideOn();
}

void UVisitorHeadset::StrideOff()
{
	Body()->StrideOff();
}

void UVisitorHeadset::Recentre()
{
	if (GEngine->XRSystem.IsValid()) { GEngine->XRSystem->ResetOrientationAndPosition(0.0f); }
	TrackingOrigin->SetRelativeLocationAndRotation(FVector(0.0, 0.0, Musee::Visitor::Headset::EyeOverCentre()), FRotator::ZeroRotator);
	Body()->Overlay()->ShowMessage(LOCTEXT("Recentred", "Recentred: your eyes at standing height, facing ahead."));
}

void UVisitorHeadset::FollowHead()
{
	// Walking about the room: the body follows the head, as far as the walls let it.
	AVisitorBody* Visitor = Body();
	FVector Drift = Visitor->Camera()->GetComponentLocation() - Visitor->GetActorLocation();
	Drift.Z = 0.0;
	const double Tolerance = Musee::ToEngine(FMuseeHumanMeasure::CapsuleRadius) * Musee::Visitor::Headset::HeadDriftFraction;
	if (Drift.Size() < Tolerance) { return; }
	const FVector Before = Visitor->GetActorLocation();
	Visitor->SetActorLocation(Before + Drift, true);
	TrackingOrigin->AddWorldOffset(-(Visitor->GetActorLocation() - Before));
}

#undef LOCTEXT_NAMESPACE
