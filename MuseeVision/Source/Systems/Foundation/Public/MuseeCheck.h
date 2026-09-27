#pragma once

#include "CoreMinimal.h"
#include "HAL/PreprocessorHelpers.h"
#include "MuseeCheckScope.h"
#include "MuseeConfiguration.h"
#include "MuseeFinding.h"
#include "Templates/ValueOrError.h"

#include <type_traits>

/**
 * A check: the pure maths that proves one physical truth of the museum holds, in the system that owns the truth.
 * `Form.Sealed`, `Light.MeetsTarget`, `Nature.WaterContained`. A check is a function of typed inputs (reflected
 * structs such as a stage's assembly, a mechanism configuration, a moment) that returns what it found wrong; it never
 * fetches anything, so no system reaches up. Studio's runner assembles the inputs its scope asks for and runs it.
 *
 * A check is named `<System>.<Truth>`, phrased positively, each part in PascalCase; the first part is its owner. A
 * module declares its checks where they are written:
 *
 *     static TArray<FMuseeFinding> CheckSealed(const FFormAssembly& Assembly, const FMuseeConfiguration& Configuration);
 *     MUSEE_CHECK("Form.Sealed", Configuration, CheckSealed);
 *
 * and each is registered while its module is loaded. A check in Configuration scope must take the configuration.
 */
class MUSEEFOUNDATION_API FMuseeCheck
{
public:
	/** The inputs a runner hands to one run of a check: structs found by their reflected type, borrowed for the run. */
	class MUSEEFOUNDATION_API FInputs
	{
	public:
		/** Lends an input; it must outlive the run. A second input of the same type replaces the first. */
		template <typename T>
		FInputs& Add(const T& Input)
		{
			Items.Add(T::StaticStruct(), &Input);
			return *this;
		}

		/** The input of a type, or null. */
		[[nodiscard]] const void* Find(const UScriptStruct* Type) const;

		/** The input of a type, or null. */
		template <typename T>
		[[nodiscard]] const T* Find() const
		{
			return static_cast<const T*>(Find(T::StaticStruct()));
		}

	private:
		TMap<const UScriptStruct*, const void*> Items;
	};

	/** Keeps a check registered while the module that declares it is loaded. MUSEE_CHECK makes one. */
	class MUSEEFOUNDATION_API FRegistrar
	{
	public:
		explicit FRegistrar(FMuseeCheck Check);
		~FRegistrar();

		FRegistrar(const FRegistrar&) = delete;
		FRegistrar& operator=(const FRegistrar&) = delete;

	private:
		FName Name;
		uint64 Serial = 0;
	};

	/** Makes a check from a pure function of typed inputs. Source is the file that declares it. */
	template <EMuseeCheckScope TheScope, typename... TInputs>
	[[nodiscard]] static FMuseeCheck Make(const TCHAR* Name, TArray<FMuseeFinding> (*Function)(const TInputs&...), const ANSICHAR* Source)
	{
		static_assert(TheScope != EMuseeCheckScope::Configuration || (std::is_same_v<TInputs, FMuseeConfiguration> || ...),
			"A check in Configuration scope takes the FMuseeConfiguration it runs under.");
		FInputTypes InputTypes = []() { return TArray<const UScriptStruct*>{ TInputs::StaticStruct()... }; };
		FBody Body = [Function](const FInputs& Inputs) { return Function(*Inputs.Find<TInputs>()...); };
		return FMuseeCheck(FName(Name), TheScope, FString(Source), InputTypes, MoveTemp(Body));
	}

	/**
	 * Registers a check. Refused, with an error, for a name that breaks the rule or one another file already
	 * registered; a registration from the same file replaces the earlier one (Live Coding re-registers a patched file).
	 */
	static bool Register(FMuseeCheck Check);

	/** Unregisters a check. */
	static void Unregister(FName Name);

	/** A registered check, or null. It stays valid until it is unregistered or registered again. */
	[[nodiscard]] static const FMuseeCheck* Find(FName Name);

	/** Every registered check, in name order. */
	[[nodiscard]] static TArray<const FMuseeCheck*> All();

	/** Whether a name follows `<System>.<Truth>`; if not, OutWhy says how it breaks the rule. */
	[[nodiscard]] static bool IsValidName(FStringView Name, FString* OutWhy = nullptr);

	[[nodiscard]] FName Name() const { return CheckName; }
	[[nodiscard]] EMuseeCheckScope Scope() const { return CheckScope; }

	/** The system (or stage) that owns the truth: the name's first part. */
	[[nodiscard]] FName Owner() const;

	/** The file that declares the check. */
	[[nodiscard]] const FString& Source() const { return SourceFile; }

	/** The reflected types of the inputs the check takes, in order. */
	[[nodiscard]] TArray<const UScriptStruct*> InputTypes() const;

	/**
	 * Runs the check over the inputs and stamps each finding with the check and its owner. Fails, naming the missing
	 * type, if an input the check takes isn't among the inputs.
	 */
	[[nodiscard]] TValueOrError<TArray<FMuseeFinding>, FString> Evaluate(const FInputs& Inputs) const;

private:
	using FInputTypes = TArray<const UScriptStruct*> (*)();
	using FBody = TFunction<TArray<FMuseeFinding>(const FInputs&)>;

	FMuseeCheck(FName InName, EMuseeCheckScope InScope, const FString& InSource, FInputTypes InInputTypes, FBody InBody);

	FName CheckName;
	EMuseeCheckScope CheckScope;
	FString SourceFile;
	FInputTypes InputTypesOf;
	FBody Body;
	uint64 Serial = 0;
};

/** Declares a check where its function is written: `MUSEE_CHECK("Form.Sealed", Configuration, CheckSealed);`. */
#define MUSEE_CHECK(Name, Scope, Function) \
	static const FMuseeCheck::FRegistrar UE_JOIN(GMuseeCheckRegistrar, __COUNTER__)( \
		FMuseeCheck::Make<EMuseeCheckScope::Scope>(TEXT(Name), &Function, __FILE__))
