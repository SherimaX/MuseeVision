#include "Stages/StageFabric.h"

#include "FormMake.h"
#include "FormPieceSource.h"
#include "FormShells.h"
#include "MuseeCheck.h"
#include "SiteBlind.h"
#include "SitePlan.h"
#include "SiteRoom.h"
#include "Stages/MakeFinding.h"
#include "Stages/StageMake.h"

namespace Musee::Studio
{
	namespace
	{
		/** Whether the site plan joins a port to a port of a stage that is built (its room is loaded). */
		bool IsMated(const FSitePlan& Plan, const FMuseeKey& Port)
		{
			for (const FSiteMating& Mating : Plan.Matings)
			{
				const FMuseeKey* Other = Mating.A == Port ? &Mating.B : (Mating.B == Port ? &Mating.A : nullptr);
				if (Other && USiteRoom::Find(Musee::Site::StageOfPort(*Other)))
				{
					return true;
				}
			}
			return false;
		}

		/** A blind for every port the plan leaves unmated, drawn by Site into the stage's pieces. */
		void AddBlinds(FSiteDescription& Description, const FStageMakeOptions& Options, TArray<FMuseeFinding>& OutFindings)
		{
			if (Description.Ports.IsEmpty())
			{
				return;
			}
			const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan(Options.PlanRecord);
			const TValueOrError<FSiteBlindDoors, FString> Doors = Musee::Site::ReadBlindDoors(Options.BlindsRecord);
			if (Plan.HasError() || Doors.HasError())
			{
				OutFindings.Add(MakeFinding(TEXT("Site.PlanResolves"), TEXT("Site"), Plan.HasError() ? Plan.GetError() : Doors.GetError()));
				return;
			}
			for (const FSitePort& Port : Description.Ports)
			{
				if (IsMated(Plan.GetValue(), Port.Key))
				{
					continue;
				}
				const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(Port.Class, Options.PortClassesRecord);
				FString Why = Class.HasError() ? Class.GetError() : FString();
				const bool bBlinded = Why.IsEmpty() && Musee::Site::Blind(Port, Class.GetValue(), Doors.GetValue(), Description.Pieces, Why);
				if (!bBlinded)
				{
					OutFindings.Add(MakeFinding(TEXT("Site.PortsMate"), TEXT("Site"), FString::Printf(TEXT("port %s gets no blind: %s"),
						*Port.Key.ToString(), *Why)));
				}
			}
		}

		/** Form.Sealed over the stage's enclosures and the shells of its made pieces. */
		void CheckShells(const FStageFabric& Fabric, TArray<FMuseeFinding>& OutFindings)
		{
			FFormShells Shells;
			Shells.Stage = Fabric.Description.Stage;
			Shells.Enclosures = Fabric.Description.Enclosures;
			for (const FFormMadePiece& Piece : Fabric.Pieces)
			{
				Shells.Pieces.Add(Musee::FormMaking::MeasureShell(Piece));
			}
			const FMuseeCheck* Sealed = FMuseeCheck::Find(TEXT("Form.Sealed"));
			if (!Sealed)
			{
				OutFindings.Add(MakeFinding(TEXT("Form.Sealed"), TEXT("Form"), TEXT("Form.Sealed is not registered: MuseeForm is not loaded")));
				return;
			}
			FMuseeCheck::FInputs Inputs;
			Inputs.Add(Shells);
			TValueOrError<TArray<FMuseeFinding>, FString> Found = Sealed->Evaluate(Inputs);
			if (Found.HasError())
			{
				OutFindings.Add(MakeFinding(TEXT("Form.Sealed"), TEXT("Form"), Found.GetError()));
				return;
			}
			OutFindings.Append(Found.GetValue());
		}
	}

	FStageFabric MakeFabric(const USiteRoom& Room, const FStageMakeOptions& Options, const FString& ScheduleDir,
		TArray<FMuseeFinding>& OutFindings)
	{
		FStageFabric Fabric;
		FMuseeDependencySet::FScope Reading(Fabric.Dependencies);
		Room.Describe(Fabric.Description);
		FSiteDescription& Description = Fabric.Description;
		if (!(Description.Stage == Room.StageKey()))
		{
			OutFindings.Add(MakeFinding(TEXT("Site.Described"), FName(*Room.StageKey().ToString()),
				FString::Printf(TEXT("the room of %s describes %s"), *Room.StageKey().ToString(), *Description.Stage.ToString())));
		}
		OutFindings.Append(Musee::Site::CheckDescription(Description));
		AddBlinds(Description, Options, OutFindings);
		for (FFormDrawing& Drawing : Description.Pieces)
		{
			if (Drawing.Drawer.IsNone())
			{
				Drawing.Drawer = FName(*Description.Stage.ToString());
			}
		}
		Musee::FormMaking::Make(Description.Pieces, Fabric.Pieces, OutFindings);
		const FMuseeStageContext Context{Description.Stage, ScheduleDir, Description.Anchors, nullptr};
		Musee::FormMaking::GatherPieces(Context, Fabric.Pieces, OutFindings);
		CheckShells(Fabric, OutFindings);
		return Fabric;
	}

	TMap<FMuseeKey, TArray<FSitePort>> PortsOfRooms(FMuseeDependencySet& Dependencies)
	{
		FMuseeDependencySet::FScope Reading(Dependencies);
		TMap<FMuseeKey, TArray<FSitePort>> Ports;
		for (const USiteRoom* Room : USiteRoom::AllRooms())
		{
			FSiteDescription Description;
			Room->Describe(Description);
			Ports.Add(Room->StageKey(), MoveTemp(Description.Ports));
		}
		return Ports;
	}
}
