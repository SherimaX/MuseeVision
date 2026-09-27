#include "Stages/StageCast.h"

#include "Cast/Caster.h"
#include "Cast/CastRules.h"
#include "DynamicMeshToMeshDescription.h"
#include "Make/InputHash.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshAttributes.h"
#include "Stages/FinishSchedule.h"
#include "Stages/MakeFinding.h"
#include "Stages/StageFabric.h"
#include "Stages/StageMake.h"

namespace Musee::Studio
{
	namespace
	{
		/** The tag each piece's component carries: musee.piece:<name>. */
		const TCHAR* const PieceTagPrefix = TEXT("musee.piece:");

		/** The anchor kind of a moving body (the stage contract's anchor kinds). */
		const TCHAR* const BodyAnchorKind = TEXT("Body");

		/** Each slot's material, or false with a finding for every slot that has none. */
		bool SlotsOf(const FFormMadePiece& Piece, const FStudioFinishSchedule* Finishes, const FMaterialResolver& Resolve,
			FName Stage, TArray<FCastSlot>& OutSlots, TArray<FMuseeFinding>& OutFindings)
		{
			bool bAll = true;
			const FString Name = Piece.Name.ToString();
			for (int32 Slot = 0; Slot < Piece.NumSlots(); ++Slot)
			{
				const FName Surface = Piece.SurfaceOfSlot(Slot);
				FName Palette = Piece.Material;
				FName Working = Piece.Working;
				FName History = Piece.History;
				if (Palette.IsNone())
				{
					const FStudioFinish* Finish = Finishes ? Finishes->Find(Surface) : nullptr;
					if (!Finish)
					{
						OutFindings.Add(MakeFinding(TEXT("Matter.Resolved"), Stage, FString::Printf(
							TEXT("piece %s: the finish schedule has no line for surface %s"), *Name, *Surface.ToString()), Name));
						bAll = false;
						continue;
					}
					Palette = Finish->Material;
					Working = Finish->Working;
					History = Finish->History;
				}
				UMaterialInterface* Material = Resolve(Palette, Working, History);
				if (!Material)
				{
					OutFindings.Add(MakeFinding(TEXT("Matter.Resolved"), TEXT("Matter"), FString::Printf(
						TEXT("piece %s, surface %s: %s (%s, %s) resolves to no material"), *Name, *Surface.ToString(), *Palette.ToString(),
						*Working.ToString(), *History.ToString()), Name));
					bAll = false;
					continue;
				}
				OutSlots.Add({Surface, FSoftObjectPath(Material)});
			}
			return bAll;
		}

		FMeshDescription DescribeMesh(const FFormMadePiece& Piece)
		{
			FMeshDescription Description;
			FStaticMeshAttributes(Description).Register();
			FDynamicMeshToMeshDescription Converter;
			// The kernel writes linear colours (Matter's per-vertex facts), never sRGB.
			Converter.ConversionOptions.bTransformVtxColorsSRGBToLinear = false;
			Converter.Convert(&*Piece.Mesh, Description, /*bCopyTangents*/ true);
			return Description;
		}

		const FMuseeStageAnchor* BodyAnchor(const FSiteDescription& Description, const FMuseeKey& Key)
		{
			return Description.Anchors.FindByPredicate([&Key](const FMuseeStageAnchor& A) { return A.Key == Key && A.Kind == BodyAnchorKind; });
		}

		/**
		 * The moving bodies the pieces ride on or the stage declares, and the bodies they ride, in an order that lists each
		 * body after the one it rides.
		 */
		TArray<FMuseeKey> BodyKeys(const FStageFabric& Fabric)
		{
			TArray<FMuseeKey> Keys;
			for (const FMuseeStageAnchor& Anchor : Fabric.Description.Anchors)
			{
				if (Anchor.Kind == BodyAnchorKind) { Keys.AddUnique(Anchor.Key); }
			}
			for (const FFormMadePiece& Piece : Fabric.Pieces)
			{
				if (!Piece.Body.IsNone()) { Keys.AddUnique(Piece.Body); }
			}
			TArray<FMuseeKey> Ordered;
			TSet<FMuseeKey> Placed;
			while (Ordered.Num() < Keys.Num())
			{
				const int32 Before = Ordered.Num();
				for (const FMuseeKey& Key : Keys)
				{
					const FMuseeKey* Parent = Fabric.Description.BodyParents.Find(Key);
					if (!Placed.Contains(Key) && (!Parent || Placed.Contains(*Parent) || !Keys.Contains(*Parent)))
					{
						Ordered.Add(Key);
						Placed.Add(Key);
					}
				}
				if (Ordered.Num() == Before)
				{
					// The rest ride round a loop (Site.Described has found it): they hang from the stage's frame.
					for (const FMuseeKey& Key : Keys)
					{
						if (!Placed.Contains(Key)) { Ordered.Add(Key); Placed.Add(Key); }
					}
				}
			}
			return Ordered;
		}

		/**
		 * A body component for every moving body the pieces ride on or the stage declares, at its anchor's frame: hung from
		 * the body it rides, in that body's frame, or from the stage's frame.
		 */
		TArray<FStageBody> BodiesOf(const FStageFabric& Fabric, TArray<FMuseeFinding>& OutFindings)
		{
			const FSiteDescription& Description = Fabric.Description;
			TArray<FStageBody> Bodies;
			TSet<FMuseeKey> Hung;
			for (const FMuseeKey& Key : BodyKeys(Fabric))
			{
				const FMuseeStageAnchor* Anchor = BodyAnchor(Description, Key);
				if (!Anchor)
				{
					OutFindings.Add(MakeFinding(TEXT("Site.Described"), FName(*Description.Stage.ToString()), FString::Printf(
						TEXT("pieces ride on body %s, which the stage declares no Body anchor for"), *Key.ToString()), Key.ToString()));
					continue;
				}
				const FMuseeKey* Parent = Description.BodyParents.Find(Key);
				const FMuseeStageAnchor* Carrier = Parent && Hung.Contains(*Parent) ? BodyAnchor(Description, *Parent) : nullptr;
				const FMuseeFrame Frame = Carrier ? Carrier->Frame.FrameFromParent(Anchor->Frame) : Anchor->Frame;
				Bodies.Add({AssetToken(Key.ToName()), Key.ToName(), Carrier ? AssetToken(Carrier->Key.ToName()) : FString(), Frame.OriginMeters, Frame.Rotation});
				Hung.Add(Key);
			}
			return Bodies;
		}

		/** Identical pieces: one cast, the pieces that share it, in the order the stage drew them. */
		struct FCastGroup
		{
			TArray<const FFormMadePiece*> Members;
		};

		/** What makes two pieces one cast: the same drawing to the last attribute, the same materials, the same collision. */
		FString IdentityOf(FCastInput& Input)
		{
			FInputHasher Hasher;
			Hasher.Add(TEXT("identity")).Add(static_cast<int64>(Input.bCollides));
			for (const FCastSlot& Slot : Input.Slots)
			{
				Hasher.Add(Slot.Name.ToString()).Add(Slot.Material.ToString());
			}
			return Hasher.Add(Input.Description).Finish();
		}

		FStagePlacement PlacementOf(const FFormMadePiece& Piece, const FCastMesh& Mesh)
		{
			FStagePlacement Placement;
			Placement.Name = AssetToken(Piece.Name) + PartSuffix(Mesh.Kind);
			Placement.Mesh = Mesh.Path;
			Placement.Body = Piece.Body.IsNone() ? FString() : AssetToken(Piece.Body.ToName());
			Placement.Collision = Piece.Collision;
			return Placement;
		}

		/**
		 * Places a cast: a single piece as one mesh at its frame; identical pieces as one instanced component for each body
		 * and collision they share, named after the first of them, with an instance at each piece's frame.
		 */
		void Place(const FCastGroup& Group, const FCastMesh& Mesh, TArray<FStagePlacement>& OutPlacements)
		{
			if (Group.Members.Num() == 1)
			{
				const FFormMadePiece& Piece = *Group.Members[0];
				FStagePlacement& Placement = OutPlacements.Add_GetRef(PlacementOf(Piece, Mesh));
				Placement.LocationMeters = Piece.Frame.OriginMeters;
				Placement.Rotation = Piece.Frame.Rotation;
				Placement.Tags.Add(FName(PieceTagPrefix + Piece.Name.ToString()));
				return;
			}
			TMap<TPair<FMuseeKey, FName>, int32> Shared;
			for (const FFormMadePiece* Piece : Group.Members)
			{
				const int32* Index = Shared.Find({Piece->Body, Piece->Collision});
				FStagePlacement& Placement = Index ? OutPlacements[*Index] : OutPlacements.Add_GetRef(PlacementOf(*Piece, Mesh));
				if (!Index)
				{
					Shared.Add({Piece->Body, Piece->Collision}, OutPlacements.Num() - 1);
				}
				Placement.Instances.Add({Piece->Frame.OriginMeters, Piece->Frame.Rotation});
				Placement.Tags.Add(FName(PieceTagPrefix + Piece->Name.ToString()));
			}
		}
	}

	FStageCast CastStage(FStageFabric& Fabric, const FStageMakeOptions& Options, const FString& ScheduleDir, FMakeReport& Report)
	{
		const FString Folder = StageFolder(Fabric.Description.Stage);
		return CastPieces(Fabric, Options, ScheduleDir, [&Options, &Folder](FName Piece) { return Options.Paths.StageMesh(Folder, AssetToken(Piece)); },
			Report);
	}

	FStageCast CastPieces(FStageFabric& Fabric, const FStageMakeOptions& Options, const FString& ScheduleDir, TFunctionRef<FString(FName)> MeshPackage,
		FMakeReport& Report)
	{
		const FName Stage(*Fabric.Description.Stage.ToString());
		TValueOrError<FStudioFinishSchedule, FString> Finishes = MakeError(FString());
		{
			FMuseeDependencySet::FScope Reading(Fabric.Dependencies);
			Finishes = ReadFinishSchedule(ScheduleDir);
		}
		const bool bNeedsSchedule = Fabric.Pieces.ContainsByPredicate([](const FFormMadePiece& P) { return P.Material.IsNone(); });
		if (Finishes.HasError() && bNeedsSchedule)
		{
			Report.Findings.Add(MakeFinding(TEXT("Matter.Resolved"), Stage, FString::Printf(TEXT("the finish schedule can't be read: %s"),
				*Finishes.GetError())));
		}
		const FMaterialResolver Resolve = Options.Resolve ? Options.Resolve : MatterResolver();

		FStageCast Cast;
		Cast.Bodies = BodiesOf(Fabric, Report.Findings);
		TArray<FCastInput> Inputs;
		TArray<FCastGroup> Groups;
		TMap<FString, int32> GroupOfIdentity;
		for (const FFormMadePiece& Piece : Fabric.Pieces)
		{
			FCastInput Input;
			if (!SlotsOf(Piece, Finishes.HasValue() ? &Finishes.GetValue() : nullptr, Resolve, Stage, Input.Slots, Report.Findings))
			{
				continue;
			}
			Input.Description = DescribeMesh(Piece);
			Input.bCollides = !Piece.Collision.IsNone();
			const FString Identity = IdentityOf(Input);
			if (const int32* Group = GroupOfIdentity.Find(Identity))
			{
				Groups[*Group].Members.Add(&Piece);
				continue;
			}
			GroupOfIdentity.Add(Identity, Groups.Num());
			Groups.AddDefaulted_GetRef().Members.Add(&Piece);
			Input.PackageName = MeshPackage(Piece.Name);
			Inputs.Add(MoveTemp(Input));
		}
		const TArray<FCastResult> Results = CastMeshes(Inputs, Report.Packages, Report.Cast);
		for (int32 Index = 0; Index < Results.Num(); ++Index)
		{
			for (const FCastMesh& Mesh : Results[Index].Meshes)
			{
				if (!Report.HasFailed(Mesh.Path))
				{
					Place(Groups[Index], Mesh, Cast.Placements);
				}
			}
		}
		for (const FCastGroup& Group : Groups)
		{
			Cast.Instanced += Group.Members.Num() > 1 ? Group.Members.Num() : 0;
		}
		return Cast;
	}
}
