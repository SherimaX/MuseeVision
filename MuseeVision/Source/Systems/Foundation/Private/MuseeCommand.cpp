#include "MuseeCommand.h"

#include "HAL/IConsoleManager.h"
#include "Misc/OutputDeviceRedirector.h"
#include "MuseeFoundationLog.h"
#include "MuseeKey.h"

namespace Musee
{
	namespace
	{
		/** Every command's first segment. */
		const TCHAR* const CommandRoot = TEXT("musee");

		/** musee, the system and the verb with its object. */
		constexpr int32 CommandSegments = 3;

		struct FRegisteredCommand
		{
			FMuseeCommand Command;
			IConsoleCommand* Console = nullptr;
		};

		TMap<FName, FRegisteredCommand>& Commands()
		{
			static TMap<FName, FRegisteredCommand> Registered;
			return Registered;
		}

		bool Refuse(FString* OutWhy, const TCHAR* Why)
		{
			if (OutWhy)
			{
				*OutWhy = Why;
			}
			return false;
		}

		bool IsLetters(const FString& Text)
		{
			for (const TCHAR Character : Text)
			{
				if (Character < TEXT('a') || Character > TEXT('z'))
				{
					return false;
				}
			}
			return !Text.IsEmpty();
		}
	}
}

bool FMuseeCommand::IsValidName(FStringView Name, FString* OutWhy)
{
	using namespace Musee;
	FString KeyWhy;
	if (!FMuseeKey::IsValid(Name, &KeyWhy))
	{
		if (OutWhy)
		{
			*OutWhy = FString::Printf(TEXT("a command's name follows the key rule, and %s"), *KeyWhy);
		}
		return false;
	}
	TArray<FString> Segments;
	FString(Name).ParseIntoArray(Segments, TEXT("."));
	if (Segments.Num() != CommandSegments || Segments[0] != CommandRoot)
	{
		return Refuse(OutWhy, TEXT("a command is named musee.<system>.<verb>[-<object>]"));
	}
	if (!IsLetters(Segments[1]))
	{
		return Refuse(OutWhy, TEXT("a command's second segment is its system's name, in letters"));
	}
	if (Segments[2][0] < TEXT('a') || Segments[2][0] > TEXT('z'))
	{
		return Refuse(OutWhy, TEXT("a command's third segment starts with its verb"));
	}
	return true;
}

bool FMuseeCommand::Register(FMuseeCommand Command)
{
	check(IsInGameThread());
	const FName Name = Command.Name;
	FString Why;
	if (!IsValidName(Name.ToString(), &Why))
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("'%s' can't be a command: %s."), *Name.ToString(), *Why);
		return false;
	}
	if (!Command.Action)
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("Command '%s' has no action."), *Name.ToString());
		return false;
	}
	if (Musee::Commands().Contains(Name))
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("Command '%s' is already registered; a command has one owner."), *Name.ToString());
		return false;
	}
	IConsoleCommand* Console = IConsoleManager::Get().RegisterConsoleCommand(*Name.ToString(), *Command.Help,
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda(
			[Name](const TArray<FString>& Args, UWorld* World, FOutputDevice& Output) { Run(Name, Args, World, Output); }),
		ECVF_Default);
	if (!Console)
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("The console already has an object named '%s'."), *Name.ToString());
		return false;
	}
	Musee::Commands().Add(Name, Musee::FRegisteredCommand{MoveTemp(Command), Console});
	return true;
}

void FMuseeCommand::Unregister(FName Name)
{
	check(IsInGameThread());
	Musee::FRegisteredCommand Registered;
	if (Musee::Commands().RemoveAndCopyValue(Name, Registered))
	{
		IConsoleManager::Get().UnregisterConsoleObject(Registered.Console, false);
	}
}

bool FMuseeCommand::Run(FName Name, const TArray<FString>& Args, UWorld* World, FOutputDevice& Output)
{
	check(IsInGameThread());
	const Musee::FRegisteredCommand* Registered = Musee::Commands().Find(Name);
	if (!Registered)
	{
		Output.Logf(ELogVerbosity::Error, TEXT("No command '%s' is registered."), *Name.ToString());
		return false;
	}
	// A copy, since an action may unregister its own command.
	const FAction Action = Registered->Command.Action;
	return Action(Args, World, Output);
}

bool FMuseeCommand::Run(FName Name, const TArray<FString>& Args)
{
	return Run(Name, Args, nullptr, *GLog);
}

bool FMuseeCommand::Exists(FName Name)
{
	return Musee::Commands().Contains(Name);
}

TArray<FName> FMuseeCommand::Names()
{
	TArray<FName> Result;
	Musee::Commands().GetKeys(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}
