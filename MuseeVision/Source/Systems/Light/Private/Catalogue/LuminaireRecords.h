#pragma once

#include "CoreMinimal.h"

#include "LuminaireRecords.generated.h"

// A luminaire record of Data/Light/Luminaires as the record reader reads it, field for field (the fields are described
// in Docs/Light/Luminaires.md). A number the maker doesn't publish is null in the record; every such number is a
// non-negative measure, so -1 marks it here.

USTRUCT()
struct FLightProductRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Manufacturer;
	UPROPERTY() FString Family;
	UPROPERTY() FString Description;
	UPROPERTY() FString Code;
	UPROPERTY() FString CodeBasis;
	UPROPERTY() FString Designer;
	UPROPERTY() FString MakerClaim;
	UPROPERTY() FString Status;
	UPROPERTY() TArray<FString> NotableProjects;
};

USTRUCT()
struct FLightOutputRecord
{
	GENERATED_BODY()

	UPROPERTY() double LuminousFluxLumens = -1.0;
	UPROPERTY() double LuminousFluxLumensPerMeter = -1.0;
	UPROPERTY() FString FluxBasis;
	UPROPERTY() double LedFluxLumens = -1.0;
	UPROPERTY() double LedPackageLumensPerMeter = -1.0;
	UPROPERTY() double ReelFluxLumens = -1.0;
	UPROPERTY() FString BeamName;
	UPROPERTY() double BeamAngleDegrees = -1.0;
	UPROPERTY() double PeakCandela = -1.0;
	UPROPERTY() double CctKelvin = -1.0;
	UPROPERTY() double Cri = -1.0;
	UPROPERTY() FString CriBasis;
	UPROPERTY() double Tm30Rf = -1.0;
	UPROPERTY() double Tm30Rg = -1.0;
	UPROPERTY() double MacAdamSteps = -1.0;
	UPROPERTY() double ColourDeviationSdcm = -1.0;
	UPROPERTY() double LedsPerMeter = -1.0;
	UPROPERTY() double UgrMax = -1.0;
	UPROPERTY() double LuminanceAt65DegreesCandelaPerSquareMeterMax = -1.0;
	UPROPERTY() double EfficacyLumensPerWattMax = -1.0;
	UPROPERTY() double AimHorizontalDegrees = -1.0;
	UPROPERTY() double AimVerticalDegreesMax = -1.0;
};

USTRUCT()
struct FLightElectricalRecord
{
	GENERATED_BODY()

	UPROPERTY() double SystemPowerWatts = -1.0;
	UPROPERTY() double LedPowerWatts = -1.0;
	UPROPERTY() double PowerWattsPerMeter = -1.0;
	UPROPERTY() double ReelPowerWatts = -1.0;
	UPROPERTY() double EfficacyLumensPerWatt = -1.0;
	UPROPERTY() double SupplyVolts = -1.0;
	UPROPERTY() FString Supply;
	UPROPERTY() FString Control;
	UPROPERTY() FString Mounting;
	UPROPERTY() TArray<double> DimmingRangePercent;
};

USTRUCT()
struct FLightHousingRecord
{
	GENERATED_BODY()

	UPROPERTY() double DiameterMillimeters = -1.0;
	UPROPERTY() double CutOutDiameterMillimeters = -1.0;
	UPROPERTY() TArray<double> EnvelopeMillimeters;
	UPROPERTY() TArray<double> SectionMillimeters;
	/** A panel's emitting face, width and height. */
	UPROPERTY() TArray<double> EmittingMillimeters;
	UPROPERTY() double LengthMillimeters = -1.0;
	UPROPERTY() TArray<double> LengthMillimetersRange;
	UPROPERTY() double ReelLengthMillimeters = -1.0;
	UPROPERTY() TArray<double> ChannelLengthMillimetersRange;
	UPROPERTY() double ChannelStepMillimeters = -1.0;
	UPROPERTY() TArray<double> BattenLengthMillimetersRange;
	UPROPERTY() TArray<double> BracketMillimeters;
	UPROPERTY() TArray<double> DrawingDimensionsMillimeters;
	UPROPERTY() TArray<double> DriverCaseMillimeters;
	UPROPERTY() FString DimensionBasis;
	UPROPERTY() double HoodHeightMillimeters = -1.0;
	UPROPERTY() double HoodWidthMillimeters = -1.0;
	UPROPERTY() double SpacingMinMillimeters = -1.0;
	UPROPERTY() double TrackAdaptorLengthMillimeters = -1.0;
	UPROPERTY() double WeightKilograms = -1.0;
	UPROPERTY() double LightOutputRatioPercent = -1.0;
	UPROPERTY() double RotationDegrees = -1.0;
	UPROPERTY() double TiltDegrees = -1.0;
	UPROPERTY() FString Materials;
	UPROPERTY() FString Finish;
	UPROPERTY() TArray<FString> Finishes;
	UPROPERTY() TArray<FString> HousingColours;
	UPROPERTY() FString Protection;
	UPROPERTY() FString Mounting;
	/** A lamp luminaire's number of lamps; 0 where the record names none (one lamp). */
	UPROPERTY() int32 LampCount = 0;
	/** The radius of the ring its lamps stand on about its axis (an electrolier's arms' reach). */
	UPROPERTY() double LampRingRadiusMillimeters = -1.0;
};

USTRUCT()
struct FLightPhotometricFileRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Url;
	UPROPERTY() FString Note;
};

USTRUCT()
struct FLightSourceRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Id;
	UPROPERTY() FString Title;
	UPROPERTY() FString Url;
	UPROPERTY() FString ReadOn;
	UPROPERTY() TArray<FString> Covers;
};

/** One luminaire record. */
USTRUCT()
struct FLightLuminaireRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Key;
	UPROPERTY(meta = (MuseeRequired)) FString Kind;
	UPROPERTY(meta = (MuseeRequired)) FString ControlClass;
	/** `complete` or `incomplete`. */
	UPROPERTY(meta = (MuseeRequired)) FString EvidenceState;
	UPROPERTY() FLightProductRecord Product;
	UPROPERTY(meta = (MuseeRequired)) FLightOutputRecord Light;
	UPROPERTY() FLightElectricalRecord Electrical;
	UPROPERTY() FLightHousingRecord Housing;
	UPROPERTY() FLightPhotometricFileRecord PhotometricFile;
	UPROPERTY() TArray<FLightSourceRecord> Sources;
	UPROPERTY() TArray<FString> Unverified;
};
