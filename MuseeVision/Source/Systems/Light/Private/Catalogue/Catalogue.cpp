#include "Catalogue/LuminaireRecords.h"
#include "Engine/TextureLightProfile.h"
#include "LightLog.h"
#include "LightLuminaire.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "Photometry/PhotometricFile.h"

namespace Musee::Light
{
	namespace
	{
		const TCHAR* const LuminairesFolder = TEXT("Light/Luminaires");
		const TCHAR* const CompleteEvidence = TEXT("complete");

		/**
		 * The evidence of a record whose maker publishes no photometry for it, and whose light is worked out from the
		 * physics of what it is made of and the numbers its parts' makers do publish (an edge-lit pane: its LED line's
		 * flux per metre and a light guide's published extraction efficiency). Its fluxBasis gives the derivation.
		 */
		const TCHAR* const DerivedEvidence = TEXT("derived");

		/** A millimetre is a thousandth of a metre. */
		constexpr double MillimetersPerMeter = 1000.0;

		/**
		 * The most a photometric file's integrated flux may differ from its record's, as a share of the record's: the file
		 * and the record state the same measurement, so they agree to the rounding of the published flux.
		 */
		constexpr double FileFluxAgreement = 0.01;

		/** LM-63's photometric type of a Type C table, the frame a light profile is imported in. */
		constexpr int32 TypeCPhotometry = 1;

		/**
		 * How each kind's light leaves it. A wallwasher's asymmetric lens shapes light in a way only its photometric file
		 * describes, so it is lit by that file; a picture light's hood has no file yet, so it has no emitter. A step light's, a
		 * pinspot's and an uplight's light is a beam about their axis, and so is a framing projector's with its flaps open
		 * (its flaps' cut to the frame is not drawn yet); an edge-lit pane lets its light out evenly over its etched face, as a
		 * laylight's diffusing face does; a pendant's lamp, and a post-top lantern's, lights all round inside its globe; an
		 * image panel's light is its picture.
		 */
		TOptional<FLightLuminaire::EEmitter> EmitterOfKind(const FString& Kind)
		{
			if (Kind == TEXT("track-wallwasher")) { return FLightLuminaire::EEmitter::Profile; }
			if (Kind == TEXT("track-spot") || Kind == TEXT("track-framing") || Kind == TEXT("downlight") || Kind == TEXT("step-light") || Kind == TEXT("pinspot")
				|| Kind == TEXT("uplight"))
			{
				return FLightLuminaire::EEmitter::Spot;
			}
			if (Kind == TEXT("linear-cove") || Kind == TEXT("linear-slot") || Kind == TEXT("linear-strip") || Kind == TEXT("case-linear"))
			{
				return FLightLuminaire::EEmitter::Linear;
			}
			if (Kind == TEXT("laylight") || Kind == TEXT("edge-lit-glass")) { return FLightLuminaire::EEmitter::Rect; }
			if (Kind == TEXT("lantern") || Kind == TEXT("electrolier") || Kind == TEXT("pendant") || Kind == TEXT("post-top"))
			{
				return FLightLuminaire::EEmitter::Point;
			}
			if (Kind == TEXT("light-field-panel")) { return FLightLuminaire::EEmitter::Image; }
			return {};
		}

		/**
		 * Whether a kind lights a face the room gives it (an extent or a zone) rather than a face of its own: a luminous
		 * ceiling's modules over the plenum, edge-lit panes over the canopy they glaze, an image panel over its wall. It is
		 * what the product is, whether or not its maker publishes how much light each square metre gives.
		 */
		bool IsArealKind(const FString& Kind)
		{
			return Kind == TEXT("laylight") || Kind == TEXT("edge-lit-glass") || Kind == TEXT("light-field-panel");
		}

		/** The steps of the Simpson's rule that integrates a beam's intensity over its hemisphere. */
		constexpr int32 BeamIntegrationSteps = 256;

		/** How many halvings find a beam's spread: to a millionth of a radian over the hemisphere's width. */
		constexpr int32 BeamBisections = 48;

		/**
		 * The solid angle, in steradians, a beam of unit peak spreads over the hemisphere in front of it when its intensity
		 * falls off as a Gaussian of spread Sigma (radians): 2 pi times the integral of exp(-theta^2 / 2 sigma^2) sin theta
		 * over theta from 0 to pi / 2. Its flux is this times its peak intensity.
		 */
		double GaussianBeamSteradians(double Sigma)
		{
			const double Half = UE_DOUBLE_HALF_PI;
			const double Step = Half / BeamIntegrationSteps;
			double Sum = 0.0;
			for (int32 I = 0; I <= BeamIntegrationSteps; ++I)
			{
				const double Theta = I * Step;
				const double Weight = (I == 0 || I == BeamIntegrationSteps) ? 1.0 : (I % 2 == 1 ? 4.0 : 2.0);
				Sum += Weight * FMath::Exp(-Theta * Theta / (2.0 * Sigma * Sigma)) * FMath::Sin(Theta);
			}
			return 2.0 * UE_DOUBLE_PI * Sum * Step / 3.0;
		}

		/**
		 * The beam angle (the full width at half the peak) of the Gaussian beam a luminaire's published flux and peak
		 * intensity fix together, for a maker who publishes those two and no beam angle (a step light's spec sheet). Unset
		 * if no beam into a hemisphere carries that much flux under that peak.
		 */
		TOptional<double> GaussianBeamAngleDegrees(double FluxLumens, double PeakCandela)
		{
			const double Steradians = FluxLumens / PeakCandela;
			double Narrow = 1e-4;
			double Wide = 1e3;
			if (Steradians <= GaussianBeamSteradians(Narrow) || Steradians >= GaussianBeamSteradians(Wide)) { return {}; }
			for (int32 I = 0; I < BeamBisections; ++I)
			{
				const double Middle = 0.5 * (Narrow + Wide);
				(GaussianBeamSteradians(Middle) < Steradians ? Narrow : Wide) = Middle;
			}
			// A Gaussian's full width at half its peak is 2 sqrt(2 ln 2) sigma.
			return FMath::RadiansToDegrees(2.0 * FMath::Sqrt(2.0 * FMath::Loge(2.0)) * 0.5 * (Narrow + Wide));
		}

		/** A spot's beam angle: as published, else the Gaussian beam its flux and peak fix, else unset. */
		TOptional<double> BeamAngleOf(const FLightLuminaireRecord& Record)
		{
			if (Record.Light.BeamAngleDegrees > 0.0) { return Record.Light.BeamAngleDegrees; }
			if (Record.Light.LuminousFluxLumens > 0.0 && Record.Light.PeakCandela > 0.0)
			{
				return GaussianBeamAngleDegrees(Record.Light.LuminousFluxLumens, Record.Light.PeakCandela);
			}
			return {};
		}

		Meters FromMillimeters(double Millimeters)
		{
			return Meters(Millimeters / MillimetersPerMeter);
		}

		/** The maker's photometric file a record names under Data, read; the reason, if it names none or it can't be read. */
		TValueOrError<FPhotometricFile, FString> PhotometryOf(const FLightLuminaireRecord& Record)
		{
			if (Record.PhotometricFile.Path.IsEmpty())
			{
				return MakeError(FString::Printf(TEXT("Light has no emitter for the kind '%s' without its photometric file"), *Record.Kind));
			}
			TValueOrError<FPhotometricFile, FString> File = ReadPhotometricFile(Records::DataRoot() / Record.PhotometricFile.Path);
			if (File.HasError())
			{
				return MakeError(FString::Printf(TEXT("its photometric file %s can't be read: %s"), *Record.PhotometricFile.Path, *File.GetError()));
			}
			return File;
		}

		/** Why a type lit by its photometric file can't be lit, or empty: the file is a Type C table carrying its record's flux. */
		FString WhyProfileUnlit(const FLightLuminaireRecord& Record, const TValueOrError<FPhotometricFile, FString>& File)
		{
			if (File.HasError()) { return File.GetError(); }
			if (File.GetValue().PhotometricType != TypeCPhotometry) { return TEXT("its photometric file isn't a Type C table"); }
			const double FileFlux = File.GetValue().FluxLumens();
			const double RecordFlux = Record.Light.LuminousFluxLumens;
			if (RecordFlux <= 0.0 || FMath::Abs(FileFlux - RecordFlux) > FileFluxAgreement * RecordFlux)
			{
				return FString::Printf(TEXT("its photometric file carries %.1f lm, not its record's flux"), FileFlux);
			}
			return FString();
		}

		/** Why the record can't be lit, or empty if it can: every number its emitter needs is published. */
		FString WhyUnlit(const FLightLuminaireRecord& Record, const TOptional<FLightLuminaire::EEmitter>& Emitter)
		{
			if (!Emitter.IsSet()) { return FString::Printf(TEXT("Light has no emitter for the kind '%s' without its photometric file"), *Record.Kind); }
			if (Record.EvidenceState != CompleteEvidence && Record.EvidenceState != DerivedEvidence) { return TEXT("its evidence is incomplete"); }
			if (Record.Light.CctKelvin <= 0.0) { return TEXT("its colour temperature isn't published"); }
			switch (Emitter.GetValue())
			{
			case FLightLuminaire::EEmitter::Spot:
				if (!BeamAngleOf(Record).IsSet()) { return TEXT("neither its beam angle nor its flux and peak intensity together are published"); }
				if (Record.Light.PeakCandela <= 0.0 && Record.Light.LuminousFluxLumens <= 0.0) { return TEXT("neither its peak intensity nor its flux is published"); }
				return FString();
			case FLightLuminaire::EEmitter::Linear:
				if (Record.Light.LuminousFluxLumensPerMeter <= 0.0) { return TEXT("its flux per metre isn't published"); }
				if (Record.Housing.SectionMillimeters.IsEmpty()) { return TEXT("the width of its emitting line isn't published"); }
				return FString();
			case FLightLuminaire::EEmitter::Point:
				if (Record.Light.LuminousFluxLumens <= 0.0) { return TEXT("its lamps' flux isn't published"); }
				if (Record.Housing.LampCount > 1 && Record.Housing.LampRingRadiusMillimeters <= 0.0) { return TEXT("the ring its lamps stand on isn't given"); }
				return FString();
			case FLightLuminaire::EEmitter::Image:
				if (Record.Light.PeakLuminanceCandelaPerSquareMeter <= 0.0) { return TEXT("its luminance at peak white isn't published"); }
				return FString();
			case FLightLuminaire::EEmitter::Profile:
				// Its photometric file holds every number its light needs: TakePhotometry reads it and checks it.
				return FString();
			default:
				if (Record.Light.LuminousFluxLumensPerSquareMeter > 0.0) { return FString(); }
				if (IsArealKind(Record.Kind)) { return TEXT("its flux per square metre of the face it lights isn't published"); }
				if (Record.Light.LuminousFluxLumens <= 0.0) { return TEXT("neither its flux nor its flux per square metre is published"); }
				if (Record.Housing.EmittingMillimeters.Num() != 2) { return TEXT("its emitting face isn't published"); }
				return FString();
			}
		}

		/**
		 * A type lit by its photometric file takes its light from the file: the flux its table integrates to, its greatest
		 * intensity, and, where the file gives a round luminous opening, that opening's radius as its source's.
		 */
		void TakePhotometry(const FLightLuminaireRecord& Record, FLightLuminaire& Type)
		{
			const TValueOrError<FPhotometricFile, FString> File = PhotometryOf(Record);
			const FString Why = WhyProfileUnlit(Record, File);
			if (Type.WhyUnlit.IsEmpty()) { Type.WhyUnlit = Why; }
			Type.bLit = Type.WhyUnlit.IsEmpty();
			if (File.HasError()) { return; }
			const FPhotometricFile& Table = File.GetValue();
			Type.PhotometricFile = FPaths::ConvertRelativePathToFull(Records::DataRoot() / Record.PhotometricFile.Path);
			Type.Flux = Lumens(Table.FluxLumens());
			Type.PeakIntensity = Candela(Table.PeakCandela());
			if (const TOptional<double> Radius = Table.RoundOpeningRadiusMeters()) { Type.SourceRadius = Meters(Radius.GetValue()); }
		}

		FLightLuminaire FromRecord(const FLightLuminaireRecord& Record, const FMuseeKey& Key)
		{
			const TOptional<FLightLuminaire::EEmitter> Emitter = EmitterOfKind(Record.Kind);
			FLightLuminaire Result;
			Result.Key = Key;
			Result.Kind = FName(*Record.Kind);
			Result.Emitter = Emitter.Get(FLightLuminaire::EEmitter::Spot);
			Result.ControlClass = FName(*Record.ControlClass);
			Result.bAreal = IsArealKind(Record.Kind);
			Result.WhyUnlit = WhyUnlit(Record, Emitter);
			Result.bLit = Result.WhyUnlit.IsEmpty();
			Result.Flux = Lumens(FMath::Max(Record.Light.LuminousFluxLumens, 0.0));
			Result.FluxPerMeterLumens = FMath::Max(Record.Light.LuminousFluxLumensPerMeter, 0.0);
			Result.FluxPerSquareMeterLumens = FMath::Max(Record.Light.LuminousFluxLumensPerSquareMeter, 0.0);
			Result.PeakLuminanceCandelaPerSquareMeter = FMath::Max(Record.Light.PeakLuminanceCandelaPerSquareMeter, 0.0);
			Result.PeakIntensity = Candela(FMath::Max(Record.Light.PeakCandela, 0.0));
			Result.BeamAngle = Degrees(Result.Emitter == FLightLuminaire::EEmitter::Spot ? BeamAngleOf(Record).Get(0.0) : FMath::Max(Record.Light.BeamAngleDegrees, 0.0));
			Result.ColourTemperature = Kelvin(FMath::Max(Record.Light.CctKelvin, 0.0));
			const double Diameter = Record.Housing.CutOutDiameterMillimeters > 0.0 ? Record.Housing.CutOutDiameterMillimeters : Record.Housing.DiameterMillimeters;
			Result.SourceRadius = FromMillimeters(FMath::Max(Diameter, 0.0) / 2.0);
			const TArray<double>& Face = Result.Emitter == FLightLuminaire::EEmitter::Rect ? Record.Housing.EmittingMillimeters : Record.Housing.SectionMillimeters;
			Result.EmittingWidth = FromMillimeters(Face.Num() > 0 ? Face[0] : 0.0);
			Result.EmittingHeight = FromMillimeters(Face.Num() > 1 ? Face[1] : 0.0);
			Result.LampCount = FMath::Max(Record.Housing.LampCount, 1);
			Result.LampRingRadius = FromMillimeters(FMath::Max(Record.Housing.LampRingRadiusMillimeters, 0.0));
			if (Result.Emitter == FLightLuminaire::EEmitter::Profile) { TakePhotometry(Record, Result); }
			return Result;
		}

		TMap<FMuseeKey, FLightLuminaire> ReadCatalogue()
		{
			TMap<FMuseeKey, FLightLuminaire> Catalogue;
			TValueOrError<TArray<FLightLuminaireRecord>, FString> Read = ReadRecords<FLightLuminaireRecord>(LuminairesFolder);
			if (Read.HasError())
			{
				UE_LOG(LogMuseeLight, Error, TEXT("The luminaire catalogue can't be read: %s"), *Read.GetError());
				return Catalogue;
			}
			for (const FLightLuminaireRecord& Record : Read.GetValue())
			{
				const TOptional<FMuseeKey> Key = FMuseeKey::Parse(Record.Key);
				if (!Key.IsSet())
				{
					UE_LOG(LogMuseeLight, Error, TEXT("A luminaire record's key '%s' isn't a key."), *Record.Key);
					continue;
				}
				Catalogue.Add(Key.GetValue(), FromRecord(Record, Key.GetValue()));
			}
			Catalogue.KeySort([](const FMuseeKey& A, const FMuseeKey& B) { return A.ToString() < B.ToString(); });
			return Catalogue;
		}

		const TMap<FMuseeKey, FLightLuminaire>& Catalogue()
		{
			static const TMap<FMuseeKey, FLightLuminaire> Kept = ReadCatalogue();
			return Kept;
		}
	}

	const FLightLuminaire* FindLuminaire(const FMuseeKey& Key)
	{
		return Catalogue().Find(Key);
	}

	TArray<FMuseeKey> LuminaireKeys()
	{
		TArray<FMuseeKey> Keys;
		Catalogue().GetKeys(Keys);
		return Keys;
	}

	FString ProfileName(const FLightLuminaire& Type)
	{
		if (Type.PhotometricFile.IsEmpty()) { return FString(); }
		return TEXT("IES_") + FPaths::GetBaseFilename(Type.PhotometricFile).Replace(TEXT("-"), TEXT("_")).Replace(TEXT("."), TEXT("_"));
	}

	FString ProfilePackage(const FLightLuminaire& Type)
	{
		const FString Name = ProfileName(Type);
		return Name.IsEmpty() ? FString() : FString(ProfilesFolder) / Name;
	}

	UTextureLightProfile* FindProfile(const FLightLuminaire& Type)
	{
		const FString Package = ProfilePackage(Type);
		if (Package.IsEmpty()) { return nullptr; }
		const FString Object = Package + TEXT(".") + FPackageName::GetShortName(Package);
		if (UTextureLightProfile* Loaded = FindObject<UTextureLightProfile>(nullptr, *Object)) { return Loaded; }
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UTextureLightProfile>(nullptr, *Object) : nullptr;
	}
}
