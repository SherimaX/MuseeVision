#include "Catalogue/LuminaireRecords.h"
#include "LightLog.h"
#include "LightLuminaire.h"
#include "MuseeRecords.h"

namespace Musee::Light
{
	namespace
	{
		const TCHAR* const LuminairesFolder = TEXT("Light/Luminaires");
		const TCHAR* const CompleteEvidence = TEXT("complete");

		/** A millimetre is a thousandth of a metre. */
		constexpr double MillimetersPerMeter = 1000.0;

		/**
		 * How each kind's light leaves it. A wallwasher's asymmetric lens, a framer's shutters, a picture light's hood and
		 * a step light's louvre shape light in ways only their photometric files describe, so those kinds have no emitter
		 * yet. An edge-lit pane lets its light out evenly over its etched face, as a laylight's diffusing face does.
		 */
		TOptional<FLightLuminaire::EEmitter> EmitterOfKind(const FString& Kind)
		{
			if (Kind == TEXT("track-spot") || Kind == TEXT("downlight")) { return FLightLuminaire::EEmitter::Spot; }
			if (Kind == TEXT("linear-cove") || Kind == TEXT("linear-slot") || Kind == TEXT("linear-strip") || Kind == TEXT("case-linear"))
			{
				return FLightLuminaire::EEmitter::Linear;
			}
			if (Kind == TEXT("laylight") || Kind == TEXT("edge-lit-glass")) { return FLightLuminaire::EEmitter::Rect; }
			if (Kind == TEXT("lantern") || Kind == TEXT("electrolier")) { return FLightLuminaire::EEmitter::Point; }
			return {};
		}

		Meters FromMillimeters(double Millimeters)
		{
			return Meters(Millimeters / MillimetersPerMeter);
		}

		/** Why the record can't be lit, or empty if it can: every number its emitter needs is published. */
		FString WhyUnlit(const FLightLuminaireRecord& Record, const TOptional<FLightLuminaire::EEmitter>& Emitter)
		{
			if (!Emitter.IsSet()) { return FString::Printf(TEXT("Light has no emitter for the kind '%s' without its photometric file"), *Record.Kind); }
			if (Record.EvidenceState != CompleteEvidence) { return TEXT("its evidence is incomplete"); }
			if (Record.Light.CctKelvin <= 0.0) { return TEXT("its colour temperature isn't published"); }
			switch (Emitter.GetValue())
			{
			case FLightLuminaire::EEmitter::Spot:
				if (Record.Light.BeamAngleDegrees <= 0.0) { return TEXT("its beam angle isn't published"); }
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
			default:
				if (Record.Light.LuminousFluxLumens <= 0.0) { return TEXT("its flux isn't published"); }
				if (Record.Housing.EmittingMillimeters.Num() != 2) { return TEXT("its emitting face isn't published"); }
				return FString();
			}
		}

		FLightLuminaire FromRecord(const FLightLuminaireRecord& Record, const FMuseeKey& Key)
		{
			const TOptional<FLightLuminaire::EEmitter> Emitter = EmitterOfKind(Record.Kind);
			FLightLuminaire Result;
			Result.Key = Key;
			Result.Kind = FName(*Record.Kind);
			Result.Emitter = Emitter.Get(FLightLuminaire::EEmitter::Spot);
			Result.ControlClass = FName(*Record.ControlClass);
			Result.WhyUnlit = WhyUnlit(Record, Emitter);
			Result.bLit = Result.WhyUnlit.IsEmpty();
			Result.Flux = Lumens(FMath::Max(Record.Light.LuminousFluxLumens, 0.0));
			Result.FluxPerMeterLumens = FMath::Max(Record.Light.LuminousFluxLumensPerMeter, 0.0);
			Result.PeakIntensity = Candela(FMath::Max(Record.Light.PeakCandela, 0.0));
			Result.BeamAngle = Degrees(FMath::Max(Record.Light.BeamAngleDegrees, 0.0));
			Result.ColourTemperature = Kelvin(FMath::Max(Record.Light.CctKelvin, 0.0));
			const double Diameter = Record.Housing.CutOutDiameterMillimeters > 0.0 ? Record.Housing.CutOutDiameterMillimeters : Record.Housing.DiameterMillimeters;
			Result.SourceRadius = FromMillimeters(FMath::Max(Diameter, 0.0) / 2.0);
			const TArray<double>& Face = Result.Emitter == FLightLuminaire::EEmitter::Rect ? Record.Housing.EmittingMillimeters : Record.Housing.SectionMillimeters;
			Result.EmittingWidth = FromMillimeters(Face.Num() > 0 ? Face[0] : 0.0);
			Result.EmittingHeight = FromMillimeters(Face.Num() > 1 ? Face[1] : 0.0);
			Result.LampCount = FMath::Max(Record.Housing.LampCount, 1);
			Result.LampRingRadius = FromMillimeters(FMath::Max(Record.Housing.LampRingRadiusMillimeters, 0.0));
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
}
