#include "MuseeSetting.h"

#include "Misc/ConfigCacheIni.h"
#include "MuseeCommand.h"
#include "MuseeFoundationLog.h"

namespace Musee
{
	namespace
	{
		/** The store's section in the user's GameUserSettings. */
		const TCHAR* const StoreSection = TEXT("MuseeSettings");

		/** The owning system and the setting's name. */
		constexpr int32 SettingSegments = 2;

		TMap<FMuseeKey, TUniquePtr<FMuseeSetting>>& Settings()
		{
			static TMap<FMuseeKey, TUniquePtr<FMuseeSetting>> Registered;
			return Registered;
		}

		/** A number as the store writes it: fifteen significant digits, which any decimal a person types survives. */
		FString NumberText(double Value)
		{
			return FString::Printf(TEXT("%.15g"), Value);
		}

		bool IsChoiceName(FName Choice)
		{
			const FString Text = Choice.ToString();
			return FMuseeKey::IsValid(Text) && !Text.Contains(TEXT("."));
		}
	}
}

FMuseeSetting::FMuseeSetting(const FMuseeKey& InKey, EKind InKind, const FString& InHelp)
	: SettingKey(InKey)
	, SettingKind(InKind)
	, HelpText(InHelp)
{
}

FMuseeSetting* FMuseeSetting::RegisterNumber(const FMuseeKey& Key, double Default, double Minimum, double Maximum, const FString& Help)
{
	if (!(Minimum <= Default && Default <= Maximum))
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("Setting '%s': the default %s lies outside its range, %s to %s."),
			*Key.ToString(), *Musee::NumberText(Default), *Musee::NumberText(Minimum), *Musee::NumberText(Maximum));
		return nullptr;
	}
	TUniquePtr<FMuseeSetting> Setting(new FMuseeSetting(Key, EKind::Number, Help));
	Setting->RangeMinimum = Minimum;
	Setting->RangeMaximum = Maximum;
	Setting->NumberValue = Default;
	return Add(MoveTemp(Setting));
}

FMuseeSetting* FMuseeSetting::RegisterChoice(const FMuseeKey& Key, FName Default, const TArray<FName>& Choices, const FString& Help)
{
	for (const FName Choice : Choices)
	{
		if (!Musee::IsChoiceName(Choice))
		{
			UE_LOG(LogMuseeFoundation, Error, TEXT("Setting '%s': the choice '%s' isn't a single key segment."), *Key.ToString(), *Choice.ToString());
			return nullptr;
		}
	}
	if (!Choices.Contains(Default))
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("Setting '%s': the default '%s' isn't one of its choices."), *Key.ToString(), *Default.ToString());
		return nullptr;
	}
	TUniquePtr<FMuseeSetting> Setting(new FMuseeSetting(Key, EKind::Choice, Help));
	Setting->ChoiceNames = Choices;
	Setting->ChoiceValue = Default;
	return Add(MoveTemp(Setting));
}

FMuseeSetting* FMuseeSetting::Add(TUniquePtr<FMuseeSetting> Setting)
{
	check(IsInGameThread());
	const FMuseeKey Key = Setting->SettingKey;
	if (Key.Segments().Num() != Musee::SettingSegments)
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("'%s' can't be a setting: a setting's key is <system>.<setting>."), *Key.ToString());
		return nullptr;
	}
	if (Musee::Settings().Contains(Key))
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("Setting '%s' is already registered; a setting has one owner."), *Key.ToString());
		return nullptr;
	}
	FString Stored;
	if (GConfig->GetString(Musee::StoreSection, *Key.ToString(), Stored, GGameUserSettingsIni) && !Setting->Load(Stored))
	{
		UE_LOG(LogMuseeFoundation, Warning, TEXT("The stored value '%s' of setting '%s' isn't %s; the default stands."),
			*Stored, *Key.ToString(), *Setting->DescribeValues());
	}

	FMuseeCommand Command;
	Command.Name = CommandName(Key);
	Command.Help = FString::Printf(TEXT("Sets %s, %s: %s. With no value, reports it."), *Key.ToString(), *Setting->DescribeValues(), *Setting->HelpText);
	Command.Action = [Key](const TArray<FString>& Args, UWorld* World, FOutputDevice& Output)
	{
		FMuseeSetting* Target = Find(Key);
		if (Target && Args.Num() == 1 && Target->SetFromText(Args[0]))
		{
			Output.Logf(TEXT("%s = %s"), *Key.ToString(), *Target->ToText());
			return true;
		}
		if (Target && Args.IsEmpty())
		{
			Output.Logf(TEXT("%s = %s (%s)"), *Key.ToString(), *Target->ToText(), *Target->DescribeValues());
			return true;
		}
		Output.Logf(ELogVerbosity::Warning, TEXT("%s takes one value, %s."), *CommandName(Key).ToString(),
			Target ? *Target->DescribeValues() : TEXT("but the setting is gone"));
		return false;
	};
	if (!FMuseeCommand::Register(MoveTemp(Command)))
	{
		return nullptr;
	}
	return Musee::Settings().Add(Key, MoveTemp(Setting)).Get();
}

void FMuseeSetting::Unregister(const FMuseeKey& Key)
{
	check(IsInGameThread());
	if (Musee::Settings().Remove(Key) > 0)
	{
		FMuseeCommand::Unregister(CommandName(Key));
	}
}

FMuseeSetting* FMuseeSetting::Find(const FMuseeKey& Key)
{
	const TUniquePtr<FMuseeSetting>* Setting = Musee::Settings().Find(Key);
	return Setting ? Setting->Get() : nullptr;
}

TArray<FMuseeKey> FMuseeSetting::Keys()
{
	TArray<FMuseeKey> Result;
	Musee::Settings().GetKeys(Result);
	Result.Sort([](const FMuseeKey& A, const FMuseeKey& B) { return A.ToString() < B.ToString(); });
	return Result;
}

FName FMuseeSetting::CommandName(const FMuseeKey& Key)
{
	const TArray<FString> Segments = Key.Segments();
	return Segments.Num() == Musee::SettingSegments ? FName(*FString::Printf(TEXT("musee.%s.set-%s"), *Segments[0], *Segments[1])) : NAME_None;
}

double FMuseeSetting::Number() const
{
	ensureMsgf(SettingKind == EKind::Number, TEXT("Setting '%s' is a choice, not a number."), *SettingKey.ToString());
	return NumberValue;
}

FName FMuseeSetting::Choice() const
{
	ensureMsgf(SettingKind == EKind::Choice, TEXT("Setting '%s' is a number, not a choice."), *SettingKey.ToString());
	return ChoiceValue;
}

bool FMuseeSetting::SetNumber(double Value)
{
	if (SettingKind != EKind::Number || !(RangeMinimum <= Value && Value <= RangeMaximum))
	{
		return false;
	}
	if (Value != NumberValue)
	{
		NumberValue = Value;
		Store();
		Changed.Broadcast(*this);
	}
	return true;
}

bool FMuseeSetting::SetChoice(FName Value)
{
	const int32 Index = ChoiceNames.IndexOfByKey(Value);
	if (SettingKind != EKind::Choice || Index == INDEX_NONE)
	{
		return false;
	}
	if (ChoiceNames[Index] != ChoiceValue)
	{
		ChoiceValue = ChoiceNames[Index];
		Store();
		Changed.Broadcast(*this);
	}
	return true;
}

bool FMuseeSetting::SetFromText(const FString& Text)
{
	const FString Value = Text.TrimStartAndEnd();
	double Number = 0.0;
	if (SettingKind == EKind::Number)
	{
		return LexTryParseString(Number, *Value) && SetNumber(Number);
	}
	return SetChoice(FName(*Value));
}

FString FMuseeSetting::ToText() const
{
	return SettingKind == EKind::Number ? Musee::NumberText(NumberValue) : ChoiceValue.ToString();
}

bool FMuseeSetting::Load(const FString& Text)
{
	const FString Value = Text.TrimStartAndEnd();
	if (SettingKind == EKind::Number)
	{
		double Number = 0.0;
		if (!LexTryParseString(Number, *Value) || !(RangeMinimum <= Number && Number <= RangeMaximum))
		{
			return false;
		}
		NumberValue = Number;
		return true;
	}
	const int32 Index = ChoiceNames.IndexOfByKey(FName(*Value));
	if (Index == INDEX_NONE)
	{
		return false;
	}
	ChoiceValue = ChoiceNames[Index];
	return true;
}

FString FMuseeSetting::DescribeValues() const
{
	if (SettingKind == EKind::Number)
	{
		return FString::Printf(TEXT("a number from %s to %s"), *Musee::NumberText(RangeMinimum), *Musee::NumberText(RangeMaximum));
	}
	TArray<FString> Names;
	for (const FName Choice : ChoiceNames)
	{
		Names.Add(Choice.ToString());
	}
	return FString::Printf(TEXT("one of %s"), *FString::Join(Names, TEXT(", ")));
}

void FMuseeSetting::Store() const
{
	GConfig->SetString(Musee::StoreSection, *SettingKey.ToString(), *ToText(), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}
