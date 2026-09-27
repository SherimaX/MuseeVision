#include "MuseeProbe.h"

#include "Misc/ScopeRWLock.h"
#include "MuseeFoundationLog.h"
#include "String/Find.h"

namespace Musee
{
	namespace
	{
		/** The verbs a probe's name may begin with, after its system. */
		const TCHAR* const ProbeVerbs[] = {TEXT("Measure"), TEXT("Find"), TEXT("List")};

		/** A probe written as a function, registered by MUSEE_PROBE. */
		class FFunctionProbe final : public IMuseeProbe
		{
		public:
			FFunctionProbe(FName InName, FFunction InFunction)
				: ProbeName(InName)
				, Function(InFunction)
			{
			}

			virtual FName Name() const override { return ProbeName; }
			virtual TArray<FMuseeMeasurement> Read(const FQuery& Query) const override { return Function(Query); }

		private:
			FName ProbeName;
			FFunction Function;
		};

		struct FRegisteredProbe
		{
			TSharedRef<IMuseeProbe> Probe;
			FString Source;
		};

		/** Probes register as their modules load; Instruments reads them on the game thread and the runner elsewhere. */
		struct FProbeTable
		{
			FRWLock Lock;
			TMap<FName, FRegisteredProbe> Probes;
		};

		FProbeTable& ProbeTable()
		{
			static FProbeTable Registered;
			return Registered;
		}

		bool IsLettersAndDigits(FStringView Text)
		{
			for (const TCHAR Character : Text)
			{
				if (!FChar::IsAlnum(Character) || Character > 0x7F)
				{
					return false;
				}
			}
			return true;
		}
	}
}

IMuseeProbe::FRegistrar::FRegistrar(const TCHAR* Name, FFunction Function, const ANSICHAR* Source)
{
	const TSharedRef<IMuseeProbe> Made = MakeShared<Musee::FFunctionProbe>(FName(Name), Function);
	if (Register(Made, FString(Source)))
	{
		Probe = Made;
	}
}

IMuseeProbe::FRegistrar::~FRegistrar()
{
	if (Probe.IsValid() && Find(Probe->Name()) == Probe)
	{
		Unregister(Probe->Name());
	}
}

bool IMuseeProbe::Register(const TSharedRef<IMuseeProbe>& Probe, const FString& Source)
{
	const FName Name = Probe->Name();
	FString Why;
	if (!IsValidName(Name.ToString(), &Why))
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("'%s' can't be a probe: %s."), *Name.ToString(), *Why);
		return false;
	}
	FWriteScopeLock Lock(Musee::ProbeTable().Lock);
	const Musee::FRegisteredProbe* Existing = Musee::ProbeTable().Probes.Find(Name);
	if (Existing && (Source.IsEmpty() || Existing->Source != Source))
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("Probe '%s' is already registered by %s; a measurement has one probe."),
			*Name.ToString(), *Existing->Source);
		return false;
	}
	Musee::ProbeTable().Probes.Add(Name, Musee::FRegisteredProbe{Probe, Source});
	return true;
}

void IMuseeProbe::Unregister(FName Name)
{
	FWriteScopeLock Lock(Musee::ProbeTable().Lock);
	Musee::ProbeTable().Probes.Remove(Name);
}

TSharedPtr<IMuseeProbe> IMuseeProbe::Find(FName Name)
{
	FReadScopeLock Lock(Musee::ProbeTable().Lock);
	const Musee::FRegisteredProbe* Registered = Musee::ProbeTable().Probes.Find(Name);
	return Registered ? Registered->Probe.ToSharedPtr() : TSharedPtr<IMuseeProbe>();
}

TArray<FName> IMuseeProbe::Names()
{
	TArray<FName> Result;
	{
		FReadScopeLock Lock(Musee::ProbeTable().Lock);
		Musee::ProbeTable().Probes.GetKeys(Result);
	}
	Result.Sort(FNameLexicalLess());
	return Result;
}

bool IMuseeProbe::IsValidName(FStringView Name, FString* OutWhy)
{
	const int32 Separator = UE::String::FindFirst(Name, TEXTVIEW("::"));
	const FStringView System = Separator == INDEX_NONE ? FStringView() : Name.Left(Separator);
	const FStringView Measure = Separator == INDEX_NONE ? FStringView() : Name.RightChop(Separator + 2);
	bool bValid = !System.IsEmpty() && FChar::IsUpper(System[0]) && Musee::IsLettersAndDigits(System) && Musee::IsLettersAndDigits(Measure);
	if (bValid)
	{
		bValid = false;
		for (const TCHAR* Verb : Musee::ProbeVerbs)
		{
			const int32 VerbLength = FCString::Strlen(Verb);
			bValid |= Measure.StartsWith(Verb, ESearchCase::CaseSensitive) && Measure.Len() > VerbLength && FChar::IsUpper(Measure[VerbLength]);
		}
	}
	if (!bValid && OutWhy)
	{
		*OutWhy = TEXT("a probe is named <System>::<Verb><Thing>, the verb Measure, Find or List, in PascalCase");
	}
	return bValid;
}
