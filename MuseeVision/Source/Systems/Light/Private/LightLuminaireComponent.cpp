#include "LightLuminaireComponent.h"

#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "LightLog.h"
#include "LightLuminaire.h"
#include "MuseeSignals.h"
#include "Operation/OperationRecords.h"
#include "Sky/SkyPhotometry.h"
#include "TimeSubsystem.h"

namespace Musee::Light
{
	namespace
	{
		/**
		 * A beam's field angle (where the intensity falls to a tenth of its peak) from its beam angle (where it falls to
		 * half), for a beam that falls off as a Gaussian: sqrt(ln 10 / ln 2) = 1.8226. The beam and field angles are
		 * those of CIE 43 and IES LM-35.
		 */
		const double FieldPerBeam = FMath::Sqrt(FMath::Loge(10.0) / FMath::Loge(2.0));

		/** The widest half-angle the engine gives a spotlight's cone. */
		constexpr double WidestConeHalfAngleDegrees = 80.0;

		/** A Lambertian face's peak intensity is its flux over pi steradians. */
		constexpr double LambertianSteradians = UE_DOUBLE_PI;

		/** Seconds in a minute. */
		constexpr double SecondsPerMinute = 60.0;

		/**
		 * How far a luminaire's light is followed: until its illuminance falls to a third of the full Moon's (so about
		 * 0.1 lx), below which it is lost under the museum's darkest night sky.
		 */
		double ReachIlluminanceLux()
		{
			return SkyPhotometry::FullMoonIlluminance().Get() / 3.0;
		}

		Meters ReachOf(Candela PeakIntensity)
		{
			return Meters(FMath::Sqrt(PeakIntensity.Get() / ReachIlluminanceLux()));
		}

		/** The flux a type gives at full output, laid to a length if it is linear. */
		double FullFlux(const FLightLuminaire& Type, double LengthMeters)
		{
			return Type.Emitter == FLightLuminaire::EEmitter::Linear ? Type.FluxPerMeterLumens * LengthMeters : Type.Flux.Get();
		}

		/** What the engine light is set to at full output: a spot's peak intensity where published, else the flux. */
		double FullSetting(const FLightLuminaire& Type, double LengthMeters)
		{
			const bool bByIntensity = Type.Emitter == FLightLuminaire::EEmitter::Spot && Type.PeakIntensity.Get() > 0.0;
			return bByIntensity ? Type.PeakIntensity.Get() : FullFlux(Type, LengthMeters);
		}

		void ColourAndShadow(ULocalLightComponent& Light, const FLightLuminaire& Type)
		{
			Light.SetMobility(EComponentMobility::Movable);
			Light.bUseTemperature = true;
			Light.Temperature = static_cast<float>(Type.ColourTemperature.Get());
			Light.SetCastShadows(true);
		}

		ULocalLightComponent* MakeSpot(USceneComponent& Mount, const FLightLuminaire& Type)
		{
			USpotLightComponent* Spot = NewObject<USpotLightComponent>(Mount.GetOwner(), NAME_None, RF_Transient);
			ColourAndShadow(*Spot, Type);
			const double Beam = Type.BeamAngle.Get() / 2.0;
			const double Field = FMath::Min(Beam * FieldPerBeam, WidestConeHalfAngleDegrees);
			Spot->InnerConeAngle = static_cast<float>(Beam);
			Spot->OuterConeAngle = static_cast<float>(Field);
			Spot->SourceRadius = static_cast<float>(ToEngine(Type.SourceRadius));
			const bool bByIntensity = Type.PeakIntensity.Get() > 0.0;
			Spot->IntensityUnits = bByIntensity ? ELightUnits::Candelas : ELightUnits::Lumens;
			const Candela Peak = bByIntensity ? Type.PeakIntensity
				: Candela(Type.Flux.Get() / (2.0 * UE_DOUBLE_PI * (1.0 - FMath::Cos(FMath::DegreesToRadians(Field)))));
			Spot->AttenuationRadius = static_cast<float>(ToEngine(ReachOf(Peak)));
			return Spot;
		}

		ULocalLightComponent* MakeFace(USceneComponent& Mount, const FLightLuminaire& Type, Meters Length)
		{
			URectLightComponent* Face = NewObject<URectLightComponent>(Mount.GetOwner(), NAME_None, RF_Transient);
			ColourAndShadow(*Face, Type);
			const bool bLinear = Type.Emitter == FLightLuminaire::EEmitter::Linear;
			Face->SourceWidth = static_cast<float>(ToEngine(bLinear ? Length : Type.EmittingWidth));
			Face->SourceHeight = static_cast<float>(ToEngine(bLinear ? Type.EmittingWidth : Type.EmittingHeight));
			Face->IntensityUnits = ELightUnits::Lumens;
			Face->AttenuationRadius = static_cast<float>(ToEngine(ReachOf(Candela(FullFlux(Type, Length.Get()) / LambertianSteradians))));
			return Face;
		}
	}
}

ULightLuminaireComponent::ULightLuminaireComponent()
{
	SetMobility(EComponentMobility::Movable);
}

void ULightLuminaireComponent::Configure(const FMuseeKey& InType, ELightOperation InOperation, const FMuseeKey& InFollows, Musee::Meters InLength)
{
	LuminaireType = InType;
	LuminaireOperation = InOperation;
	FollowedChannel = InFollows;
	LengthMeters = InLength.Get();
	if (EmitterLight)
	{
		EmitterLight->DestroyComponent();
		EmitterLight = nullptr;
	}
	if (IsRegistered()) { MakeEmitter(); }
}

double ULightLuminaireComponent::Level() const
{
	return bSwitchedOn ? ChannelLevel : 0.0;
}

Musee::Lumens ULightLuminaireComponent::EmittedFlux() const
{
	const FLightLuminaire* Luminaire = Musee::Light::FindLuminaire(LuminaireType);
	if (!Luminaire || !Luminaire->bLit || !EmitterLight) { return Musee::Lumens(0.0); }
	return Musee::Lumens(Musee::Light::FullFlux(*Luminaire, LengthMeters) * Level());
}

void ULightLuminaireComponent::OnRegister()
{
	Super::OnRegister();
	if (!EmitterLight) { MakeEmitter(); }
}

void ULightLuminaireComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (EmitterLight)
	{
		EmitterLight->DestroyComponent();
		EmitterLight = nullptr;
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void ULightLuminaireComponent::MakeEmitter()
{
	const FLightLuminaire* Luminaire = Musee::Light::FindLuminaire(LuminaireType);
	if (!Luminaire || !Luminaire->bLit || !GetOwner()) { return; }
	if (Luminaire->Emitter == FLightLuminaire::EEmitter::Linear && LengthMeters <= 0.0) { return; }
	EmitterLight = Luminaire->Emitter == FLightLuminaire::EEmitter::Spot
		? Musee::Light::MakeSpot(*this, *Luminaire)
		: Musee::Light::MakeFace(*this, *Luminaire, Length());
	EmitterLight->SetupAttachment(this);
	EmitterLight->RegisterComponent();
	Apply();
}

void ULightLuminaireComponent::Apply()
{
	const FLightLuminaire* Luminaire = Musee::Light::FindLuminaire(LuminaireType);
	if (!EmitterLight || !Luminaire) { return; }
	EmitterLight->SetIntensity(static_cast<float>(Musee::Light::FullSetting(*Luminaire, LengthMeters) * Level()));
	EmitterLight->SetVisibility(Level() > 0.0);
}

void ULightLuminaireComponent::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	if (LuminaireOperation != ELightOperation::Always)
	{
		if (UTimeSubsystem* Time = World->GetSubsystem<UTimeSubsystem>())
		{
			FollowMoment(Time->Now());
			MomentHandle = Time->OnMoment().AddUObject(this, &ULightLuminaireComponent::FollowMoment);
		}
	}
	if (!FollowedChannel.IsNone())
	{
		UMuseeSignals* Signals = World->GetSubsystem<UMuseeSignals>();
		// A luminaire that follows a channel waits dark for the channel's first value.
		FollowChannel(Signals->Read(FollowedChannel).Get(0.0f));
		ChannelHandle = Signals->Subscribe(FollowedChannel, UMuseeSignals::FChannelDelegate::CreateUObject(this, &ULightLuminaireComponent::HearChannel));
	}
	Apply();
}

void ULightLuminaireComponent::EndPlay(EEndPlayReason::Type Reason)
{
	UWorld* World = GetWorld();
	if (UTimeSubsystem* Time = World ? World->GetSubsystem<UTimeSubsystem>() : nullptr) { Time->OnMoment().Remove(MomentHandle); }
	if (UMuseeSignals* Signals = World ? World->GetSubsystem<UMuseeSignals>() : nullptr) { Signals->Unsubscribe(FollowedChannel, ChannelHandle); }
	Super::EndPlay(Reason);
}

void ULightLuminaireComponent::HearChannel(const FMuseeChannel& Channel)
{
	FollowChannel(Channel.Value);
}

void ULightLuminaireComponent::FollowChannel(float Value)
{
	ChannelLevel = FMath::Clamp(static_cast<double>(Value), 0.0, 1.0);
	Apply();
}

void ULightLuminaireComponent::FollowMoment(const FTimeMoment& Moment)
{
	if (LuminaireOperation == ELightOperation::Dusk)
	{
		bSwitchedOn = !Moment.bSunUp;
	}
	else if (LuminaireOperation == ELightOperation::Presence)
	{
		const FLightOperationRecord* Operation = Musee::Light::OperationDesign();
		const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		if (SeesVisitor()) { LastSeenSeconds = Now; }
		bSwitchedOn = Operation && LastSeenSeconds.IsSet()
			&& Now - LastSeenSeconds.GetValue() <= Operation->Presence.HoldMinutes * Musee::Light::SecondsPerMinute;
	}
	Apply();
}

bool ULightLuminaireComponent::SeesVisitor() const
{
	const FLightOperationRecord* Operation = Musee::Light::OperationDesign();
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* Visitor = Controller ? Controller->GetPawn() : nullptr;
	if (!Operation || !Visitor) { return false; }
	// The detector's square field lies on the floor under it, aligned with the stage's axes.
	const FVector Offset = Visitor->GetActorLocation() - GetComponentLocation();
	const double HalfWidth = Musee::ToEngine(Musee::Meters(Operation->Presence.DetectionHalfWidthMeters));
	return FMath::Abs(Offset.X) <= HalfWidth && FMath::Abs(Offset.Y) <= HalfWidth;
}
