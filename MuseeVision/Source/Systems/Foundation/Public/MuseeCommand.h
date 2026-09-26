#pragma once

#include "CoreMinimal.h"

/**
 * A command the museum answers to: `musee.<system>.<verb>[-<object>]`, such as `musee.time.advance-hour` or
 * `musee.studio.make`. A command is registered once, by its owner, and every way of asking for it calls the same
 * action through Run: the console (each command is also a console command of the same name), the keyboard, the
 * headset's buttons and Studio.
 *
 * Commands are registered, unregistered and run on the game thread.
 */
struct MUSEEFOUNDATION_API FMuseeCommand
{
	/**
	 * What a command does: act on the arguments (in the world, when it needs one; null otherwise), report to the
	 * output, and say whether it ran. A command refuses arguments it can't use by reporting why and returning false.
	 */
	using FAction = TFunction<bool(const TArray<FString>& Args, UWorld* World, FOutputDevice& Output)>;

	/** The command's name, `musee.<system>.<verb>[-<object>]`. */
	FName Name;

	/** One line for the console: what the command does and the arguments it takes. */
	FString Help;

	/** What it does. */
	FAction Action;

	/**
	 * Registers a command and its console command. Refused, with an error, for a name that breaks the rule, a name
	 * already registered, or a command without an action.
	 */
	static bool Register(FMuseeCommand Command);

	/** Unregisters a command and its console command. */
	static void Unregister(FName Name);

	/** Runs a command. False, with an error on the output, for a command nobody registered or one that refused. */
	static bool Run(FName Name, const TArray<FString>& Args, UWorld* World, FOutputDevice& Output);

	/** Runs a command that needs no world, reporting to the log. */
	static bool Run(FName Name, const TArray<FString>& Args);

	/** Whether a command is registered. */
	[[nodiscard]] static bool Exists(FName Name);

	/** Every registered command's name, in name order. */
	[[nodiscard]] static TArray<FName> Names();

	/** Whether a name follows the rule; if not, OutWhy says how it breaks it. */
	[[nodiscard]] static bool IsValidName(FStringView Name, FString* OutWhy = nullptr);
};
