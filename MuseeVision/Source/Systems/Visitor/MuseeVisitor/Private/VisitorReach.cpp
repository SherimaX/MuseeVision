#include "VisitorReach.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "MuseeCaptioned.h"
#include "MuseeUsable.h"

namespace Musee::Visitor
{
	namespace
	{
		template <typename TInterface>
		UObject* FindImplementing(UPrimitiveComponent* Hit, AActor* Actor)
		{
			for (USceneComponent* Component = Hit; Component; Component = Component->GetAttachParent())
			{
				if (Component->GetClass()->ImplementsInterface(TInterface::StaticClass())) { return Component; }
			}
			for (AActor* Owner = Actor; Owner; Owner = Owner->GetAttachParentActor())
			{
				if (Owner->GetClass()->ImplementsInterface(TInterface::StaticClass())) { return Owner; }
			}
			return nullptr;
		}
	}

	UObject* FindUsable(UPrimitiveComponent* Hit, AActor* Actor)
	{
		return FindImplementing<UMuseeUsable>(Hit, Actor);
	}

	UObject* FindCaptioned(UPrimitiveComponent* Hit, AActor* Actor)
	{
		return FindImplementing<UMuseeCaptioned>(Hit, Actor);
	}
}
