#include "Cast/CastParts.h"

#include "Make/EngineUnits.h"
#include "StaticMeshAttributes.h"

namespace Musee::Studio
{
	namespace
	{
		/** Names each polygon group of a part after its slot, so the build maps group i to material slot i by name. */
		void NameGroups(FCastPart& Part)
		{
			FStaticMeshAttributes Attributes(Part.Description);
			TPolygonGroupAttributesRef<FName> SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
			for (int32 Slot = 0; Slot < Part.Slots.Num(); ++Slot)
			{
				SlotNames[FPolygonGroupID(Slot)] = Part.Slots[Slot].Name;
			}
		}
	}

	FMeshDescription ExtractGroups(const FMeshDescription& Description, TConstArrayView<FPolygonGroupID> Keep)
	{
		FMeshDescription Part = Description;
		TArray<FPolygonGroupID> Dropped;
		TArray<FPolygonID> DroppedPolygons;
		for (const FPolygonGroupID Group : Part.PolygonGroups().GetElementIDs())
		{
			if (!Keep.Contains(Group))
			{
				Dropped.Add(Group);
				DroppedPolygons.Append(Part.GetPolygonGroupPolygonIDs(Group));
			}
		}
		// Deleting the polygons also deletes the vertex instances, edges and vertices only they used.
		Part.DeletePolygons(DroppedPolygons);
		for (const FPolygonGroupID Group : Dropped)
		{
			if (Part.IsPolygonGroupValid(Group))
			{
				Part.DeletePolygonGroup(Group);
			}
		}
		FElementIDRemappings Remappings;
		Part.Compact(Remappings);
		return Part;
	}

	TArray<FCastPart> SplitByKind(const FMeshDescription& Description, TConstArrayView<FCastSlot> Slots, TConstArrayView<ECastKind> SlotKinds)
	{
		check(Slots.Num() == SlotKinds.Num());
		TArray<FCastPart> Parts;
		for (const ECastKind Kind : {ECastKind::Opaque, ECastKind::Translucent})
		{
			FCastPart Part;
			Part.Kind = Kind;
			TArray<FPolygonGroupID> Keep;
			for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
			{
				if (SlotKinds[Slot] == Kind)
				{
					Keep.Add(FPolygonGroupID(Slot));
					Part.Slots.Add(Slots[Slot]);
				}
			}
			if (Keep.IsEmpty())
			{
				continue;
			}
			Part.Description = Keep.Num() == Slots.Num() ? Description : ExtractGroups(Description, Keep);
			NameGroups(Part);
			Parts.Add(MoveTemp(Part));
		}
		return Parts;
	}

	double LargestExtentMeters(const FMeshDescription& Description)
	{
		const FStaticMeshConstAttributes Attributes(Description);
		const TVertexAttributesConstRef<FVector3f> Positions = Attributes.GetVertexPositions();
		FBox3f Bounds(ForceInit);
		for (const FVertexID Vertex : Description.Vertices().GetElementIDs())
		{
			Bounds += Positions[Vertex];
		}
		return Bounds.IsValid ? MetersFromEngine(Bounds.GetSize().GetMax()) : 0.0;
	}
}
