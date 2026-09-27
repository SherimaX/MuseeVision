#pragma once

#include "CoreMinimal.h"
#include "HAL/PreprocessorHelpers.h"
#include "MuseeFrame.h"
#include "MuseeKey.h"
#include "MuseeMeasurement.h"

/**
 * An instrument's probe: the owner's own way of measuring its truth where the visitor stands. `Light::MeasureIlluminance`,
 * `Form::FindLeakPaths`, `Light::ListSources`. Instruments lists the registered probes and reads them at the reticle
 * without depending on their owners; the physics of each reading stays with its owner.
 *
 * A probe is named `<System>::<Verb><Thing>`, the verb one of Measure, Find or List. A module declares a probe where
 * its function is written:
 *
 *     static TArray<FMuseeMeasurement> MeasureIlluminance(const IMuseeProbe::FQuery& Query);
 *     MUSEE_PROBE("Light::MeasureIlluminance", MeasureIlluminance);
 *
 * and it is registered while its module is loaded. A probe with state implements the interface and registers itself.
 */
class MUSEEFOUNDATION_API IMuseeProbe
{
public:
	/** Where a probe is read: the reticle's point and direction in the museum's frame, and the thing under it. */
	struct FQuery
	{
		/** The world the probe reads. */
		UWorld* World = nullptr;

		/** The point the visitor aims at, in metres in the museum's frame. */
		FVector PointMeters = FVector::ZeroVector;

		/** The direction the visitor looks along, unit length. */
		FVector Direction = FMuseeFrame::East();

		/** The keyed thing under the reticle, or the none key. */
		FMuseeKey Subject;
	};

	/** A probe written as a function. */
	using FFunction = TArray<FMuseeMeasurement> (*)(const FQuery& Query);

	/** Keeps a function probe registered while the module that declares it is loaded. MUSEE_PROBE makes one. */
	class MUSEEFOUNDATION_API FRegistrar
	{
	public:
		FRegistrar(const TCHAR* Name, FFunction Function, const ANSICHAR* Source);
		~FRegistrar();

		FRegistrar(const FRegistrar&) = delete;
		FRegistrar& operator=(const FRegistrar&) = delete;

	private:
		TSharedPtr<IMuseeProbe> Probe;
	};

	virtual ~IMuseeProbe() = default;

	/** The probe's name. */
	[[nodiscard]] virtual FName Name() const = 0;

	/** Reads the probe at a query. */
	[[nodiscard]] virtual TArray<FMuseeMeasurement> Read(const FQuery& Query) const = 0;

	/**
	 * Registers a probe. Refused, with an error, for a name that breaks the rule or one already registered from
	 * another source; a registration from the same source replaces the earlier one (Live Coding re-registers a file).
	 */
	static bool Register(const TSharedRef<IMuseeProbe>& Probe, const FString& Source);

	/** Unregisters a probe. */
	static void Unregister(FName Name);

	/** A registered probe, or null. */
	[[nodiscard]] static TSharedPtr<IMuseeProbe> Find(FName Name);

	/** Every registered probe's name, in name order. */
	[[nodiscard]] static TArray<FName> Names();

	/** Whether a name follows `<System>::<Verb><Thing>`; if not, OutWhy says how it breaks the rule. */
	[[nodiscard]] static bool IsValidName(FStringView Name, FString* OutWhy = nullptr);
};

/** Declares a probe where its function is written: `MUSEE_PROBE("Light::MeasureIlluminance", MeasureIlluminance);`. */
#define MUSEE_PROBE(Name, Function) \
	static const IMuseeProbe::FRegistrar UE_JOIN(GMuseeProbeRegistrar, __COUNTER__)(TEXT(Name), &Function, __FILE__)
