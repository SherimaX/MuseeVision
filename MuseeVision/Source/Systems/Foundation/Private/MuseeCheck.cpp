#include "MuseeCheck.h"

#include "Misc/ScopeRWLock.h"
#include "MuseeFoundationLog.h"

namespace Musee
{
	namespace
	{
		/** The system and the truth. */
		constexpr int32 CheckNameParts = 2;

		/**
		 * Checks are registered as their modules load and read by the runner, which may run them in parallel once the
		 * modules are loaded; the lock keeps the table whole meanwhile.
		 */
		struct FCheckTable
		{
			FRWLock Lock;
			TMap<FName, TUniquePtr<FMuseeCheck>> Checks;
			uint64 NextSerial = 1;
		};

		FCheckTable& CheckTable()
		{
			static FCheckTable Registered;
			return Registered;
		}

		bool IsPascalCase(const FString& Word)
		{
			if (Word.IsEmpty() || !FChar::IsUpper(Word[0]))
			{
				return false;
			}
			for (const TCHAR Character : Word)
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

const void* FMuseeCheck::FInputs::Find(const UScriptStruct* Type) const
{
	const void* const* Input = Items.Find(Type);
	return Input ? *Input : nullptr;
}

FMuseeCheck::FRegistrar::FRegistrar(FMuseeCheck Check)
	: Name(Check.CheckName)
{
	if (Register(MoveTemp(Check)))
	{
		FReadScopeLock Lock(Musee::CheckTable().Lock);
		Serial = Musee::CheckTable().Checks[Name]->Serial;
	}
}

FMuseeCheck::FRegistrar::~FRegistrar()
{
	FWriteScopeLock Lock(Musee::CheckTable().Lock);
	const TUniquePtr<FMuseeCheck>* Registered = Musee::CheckTable().Checks.Find(Name);
	if (Serial != 0 && Registered && (*Registered)->Serial == Serial)
	{
		Musee::CheckTable().Checks.Remove(Name);
	}
}

FMuseeCheck::FMuseeCheck(FName InName, EMuseeCheckScope InScope, const FString& InSource, FInputTypes InInputTypes, FBody InBody)
	: CheckName(InName)
	, CheckScope(InScope)
	, SourceFile(InSource)
	, InputTypesOf(InInputTypes)
	, Body(MoveTemp(InBody))
{
}

bool FMuseeCheck::Register(FMuseeCheck Check)
{
	FString Why;
	if (!IsValidName(Check.CheckName.ToString(), &Why))
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("'%s' can't be a check: %s."), *Check.CheckName.ToString(), *Why);
		return false;
	}
	FWriteScopeLock Lock(Musee::CheckTable().Lock);
	const TUniquePtr<FMuseeCheck>* Existing = Musee::CheckTable().Checks.Find(Check.CheckName);
	if (Existing && (*Existing)->SourceFile != Check.SourceFile)
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("Check '%s' is already registered by %s; a truth has one check."),
			*Check.CheckName.ToString(), *(*Existing)->SourceFile);
		return false;
	}
	Check.Serial = Musee::CheckTable().NextSerial++;
	const FName Name = Check.CheckName;
	Musee::CheckTable().Checks.Add(Name, MakeUnique<FMuseeCheck>(MoveTemp(Check)));
	return true;
}

void FMuseeCheck::Unregister(FName Name)
{
	FWriteScopeLock Lock(Musee::CheckTable().Lock);
	Musee::CheckTable().Checks.Remove(Name);
}

const FMuseeCheck* FMuseeCheck::Find(FName Name)
{
	FReadScopeLock Lock(Musee::CheckTable().Lock);
	const TUniquePtr<FMuseeCheck>* Check = Musee::CheckTable().Checks.Find(Name);
	return Check ? Check->Get() : nullptr;
}

TArray<const FMuseeCheck*> FMuseeCheck::All()
{
	TArray<const FMuseeCheck*> Result;
	{
		FReadScopeLock Lock(Musee::CheckTable().Lock);
		for (const TPair<FName, TUniquePtr<FMuseeCheck>>& Check : Musee::CheckTable().Checks)
		{
			Result.Add(Check.Value.Get());
		}
	}
	Result.Sort([](const FMuseeCheck& A, const FMuseeCheck& B) { return A.CheckName.LexicalLess(B.CheckName); });
	return Result;
}

bool FMuseeCheck::IsValidName(FStringView Name, FString* OutWhy)
{
	TArray<FString> Parts;
	FString(Name).ParseIntoArray(Parts, TEXT("."), false);
	if (Parts.Num() != Musee::CheckNameParts || !Musee::IsPascalCase(Parts[0]) || !Musee::IsPascalCase(Parts[1]))
	{
		if (OutWhy)
		{
			*OutWhy = TEXT("a check is named <System>.<Truth>, each part a PascalCase word of letters and digits");
		}
		return false;
	}
	return true;
}

FName FMuseeCheck::Owner() const
{
	FString System;
	CheckName.ToString().Split(TEXT("."), &System, nullptr);
	return FName(*System);
}

TArray<const UScriptStruct*> FMuseeCheck::InputTypes() const
{
	return InputTypesOf();
}

TValueOrError<TArray<FMuseeFinding>, FString> FMuseeCheck::Evaluate(const FInputs& Inputs) const
{
	for (const UScriptStruct* Type : InputTypes())
	{
		if (!Inputs.Find(Type))
		{
			return MakeError(FString::Printf(TEXT("Check %s takes an input of type %s%s, which the runner didn't supply."),
				*CheckName.ToString(), Type->GetPrefixCPP(), *Type->GetName()));
		}
	}
	TArray<FMuseeFinding> Findings = Body(Inputs);
	const FName System = Owner();
	for (FMuseeFinding& Finding : Findings)
	{
		Finding.Check = CheckName;
		Finding.Owner = System;
	}
	return MakeValue(MoveTemp(Findings));
}
