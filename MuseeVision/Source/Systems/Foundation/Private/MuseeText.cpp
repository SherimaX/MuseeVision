#include "MuseeText.h"

FText FMuseeText::ToText() const
{
	return FText::AsLocalizable_Advanced(Namespace, Key.ToString(), Source);
}
