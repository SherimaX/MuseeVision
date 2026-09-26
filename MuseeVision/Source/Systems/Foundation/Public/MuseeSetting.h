#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

/**
 * A setting the visitor or the owner may change while the museum runs: the hour (`time.hour`), the light profile
 * (`light.profile`), the visitor's comfort (`visitor.comfort`). Every setting has exactly one owner, the system named
 * by its key's first segment, which registers it once when its module starts and unregisters it when the module
 * stops. There is one settings store: each value is kept in the user's config (GameUserSettings, section
 * `MuseeSettings`), so it survives a restart, and there is no other copy.
 *
 * Registering a setting also registers its command, `musee.<system>.set-<setting>` (FMuseeCommand), which the
 * console, the keyboard, the headset and Studio all use. With no argument the command reports the value.
 *
 * A setting is a number within an inclusive range, or a choice among named values (each a key segment, like
 * `headset`). Settings are read and changed on the game thread.
 */
class MUSEEFOUNDATION_API FMuseeSetting
{
public:
	/** What a setting holds. */
	enum class EKind : uint8
	{
		Number,
		Choice,
	};

	/** Announces a changed value. */
	using FChanged = TMulticastDelegate<void(const FMuseeSetting&)>;

	/**
	 * Registers a number setting: the key is `<system>.<setting>`, the default lies in [Minimum, Maximum], and Help says
	 * what the number is and its unit. Returns the setting, holding its stored value if the store has a valid one, or
	 * null (with an error) if the key is taken or breaks the rule.
	 */
	static FMuseeSetting* RegisterNumber(const FMuseeKey& Key, double Default, double Minimum, double Maximum, const FString& Help);

	/** Registers a choice setting, as RegisterNumber; the default is one of the choices. */
	static FMuseeSetting* RegisterChoice(const FMuseeKey& Key, FName Default, const TArray<FName>& Choices, const FString& Help);

	/** Unregisters a setting and its command. The stored value stays in the user's config. */
	static void Unregister(const FMuseeKey& Key);

	/** A registered setting, or null. */
	[[nodiscard]] static FMuseeSetting* Find(const FMuseeKey& Key);

	/** Every registered setting's key, in key order. */
	[[nodiscard]] static TArray<FMuseeKey> Keys();

	/** The command a setting's key creates: `musee.<system>.set-<setting>`. */
	[[nodiscard]] static FName CommandName(const FMuseeKey& Key);

	[[nodiscard]] const FMuseeKey& Key() const { return SettingKey; }
	[[nodiscard]] EKind Kind() const { return SettingKind; }
	[[nodiscard]] const FString& Help() const { return HelpText; }

	/** A number setting's value. */
	[[nodiscard]] double Number() const;

	/** A choice setting's value. */
	[[nodiscard]] FName Choice() const;

	/** A choice setting's choices. */
	[[nodiscard]] const TArray<FName>& Choices() const { return ChoiceNames; }

	/** A number setting's range. */
	[[nodiscard]] double Minimum() const { return RangeMinimum; }
	[[nodiscard]] double Maximum() const { return RangeMaximum; }

	/** Sets a number setting and stores it. False, changing nothing, for a choice setting or a value out of range. */
	bool SetNumber(double Value);

	/** Sets a choice setting and stores it. False, changing nothing, for a number setting or an unknown choice. */
	bool SetChoice(FName Value);

	/** Sets the setting from text as the command receives it. False, changing nothing, if the text isn't a valid value. */
	bool SetFromText(const FString& Text);

	/** The value as text, as the store keeps it. */
	[[nodiscard]] FString ToText() const;

	/** Fires after the value changes. */
	FChanged& OnChanged() { return Changed; }

private:
	/** Settings are made only by RegisterNumber and RegisterChoice. */
	FMuseeSetting(const FMuseeKey& InKey, EKind InKind, const FString& InHelp);

	static FMuseeSetting* Add(TUniquePtr<FMuseeSetting> Setting);

	/** Takes a value from text without storing it or announcing it, as the store's copy is read back. */
	bool Load(const FString& Text);

	/** What the setting accepts, in words, for the command's help and its refusals. */
	FString DescribeValues() const;

	void Store() const;

	FMuseeKey SettingKey;
	EKind SettingKind;
	FString HelpText;
	double NumberValue = 0.0;
	double RangeMinimum = 0.0;
	double RangeMaximum = 0.0;
	FName ChoiceValue;
	TArray<FName> ChoiceNames;
	FChanged Changed;
};
