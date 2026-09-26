#pragma once

#include "CoreMinimal.h"
#include "Salon/SalonKit.h"

/** The garden pond's water surface (Single Layer Water, AChenghuaiStructure's Water component). */
namespace ChenghuaiWater
{
	/** The pond's outline in plan (metres): the board's edge, drawn in under the water pavilion so it stands in the water. */
	TArray<FVector2D> PondOutline();

	/** The water's surface at Chenghuai::WaterLevel, a little inside the edge (the bank's wet line runs under it). */
	SalonKit::FMeshData BuildWater();
}
