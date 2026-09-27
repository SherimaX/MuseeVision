#include "Stages/StageCast.h"

#include "Cast/Caster.h"
#include "Cast/CastRules.h"
#include "DynamicMeshToMeshDescription.h"
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
			Converter.Convert(&Piece.Mesh, Description, /*bCopyTangents*/ true);
			return Description;
		}

		/** A body component for every moving body the pieces ride on or the stage declares, at its anchor's frame. */
		TArray<FStageBody> BodiesOf(const FStageFabric& Fabric, TArray<FMuseeFinding>& OutFindings)
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
			TArray<FStageBody> Bodies;
			for (const FMuseeKey& Key : Keys)
			{
				const FMuseeStageAnchor* Anchor = Fabric.Description.Anchors.FindByPredicate([&Key](const FMuseeStageAnchor& A)
				{
					return A.Key == Key && A.Kind == BodyAnchorKind;
				});
				if (!Anchor)
				{
					OutFindings.Add(MakeFinding(TEXT("Site.Described"), FName(*Fabric.Description.Stage.ToString()), FString::Printf(
						TEXT("pieces ride on body %s, which the stage declares no Body anchor for"), *Key.ToString()), Key.ToString()));
					continue;
				}
				Bodies.Add({AssetToken(Key.ToName()), Key.ToName(), Anchor->Frame.OriginMeters, Anchor->Frame.Rotation});
			}
			return Bodies;
		}

		bool Failed(const FMakeReport& Report, const FSoftObjectPath& Mesh)
		{
			const FString Package = Mesh.GetLongPackageName();
			return Report.Packages.ContainsByPredicate([&Package](const FMadePackage& Made)
			{
				return Made.PackageName == Package && Made.Outcome == EMadeOutcome::Failed;
			});
		}

		void Place(const FFormMadePiece& Piece, const FCastMesh& Mesh, TArray<FStagePlacement>& OutPlacements)
		{
			FStagePlacement& Placement = OutPlacements.AddDefaulted_GetRef();
			Placement.Name = AssetToken(Piece.Name) + PartSuffix(Mesh.Kind);
			Placement.Mesh = Mesh.Path;
			Placement.LocationMeters = Piece.Frame.OriginMeters;
			Placement.Rotation = Piece.Frame.Rotation;
			Placement.Body = Piece.Body.IsNone() ? FString() : AssetToken(Piece.Body.ToName());
			Placement.Collision = Piece.Collision;
			Placement.Tags.Add(FName(PieceTagPrefix + Piece.Name.ToString()));
		}
	}

	FStageCast CastStage(FStageFabric& Fabric, const FStageMakeOptions& Options, const FString& ScheduleDir, FMakeReport& Report)
	{
		const FName Stage(*Fabric.Description.Stage.ToString());
		const FString Folder = StageFolder(Fabric.Description.Stage);
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
		TArray<const FFormMadePiece*> Owners;
		for (const FFormMadePiece& Piece : Fabric.Pieces)
		{
			TArray<FCastSlot> Slots;
			if (!SlotsOf(Piece, Finishes.HasValue() ? &Finishes.GetValue() : nullptr, Resolve, Stage, Slots, Report.Findings))
			{
				continue;
			}
			FCastInput& Input = Inputs.AddDefaulted_GetRef();
			Input.PackageName = Options.Paths.StageMesh(Folder, AssetToken(Piece.Name));
			Input.Description = DescribeMesh(Piece);
			Input.Slots = MoveTemp(Slots);
			Input.bCollides = !Piece.Collision.IsNone();
			Owners.Add(&Piece);
		}
		const TArray<FCastResult> Results = CastMeshes(Inputs, Report.Packages, Report.Cast);
		for (int32 Index = 0; Index < Results.Num(); ++Index)
		{
			for (const FCastMesh& Mesh : Results[Index].Meshes)
			{
				if (!Failed(Report, Mesh.Path))
				{
					Place(*Owners[Index], Mesh, Cast.Placements);
				}
			}
		}
		return Cast;
	}
}
