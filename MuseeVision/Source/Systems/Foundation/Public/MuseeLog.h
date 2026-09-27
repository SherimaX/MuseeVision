#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"

/**
 * The log category and the stat group every system and stage keeps, under one naming rule: the log is
 * `LogMusee<System>` and the CPU stat group `STATGROUP_Musee<System>`. A module declares them in a private header
 * its sources share and defines the log once in its module source:
 *
 *     MUSEE_DECLARE_LOG_CATEGORY(Light);     // LogMuseeLight
 *     MUSEE_DECLARE_STAT_GROUP(Light);       // STATGROUP_MuseeLight
 *     MUSEE_DEFINE_LOG_CATEGORY(Light);      // in MuseeLight.cpp
 */
#define MUSEE_DECLARE_LOG_CATEGORY(System) DECLARE_LOG_CATEGORY_EXTERN(LogMusee##System, Log, All)

/** Defines a system's log category, once, in its module source. */
#define MUSEE_DEFINE_LOG_CATEGORY(System) DEFINE_LOG_CATEGORY(LogMusee##System)

/** Declares a system's CPU stat group. GPU cost is attributed by render pass, through each README's declared passes. */
#define MUSEE_DECLARE_STAT_GROUP(System) DECLARE_STATS_GROUP(TEXT("Musee" #System), STATGROUP_Musee##System, STATCAT_Advanced)
