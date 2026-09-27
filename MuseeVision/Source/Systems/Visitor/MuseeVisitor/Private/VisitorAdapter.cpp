#include "VisitorAdapter.h"

namespace Musee::Visitor
{
	namespace
	{
		TArray<TSharedRef<IVisitorAdapter>>& Adapters()
		{
			static TArray<TSharedRef<IVisitorAdapter>> Registered;
			return Registered;
		}
	}

	void RegisterAdapter(const TSharedRef<IVisitorAdapter>& Adapter)
	{
		UnregisterAdapter(Adapter->Name());
		Adapters().Add(Adapter);
	}

	void UnregisterAdapter(FName Name)
	{
		Adapters().RemoveAll([Name](const TSharedRef<IVisitorAdapter>& Adapter) { return Adapter->Name() == Name; });
	}

	TSharedPtr<IVisitorAdapter> ActiveAdapter()
	{
		for (const TSharedRef<IVisitorAdapter>& Adapter : Adapters())
		{
			if (Adapter->IsActive()) { return Adapter; }
		}
		return nullptr;
	}
}
