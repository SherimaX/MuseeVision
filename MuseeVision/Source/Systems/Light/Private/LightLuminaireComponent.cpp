#include "LightLuminaireComponent.h"

#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/TextureLightProfile.h"
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

		/** A lamp that lights all round spreads its flux over the whole sphere, 4 pi steradians. */
		constexpr double SphereSteradians = 4.0 * UE_DOUBLE_PI;

		/** Seconds in a minute. */
		constexpr double SecondsPerMinute = 60.0;

		/**
		 * A photometric file's Type C frame in the luminaire's frame, its throw on the luminaire's aim. The engine reads a
		 * light profile about its light's own axes (Engine/Shaders/Private/IESLightProfilesCommon.ush): the vertical angle
		 * from the light's x axis, and the horizontal angle from its -y axis, turning towards -z (a right-handed turn about
		 * x). The luminaire's x points at the schedule's aim, and a designer aims a luminaire's light, not its housing: so
		 * the direction of the file's greatest intensity, its throw, is carried onto x. The file's nadir lies below the throw
		 * in the vertical plane through it (the luminaire's x-z plane, z its seat's up taken square to its light), the throw's
		 * vertical angle down from x; the throw's horizontal plane is the side of the nadir towards the throw, which is up,
		 * towards +z. A track wallwasher tilted towards its wall so carries its asymmetric lens's brighter half onto the wall,
		 * its peak on the aim; a file whose peak is on its nadir (a spot's) keeps its nadir on x.
		 */
		FQuat TypeCInLuminaire(const FLightLuminaire& Type)
		{
			const double Tilt = FMath::DegreesToRadians(Type.PeakVertical.Get());
			const FVector Nadir(FMath::Cos(Tilt), 0.0, -FMath::Sin(Tilt));
			const FVector ThrowSide(FMath::Sin(Tilt), 0.0, FMath::Cos(Tilt));
			const FQuat ThrowPlaneUp = FRotationMatrix::MakeFromXY(Nadir, -ThrowSide).ToQuat();
			return ThrowPlaneUp * FQuat(FVector::ForwardVector, -FMath::DegreesToRadians(Type.PeakHorizontal.Get()));
		}

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

		/**
		 * The flux a type gives at full output: laid to a length if it is linear, over its face if it is areal; an image
		 * panel's at its peak white, whose face is Lambertian, so its flux is pi times its luminance times its area.
		 */
		double FullFlux(const FLightLuminaire& Type, double LengthMeters, const FVector2D& Face)
		{
			switch (Type.Emitter)
			{
			case FLightLuminaire::EEmitter::Linear: return Type.FluxPerMeterLumens * LengthMeters;
			case FLightLuminaire::EEmitter::Image: return LambertianSteradians * Type.PeakLuminanceCandelaPerSquareMeter * Face.X * Face.Y;
			default: return Type.IsAreal() ? Type.FluxPerSquareMeterLumens * Face.X * Face.Y : Type.Flux.Get();
			}
		}

		/**
		 * What each engine light is set to at full output: a spot's peak intensity where published, else the flux, shared
		 * equally between a luminaire's lamps.
		 */
		double FullSetting(const FLightLuminaire& Type, double LengthMeters, const FVector2D& Face)
		{
			const bool bByIntensity = Type.Emitter == FLightLuminaire::EEmitter::Spot && Type.PeakIntensity.Get() > 0.0;
			return bByIntensity ? Type.PeakIntensity.Get() : FullFlux(Type, LengthMeters, Face) / FMath::Max(Type.LampCount, 1);
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

		/** One lamp of a lamp luminaire: a sphere of light of the lamp's own size, in lumens. */
		ULocalLightComponent* MakeLamp(USceneComponent& Mount, const FLightLuminaire& Type)
		{
			UPointLightComponent* Lamp = NewObject<UPointLightComponent>(Mount.GetOwner(), NAME_None, RF_Transient);
			ColourAndShadow(*Lamp, Type);
			Lamp->SourceRadius = static_cast<float>(ToEngine(Type.SourceRadius));
			Lamp->IntensityUnits = ELightUnits::Lumens;
			const double LampFlux = Type.Flux.Get() / FMath::Max(Type.LampCount, 1);
			Lamp->AttenuationRadius = static_cast<float>(ToEngine(ReachOf(Candela(LampFlux / SphereSteradians))));
			return Lamp;
		}

		/**
		 * A type's measured distribution: a source the size of its luminous opening, carrying its photometric file's candela
		 * table through the profile Studio imported from the file. The engine takes the profile's own intensity, the file's
		 * greatest candela value, over a table normalised to it (bUseIESBrightness), in candelas, the unit the file measures
		 * in, so every direction gives the file's absolute candelas and the whole the file's flux; IESBrightnessScale is the
		 * dimming level (Apply). The light is placed in the file's Type C frame, its throw on the aim.
		 */
		ULocalLightComponent* MakeProfileLamp(USceneComponent& Mount, const FLightLuminaire& Type, UTextureLightProfile& Profile)
		{
			UPointLightComponent* Lamp = NewObject<UPointLightComponent>(Mount.GetOwner(), NAME_None, RF_Transient);
			ColourAndShadow(*Lamp, Type);
			Lamp->SourceRadius = static_cast<float>(ToEngine(Type.SourceRadius));
			Lamp->IntensityUnits = ELightUnits::Candelas;
			Lamp->IESTexture = &Profile;
			Lamp->bUseIESBrightness = true;
			Lamp->AttenuationRadius = static_cast<float>(ToEngine(ReachOf(Type.PeakIntensity)));
			Lamp->SetRelativeRotation(TypeCInLuminaire(Type));
			return Lamp;
		}

		/**
		 * Where lamp Index stands: on the axis for a single lamp, else at equal angles round a ring of the type's radius in
		 * the plane square to the luminaire's x axis, the first on its z axis.
		 */
		FVector LampOffset(const FLightLuminaire& Type, int32 Index)
		{
			if (Type.Emitter != FLightLuminaire::EEmitter::Point || Type.LampCount <= 1) { return FVector::ZeroVector; }
			const double Angle = 2.0 * UE_DOUBLE_PI * Index / Type.LampCount;
			const double Radius = ToEngine(Type.LampRingRadius);
			return FVector(0.0, Radius * FMath::Sin(Angle), Radius * FMath::Cos(Angle));
		}

		/** A face of light: a linear type's line of its length, a panel of its published face, or the face an areal type lights. */
		ULocalLightComponent* MakeFace(USceneComponent& Mount, const FLightLuminaire& Type, Meters Length, const FVector2D& AreaFace)
		{
			URectLightComponent* Face = NewObject<URectLightComponent>(Mount.GetOwner(), NAME_None, RF_Transient);
			ColourAndShadow(*Face, Type);
			const bool bLinear = Type.Emitter == FLightLuminaire::EEmitter::Linear;
			const FVector2D Size = bLinear ? FVector2D(Length.Get(), Type.EmittingWidth.Get())
				: Type.IsAreal() ? AreaFace : FVector2D(Type.EmittingWidth.Get(), Type.EmittingHeight.Get());
			Face->SourceWidth = static_cast<float>(ToEngine(Meters(Size.X)));
			Face->SourceHeight = static_cast<float>(ToEngine(Meters(Size.Y)));
			Face->IntensityUnits = ELightUnits::Lumens;
			Face->AttenuationRadius = static_cast<float>(ToEngine(ReachOf(Candela(FullFlux(Type, Length.Get(), AreaFace) / LambertianSteradians))));
			return Face;
		}
	}
}

ULightLuminaireComponent::ULightLuminaireComponent()
{
	SetMobility(EComponentMobility::Movable);
}

void ULightLuminaireComponent::Configure(const FMuseeKey& InType, ELightOperation InOperation, const FMuseeKey& InFollows, Musee::Meters InLength,
	const FVector2D& InFaceMeters, bool bInFaceEncloses)
{
	LuminaireType = InType;
	LuminaireOperation = InOperation;
	FollowedChannel = InFollows;
	LengthMeters = InLength.Get();
	FaceMeters = InFaceMeters;
	bFaceEncloses = bInFaceEncloses;
	const FLightLuminaire* Luminaire = Musee::Light::FindLuminaire(LuminaireType);
	Profile = Luminaire && Luminaire->Emitter == FLightLuminaire::EEmitter::Profile ? Musee::Light::FindProfile(*Luminaire) : nullptr;
	DestroyEmitters();
	if (IsRegistered()) { MakeEmitters(); }
}

double ULightLuminaireComponent::Level() const
{
	const FLightLuminaire* Luminaire = Musee::Light::FindLuminaire(LuminaireType);
	const double Picture = Luminaire && Luminaire->Emitter == FLightLuminaire::EEmitter::Image ? ImageLevel : 1.0;
	return bSwitchedOn ? ChannelLevel * Picture : 0.0;
}

Musee::Lumens ULightLuminaireComponent::EmittedFlux() const
{
	const FLightLuminaire* Luminaire = Musee::Light::FindLuminaire(LuminaireType);
	// Only the engine lights it drives give its light: a face that encloses it drives none, so it gives none (the dresser
	// reports it), until a surface of the picture it shows gives the light itself.
	if (!Luminaire || !Luminaire->bLit || EmitterLights.IsEmpty()) { return Musee::Lumens(0.0); }
	return Musee::Lumens(Musee::Light::FullFlux(*Luminaire, LengthMeters, FaceMeters) * Level());
}

void ULightLuminaireComponent::OnRegister()
{
	Super::OnRegister();
	if (EmitterLights.IsEmpty()) { MakeEmitters(); }
}

void ULightLuminaireComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	DestroyEmitters();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void ULightLuminaireComponent::DestroyEmitters()
{
	for (ULocalLightComponent* Light : EmitterLights)
	{
		if (Light) { Light->DestroyComponent(); }
	}
	EmitterLights.Reset();
}

void ULightLuminaireComponent::MakeEmitters()
{
	using EEmitter = FLightLuminaire::EEmitter;
	const FLightLuminaire* Luminaire = Musee::Light::FindLuminaire(LuminaireType);
	if (!Luminaire || !Luminaire->bLit || !GetOwner()) { return; }
	if (Luminaire->Emitter == EEmitter::Linear && LengthMeters <= 0.0) { return; }
	// An enclosing face shines in from all round, which no engine light (each shining out from one place) can stand for.
	if (Luminaire->IsAreal() && (FaceMeters.X * FaceMeters.Y <= 0.0 || bFaceEncloses)) { return; }
	if (Luminaire->Emitter == EEmitter::Profile && !Profile) { return; }
	const int32 Count = Luminaire->Emitter == EEmitter::Point ? FMath::Max(Luminaire->LampCount, 1) : 1;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		ULocalLightComponent* Light = Luminaire->Emitter == EEmitter::Spot ? Musee::Light::MakeSpot(*this, *Luminaire)
			: Luminaire->Emitter == EEmitter::Point ? Musee::Light::MakeLamp(*this, *Luminaire)
			: Luminaire->Emitter == EEmitter::Profile ? Musee::Light::MakeProfileLamp(*this, *Luminaire, *Profile)
			: Musee::Light::MakeFace(*this, *Luminaire, Length(), FaceMeters);
		Light->SetupAttachment(this);
		Light->SetRelativeLocation(Musee::Light::LampOffset(*Luminaire, Index));
		Light->RegisterComponent();
		EmitterLights.Add(Light);
	}
	Apply();
}

void ULightLuminaireComponent::Apply()
{
	const FLightLuminaire* Luminaire = Musee::Light::FindLuminaire(LuminaireType);
	if (!Luminaire) { return; }
	const bool bByProfile = Luminaire->Emitter == FLightLuminaire::EEmitter::Profile;
	const float Setting = static_cast<float>(Musee::Light::FullSetting(*Luminaire, LengthMeters, FaceMeters) * Level());
	for (ULocalLightComponent* Light : EmitterLights)
	{
		// A profile carries its own intensity, the file's; dimming turns its level down, as the driver's current does.
		if (bByProfile) { Light->SetIESBrightnessScale(static_cast<float>(Level())); }
		else { Light->SetIntensity(Setting); }
		Light->SetVisibility(Level() > 0.0);
	}
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

void ULightLuminaireComponent::ShowImage(double MeanLuminanceShare)
{
	ImageLevel = FMath::Clamp(MeanLuminanceShare, 0.0, 1.0);
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
