#pragma once

#include "CoreMinimal.h"
#include "Make/MakeReport.h"
#include "MuseeKey.h"
#include "SitePort.h"

struct FSitePlan;
struct FSiteLayout;

namespace Musee::Studio
{
	struct FStageMakeOptions;

	/**
	 * The site's joints, the seams between the placed stages: for every mating the layout joins, Site's joint between the
	 * two ports as their stages are placed (the threshold, the strip on the mating plane, the linings, the surrounds, and
	 * the wedge and passage masonry), drawn in the museum's frame, made by Form, cast in the joint record's materials, and
	 * written into the joints' level, L_Joints. The matings are taken in the plan's order and each joint's parts in Site's,
	 * so a second make with no change writes nothing. A mating whose joint can't be drawn is a finding (Site.PortsMate).
	 * True when the joints' level holds a joint and was made; the museum's level then instances it at its origin.
	 */
	bool MakeJoints(const FSitePlan& Plan, const FSiteLayout& Layout, const TMap<FMuseeKey, TArray<FSitePort>>& Ports,
		const FStageMakeOptions& Options, FMakeReport& Report);
}
