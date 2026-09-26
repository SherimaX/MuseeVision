#include "MuseeVRCharacter.h"

#include "MuseeVRButton.h"
#include "MuseeVRExposure.h"
#include "MuseeVRStyle.h"
#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "UI/MuseeStyle.h"
#include "UI/SMuseeOverlay.h"
#include "UI/SMuseePlacard.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "IHeadMountedDisplay.h"
#include "IXRTrackingSystem.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "HeadMountedDisplayTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "MotionControllerComponent.h"
#include "StereoRendering.h"
#include "TimerManager.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// Where the signs sit, from the eyes (cm): forward, right, up.
	const FVector DotAt(200.f, 0.f, 0.f);
	const FVector HintAt(200.f, 0.f, -7.f);
	const FVector LabelAt(120.f, -32.f, -26.f);
	const FVector MessageAt(170.f, 0.f, 34.f);
	const FVector ButtonsAt(85.f, 30.f, -30.f);
	constexpr float ButtonSpacing = 9.f;
	/** How far the head can turn from the buttons before they are put out again ahead. */
	constexpr float ButtonsTurn = 70.f;
	/** The walkable floor for a step: up to about 45°. */
	constexpr float StepFloorZ = 0.7f;
	/** Dots in the right hand's beam. */
	constexpr int32 BeamDotCount = 14;
	/** The step arc's launch speed (cm/s): level from the eyes, it lands about 6 m ahead. */
	constexpr float StepSpeed = 750.f;
	constexpr float FadeOut = 0.08f, FadeIn = 0.18f;
	/** A pinch closes under 2 cm between fingertip centres and opens over 3.5 cm. */
	constexpr float PinchClose = 2.0f, PinchOpen = 3.5f;
	/** The signs' white, after exposure (1: the tonemapper's white point, roughly). */
	constexpr float SignBrightness = 1.f;

	/**
	 * The headset's quality: two views at 90 fps is about a third of the desktop's frame time. Lumen
	 * and MegaLights stay; they work at High instead of Epic, reflections use Lumen's surface cache
	 * rather than hit lighting, and two thirds of the pixels are rendered and upscaled to the headset's
	 * resolution by DLSS (TSR where DLSS isn't available).
	 */
	const TPair<const TCHAR*, const TCHAR*> HeadsetQuality[] = {
		{TEXT("sg.GlobalIlluminationQuality"), TEXT("2")},
		{TEXT("sg.ReflectionQuality"), TEXT("2")},
		{TEXT("sg.ShadowQuality"), TEXT("2")},
		{TEXT("sg.PostProcessQuality"), TEXT("2")},
		{TEXT("r.Lumen.HardwareRayTracing.LightingMode"), TEXT("0")},
		{TEXT("r.ScreenPercentage"), TEXT("67")},
		// The desktop's DLSS Frame Generation can't make frames for a headset.
		{TEXT("r.Streamline.DLSSG.Enable"), TEXT("0")},
	};

	void ApplyHeadsetQuality()
	{
		for (const TPair<const TCHAR*, const TCHAR*>& Setting : HeadsetQuality)
		{
			if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Setting.Key)) { CVar->Set(Setting.Value, ECVF_SetByCode); }
		}
		UE_LOG(LogMusee, Log, TEXT("VR visitor: headset quality (High, surface-cache reflections, 67%% upscaled by %s)."),
			IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable")) ? TEXT("DLSS") : TEXT("TSR"));
	}

	bool HeadsetConnected()
	{
		if (!GEngine || !GEngine->XRSystem.IsValid()) { return false; }
		IHeadMountedDisplay* Hmd = GEngine->XRSystem->GetHMDDevice();
		return Hmd && Hmd->IsHMDConnected();
	}

	/** Fade a line toward the wanted text: out, swap, in (like the desktop overlay). */
	float Crossfade(const FText& Wanted, FString& Shown, STextBlock& Line, float Opacity, float DeltaSeconds)
	{
		const FString WantedString = Wanted.ToString();
		if (!WantedString.IsEmpty() && Opacity <= 0.f && !WantedString.Equals(Shown, ESearchCase::CaseSensitive))
		{
			Shown = WantedString;
			Line.SetText(Wanted);
		}
		const bool bUp = !WantedString.IsEmpty() && WantedString.Equals(Shown, ESearchCase::CaseSensitive);
		return FMath::FInterpConstantTo(Opacity, bUp ? 1.f : 0.f, DeltaSeconds, bUp ? 5.f : 8.f);
	}

	void Present(SWidget& Widget, float Opacity)
	{
		Widget.SetRenderOpacity(FMath::SmoothStep(0.f, 1.f, Opacity));
	}

	bool SamePrompts(const TArray<FMuseeActionPrompt>& A, const TArray<FMuseeActionPrompt>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (int32 i = 0; i < A.Num(); ++i)
		{
			if (A[i].Id != B[i].Id || !A[i].Title.ToString().Equals(B[i].Title.ToString(), ESearchCase::CaseSensitive)) { return false; }
		}
		return true;
	}
}

AMuseeVRCharacter::AMuseeVRCharacter()
{
	const float EyeOverCentre = MuseePlan::EyeHeight * MuseePlan::Cm - GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	TrackingOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("TrackingOrigin"));
	TrackingOrigin->SetupAttachment(GetCapsuleComponent());
	TrackingOrigin->SetRelativeLocation(FVector(0, 0, EyeOverCentre));
	// The eyes: where the desktop visitor's are until a headset moves them.
	Camera->SetupAttachment(TrackingOrigin);
	Camera->SetRelativeLocation(FVector::ZeroVector);

	RightHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightHand"));
	RightHand->SetupAttachment(TrackingOrigin);
	RightHand->SetTrackingMotionSource(FName(TEXT("RightAim")));
	LeftHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHand"));
	LeftHand->SetupAttachment(TrackingOrigin);
	LeftHand->SetTrackingMotionSource(FName(TEXT("LeftAim")));

	auto Panel = [this](const TCHAR* Name, USceneComponent* Parent)
	{
		UWidgetComponent* W = CreateDefaultSubobject<UWidgetComponent>(Name);
		W->SetupAttachment(Parent);
		MuseeVRStyle::SetUpPanel(*W);
		W->SetVisibility(false);
		return W;
	};
	// Facing back at the eyes (a widget shows its face along its +X).
	DotPanel = Panel(TEXT("DotPanel"), Camera);
	DotPanel->SetRelativeLocationAndRotation(DotAt, FRotator(0, 180, 0));
	DotPanel->SetRelativeScale3D(FVector(MuseeVRStyle::UnitCm(DotAt.X)));
	HintPanel = Panel(TEXT("HintPanel"), Camera);
	HintPanel->SetRelativeLocationAndRotation(HintAt, FRotator(0, 180, 0));
	HintPanel->SetRelativeScale3D(FVector(MuseeVRStyle::UnitCm(HintAt.X)));
	HintPanel->SetPivot(FVector2D(0.5, 0.0));
	// Placed in the room each frame, gliding after the view.
	LabelPanel = Panel(TEXT("LabelPanel"), GetCapsuleComponent());
	MessagePanel = Panel(TEXT("MessagePanel"), GetCapsuleComponent());
	StepRing = Panel(TEXT("StepRing"), GetCapsuleComponent());
	for (UWidgetComponent* W : {LabelPanel.Get(), MessagePanel.Get(), StepRing.Get()})
	{
		W->SetUsingAbsoluteLocation(true);
		W->SetUsingAbsoluteRotation(true);
		W->SetUsingAbsoluteScale(true);
	}
	LabelPanel->SetWorldScale3D(FVector(MuseeVRStyle::UnitCm(LabelAt.Size())));
	MessagePanel->SetWorldScale3D(FVector(MuseeVRStyle::UnitCm(MessageAt.Size())));
	// The ring: the dot's gilt ring (18 units) about 50 cm across, drawn finely.
	StepRing->SetWorldScale3D(FVector(50.f / 18.f / 8.f));
}

void AMuseeVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	bTracked = HeadsetConnected();
	bVR = bTracked || FParse::Param(FCommandLine::Get(), TEXT("MuseeVR"));
	UE_LOG(LogMusee, Log, TEXT("VR visitor: headset %s%s."), bTracked ? TEXT("tracking") : TEXT("not connected"),
		bVR && !bTracked ? TEXT(" (-MuseeVR: the headset's controls on the desktop)") : TEXT(""));
	// The hands' input goes to the log for the first five minutes in a headset (always with -MuseeVRInputLog).
	InputLogFor = FParse::Param(FCommandLine::Get(), TEXT("MuseeVRInputLog")) ? 1.0e9f : (bTracked ? 300.f : 0.f);
	// -MuseeVRQuality: the headset's quality without one (e.g. with -emulatestereo, to measure it).
	if (bTracked || FParse::Param(FCommandLine::Get(), TEXT("MuseeVRQuality"))) { ApplyHeadsetQuality(); }
	if (!bVR) { return; }

	if (bTracked)
	{
		// The camera follows the headset; turning is the visitor's own (TurnDegrees), not the mouse's.
		Camera->bUsePawnControlRotation = false;
		Camera->bLockToHmd = true;
		bUseControllerRotationYaw = false;
		GEngine->XRSystem->SetTrackingOrigin(EHMDTrackingOrigin::Local);
		if (GEngine->StereoRenderingDevice.IsValid()) { GEngine->StereoRenderingDevice->EnableStereo(true); }
		GEngine->XRSystem->ResetOrientationAndPosition(0.f);
		// The headset paces the frames; the desktop window's vsync would hold it to the monitor's rate.
		if (IConsoleVariable* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"))) { VSync->Set(0); }
	}

	Exposure = FSceneViewExtensions::NewExtension<FMuseeVRExposure>();

	Dot = SNew(SMuseeReticle);
	DotPanel->SetSlateWidget(MuseeVRStyle::Scaled(Dot.ToSharedRef()));
	DotPanel->SetVisibility(true);

	HintLabel = MuseeStyle::MakeCaps(FText::GetEmpty());
	HintSign = MuseeVRStyle::Glass(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			MuseeStyle::MakeKeycap(MuseeVRStyle::PinchKey())
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.f, 0.f, 0.f, 0.f))
		[
			HintLabel.ToSharedRef()
		],
		17.f, FMargin(7.f, 7.f, 18.f, 7.f));
	HintSign->SetRenderOpacity(0.f);
	HintPanel->SetSlateWidget(MuseeVRStyle::Scaled(HintSign.ToSharedRef()));

	Placard = SNew(SMuseePlacard);
	Placard->SetRenderOpacity(0.f);
	LabelPanel->SetSlateWidget(MuseeVRStyle::Scaled(Placard.ToSharedRef()));

	MessageLine = SNew(STextBlock)
		.Font(MuseeStyle::Font("Light", MuseeStyle::MessageSize, 20))
		.ColorAndOpacity(MuseeStyle::Travertine)
		.WrapTextAt(620.f)
		.Justification(ETextJustify::Center);
	MessageSign = MuseeVRStyle::Glass(MessageLine.ToSharedRef(), 6.f, FMargin(24.f, 12.f));
	MessageSign->SetRenderOpacity(0.f);
	MessagePanel->SetSlateWidget(MuseeVRStyle::Scaled(MessageSign.ToSharedRef()));

	Ring = SNew(SMuseeReticle);
	Ring->SetEngaged(1.f);
	StepRing->SetSlateWidget(MuseeVRStyle::Scaled(Ring.ToSharedRef(), 8.f));

	// The right hand's beam: the dot drawn small along the ray, and the dot and ring where it lands.
	auto WorldPanel = [this](const TSharedRef<SWidget>& Content)
	{
		UWidgetComponent* W = NewObject<UWidgetComponent>(this);
		MuseeVRStyle::SetUpPanel(*W);
		W->SetUsingAbsoluteLocation(true);
		W->SetUsingAbsoluteRotation(true);
		W->SetUsingAbsoluteScale(true);
		W->SetupAttachment(GetCapsuleComponent());
		W->RegisterComponent();
		W->SetSlateWidget(MuseeVRStyle::Scaled(Content));
		W->SetVisibility(false);
		return W;
	};
	for (int32 i = 0; i < BeamDotCount; ++i) { BeamDots.Add(WorldPanel(SNew(SMuseeReticle))); }
	CursorRing = SNew(SMuseeReticle);
	Cursor = WorldPanel(CursorRing.ToSharedRef());

	// Development: -MuseeVRAim holds a pinch from the start (the ring shows where it would step);
	// -MuseeVRStep=<seconds> lets go after that long and logs where the visitor landed.
	bAiming = FParse::Param(FCommandLine::Get(), TEXT("MuseeVRAim"));
	float StepAfter = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("-MuseeVRStep="), StepAfter) && StepAfter > 0.f)
	{
		bAiming = true;
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			const FVector From = FeetLocation() / MuseePlan::Cm;
			const FVector To = StepTarget.IsSet() ? StepTarget.GetValue() / MuseePlan::Cm : FVector::ZeroVector;
			const FRotator Aim = GetControlRotation();
			UE_LOG(LogMusee, Log, TEXT("VR step: from (%.2f, %.2f, %.2f) looking yaw %.0f pitch %.0f, to %s (%.2f, %.2f, %.2f)."), From.X, From.Y, From.Z,
				Aim.Yaw, FRotator::NormalizeAxis(Aim.Pitch), StepTarget.IsSet() ? TEXT("the ring at") : TEXT("nowhere; no ring"), To.X, To.Y, To.Z);
			PinchRightEnded();
			FTimerHandle After;
			GetWorldTimerManager().SetTimer(After, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				const FVector Feet = FeetLocation() / MuseePlan::Cm;
				UE_LOG(LogMusee, Log, TEXT("VR step: landed at (%.2f, %.2f, %.2f)."), Feet.X, Feet.Y, Feet.Z);
			}), 1.f, false);
		}), StepAfter, false);
	}
}

void AMuseeVRCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearButtons();
	Exposure.Reset();
	Super::EndPlay(EndPlayReason);
}

void AMuseeVRCharacter::BuildVRInput()
{
	if (VRContext) { return; }
	auto Action = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(this, FName(Name));
		A->ValueType = Type;
		return A;
	};
	PinchRightAction = Action(TEXT("IA_VRPinchRight"), EInputActionValueType::Boolean);
	PinchLeftAction = Action(TEXT("IA_VRPinchLeft"), EInputActionValueType::Boolean);
	StickMoveAction = Action(TEXT("IA_VRStickMove"), EInputActionValueType::Axis2D);
	StickTurnAction = Action(TEXT("IA_VRStickTurn"), EInputActionValueType::Axis1D);
	TurnRightAction = Action(TEXT("IA_VRTurnRight"), EInputActionValueType::Boolean);
	TurnLeftAction = Action(TEXT("IA_VRTurnLeft"), EInputActionValueType::Boolean);
	RecentreAction = Action(TEXT("IA_VRRecentre"), EInputActionValueType::Boolean);
	SkyHourAction = Action(TEXT("IA_VRHour"), EInputActionValueType::Boolean);
	VRUseAction = Action(TEXT("IA_VRUse"), EInputActionValueType::Boolean);
	StickAimAction = Action(TEXT("IA_VRStickAim"), EInputActionValueType::Axis1D);
	VRCityAction = Action(TEXT("IA_VRCity"), EInputActionValueType::Boolean);
	VRBriskAction = Action(TEXT("IA_VRBrisk"), EInputActionValueType::Boolean);

	// Quest Touch controllers (the Quest 3 through Steam Link, or ALVR's emulated hands on the Vision
	// Pro: trigger = thumb and index, B/Y = thumb and middle, A/X = thumb and ring):
	//   right trigger: use what the beam points at; hold on the floor to aim a step, let go to take it
	//   right stick: left and right turn 30 degrees, forward aims a step (let go to take it)
	//   left stick: walk (click for a brisker walk)   left trigger: walk where you look
	//   A: use   B: another hour of the sky   X: Beijing / New York   Y (or the menu button): recentre
	// Index controllers keep the gesture layout (B turns, left A for the hour).
	VRContext = NewObject<UInputMappingContext>(this, TEXT("IMC_VRVisitor"));
	auto Map = [this](UInputAction* A, std::initializer_list<FKey> Keys) { for (const FKey& Key : Keys) { VRContext->MapKey(A, Key); } };
	Map(PinchRightAction, {EKeys::OculusTouch_Right_Trigger_Click, EKeys::ValveIndex_Right_Trigger_Click});
	Map(PinchLeftAction, {EKeys::OculusTouch_Left_Trigger_Click, EKeys::ValveIndex_Left_Trigger_Click});
	Map(VRUseAction, {EKeys::OculusTouch_Right_A_Click, EKeys::ValveIndex_Right_A_Click});
	Map(SkyHourAction, {EKeys::OculusTouch_Right_B_Click, EKeys::ValveIndex_Left_A_Click});
	Map(VRCityAction, {EKeys::OculusTouch_Left_X_Click});
	Map(RecentreAction, {EKeys::OculusTouch_Left_Y_Click, EKeys::OculusTouch_Left_Menu_Click});
	Map(VRBriskAction, {EKeys::OculusTouch_Left_Thumbstick_Click, EKeys::ValveIndex_Left_Thumbstick_Click});
	Map(TurnRightAction, {EKeys::ValveIndex_Right_B_Click});
	Map(TurnLeftAction, {EKeys::ValveIndex_Left_B_Click});
	Map(StickAimAction, {EKeys::OculusTouch_Right_Thumbstick_Y, EKeys::ValveIndex_Right_Thumbstick_Y});
	// Trying the headset's controls on the desktop (-MuseeVR): right mouse button to use or step,
	// F to glide, Z and C to turn.
	Map(PinchRightAction, {EKeys::RightMouseButton});
	Map(PinchLeftAction, {EKeys::F});
	Map(TurnLeftAction, {EKeys::Z});
	Map(TurnRightAction, {EKeys::C});
	for (const FKey& Key : {EKeys::OculusTouch_Left_Thumbstick_2D, EKeys::ValveIndex_Left_Thumbstick_2D})
	{
		UInputModifierDeadZone* DeadZone = NewObject<UInputModifierDeadZone>(this);
		DeadZone->LowerThreshold = 0.25f;
		VRContext->MapKey(StickMoveAction, Key).Modifiers.Add(DeadZone);
	}
	Map(StickTurnAction, {EKeys::OculusTouch_Right_Thumbstick_X, EKeys::ValveIndex_Right_Thumbstick_X});
}

void AMuseeVRCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildVRInput();
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Input->AddMappingContext(VRContext, 1);
		}
	}
}

void AMuseeVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildVRInput();
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(PinchRightAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::TriggerRightStarted);
	Input->BindAction(PinchRightAction, ETriggerEvent::Completed, this, &AMuseeVRCharacter::TriggerRightEnded);
	Input->BindAction(PinchLeftAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::TriggerLeftStarted);
	Input->BindAction(PinchLeftAction, ETriggerEvent::Completed, this, &AMuseeVRCharacter::TriggerLeftEnded);
	Input->BindAction(StickMoveAction, ETriggerEvent::Triggered, this, &AMuseeVRCharacter::StickMove);
	Input->BindAction(StickTurnAction, ETriggerEvent::Triggered, this, &AMuseeVRCharacter::StickTurn);
	Input->BindAction(StickTurnAction, ETriggerEvent::Completed, this, &AMuseeVRCharacter::StickTurnEnded);
	Input->BindAction(TurnRightAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::TurnRight);
	Input->BindAction(TurnLeftAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::TurnLeft);
	Input->BindAction(RecentreAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::Recentre);
	Input->BindAction(SkyHourAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::SkyHour);
	Input->BindAction(VRUseAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::UsePointed);
	Input->BindAction(StickAimAction, ETriggerEvent::Triggered, this, &AMuseeVRCharacter::StickAim);
	Input->BindAction(StickAimAction, ETriggerEvent::Completed, this, &AMuseeVRCharacter::StickAimEnded);
	Input->BindAction(VRCityAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::City);
	Input->BindAction(VRBriskAction, ETriggerEvent::Started, this, &AMuseeVRCharacter::BriskStart);
	Input->BindAction(VRBriskAction, ETriggerEvent::Completed, this, &AMuseeVRCharacter::BriskEnd);
}

FVector AMuseeVRCharacter::HeadForward() const
{
	const float Yaw = bTracked ? Camera->GetComponentRotation().Yaw : GetControlRotation().Yaw;
	return FRotator(0, Yaw, 0).Vector();
}

void AMuseeVRCharacter::AimRay(FVector& From, FVector& Direction) const
{
	if (bTracked && RightPinches.bTracked)
	{
		// A bare hand: the ray from the shoulder through the pinch, steadier than the wrist's angle.
		const FRotationMatrix Yaw(FRotator(0, Camera->GetComponentRotation().Yaw, 0));
		const FVector Shoulder = Camera->GetComponentLocation() + Yaw.GetUnitAxis(EAxis::Y) * 18.f - FVector(0, 0, 15.f);
		From = RightPinches.PinchPoint;
		Direction = (RightPinches.PinchPoint - Shoulder).GetSafeNormal();
		return;
	}
	if (bTracked && RightHand->IsTracked())
	{
		From = RightHand->GetComponentLocation();
		Direction = RightHand->GetForwardVector();
		return;
	}
	From = EyeLocation();
	Direction = GetControlRotation().Vector();
}

bool AMuseeVRCharacter::UseAlong(const FVector& From, const FVector& Direction)
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MuseeVRUse), true, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, From, From + Direction * ReachDistance, ECC_Visibility, Params)) { return false; }
	IMuseeInteractable* Thing = Hit.GetActor() ? InteractableFor(Hit.GetActor()) : nullptr;
	if (!Thing || !Thing->CanInteract(this, Hit)) { return false; }
	Thing->Interact(this, Hit);
	return true;
}

void AMuseeVRCharacter::TriggerRightStarted() { if (!RightPinches.bTracked) { PinchRightStarted(); } }
void AMuseeVRCharacter::TriggerRightEnded() { if (!RightPinches.bTracked) { PinchRightEnded(); } }
void AMuseeVRCharacter::TriggerLeftStarted() { if (!LeftPinches.bTracked) { PinchLeftStarted(); } }
void AMuseeVRCharacter::TriggerLeftEnded() { if (!LeftPinches.bTracked) { PinchLeftEnded(); } }

void AMuseeVRCharacter::UpdateHands()
{
	for (const bool bRight : {false, true})
	{
		FHandPinches& Hand = bRight ? RightPinches : LeftPinches;
		const bool bWasTracked = Hand.bTracked;
		FXRHandTrackingState State;
		if (bTracked) { GEngine->XRSystem->GetHandTrackingState(this, EXRSpaceType::UnrealWorldSpace, bRight ? EControllerHand::Right : EControllerHand::Left, State); }
		const int32 Keys = State.HandKeyLocations.Num();
		Hand.bTracked = State.bValid && State.TrackingStatus == ETrackingStatus::Tracked && Keys > int32(EHandKeypoint::LittleTip);
		if (Hand.bTracked != bWasTracked && InputLogFor > 0.f)
		{
			UE_LOG(LogMusee, Log, TEXT("VR input: %s hand %s."), bRight ? TEXT("right") : TEXT("left"), Hand.bTracked ? TEXT("tracked (bare hand)") : TEXT("gone"));
		}

		// The joints come in world space (the tracker applies the tracking origin).
		auto At = [&](EHandKeypoint Key) { return State.HandKeyLocations[int32(Key)]; };
		float IndexGap = 1e6f, MiddleGap = 1e6f, RingGap = 1e6f;
		if (Hand.bTracked)
		{
			const FVector Thumb = At(EHandKeypoint::ThumbTip);
			IndexGap = FVector::Dist(Thumb, At(EHandKeypoint::IndexTip));
			MiddleGap = FVector::Dist(Thumb, At(EHandKeypoint::MiddleTip));
			RingGap = FVector::Dist(Thumb, At(EHandKeypoint::RingTip));
			Hand.PinchPoint = (Thumb + At(EHandKeypoint::IndexTip)) * 0.5f;
		}
		// Only the closest finger pinches; each closes under PinchClose and opens over PinchOpen.
		const float Closest = FMath::Min3(IndexGap, MiddleGap, RingGap);
		auto Pinch = [&](bool& bOn, float Distance) -> int32
		{
			const bool bNow = bOn ? Distance < PinchOpen : (Distance < PinchClose && Distance <= Closest);
			const int32 Edge = bNow == bOn ? 0 : (bNow ? 1 : -1);
			bOn = bNow;
			return Edge;
		};
		const int32 IndexEdge = Pinch(Hand.bIndex, IndexGap);
		const int32 MiddleEdge = Pinch(Hand.bMiddle, MiddleGap);
		const int32 RingEdge = Pinch(Hand.bRing, RingGap);
		if (InputLogFor > 0.f)
		{
			for (const TPair<int32, const TCHAR*>& E : {TPair<int32, const TCHAR*>(IndexEdge, TEXT("index")), TPair<int32, const TCHAR*>(MiddleEdge, TEXT("middle")), TPair<int32, const TCHAR*>(RingEdge, TEXT("ring"))})
			{
				if (E.Key) { UE_LOG(LogMusee, Log, TEXT("VR input: %s thumb-%s pinch %s."), bRight ? TEXT("right") : TEXT("left"), E.Value, E.Key > 0 ? TEXT("closed") : TEXT("opened")); }
			}
		}
		if (bRight)
		{
			if (IndexEdge > 0) { PinchRightStarted(); } else if (IndexEdge < 0) { PinchRightEnded(); }
			if (MiddleEdge > 0) { TurnRight(); }
			if (RingEdge > 0) { SkyHour(); }
		}
		else
		{
			if (IndexEdge > 0) { PinchLeftStarted(); } else if (IndexEdge < 0) { PinchLeftEnded(); }
			if (MiddleEdge > 0) { TurnLeft(); }
			if (RingEdge > 0) { Recentre(); }
		}
	}
}

void AMuseeVRCharacter::PinchRightStarted()
{
	if (InputLogFor > 0.f) { UE_LOG(LogMusee, Log, TEXT("VR input: right pinch started (hand %s)."), RightHand->IsTracked() ? TEXT("tracked") : TEXT("not tracked")); }
	if (!bVR) { return; }
	// What the hand points at, then what the eyes rest on; else a step.
	FVector From, Direction;
	AimRay(From, Direction);
	if (UseAlong(From, Direction)) { return; }
	if (UseAlong(EyeLocation(), GetControlRotation().Vector())) { return; }
	bAiming = true;
}

void AMuseeVRCharacter::PinchRightEnded()
{
	if (InputLogFor > 0.f) { UE_LOG(LogMusee, Log, TEXT("VR input: right pinch ended (%s)."), StepTarget.IsSet() ? TEXT("stepping") : TEXT("no ring")); }
	if (bAiming && StepTarget.IsSet()) { StepTo(StepTarget.GetValue()); }
	bAiming = false;
	StepTarget.Reset();
	StepRing->SetVisibility(false);
}

void AMuseeVRCharacter::PinchLeftStarted()
{
	if (InputLogFor > 0.f) { UE_LOG(LogMusee, Log, TEXT("VR input: left pinch started.")); }
	bGliding = bVR;
}
void AMuseeVRCharacter::PinchLeftEnded() { bGliding = false; }

void AMuseeVRCharacter::StickMove(const FInputActionValue& Value)
{
	if (!bVR) { return; }
	const FVector2D Axis = Value.Get<FVector2D>();
	const FVector Forward = HeadForward();
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(FVector(-Forward.Y, Forward.X, 0.f), Axis.X);
}

void AMuseeVRCharacter::StickTurn(const FInputActionValue& Value)
{
	if (!bVR || bStickTurned) { return; }
	bStickTurned = true;
	Turn(FMath::Sign(Value.Get<float>()) * TurnDegrees);
}

void AMuseeVRCharacter::StickTurnEnded() { bStickTurned = false; }
void AMuseeVRCharacter::TurnRight() { if (bVR) { Turn(TurnDegrees); } }
void AMuseeVRCharacter::TurnLeft() { if (bVR) { Turn(-TurnDegrees); } }

void AMuseeVRCharacter::Turn(float Degrees)
{
	if (!bTracked)
	{
		if (AController* C = GetController()) { C->SetControlRotation(C->GetControlRotation() + FRotator(0, Degrees, 0)); }
		return;
	}
	// Turn about the head, not the body's centre.
	const FVector Head = Camera->GetComponentLocation();
	AddActorWorldRotation(FRotator(0, Degrees, 0));
	FVector Drift = Camera->GetComponentLocation() - Head;
	Drift.Z = 0;
	TrackingOrigin->AddWorldOffset(-Drift);
}

void AMuseeVRCharacter::Recentre()
{
	if (!bVR) { return; }
	if (bTracked) { GEngine->XRSystem->ResetOrientationAndPosition(0.f); }
	const float EyeOverCentre = MuseePlan::EyeHeight * MuseePlan::Cm - GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	TrackingOrigin->SetRelativeLocationAndRotation(FVector(0, 0, EyeOverCentre), FRotator::ZeroRotator);
	SetMessage(NSLOCTEXT("Musee", "Recentred", "Recentred: your eyes at 1.6 m, facing ahead."));
}

void AMuseeVRCharacter::SkyHour()
{
	if (!bVR) { return; }
	NextHour();
	static const float Hours[] = {-1.f, 10.f, 16.f, 21.f};
	const float Hour = Hours[HourStep % UE_ARRAY_COUNT(Hours)];
	SetMessage(Hour < 0.f ? NSLOCTEXT("Musee", "VRHourNow", "The sky now, over you.")
						  : FText::Format(NSLOCTEXT("Musee", "VRHourAt", "The sky at {0}:00."), FText::AsNumber(FMath::RoundToInt(Hour))));
}

void AMuseeVRCharacter::UsePointed()
{
	if (!bVR) { return; }
	FVector From, Direction;
	AimRay(From, Direction);
	if (!UseAlong(From, Direction)) { UseAlong(EyeLocation(), GetControlRotation().Vector()); }
}

void AMuseeVRCharacter::StickAim(const FInputActionValue& Value)
{
	if (!bVR || bAiming || Value.Get<float>() < 0.6f) { return; }
	bStickAiming = true;
	bAiming = true;
}

void AMuseeVRCharacter::StickAimEnded()
{
	if (!bStickAiming) { return; }
	bStickAiming = false;
	PinchRightEnded();
}

void AMuseeVRCharacter::City() { if (bVR) { ToggleCity(); } }
void AMuseeVRCharacter::BriskStart() { if (bVR) { BriskOn(); } }
void AMuseeVRCharacter::BriskEnd() { if (bVR) { BriskOff(); } }

void AMuseeVRCharacter::HidePointer()
{
	for (UWidgetComponent* BeamDot : BeamDots) { BeamDot->SetVisibility(false); }
	if (Cursor) { Cursor->SetVisibility(false); }
}

void AMuseeVRCharacter::UpdatePointer()
{
	PointerHint = FText::GetEmpty();
	if (!bTracked || !(RightPinches.bTracked || RightHand->IsTracked()) || BeamDots.IsEmpty() || !Cursor) { HidePointer(); return; }
	const FVector Eye = EyeLocation();
	FVector From, Direction;
	AimRay(From, Direction);

	// The path: the step's arc while aiming, else the ray to what it points at (a short one at nothing).
	TArray<FVector> Path;
	bool bUsable = false;
	bool bHit = false;
	if (bAiming && ArcPoints.Num() > 1)
	{
		Path = ArcPoints;
		bHit = StepTarget.IsSet();
	}
	else
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MuseeVRBeam), true, this);
		bHit = GetWorld()->LineTraceSingleByChannel(Hit, From, From + Direction * ReachDistance, ECC_Visibility, Params);
		Path = {From, bHit ? Hit.ImpactPoint : From + Direction * 150.f};
		if (bHit && Hit.GetActor())
		{
			if (IMuseeInteractable* Thing = InteractableFor(Hit.GetActor()))
			{
				bUsable = Thing->CanInteract(this, Hit);
				if (bUsable) { PointerHint = Thing->InteractHint(this, Hit); }
			}
		}
	}

	// Spread the dots evenly along the path, leaving the hand itself clear.
	float Length = 0.f;
	TArray<float> Along = {0.f};
	for (int32 i = 1; i < Path.Num(); ++i) { Length += FVector::Dist(Path[i - 1], Path[i]); Along.Add(Length); }
	for (int32 i = 0; i < BeamDots.Num(); ++i)
	{
		const float At = FMath::Lerp(8.f, Length, float(i + 1) / float(BeamDots.Num() + 1));
		int32 Segment = 1;
		while (Segment < Path.Num() - 1 && Along[Segment] < At) { ++Segment; }
		const float Span = FMath::Max(Along[Segment] - Along[Segment - 1], 1e-3f);
		const FVector Point = FMath::Lerp(Path[Segment - 1], Path[Segment], (At - Along[Segment - 1]) / Span);
		UWidgetComponent* BeamDot = BeamDots[i];
		BeamDot->SetWorldLocationAndRotation(Point, (Eye - Point).Rotation());
		BeamDot->SetWorldScale3D(FVector(MuseeVRStyle::UnitCm(FVector::Dist(Eye, Point)) * 0.8f));
		BeamDot->SetVisibility(Length > 10.f);
	}
	// Where it lands: a dot, opening to the gilt ring on what can be used (the step has its own ring).
	const bool bCursor = bHit && !bAiming;
	if (bCursor)
	{
		const FVector End = Path.Last();
		const FVector Back = (Eye - End).GetSafeNormal();
		Cursor->SetWorldLocationAndRotation(End + Back * 2.f, Back.Rotation());
		Cursor->SetWorldScale3D(FVector(MuseeVRStyle::UnitCm(FVector::Dist(Eye, End)) * 2.f));
		CursorRing->SetEngaged(bUsable ? 1.f : 0.f);
	}
	Cursor->SetVisibility(bCursor);
}

void AMuseeVRCharacter::FollowHead()
{
	// Walking about the room: the body follows the head, as far as the walls let it.
	FVector Delta = Camera->GetComponentLocation() - GetActorLocation();
	Delta.Z = 0;
	if (Delta.SizeSquared() < 4.f) { return; }
	const FVector Before = GetActorLocation();
	SetActorLocation(Before + Delta, true);
	TrackingOrigin->AddWorldOffset(-(GetActorLocation() - Before));
}

void AMuseeVRCharacter::CentreHead()
{
	FVector Offset = Camera->GetComponentLocation() - GetActorLocation();
	Offset.Z = 0;
	TrackingOrigin->AddWorldOffset(-Offset);
}

void AMuseeVRCharacter::UpdateStep()
{
	StepTarget.Reset();
	ArcPoints.Reset();
	if (bAiming)
	{
		// An arc, like a thrown ball, from the hand (or the eyes): it comes down on the floor ahead
		// wherever you point, level or down; against a wall it lands on the floor before the wall.
		FVector From, Direction;
		AimRay(From, Direction);
		FPredictProjectilePathParams Path(0.f, From, Direction * StepSpeed, 2.5f, ECC_Visibility, this);
		Path.bTraceWithCollision = true;
		Path.SimFrequency = 20.f;
		for (AMuseeVRButton* Button : Buttons) { if (IsValid(Button)) { Path.ActorsToIgnore.Add(Button); } }
		FPredictProjectilePathResult Result;
		const bool bLands = UGameplayStatics::PredictProjectilePath(this, Path, Result);
		for (const FPredictProjectilePathPointData& Point : Result.PathData) { ArcPoints.Add(Point.Location); }
		if (bLands && Result.HitResult.bBlockingHit)
		{
			const FHitResult& Hit = Result.HitResult;
			FVector Floor = Hit.ImpactPoint;
			bool bFloor = Hit.ImpactNormal.Z >= StepFloorZ;
			if (!bFloor)
			{
				// A wall (or a frame): the floor half a metre back from it.
				FVector Back = Hit.ImpactNormal;
				Back.Z = 0.f;
				const FVector Above = Hit.ImpactPoint + Back.GetSafeNormal() * 50.f + FVector(0, 0, 20.f);
				FHitResult Down;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(MuseeVRStepDown), true, this);
				if (GetWorld()->LineTraceSingleByChannel(Down, Above, Above - FVector(0, 0, 400.f), ECC_Visibility, Params) && Down.ImpactNormal.Z >= StepFloorZ)
				{
					Floor = Down.ImpactPoint;
					bFloor = true;
				}
			}
			// Not onto a ledge far above the eyes (a cornice, the top of a case).
			if (bFloor && Floor.Z < EyeLocation().Z)
			{
				StepTarget = Floor;
				StepRing->SetWorldLocationAndRotation(Floor + FVector(0, 0, 2.f), FRotator(90, 0, 0));
			}
		}
	}
	StepRing->SetVisibility(StepTarget.IsSet());
}

void AMuseeVRCharacter::StepTo(const FVector& Floor)
{
	// A blink: dark for a moment, and you stand there.
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PC->PlayerCameraManager) { PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeOut, FLinearColor::Black, false, true); }
	GetWorldTimerManager().SetTimer(StepTimer, FTimerDelegate::CreateWeakLambda(this, [this, Floor]()
	{
		const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (TeleportTo(Floor + FVector(0, 0, HalfHeight + 2.f), GetActorRotation()))
		{
			GetCharacterMovement()->StopMovementImmediately();
			CentreHead();
		}
		APlayerController* Controller = Cast<APlayerController>(GetController());
		if (Controller && Controller->PlayerCameraManager) { Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeIn, FLinearColor::Black, false, false); }
	}), FadeOut + 0.01f, false);
}

void AMuseeVRCharacter::ClearButtons()
{
	for (AMuseeVRButton* Button : Buttons) { if (IsValid(Button)) { Button->Destroy(); } }
	Buttons.Reset();
	ButtonPrompts.Reset();
}

void AMuseeVRCharacter::UpdateButtons()
{
	const float HeadYaw = HeadForward().Rotation().Yaw;
	const bool bSame = SamePrompts(Prompts, ButtonPrompts);
	if (bSame && (Buttons.IsEmpty() || FMath::Abs(FRotator::NormalizeAxis(HeadYaw - ButtonsYaw)) < ButtonsTurn)) { return; }
	if (!bSame)
	{
		ClearButtons();
		for (int32 i = 0; i < Prompts.Num(); ++i)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.Owner = this;
			AMuseeVRButton* Button = GetWorld()->SpawnActor<AMuseeVRButton>(Params);
			Button->SetPrompt(Prompts[i], PromptOwners[i].Get());
			// They ride with the visitor (in the car, on the pond).
			Button->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepWorldTransform);
			Buttons.Add(Button);
		}
		ButtonPrompts = Prompts;
	}
	// Out ahead, low and to the right, one under another, each facing the eyes.
	ButtonsYaw = HeadYaw;
	const FVector Eye = EyeLocation();
	const FRotationMatrix Facing(FRotator(0, HeadYaw, 0));
	for (int32 i = 0; i < Buttons.Num(); ++i)
	{
		const FVector Offset = ButtonsAt - FVector(0, 0, ButtonSpacing * i);
		const FVector At = Eye + Facing.TransformVector(Offset);
		Buttons[i]->SetActorLocationAndRotation(At, (Eye - At).Rotation());
	}
}

void AMuseeVRCharacter::Follow(UWidgetComponent& Panel, const FVector& Offset, float DeltaSeconds, bool bSnap) const
{
	const FVector Eye = EyeLocation();
	const FVector Target = Eye + FRotationMatrix(FRotator(0, HeadForward().Rotation().Yaw, 0)).TransformVector(Offset);
	const FVector At = bSnap ? Target : FMath::VInterpTo(Panel.GetComponentLocation(), Target, DeltaSeconds, 3.f);
	Panel.SetWorldLocationAndRotation(At, (Eye - At).Rotation());
}

void AMuseeVRCharacter::UpdatePanels(float DeltaSeconds)
{
	// Unlit in the room, the signs would take the museum's exposure: lift them by its inverse.
	const float Lift = FMath::Clamp(SignBrightness / (Exposure.IsValid() ? Exposure->Exposure() : 1.f), 0.25f, 1.0e5f);
	const FLinearColor Tint(Lift, Lift, Lift, 1.f);
	for (UWidgetComponent* Panel : {DotPanel.Get(), HintPanel.Get(), LabelPanel.Get(), MessagePanel.Get(), StepRing.Get()}) { Panel->SetTintColorAndOpacity(Tint); }
	for (AMuseeVRButton* Button : Buttons) { if (IsValid(Button)) { Button->Sign->SetTintColorAndOpacity(Tint); } }
	for (UWidgetComponent* BeamDot : BeamDots) { BeamDot->SetTintColorAndOpacity(Tint); }
	if (Cursor) { Cursor->SetTintColorAndOpacity(Tint); }

	// The dot and its hint, as on the desktop.
	HintAlpha = Crossfade(PointerHint.IsEmpty() ? LookHint() : PointerHint, ShownHint, *HintLabel, HintAlpha, DeltaSeconds);
	Present(*HintSign, HintAlpha);
	HintPanel->SetVisibility(HintAlpha > 0.f);
	Dot->SetEngaged(FMath::SmoothStep(0.f, 1.f, HintAlpha));

	// The wall label: faded in by the visitor after a moment's look.
	const FMuseeArtwork* Work = LookedAtWork();
	if (Work && Work != ShownWork)
	{
		Placard->SetWork(*Work);
		ShownWork = Work;
	}
	const float LabelAlpha = Work ? PlacardAlpha() : 0.f;
	Present(*Placard, LabelAlpha);
	const bool bLabel = LabelAlpha > 0.f;
	if (bLabel) { Follow(*LabelPanel, LabelAt, DeltaSeconds, !bLabelShown); }
	LabelPanel->SetVisibility(bLabel);
	bLabelShown = bLabel;

	MessageAlpha = Crossfade(Message(), ShownMessage, *MessageLine, MessageAlpha, DeltaSeconds);
	Present(*MessageSign, MessageAlpha);
	const bool bMessage = MessageAlpha > 0.f;
	if (bMessage) { Follow(*MessagePanel, MessageAt, DeltaSeconds, !bMessageShown); }
	MessagePanel->SetVisibility(bMessage);
	bMessageShown = bMessage;
}

void AMuseeVRCharacter::LogInput(float DeltaSeconds)
{
	if (InputLogFor <= 0.f) { return; }
	InputLogFor -= DeltaSeconds;
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) { return; }
	// Every hand key as it goes down or up, whatever the museum makes of it.
	static const FKey Keys[] = {
		EKeys::OculusTouch_Left_Trigger_Click, EKeys::OculusTouch_Right_Trigger_Click,
		EKeys::OculusTouch_Left_X_Click, EKeys::OculusTouch_Left_Y_Click, EKeys::OculusTouch_Left_Menu_Click,
		EKeys::OculusTouch_Right_A_Click, EKeys::OculusTouch_Right_B_Click,
		EKeys::OculusTouch_Left_Grip_Click, EKeys::OculusTouch_Right_Grip_Click,
		EKeys::OculusTouch_Left_Thumbstick_Click, EKeys::OculusTouch_Right_Thumbstick_Click,
		EKeys::ValveIndex_Left_Trigger_Click, EKeys::ValveIndex_Right_Trigger_Click,
		EKeys::ValveIndex_Left_A_Click, EKeys::ValveIndex_Left_B_Click, EKeys::ValveIndex_Right_A_Click, EKeys::ValveIndex_Right_B_Click,
	};
	for (const FKey& Key : Keys)
	{
		const bool bDown = PC->IsInputKeyDown(Key);
		bool& bWas = LoggedKeys.FindOrAdd(Key.GetFName());
		if (bDown != bWas) { UE_LOG(LogMusee, Log, TEXT("VR input: %s %s."), *Key.ToString(), bDown ? TEXT("down") : TEXT("up")); bWas = bDown; }
	}
	InputLogTime -= DeltaSeconds;
	if (InputLogTime > 0.f) { return; }
	InputLogTime = 2.f;
	UE_LOG(LogMusee, Log, TEXT("VR input: hands %s/%s; triggers L %.2f (Index %.2f) R %.2f (Index %.2f)."),
		LeftHand->IsTracked() ? TEXT("tracked") : TEXT("lost"), RightHand->IsTracked() ? TEXT("tracked") : TEXT("lost"),
		PC->GetInputAnalogKeyState(EKeys::OculusTouch_Left_Trigger_Axis), PC->GetInputAnalogKeyState(EKeys::ValveIndex_Left_Trigger_Axis),
		PC->GetInputAnalogKeyState(EKeys::OculusTouch_Right_Trigger_Axis), PC->GetInputAnalogKeyState(EKeys::ValveIndex_Right_Trigger_Axis));
}

void AMuseeVRCharacter::Tick(float DeltaSeconds)
{
	if (bTracked && !bRecentred)
	{
		// The headset has no pose at BeginPlay; recentre as soon as it does.
		FQuat Orientation;
		FVector Position;
		if (GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, Orientation, Position) && !(Orientation.Equals(FQuat::Identity, 1e-4) && Position.IsNearlyZero()))
		{
			bRecentred = true;
			GEngine->XRSystem->ResetOrientationAndPosition(0.f);
			UE_LOG(LogMusee, Log, TEXT("VR visitor: recentred on the first head pose."));
		}
	}
	LogInput(DeltaSeconds);
	if (bVR) { UpdateHands(); }
	if (bTracked && !InPhotoMode())
	{
		FollowHead();
		// The look (placards, hints, what a pinch uses) goes where the head points.
		if (AController* C = GetController()) { C->SetControlRotation(Camera->GetComponentRotation()); }
	}
	if (bGliding) { AddMovementInput(HeadForward(), 1.f); }
	Super::Tick(DeltaSeconds);
	if (!bVR) { return; }
	UpdateStep();
	UpdatePointer();
	UpdateButtons();
	UpdatePanels(DeltaSeconds);
}
