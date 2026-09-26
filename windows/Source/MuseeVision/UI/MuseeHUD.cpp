#include "UI/MuseeHUD.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"
#include "Settings/MuseeGraphics.h"
#include "UI/SMuseeOverlay.h"
#include "UI/SMuseeSettings.h"
#include "Visitor/MuseeCharacter.h"

void AMuseeHUD::BeginPlay()
{
	Super::BeginPlay();
	UMuseeGraphics* Graphics = UMuseeGraphics::Get(this);
	// DLSS and Streamline are loaded now: the settings take hold of them.
	if (Graphics) { Graphics->Reapply(); }
	if (UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		MuseeOverlay = SNew(SMuseeOverlay).Visibility(EVisibility::HitTestInvisible);
		ViewportClient->AddViewportWidgetContent(MuseeOverlay.ToSharedRef(), 10);
		if (Graphics && Graphics->IsAvailable())
		{
			SettingsLayer = SNew(SMuseeSettingsLayer).Graphics(Graphics).Visibility(EVisibility::SelfHitTestInvisible);
			ViewportClient->AddViewportWidgetContent(SettingsLayer.ToSharedRef(), 20);
		}
	}
}

void AMuseeHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bSettingsOpen) { SetSettingsOpen(false); }
	if (UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		if (MuseeOverlay.IsValid()) { ViewportClient->RemoveViewportWidgetContent(MuseeOverlay.ToSharedRef()); }
		if (SettingsLayer.IsValid()) { ViewportClient->RemoveViewportWidgetContent(SettingsLayer.ToSharedRef()); }
	}
	MuseeOverlay.Reset();
	SettingsLayer.Reset();
	Super::EndPlay(EndPlayReason);
}

void AMuseeHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(GetOwningPawn());
	if (MuseeOverlay.IsValid())
	{
		// The overlay never takes the mouse; showhud hides it like the canvas, and the settings card clears it away.
		MuseeOverlay->SetVisibility(Visitor && bShowHUD && !bSettingsOpen ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
		if (Visitor) { MuseeOverlay->Update(*Visitor, DeltaSeconds); }
	}

	UMuseeGraphics* Graphics = UMuseeGraphics::Get(this);
	if (!Graphics || !SettingsLayer.IsValid()) { return; }
	// Real time, not the world's: the numbers are the machine's.
	const float RealDelta = float(FApp::GetDeltaTime());
	Graphics->SampleFrame(RealDelta);
	const bool bWantOpen = Graphics->IsPanelOpen() && bShowHUD;
	if (bWantOpen != bSettingsOpen) { SetSettingsOpen(bWantOpen); }
	SettingsLayer->SetVisibility(bShowHUD ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	SettingsLayer->Update(RealDelta, Visitor && Visitor->InPhotoMode());
}

void AMuseeHUD::SetSettingsOpen(bool bOpen)
{
	bSettingsOpen = bOpen;
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) { return; }
	// The view holds still while the card is up: no looking, no walking (paired with the release below).
	PC->SetIgnoreLookInput(bOpen);
	PC->SetIgnoreMoveInput(bOpen);
	if (bOpen && SettingsLayer.IsValid() && SettingsLayer->Card().IsValid())
	{
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(SettingsLayer->Card());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		if (FSlateApplication::IsInitialized()) { FSlateApplication::Get().SetAllUserFocusToGameViewport(); }
		if (UMuseeGraphics* Graphics = UMuseeGraphics::Get(this)) { Graphics->SetPanelOpen(false); }
	}
}
