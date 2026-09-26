#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MuseeHUD.generated.h"

class SMuseeOverlay;
class SMuseeSettingsLayer;

/**
 * The visitor's overlay (iOS/Overlays.swift), in Slate over the game viewport (SMuseeOverlay):
 * a small dot at the centre of the view, the hint for what is looked at, the wall label for the
 * work (artist, title, date, medium, collection, note), the buttons for what can be done where
 * you stand, and the museum's message line. Scaled by the viewport's DPI; no assets needed.
 * Above it, the graphics settings (SMuseeSettingsLayer): the card, which takes the keys, the mouse
 * and the gamepad while it is open (the view holds still), the toast and the frame-rate readout.
 */
UCLASS()
class MUSEEVISION_API AMuseeHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Give the card the input, or give it back to the visitor. */
	void SetSettingsOpen(bool bOpen);

	TSharedPtr<SMuseeOverlay> MuseeOverlay;
	TSharedPtr<SMuseeSettingsLayer> SettingsLayer;
	bool bSettingsOpen = false;
};
