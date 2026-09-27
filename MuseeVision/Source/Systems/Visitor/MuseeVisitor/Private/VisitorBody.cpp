#include "VisitorBody.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/StringOutputDevice.h"
#include "MuseeCaptioned.h"
#include "MuseeCollision.h"
#include "MuseeCommand.h"
#include "MuseeHumanMeasure.h"
#include "MuseeUsable.h"
#include "VisitorAdapter.h"
#include "VisitorEye.h"
#include "VisitorOverlay.h"
#include "VisitorReach.h"

namespace Musee::Visitor
{
	namespace
	{
		/**
		 * The mean free walking speed of adult pedestrians, about 1.35 m/s (J. J. Fruin, Pedestrian Planning and Design,
		 * 1971), the pace at which a visitor walks a gallery.
		 */
		constexpr double WalkingMetersPerSecond = 1.35;

		/**
		 * The fastest walk: people change from walking to running at about 2.0 m/s (A. Hreljac, "Preferred and
		 * energetically optimal gait transition speeds in human locomotion", Medicine and Science in Sports and
		 * Exercise 25, 1993: 2.06 m/s). Striding is walking at that speed.
		 */
		constexpr double StridingMetersPerSecond = 2.0;

		/** Standard gravity, 9.80665 m/s^2 (3rd CGPM, 1901); the project's gravity is the same. */
		constexpr double StandardGravity = 9.80665;

		/**
		 * How far the view turns for one count of the mouse: the as-built museum's feel (MuseeCharacter, a look scale of
		 * 0.35 under the engine's input scale of 2.5), applied directly so no engine setting changes it.
		 */
		constexpr double LookDegreesPerCount = 0.35 * 2.5;

		/** The view turns up and down short of straight up and down, where yaw would lose its meaning. */
		constexpr double SteepestLookDegrees = 89.0;

		/** Time's commands for the hour key and the city key. */
		const TCHAR* const HourCommand = TEXT("musee.time.advance-hour");
		const TCHAR* const CityCommand = TEXT("musee.time.advance-city");

		UInputAction* MakeAction(UObject& Outer, const TCHAR* Name, EInputActionValueType Type)
		{
			UInputAction* Action = NewObject<UInputAction>(&Outer, FName(Name));
			Action->ValueType = Type;
			return Action;
		}
	}
}

AVisitorBody::AVisitorBody()
{
	using namespace Musee;
	PrimaryActorTick.bCanEverTick = true;
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(static_cast<float>(ToEngine(FMuseeHumanMeasure::CapsuleRadius)), static_cast<float>(ToEngine(FMuseeHumanMeasure::CapsuleHalfHeight)));
	Capsule->SetCollisionProfileName(Collision::Body.Profile());

	EyeCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("EyeCamera"));
	EyeCamera->SetupAttachment(Capsule);
	EyeCamera->SetRelativeLocation(FVector(0.0, 0.0, ToEngine(FMuseeHumanMeasure::EyeHeight - FMuseeHumanMeasure::CapsuleHalfHeight)));
	EyeCamera->bUsePawnControlRotation = true;

	VisitorEye = CreateDefaultSubobject<UVisitorEye>(TEXT("Eye"));
	VisitorOverlay = CreateDefaultSubobject<UVisitorOverlay>(TEXT("Overlay"));

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = static_cast<float>(ToEngine(WalkSpeed()));
	Movement->MaxStepHeight = static_cast<float>(ToEngine(FMuseeHumanMeasure::MaxStepHeight));
	Movement->SetWalkableFloorAngle(static_cast<float>(FMuseeHumanMeasure::MaxWalkableSlope.Get()));
	Movement->JumpZVelocity = static_cast<float>(ToEngine(HopSpeed()));
	Movement->NavAgentProps.bCanJump = true;
	// The body rides what it stands on: the lifting pond, a lift's car.
	Movement->bImpartBaseVelocityX = true;
	Movement->bImpartBaseVelocityY = true;
	Movement->bImpartBaseVelocityZ = true;
	Movement->bIgnoreBaseRotation = false;
}

Musee::Meters AVisitorBody::UseDistance()
{
	const double Out = (FMuseeHumanMeasure::CapsuleRadius + FMuseeHumanMeasure::Pace + FMuseeHumanMeasure::Reach).Get();
	return Musee::Meters(FMath::Sqrt(FMath::Square(FMuseeHumanMeasure::EyeHeight.Get()) + FMath::Square(Out)));
}

Musee::MetersPerSecond AVisitorBody::WalkSpeed()
{
	return Musee::MetersPerSecond(Musee::Visitor::WalkingMetersPerSecond);
}

Musee::MetersPerSecond AVisitorBody::StrideSpeed()
{
	return Musee::MetersPerSecond(Musee::Visitor::StridingMetersPerSecond);
}

Musee::MetersPerSecond AVisitorBody::HopSpeed()
{
	// A ballistic rise of h needs a take-off speed of sqrt(2 g h).
	return Musee::MetersPerSecond(FMath::Sqrt(2.0 * Musee::Visitor::StandardGravity * FMuseeHumanMeasure::MaxStepHeight.Get()));
}

FVector AVisitorBody::EyeLocation() const
{
	return EyeCamera->GetComponentLocation();
}

void AVisitorBody::BeginPlay()
{
	Super::BeginPlay();
	if (const TSharedPtr<IVisitorAdapter> Adapter = Musee::Visitor::ActiveAdapter())
	{
		AdapterName = Adapter->Name();
		Adapter->Adopt(*this);
	}
}

void AVisitorBody::TurnBy(Musee::Degrees Angle)
{
	if (AController* Mind = GetController())
	{
		Mind->SetControlRotation(Mind->GetControlRotation() + FRotator(0.0, Angle.Get(), 0.0));
	}
}

void AVisitorBody::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsLocallyControlled()) { Look(); }
}

void AVisitorBody::FindAlong(const FVector& From, const FVector& Direction, UObject*& OutUsable, UObject*& OutCaptioned) const
{
	const FVector To = From + Direction.GetSafeNormal() * Musee::ToEngine(UseDistance());
	FCollisionQueryParams Query(SCENE_QUERY_STAT(MuseeVisitorLook), true, this);
	TArray<FHitResult> Hits;
	GetWorld()->LineTraceMultiByChannel(Hits, From, To, ECC_Visibility, Query);
	OutUsable = nullptr;
	OutCaptioned = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		OutUsable = OutUsable ? OutUsable : Musee::Visitor::FindUsable(Hit.GetComponent(), Hit.GetActor());
		OutCaptioned = OutCaptioned ? OutCaptioned : Musee::Visitor::FindCaptioned(Hit.GetComponent(), Hit.GetActor());
	}
}

void AVisitorBody::Look()
{
	UObject* Usable = nullptr;
	UObject* Captioned = nullptr;
	FindAlong(EyeLocation(), EyeCamera->GetForwardVector(), Usable, Captioned);
	Looked = Usable;
	if (Usable)
	{
		VisitorOverlay->ShowUse(Cast<IMuseeUsable>(Usable)->Usage());
	}
	else if (Captioned)
	{
		VisitorOverlay->ShowCaption(Cast<IMuseeCaptioned>(Captioned)->Caption());
	}
	else
	{
		VisitorOverlay->Clear();
	}
}

void AVisitorBody::UseLooked()
{
	IMuseeUsable* Usable = Cast<IMuseeUsable>(Looked.Get());
	if (Usable && Usable->Usage().bAvailable) { Usable->Use(); }
}

void AVisitorBody::BuildInput()
{
	using namespace Musee::Visitor;
	if (Controls) { return; }
	Controls = NewObject<UInputMappingContext>(this, TEXT("VisitorDesktop"));
	UInputAction* Move = Actions.Add(TEXT("Walk"), MakeAction(*this, TEXT("Walk"), EInputActionValueType::Axis2D));
	UInputAction* View = Actions.Add(TEXT("Look"), MakeAction(*this, TEXT("Look"), EInputActionValueType::Axis2D));
	const auto Button = [this](const TCHAR* Name, std::initializer_list<FKey> Keys)
	{
		UInputAction* Action = Actions.Add(Name, MakeAction(*this, Name, EInputActionValueType::Boolean));
		for (const FKey& Key : Keys) { Controls->MapKey(Action, Key); }
	};
	// W and S walk forward and back (y), A and D sideways (x).
	UInputModifierSwizzleAxis* Forward = NewObject<UInputModifierSwizzleAxis>(this);
	Forward->Order = EInputAxisSwizzle::YXZ;
	UInputModifierNegate* Back = NewObject<UInputModifierNegate>(this);
	UInputModifierNegate* Left = NewObject<UInputModifierNegate>(this);
	Left->bY = false;
	Left->bZ = false;
	Controls->MapKey(Move, EKeys::W).Modifiers.Add(Forward);
	FEnhancedActionKeyMapping& Backward = Controls->MapKey(Move, EKeys::S);
	Backward.Modifiers.Add(Forward);
	Backward.Modifiers.Add(Back);
	Controls->MapKey(Move, EKeys::A).Modifiers.Add(Left);
	Controls->MapKey(Move, EKeys::D);
	Controls->MapKey(View, EKeys::Mouse2D);
	Button(TEXT("Use"), { EKeys::LeftMouseButton, EKeys::E });
	Button(TEXT("Stride"), { EKeys::LeftShift });
	Button(TEXT("Hop"), { EKeys::SpaceBar });
	Button(TEXT("Hour"), { EKeys::H });
	Button(TEXT("City"), { EKeys::C });
	Button(TEXT("Leave"), { EKeys::Escape });
}

void AVisitorBody::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildInput();
	const APlayerController* Player = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* Input = Player ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer()) : nullptr;
	if (Input)
	{
		Input->ClearAllMappings();
		Input->AddMappingContext(Controls, 0);
	}
	ControlsReady.Broadcast(*this);
}

void AVisitorBody::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(Actions[TEXT("Walk")], ETriggerEvent::Triggered, this, &AVisitorBody::Walk);
	Input->BindAction(Actions[TEXT("Look")], ETriggerEvent::Triggered, this, &AVisitorBody::Turn);
	Input->BindAction(Actions[TEXT("Use")], ETriggerEvent::Started, this, &AVisitorBody::UseLooked);
	Input->BindAction(Actions[TEXT("Stride")], ETriggerEvent::Started, this, &AVisitorBody::StrideOn);
	Input->BindAction(Actions[TEXT("Stride")], ETriggerEvent::Completed, this, &AVisitorBody::StrideOff);
	Input->BindAction(Actions[TEXT("Hop")], ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(Actions[TEXT("Hop")], ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(Actions[TEXT("Hour")], ETriggerEvent::Started, this, &AVisitorBody::AnotherHour);
	Input->BindAction(Actions[TEXT("City")], ETriggerEvent::Started, this, &AVisitorBody::OtherCity);
	Input->BindAction(Actions[TEXT("Leave")], ETriggerEvent::Started, this, &AVisitorBody::Leave);
}

void AVisitorBody::Walk(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Heading(0.0, GetControlRotation().Yaw, 0.0);
	AddMovementInput(FRotationMatrix(Heading).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Heading).GetUnitAxis(EAxis::Y), Axis.X);
}

void AVisitorBody::Turn(const FInputActionValue& Value)
{
	using namespace Musee::Visitor;
	AController* Mind = GetController();
	if (!Mind) { return; }
	const FVector2D Counts = Value.Get<FVector2D>();
	FRotator View = Mind->GetControlRotation();
	View.Yaw += Counts.X * LookDegreesPerCount;
	View.Pitch = FMath::Clamp(FRotator::NormalizeAxis(View.Pitch + Counts.Y * LookDegreesPerCount), -SteepestLookDegrees, SteepestLookDegrees);
	Mind->SetControlRotation(View);
}

void AVisitorBody::StrideOn()
{
	GetCharacterMovement()->MaxWalkSpeed = static_cast<float>(Musee::ToEngine(StrideSpeed()));
}

void AVisitorBody::StrideOff()
{
	GetCharacterMovement()->MaxWalkSpeed = static_cast<float>(Musee::ToEngine(WalkSpeed()));
}

void AVisitorBody::RunCommand(const TCHAR* Name)
{
	FStringOutputDevice Said;
	if (FMuseeCommand::Run(Name, {}, GetWorld(), Said))
	{
		VisitorOverlay->ShowMessage(FText::FromString(Said.TrimStartAndEnd()));
	}
}

void AVisitorBody::AnotherHour()
{
	RunCommand(Musee::Visitor::HourCommand);
}

void AVisitorBody::OtherCity()
{
	RunCommand(Musee::Visitor::CityCommand);
}

void AVisitorBody::Leave()
{
	UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(GetController()), EQuitPreference::Quit, false);
}
