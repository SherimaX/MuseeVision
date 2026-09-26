#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/OutputDeviceRedirector.h"
#include "MuseeCommand.h"
#include "MuseeFoundationTests.h"
#include "MuseeSetting.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Tests
{
	/** The store's section, as the settings store names it. */
	const TCHAR* const SettingsSection = TEXT("MuseeSettings");

	/** Removes a test setting's stored copy, so the user's config holds nothing of the test. */
	void ForgetStoredSetting(const FMuseeKey& Key)
	{
		GConfig->RemoveKey(SettingsSection, *Key.ToString(), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationSettingsTest, "Musee.Foundation.Settings", Musee::Tests::Flags)

bool FMuseeFoundationSettingsTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;
	const FMuseeKey Hour(TEXTVIEW("foundation.sample-hour"));
	const FMuseeKey Profile(TEXTVIEW("foundation.sample-profile"));
	const FName HourCommand(TEXT("musee.foundation.set-sample-hour"));
	ForgetStoredSetting(Hour);
	ForgetStoredSetting(Profile);

	FMuseeSetting* Setting = FMuseeSetting::RegisterNumber(Hour, 12.0, 0.0, 24.0, TEXT("The hour of the day, in hours."));
	if (!TestNotNull(TEXT("A number setting registers"), Setting))
	{
		return false;
	}
	TestEqual(TEXT("It starts at its default"), Setting->Number(), 12.0);
	TestTrue(TEXT("Its command is named for its key"), FMuseeSetting::CommandName(Hour) == HourCommand);
	TestTrue(TEXT("Registering it made its command"), FMuseeCommand::Exists(HourCommand));
	TestNotNull(TEXT("And its console command"), IConsoleManager::Get().FindConsoleObject(TEXT("musee.foundation.set-sample-hour")));
	int32 Changes = 0;
	Setting->OnChanged().AddLambda([&Changes](const FMuseeSetting&) { ++Changes; });

	TestTrue(TEXT("The command sets the value"), FMuseeCommand::Run(HourCommand, {TEXT("15.5")}));
	TestEqual(TEXT("To the value given"), Setting->Number(), 15.5);
	FString Stored;
	GConfig->GetString(SettingsSection, *Hour.ToString(), Stored, GGameUserSettingsIni);
	TestEqual(TEXT("The value is kept in the user's config"), Stored, FString(TEXT("15.5")));
	AddExpectedMessagePlain(TEXT("takes one value, a number from 0 to 24"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestFalse(TEXT("A value out of range is refused"), FMuseeCommand::Run(HourCommand, {TEXT("30")}));
	TestFalse(TEXT("Text that isn't a number is refused"), Setting->SetFromText(TEXT("noon")));
	TestTrue(TEXT("Setting the value it has changes nothing"), Setting->SetNumber(15.5));
	TestEqual(TEXT("Only changes are announced"), Changes, 1);
	IConsoleManager::Get().ProcessUserConsoleInput(TEXT("musee.foundation.set-sample-hour 9"), *GLog, nullptr);
	TestEqual(TEXT("The console sets it through the same command"), Setting->Number(), 9.0);
	TestFalse(TEXT("A number setting takes no choice"), Setting->SetChoice(TEXT("dusk")));

	AddExpectedErrorPlain(TEXT("is already registered; a setting has one owner"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("A setting has one owner"), FMuseeSetting::RegisterNumber(Hour, 12.0, 0.0, 24.0, TEXT("Again.")));
	FMuseeSetting::Unregister(Hour);
	TestNull(TEXT("An unregistered setting is gone"), FMuseeSetting::Find(Hour));
	TestFalse(TEXT("And its command with it"), FMuseeCommand::Exists(HourCommand));
	Setting = FMuseeSetting::RegisterNumber(Hour, 12.0, 0.0, 24.0, TEXT("The hour of the day, in hours."));
	TestTrue(TEXT("A setting registered again finds its stored value"), Setting && Setting->Number() == 9.0);
	FMuseeSetting::Unregister(Hour);

	const TArray<FName> Profiles = {TEXT("desktop"), TEXT("headset")};
	FMuseeSetting* Choice = FMuseeSetting::RegisterChoice(Profile, TEXT("desktop"), Profiles, TEXT("Which display the museum is seen on."));
	if (TestNotNull(TEXT("A choice setting registers"), Choice))
	{
		TestTrue(TEXT("It takes one of its choices"), Choice->SetFromText(TEXT("headset")) && Choice->Choice() == TEXT("headset"));
		TestFalse(TEXT("And refuses anything else"), Choice->SetFromText(TEXT("cinema")));
		TestFalse(TEXT("A choice setting takes no number"), Choice->SetNumber(1.0));
		TestEqual(TEXT("Its value as text"), Choice->ToText(), FString(TEXT("headset")));
	}
	FMuseeSetting::Unregister(Profile);

	AddExpectedErrorPlain(TEXT("a setting's key is <system>.<setting>"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("A setting's key has two segments"), FMuseeSetting::RegisterNumber(FMuseeKey(TEXTVIEW("foundation.sample.hour")), 1.0, 0.0, 2.0, TEXT("Too deep.")));
	AddExpectedErrorPlain(TEXT("lies outside its range"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("A default lies in its range"), FMuseeSetting::RegisterNumber(Hour, 30.0, 0.0, 24.0, TEXT("Out of range.")));
	AddExpectedErrorPlain(TEXT("isn't one of its choices"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("A default is one of the choices"), FMuseeSetting::RegisterChoice(Profile, TEXT("cinema"), Profiles, TEXT("Wrong default.")));

	ForgetStoredSetting(Hour);
	ForgetStoredSetting(Profile);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationCommandsTest, "Musee.Foundation.Commands", Musee::Tests::Flags)

bool FMuseeFoundationCommandsTest::RunTest(const FString& Parameters)
{
	const FName Name(TEXT("musee.foundation.echo-arguments"));
	TArray<FString> Heard;
	FMuseeCommand Command;
	Command.Name = Name;
	Command.Help = TEXT("Repeats its arguments.");
	Command.Action = [&Heard](const TArray<FString>& Args, UWorld*, FOutputDevice&)
	{
		Heard = Args;
		return !Args.IsEmpty();
	};
	TestTrue(TEXT("A command registers"), FMuseeCommand::Register(Command));
	TestTrue(TEXT("It is listed"), FMuseeCommand::Names().Contains(Name));
	TestTrue(TEXT("It runs with its arguments"), FMuseeCommand::Run(Name, {TEXT("a"), TEXT("b")}) && Heard == TArray<FString>{TEXT("a"), TEXT("b")});
	TestFalse(TEXT("It says when it refused"), FMuseeCommand::Run(Name, {}));
	IConsoleManager::Get().ProcessUserConsoleInput(TEXT("musee.foundation.echo-arguments x y"), *GLog, nullptr);
	TestTrue(TEXT("The console runs the same command"), Heard == TArray<FString>{TEXT("x"), TEXT("y")});

	AddExpectedErrorPlain(TEXT("is already registered; a command has one owner"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("A command has one owner"), FMuseeCommand::Register(Command));
	FMuseeCommand::Unregister(Name);
	TestFalse(TEXT("An unregistered command is gone"), FMuseeCommand::Exists(Name));
	TestNull(TEXT("From the console too"), IConsoleManager::Get().FindConsoleObject(TEXT("musee.foundation.echo-arguments")));
	AddExpectedErrorPlain(TEXT("No command 'musee.foundation.echo-arguments' is registered"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Running it fails"), FMuseeCommand::Run(Name, {TEXT("a")}));

	TestTrue(TEXT("musee.time.advance-hour is a command's name"), FMuseeCommand::IsValidName(TEXT("musee.time.advance-hour")));
	TestTrue(TEXT("musee.studio.make is a command's name"), FMuseeCommand::IsValidName(TEXT("musee.studio.make")));
	for (const TCHAR* Wrong : {TEXT("musee.Time.advance-hour"), TEXT("time.advance-hour"), TEXT("musee.time"), TEXT("musee.time.advance.hour"),
		TEXT("musee.ti-me.advance"), TEXT("musee.time.3d-view"), TEXT("studio.musee.make")})
	{
		TestFalse(FString::Printf(TEXT("%s is not a command's name"), Wrong), FMuseeCommand::IsValidName(Wrong));
	}
	AddExpectedErrorPlain(TEXT("can't be a command"), EAutomationExpectedErrorFlags::Contains, 1);
	Command.Name = TEXT("foundation.echo");
	TestFalse(TEXT("A name that breaks the rule is refused"), FMuseeCommand::Register(Command));
	return true;
}

#endif
