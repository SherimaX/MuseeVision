#include "ArtWorkComponent.h"

FName UArtWorkComponent::CareTag(FName CareClass)
{
	return FName(FString::Printf(TEXT("musee.care:%s"), *CareClass.ToString()));
}

FName UArtWorkComponent::WorkTag(const FString& WorkId)
{
	return FName(FString::Printf(TEXT("musee.work:%s"), *WorkId));
}

void UArtWorkComponent::Describe(const FString& InWorkId, FName InCare, const FMuseeCaption& InCaption)
{
	Work = InWorkId;
	Care = InCare;
	Label = InCaption;
}
