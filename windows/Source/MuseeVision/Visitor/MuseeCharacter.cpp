#include "Visitor/MuseeCharacter.h"

#include "MuseeVision.h"
#include "Catalog/MuseeCatalog.h"
#include "Plan/MuseePlan.h"
#include "Settings/MuseeGraphics.h"
#include "Visitor/MuseeWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "HAL/IConsoleManager.h"
#include "Sky/Ephemeris.h"
#include "Sky/MuseeSky.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	constexpr float CapsuleRadius = 30.f;        // the iPhone walker's 0.3 m
	constexpr float CapsuleHalfHeight = 90.f;    // 1.8 m tall
	constexpr float PlacardDwell = 0.35f;        // seconds of looking before the placard shows
}

AMuseeCharacter::AMuseeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0, 0, MuseePlan::EyeHeight * MuseePlan::Cm - CapsuleHalfHeight));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(75.f);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxAcceleration = 1500.f;       // quicker to pace now the walk is brisker
	Move->BrakingDecelerationWalking = 2000.f;
	Move->GroundFriction = 10.f;
	Move->MaxStepHeight = 45.f;           // the Reserve stair and the terraces
	Move->SetWalkableFloorAngle(46.f);
	Move->bCanWalkOffLedges = true;
	Move->NavAgentProps.bCanJump = true;
	Move->JumpZVelocity = JumpVelocity;
	Move->AirControl = 0.35f;
	Move->bImpartBaseVelocityX = true;
	Move->bImpartBaseVelocityY = true;
	Move->bImpartBaseVelocityZ = true;
	Move->bImpartBaseAngularVelocity = true;
	Move->bIgnoreBaseRotation = false;
}

FVector AMuseeCharacter::FeetLocation() const
{
	return GetActorLocation() - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

FVector AMuseeCharacter::EyeLocation() const
{
	return Camera->GetComponentLocation();
}

void AMuseeCharacter::SetMessage(const FText& Text)
{
	MessageText = Text;
}

void AMuseeCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->ImplementsInterface(UMuseePromptProvider::StaticClass())) { PromptProviders.Add(*It); }
	}
}

void AMuseeCharacter::BuildInput()
{
	if (Context) { return; }
	auto Action = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(this, FName(Name));
		A->ValueType = Type;
		return A;
	};
	MoveAction = Action(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = Action(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	LookPadAction = Action(TEXT("IA_LookPad"), EInputActionValueType::Axis2D);
	UseAction = Action(TEXT("IA_Use"), EInputActionValueType::Boolean);
	BriskAction = Action(TEXT("IA_Brisk"), EInputActionValueType::Boolean);
	JumpAction = Action(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	PhotoAction = Action(TEXT("IA_Photo"), EInputActionValueType::Boolean);
	SnapshotAction = Action(TEXT("IA_Snapshot"), EInputActionValueType::Boolean);
	HourAction = Action(TEXT("IA_Hour"), EInputActionValueType::Boolean);
	CityAction = Action(TEXT("IA_City"), EInputActionValueType::Boolean);
	QuitAction = Action(TEXT("IA_Quit"), EInputActionValueType::Boolean);
	SettingsAction = Action(TEXT("IA_Settings"), EInputActionValueType::Boolean);
	LighterAction = Action(TEXT("IA_Lighter"), EInputActionValueType::Boolean);
	HeavierAction = Action(TEXT("IA_Heavier"), EInputActionValueType::Boolean);
	CycleAction = Action(TEXT("IA_CyclePreset"), EInputActionValueType::Boolean);
	ReadoutAction = Action(TEXT("IA_Readout"), EInputActionValueType::Boolean);
	for (int32 i = 1; i <= 4; ++i)
	{
		PromptActions.Add(Action(*FString::Printf(TEXT("IA_Prompt%d"), i), EInputActionValueType::Boolean));
	}

	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Visitor"));
	auto Swizzle = [this]() { UInputModifierSwizzleAxis* S = NewObject<UInputModifierSwizzleAxis>(this); S->Order = EInputAxisSwizzle::YXZ; return S; };
	auto Negate = [this](bool X, bool Y) { UInputModifierNegate* N = NewObject<UInputModifierNegate>(this); N->bX = X; N->bY = Y; N->bZ = false; return N; };

	// Move: W/S forward and back (Y), A/D left and right (X); arrows too; the left stick.
	for (const FKey& Key : {EKeys::W, EKeys::Up}) { Context->MapKey(MoveAction, Key).Modifiers.Add(Swizzle()); }
	for (const FKey& Key : {EKeys::S, EKeys::Down})
	{
		FEnhancedActionKeyMapping& M = Context->MapKey(MoveAction, Key);
		M.Modifiers.Add(Swizzle());
		M.Modifiers.Add(Negate(true, true));
	}
	for (const FKey& Key : {EKeys::A, EKeys::Left}) { Context->MapKey(MoveAction, Key).Modifiers.Add(Negate(true, false)); }
	for (const FKey& Key : {EKeys::D, EKeys::Right}) { Context->MapKey(MoveAction, Key); }
	Context->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	// Look: the mouse (y inverted to pitch up when moving up), the right stick.
	Context->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(Negate(false, true));
	Context->MapKey(LookPadAction, EKeys::Gamepad_Right2D);

	for (const FKey& Key : {EKeys::E, EKeys::LeftMouseButton, EKeys::Gamepad_FaceButton_Bottom}) { Context->MapKey(UseAction, Key); }
	for (const FKey& Key : {EKeys::LeftShift, EKeys::Gamepad_LeftThumbstick}) { Context->MapKey(BriskAction, Key); }
	for (const FKey& Key : {EKeys::SpaceBar, EKeys::Gamepad_RightThumbstick}) { Context->MapKey(JumpAction, Key); }
	const FKey PromptKeys[4][2] = {
		{EKeys::One, EKeys::Gamepad_FaceButton_Right}, {EKeys::Two, EKeys::Gamepad_FaceButton_Left},
		{EKeys::Three, EKeys::Gamepad_FaceButton_Top}, {EKeys::Four, EKeys::Gamepad_DPad_Down},
	};
	for (int32 i = 0; i < 4; ++i)
	{
		Context->MapKey(PromptActions[i], PromptKeys[i][0]);
		Context->MapKey(PromptActions[i], PromptKeys[i][1]);
	}
	Context->MapKey(PhotoAction, EKeys::P);
	Context->MapKey(PhotoAction, EKeys::Gamepad_Special_Left);
	Context->MapKey(SnapshotAction, EKeys::F9);
	Context->MapKey(HourAction, EKeys::T);
	Context->MapKey(HourAction, EKeys::Gamepad_DPad_Up);
	Context->MapKey(CityAction, EKeys::C);
	Context->MapKey(CityAction, EKeys::Gamepad_DPad_Left);
	Context->MapKey(QuitAction, EKeys::Escape);
	// Graphics, as in a game: F10 or O (a gamepad's Start) for the settings, F6 and F7 lighter or heavier
	// (D-pad right: heavier, round again to Performance), F8 the frame-rate readout. (F5 is the engine's
	// shader-complexity view in development builds.)
	Context->MapKey(SettingsAction, EKeys::F10);
	Context->MapKey(SettingsAction, EKeys::O);
	Context->MapKey(SettingsAction, EKeys::Gamepad_Special_Right);
	Context->MapKey(LighterAction, EKeys::F6);
	Context->MapKey(HeavierAction, EKeys::F7);
	Context->MapKey(CycleAction, EKeys::Gamepad_DPad_Right);
	Context->MapKey(ReadoutAction, EKeys::F8);
}

void AMuseeCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildInput();
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Input->ClearAllMappings();
			Input->AddMappingContext(Context, 0);
		}
	}
}

void AMuseeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMuseeCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMuseeCharacter::Look);
	Input->BindAction(LookPadAction, ETriggerEvent::Triggered, this, &AMuseeCharacter::LookGamepad);
	Input->BindAction(UseAction, ETriggerEvent::Started, this, &AMuseeCharacter::Use);
	Input->BindAction(BriskAction, ETriggerEvent::Started, this, &AMuseeCharacter::BriskOn);
	Input->BindAction(BriskAction, ETriggerEvent::Completed, this, &AMuseeCharacter::BriskOff);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	for (int32 i = 0; i < PromptActions.Num(); ++i)
	{
		Input->BindAction(PromptActions[i], ETriggerEvent::Started, this, &AMuseeCharacter::Prompt, i + 1);
	}
	Input->BindAction(PhotoAction, ETriggerEvent::Started, this, &AMuseeCharacter::TogglePhoto);
	Input->BindAction(SnapshotAction, ETriggerEvent::Started, this, &AMuseeCharacter::Snapshot);
	Input->BindAction(HourAction, ETriggerEvent::Started, this, &AMuseeCharacter::NextHour);
	Input->BindAction(CityAction, ETriggerEvent::Started, this, &AMuseeCharacter::ToggleCity);
	Input->BindAction(QuitAction, ETriggerEvent::Started, this, &AMuseeCharacter::Quit);
	Input->BindAction(SettingsAction, ETriggerEvent::Started, this, &AMuseeCharacter::ToggleSettings);
	Input->BindAction(LighterAction, ETriggerEvent::Started, this, &AMuseeCharacter::LighterPreset);
	Input->BindAction(HeavierAction, ETriggerEvent::Started, this, &AMuseeCharacter::HeavierPreset);
	Input->BindAction(CycleAction, ETriggerEvent::Started, this, &AMuseeCharacter::CyclePreset);
	Input->BindAction(ReadoutAction, ETriggerEvent::Started, this, &AMuseeCharacter::ToggleReadout);
}

void AMuseeCharacter::NextHour()
{
	static const float Hours[] = {-1.f, 10.f, 16.f, 21.f};
	HourStep = (HourStep + 1) % UE_ARRAY_COUNT(Hours);
	const float Hour = Hours[HourStep];
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("musee.Hour"))) { CVar->Set(Hour); }
	SetMessage(Hour < 0.f ? NSLOCTEXT("Musee", "HourNow", "The sky now, over you. (T: another hour)")
						  : FText::Format(NSLOCTEXT("Musee", "HourAt", "The sky at {0}:00. (T: another hour)"), FText::AsNumber(FMath::RoundToInt(Hour))));
}

void AMuseeCharacter::ToggleCity()
{
	using namespace MuseeClock;
	const ECity Next = City() == ECity::Beijing ? ECity::NewYork : ECity::Beijing;
	SetCity(Next);
	for (TActorIterator<AMuseeSky> It(GetWorld()); It; ++It) { It->Refresh(); }
	const FDateTime Local = MuseeClock::LocalNow();
	SetMessage(FText::Format(NSLOCTEXT("Musee", "CityTime", "{0} time, {1}. (C: {2})"), CityName(Next),
							 FText::FromString(Local.ToString(TEXT("%H:%M"))),
							 CityName(Next == ECity::Beijing ? ECity::NewYork : ECity::Beijing)));
}

void AMuseeCharacter::Quit()
{
	// Esc closes the settings card first (should the card have lost the keyboard to the view).
	if (UMuseeGraphics* Graphics = UMuseeGraphics::Get(this))
	{
		if (Graphics->IsPanelOpen()) { Graphics->SetPanelOpen(false); return; }
	}
	UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(GetController()), EQuitPreference::Quit, false);
}

void AMuseeCharacter::ToggleSettings()
{
	if (UMuseeGraphics* Graphics = UMuseeGraphics::Get(this)) { Graphics->TogglePanel(); }
}

void AMuseeCharacter::LighterPreset()
{
	if (UMuseeGraphics* Graphics = UMuseeGraphics::Get(this); Graphics && Graphics->IsAvailable()) { Graphics->StepPreset(-1); }
}

void AMuseeCharacter::HeavierPreset()
{
	if (UMuseeGraphics* Graphics = UMuseeGraphics::Get(this); Graphics && Graphics->IsAvailable()) { Graphics->StepPreset(1); }
}

void AMuseeCharacter::CyclePreset()
{
	UMuseeGraphics* Graphics = UMuseeGraphics::Get(this);
	if (!Graphics || !Graphics->IsAvailable()) { return; }
	if (Graphics->PresetIndex() == UMuseeGraphics::NumPresets - 1) { Graphics->ApplyPreset(0); }
	else { Graphics->StepPreset(1); }
}

void AMuseeCharacter::ToggleReadout()
{
	if (UMuseeGraphics* Graphics = UMuseeGraphics::Get(this); Graphics && Graphics->IsAvailable()) { Graphics->ToggleReadout(); }
}

void AMuseeCharacter::TogglePhoto()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport) { return; }
	bPhoto = !bPhoto;
	Viewport->EngineShowFlags.SetPathTracing(bPhoto);
	// Hold still in photo mode: the image converges while nothing moves.
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (!bPhoto) { GetCharacterMovement()->SetMovementMode(MOVE_Walking); }
	SetMessage(bPhoto ? NSLOCTEXT("Musee", "Photo", "Photo mode: path traced. Hold still; F9 saves it, P returns.") : FText::GetEmpty());
}

void AMuseeCharacter::Snapshot()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->ConsoleCommand(TEXT("HighResShot 2"));
	}
}

void AMuseeCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0, GetControlRotation().Yaw, 0);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AMuseeCharacter::Look(const FInputActionValue& Value)
{
	if (bPhoto) { return; }
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X * MouseSensitivity);
	AddControllerPitchInput(Axis.Y * MouseSensitivity);
}

void AMuseeCharacter::LookGamepad(const FInputActionValue& Value)
{
	if (bPhoto) { return; }
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Dt = GetWorld()->GetDeltaSeconds();
	AddControllerYawInput(Axis.X * GamepadLookRate * Dt);
	AddControllerPitchInput(-Axis.Y * GamepadLookRate * Dt);
}

void AMuseeCharacter::BriskOn() { GetCharacterMovement()->MaxWalkSpeed = BriskSpeed; }
void AMuseeCharacter::BriskOff() { GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }

void AMuseeCharacter::Use()
{
	AActor* Actor = LookedActor.Get();
	if (!Actor) { return; }
	if (IMuseeInteractable* Thing = InteractableFor(Actor))
	{
		if (Thing->CanInteract(this, LookHit)) { Thing->Interact(this, LookHit); }
	}
}

void AMuseeCharacter::Prompt(int32 Slot)
{
	for (int32 i = 0; i < Prompts.Num(); ++i)
	{
		if (Prompts[i].Slot != Slot) { continue; }
		if (AActor* PromptOwner = PromptOwners[i].Get())
		{
			if (IMuseePromptProvider* Provider = Cast<IMuseePromptProvider>(PromptOwner)) { Provider->RunPrompt(this, Prompts[i].Id); }
		}
		return;
	}
}

const FMuseeArtwork* AMuseeCharacter::WorkFor(const AActor* Actor) const
{
	const UMuseeCatalog* Catalog = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMuseeCatalog>() : nullptr;
	if (!Catalog) { return nullptr; }
	for (const AActor* A = Actor; A; A = A->GetAttachParentActor())
	{
		for (const FName& Tag : A->Tags)
		{
			FString S = Tag.ToString();
			if (S.RemoveFromStart(TEXT("work:")))
			{
				if (const FMuseeArtwork* Work = Catalog->Find(S)) { return Work; }
			}
		}
	}
	return nullptr;
}

IMuseeInteractable* AMuseeCharacter::InteractableFor(AActor* Actor) const
{
	for (AActor* A = Actor; A; A = A->GetAttachParentActor())
	{
		if (IMuseeInteractable* Thing = Cast<IMuseeInteractable>(A)) { return Thing; }
		if (IMuseeInteractable* Thing = Cast<IMuseeInteractable>(MuseeWorld::UseTarget(A))) { return Thing; }
	}
	return nullptr;
}

void AMuseeCharacter::UpdateLook(float DeltaSeconds)
{
	const FVector From = EyeLocation();
	const FVector To = From + GetControlRotation().Vector() * ReachDistance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MuseeLook), true, this);
	LookHit = FHitResult();
	GetWorld()->LineTraceSingleByChannel(LookHit, From, To, ECC_Visibility, Params);
	AActor* Actor = LookHit.GetActor();
	LookedActor = Actor;

	Hint = FText::GetEmpty();
	if (Actor)
	{
		if (IMuseeInteractable* Thing = InteractableFor(Actor))
		{
			if (Thing->CanInteract(this, LookHit)) { Hint = Thing->InteractHint(this, LookHit); }
		}
	}

	// The placard: shown after a moment's look, fading in and out.
	const FMuseeArtwork* Work = Actor ? WorkFor(Actor) : nullptr;
	if (Work && Work == DwellWork) { Dwell += DeltaSeconds; }
	else { DwellWork = Work; Dwell = 0.f; }
	if (Work && Dwell >= PlacardDwell) { LookedWork = Work; }
	const bool bShow = LookedWork && LookedWork == Work;
	PlacardFade = FMath::FInterpConstantTo(PlacardFade, bShow ? 1.f : 0.f, DeltaSeconds, 4.f);
	if (PlacardFade <= 0.f && !bShow) { LookedWork = nullptr; }
}

void AMuseeCharacter::UpdatePrompts()
{
	Prompts.Reset();
	PromptOwners.Reset();
	for (const TWeakObjectPtr<AActor>& Weak : PromptProviders)
	{
		AActor* ProviderActor = Weak.Get();
		const IMuseePromptProvider* Provider = ProviderActor ? Cast<IMuseePromptProvider>(ProviderActor) : nullptr;
		if (!Provider) { continue; }
		TArray<FMuseeActionPrompt> Mine;
		Provider->GetPrompts(this, Mine);
		for (FMuseeActionPrompt& P : Mine)
		{
			P.Slot = Prompts.Num() + 1;
			Prompts.Add(P);
			PromptOwners.Add(ProviderActor);
		}
	}
}

void AMuseeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ProviderRefresh -= DeltaSeconds;
	if (ProviderRefresh <= 0.f)
	{
		// Pick up providers spawned after we began (e.g. by a streamed level).
		ProviderRefresh = 2.f;
		PromptProviders.Reset();
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->GetClass()->ImplementsInterface(UMuseePromptProvider::StaticClass())) { PromptProviders.Add(*It); }
		}
	}
	UpdateLook(DeltaSeconds);
	UpdatePrompts();
}
