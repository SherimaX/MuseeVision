#include "MuseeDefect.h"

#include "MuseeCheck.h"

namespace Musee
{
	namespace
	{
		const TCHAR* const EntryHeading = TEXT("## ");
		const TCHAR* const NumberPrefix = TEXT("D-");
		constexpr int32 NumberDigits = 4;
		/** The middle dot between a number and its title, as the Journal's decisions are headed. */
		const TCHAR* const TitleSeparator = TEXT("\u00B7");
		const TCHAR* const FieldOpening = TEXT("- **");
		const TCHAR* const FieldClosing = TEXT(":** ");
		const TCHAR* const OpenStatus = TEXT("open");
		const TCHAR* const ClosedStatus = TEXT("closed");

		enum class EField : uint8
		{
			Symptom,
			Check,
			Drawer,
			Cause,
			Fix,
			Status,
		};

		const TCHAR* const FieldLabels[] = {TEXT("Symptom"), TEXT("Check"), TEXT("Drawer"), TEXT("Cause"), TEXT("Fix"), TEXT("Status")};

		/** One entry as it is parsed: the defect and which fields it has given. */
		struct FEntry
		{
			FMuseeDefect Defect;
			TSet<EField> Given;
		};

		TOptional<EField> FieldOf(const FString& Label)
		{
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(FieldLabels); ++Index)
			{
				if (Label.Equals(FieldLabels[Index], ESearchCase::CaseSensitive))
				{
					return static_cast<EField>(Index);
				}
			}
			return {};
		}

		/** Takes a field's value into the entry; returns what is wrong with it, or nothing. */
		FString Take(FEntry& Entry, EField Field, const FString& Value)
		{
			bool bGiven = false;
			Entry.Given.Add(Field, &bGiven);
			if (bGiven)
			{
				return FString::Printf(TEXT("gives %s twice"), FieldLabels[static_cast<int32>(Field)]);
			}
			FMuseeDefect& Defect = Entry.Defect;
			switch (Field)
			{
			case EField::Symptom: Defect.Symptom = Value; break;
			case EField::Check: Defect.Check = FName(*Value); break;
			case EField::Drawer: Defect.Drawer = FName(*Value); break;
			case EField::Cause: Defect.Cause = Value; break;
			case EField::Fix: Defect.Fix = Value; break;
			case EField::Status:
				if (!Value.Equals(OpenStatus, ESearchCase::CaseSensitive) && !Value.Equals(ClosedStatus, ESearchCase::CaseSensitive))
				{
					return FString::Printf(TEXT("has status '%s'; a defect is %s or %s"), *Value, OpenStatus, ClosedStatus);
				}
				Defect.bClosed = Value.Equals(ClosedStatus, ESearchCase::CaseSensitive);
				break;
			}
			return FString();
		}

		/** What is missing or wrong in a finished entry, or nothing. */
		FString Complete(const FEntry& Entry)
		{
			for (const EField Field : {EField::Symptom, EField::Check, EField::Drawer, EField::Cause, EField::Status})
			{
				if (!Entry.Given.Contains(Field))
				{
					return FString::Printf(TEXT("has no %s"), FieldLabels[static_cast<int32>(Field)]);
				}
			}
			FString Why;
			if (!FMuseeCheck::IsValidName(Entry.Defect.Check.ToString(), &Why))
			{
				return FString::Printf(TEXT("names '%s' as its check, but %s"), *Entry.Defect.Check.ToString(), *Why);
			}
			if (Entry.Defect.bClosed && Entry.Defect.Fix.IsEmpty())
			{
				return TEXT("is closed without its fix");
			}
			return FString();
		}

		/** Reads a heading's number and title; returns what is wrong with it, or nothing. */
		FString ReadHeading(const FString& Heading, FMuseeDefect& Defect)
		{
			FString Number;
			FString Title;
			if (!Heading.Split(FString::Printf(TEXT(" %s "), TitleSeparator), &Number, &Title) || !FMuseeDefect::IsValidNumber(Number))
			{
				return FString::Printf(TEXT("'%s' is not a defect's heading, 'D-0000 %s title'"), *Heading, TitleSeparator);
			}
			Defect.Number = Number;
			Defect.Title = Title;
			return FString();
		}
	}
}

bool FMuseeDefect::IsValidNumber(FStringView Text)
{
	const int32 PrefixLength = FCString::Strlen(Musee::NumberPrefix);
	if (Text.Len() != PrefixLength + Musee::NumberDigits || !Text.StartsWith(Musee::NumberPrefix, ESearchCase::CaseSensitive))
	{
		return false;
	}
	for (int32 Index = PrefixLength; Index < Text.Len(); ++Index)
	{
		if (!FChar::IsDigit(Text[Index]))
		{
			return false;
		}
	}
	return true;
}

bool FMuseeDefect::NamesExistingCheck() const
{
	return FMuseeCheck::Find(Check) != nullptr;
}

FString FMuseeDefect::ToMarkdown() const
{
	using namespace Musee;
	FString Text = FString::Printf(TEXT("%s%s %s %s\n"), EntryHeading, *Number, TitleSeparator, *Title);
	const auto Line = [&Text](EField Field, const FString& Value)
	{
		Text += FString::Printf(TEXT("%s%s%s%s\n"), FieldOpening, FieldLabels[static_cast<int32>(Field)], FieldClosing, *Value);
	};
	Line(EField::Symptom, Symptom);
	Line(EField::Check, Check.ToString());
	Line(EField::Drawer, Drawer.ToString());
	Line(EField::Cause, Cause);
	if (!Fix.IsEmpty())
	{
		Line(EField::Fix, Fix);
	}
	Line(EField::Status, bClosed ? ClosedStatus : OpenStatus);
	return Text;
}

TValueOrError<TArray<FMuseeDefect>, FString> FMuseeDefect::ParseJournal(FStringView Markdown)
{
	using namespace Musee;
	TArray<FString> Lines;
	FString(Markdown).ParseIntoArrayLines(Lines, false);
	TArray<FEntry> Entries;
	FString Wrong;
	for (int32 LineIndex = 0; LineIndex < Lines.Num() && Wrong.IsEmpty(); ++LineIndex)
	{
		const FString Line = Lines[LineIndex].TrimEnd();
		if (Line.StartsWith(EntryHeading, ESearchCase::CaseSensitive))
		{
			Wrong = Entries.IsEmpty() ? FString() : Complete(Entries.Last());
			Wrong = Wrong.IsEmpty() ? ReadHeading(Line.RightChop(FCString::Strlen(EntryHeading)), Entries.AddDefaulted_GetRef().Defect) : Wrong;
		}
		else if (!Entries.IsEmpty() && Line.StartsWith(FieldOpening, ESearchCase::CaseSensitive))
		{
			FString Label;
			FString Value;
			const TOptional<EField> Field = Line.RightChop(FCString::Strlen(FieldOpening)).Split(FieldClosing, &Label, &Value) ? FieldOf(Label) : TOptional<EField>();
			Wrong = Field.IsSet() ? Take(Entries.Last(), Field.GetValue(), Value.TrimStartAndEnd())
				: FString::Printf(TEXT("line %d is not a field: '- **Label:** value', the label one of Symptom, Check, Drawer, Cause, Fix, Status"), LineIndex + 1);
		}
		else if (!Entries.IsEmpty() && !Line.IsEmpty())
		{
			Wrong = FString::Printf(TEXT("line %d is neither a field nor a heading; each field is one line"), LineIndex + 1);
		}
	}
	if (Wrong.IsEmpty() && !Entries.IsEmpty())
	{
		Wrong = Complete(Entries.Last());
	}
	if (!Wrong.IsEmpty())
	{
		const FString Number = Entries.Last().Defect.Number;
		return MakeError(FString::Printf(TEXT("%s %s."), Number.IsEmpty() ? TEXT("An entry") : *Number, *Wrong));
	}
	TArray<FMuseeDefect> Defects;
	TSet<FString> Numbers;
	for (const FEntry& Entry : Entries)
	{
		bool bRepeated = false;
		Numbers.Add(Entry.Defect.Number, &bRepeated);
		if (bRepeated)
		{
			return MakeError(FString::Printf(TEXT("%s is numbered twice."), *Entry.Defect.Number));
		}
		Defects.Add(Entry.Defect);
	}
	return MakeValue(MoveTemp(Defects));
}
