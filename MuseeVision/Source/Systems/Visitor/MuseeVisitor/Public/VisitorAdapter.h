#pragma once

#include "CoreMinimal.h"

class AVisitorBody;

/**
 * A way of being the visitor: the screen (keyboard and mouse, which the body itself has) or the headset (the
 * MuseeVisitorHeadset module, built only into the VR target). An adapter registers itself when its module starts; as
 * the body begins, the first registered adapter that is active (a headset whose OpenXR session runs) adopts it and
 * gives it its own controls and signs. With none active the body keeps the screen's, so exactly one adapter is active.
 */
class MUSEEVISITOR_API IVisitorAdapter
{
public:
	virtual ~IVisitorAdapter() = default;

	/** The adapter's name (`headset`). */
	[[nodiscard]] virtual FName Name() const = 0;

	/** Whether this adapter's device is running now. */
	[[nodiscard]] virtual bool IsActive() const = 0;

	/** Takes over a body as it begins: adds the components that give it this device's controls and signs. */
	virtual void Adopt(AVisitorBody& Body) = 0;
};

namespace Musee::Visitor
{
	/** Registers an adapter (its module's startup). */
	MUSEEVISITOR_API void RegisterAdapter(const TSharedRef<IVisitorAdapter>& Adapter);

	/** Unregisters an adapter by name (its module's shutdown). */
	MUSEEVISITOR_API void UnregisterAdapter(FName Name);

	/** The first registered adapter that is active now, or null: the screen's own controls. */
	MUSEEVISITOR_API TSharedPtr<IVisitorAdapter> ActiveAdapter();
}
