#include "Stages/StageFabric.h"

#include "FormMake.h"
#include "FormPieceSource.h"
#include "FormShells.h"
#include "MuseeCheck.h"
#include "MuseeUnits.h"
#include "SiteBlind.h"
#include "SitePlan.h"
#include "SiteReceivedWell.h"
#include "SiteRoom.h"
#include "Stages/MakeFinding.h"
#include "Stages/StageMake.h"

namespace Musee::Studio
{
	namespace
	{
		/**
		 * The stage's ports the site plan joins to a built neighbour, as Site solves it: a mating whose other stage isn't
		 * built, or whose ports don't agree, leaves its port blind. The plan's own findings are the site make's.
		 */
		TArray<FMuseeKey> SolveMatedPorts(const FSiteDescription& Description, const FSitePlan& Plan, FMuseeDependencySet& Dependencies)
		{
			TArray<FMuseeFinding> PlanFindings;
			const FSiteLayout Layout = Musee::Site::Solve(Plan, DescribeRooms(Dependencies).Ports, PlanFindings);
			TArray<FMuseeKey> Mated;
			for (const FSitePort& Port : Description.Ports)
			{
				if (Layout.MatedPorts.Contains(Port.Key)) { Mated.Add(Port.Key); }
			}
			return Mated;
		}

		/** A blind for every port the plan leaves unmated, drawn by Site into the stage's pieces. */
		void AddBlinds(FStageFabric& Fabric, const FStageMakeOptions& Options, TArray<FMuseeFinding>& OutFindings)
		{
			FSiteDescription& Description = Fabric.Description;
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
			Fabric.MatedPorts = SolveMatedPorts(Description, Plan.GetValue(), Fabric.Dependencies);
			for (const FSitePort& Port : Description.Ports)
			{
				if (Fabric.MatedPorts.Contains(Port.Key))
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

		/** A made piece's triangles in the stage's frame, as it is drawn (its body at its anchor's frame), metres. */
		FSiteWellPiece WellPieceOf(const FFormMadePiece& Piece, const FSiteDescription& Description)
		{
			FMuseeFrame Frame = Piece.Frame;
			if (!Piece.Body.IsNone())
			{
				const FMuseeStageAnchor* Body = Description.Anchors.FindByPredicate([&Piece](const FMuseeStageAnchor& A)
				{
					return A.Key == Piece.Body && A.Kind == FName(TEXT("Body"));
				});
				Frame = Body ? Body->Frame.FrameToParent(Frame) : Frame;
			}
			FSiteWellPiece Out;
			Out.Name = Piece.Name;
			const UE::Geometry::FDynamicMesh3& Mesh = *Piece.Mesh;
			for (const int32 Triangle : Mesh.TriangleIndicesItr())
			{
				const UE::Geometry::FIndex3i Corners = Mesh.GetTriangle(Triangle);
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					Out.TrianglesMeters.Add(Frame.PointToParent(Musee::FromEngine(Mesh.GetVertex(Corners[Corner]))));
				}
			}
			return Out;
		}

		/** Site.RoomStaysInside over every well the stage receives: only the parts its class admits reach into it. */
		void CheckReceivedWells(const FStageFabric& Fabric, const FStageMakeOptions& Options, TArray<FMuseeFinding>& OutFindings)
		{
			for (const FSitePort& Port : Fabric.Description.Ports)
			{
				if (!Port.bHasWell || Port.Well.bServing)
				{
					continue;
				}
				const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(Port.Class, Options.PortClassesRecord);
				if (Class.HasError())
				{
					continue;
				}
				TArray<FSiteWellPiece> Pieces;
				for (const FFormMadePiece& Piece : Fabric.Pieces)
				{
					if (Musee::Site::PartOf(Port.Well, Piece.Name).IsNone())
					{
						Pieces.Add(WellPieceOf(Piece, Fabric.Description));
					}
				}
				OutFindings.Append(Musee::Site::CheckReceivedWell(Port, Class.GetValue(), Pieces));
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
		AddBlinds(Fabric, Options, OutFindings);
		for (FFormDrawing& Drawing : Description.Pieces)
		{
			if (Drawing.Drawer.IsNone())
			{
				Drawing.Drawer = FName(*Description.Stage.ToString());
			}
		}
		Musee::FormMaking::Make(Description.Pieces, Fabric.Pieces, OutFindings);
		const FMuseeStageContext Context{Description.Stage, ScheduleDir, Description.Anchors, nullptr, Fabric.MatedPorts};
		Musee::FormMaking::GatherPieces(Context, Fabric.Pieces, OutFindings);
		CheckShells(Fabric, OutFindings);
		CheckReceivedWells(Fabric, Options, OutFindings);
		return Fabric;
	}

	FSiteRooms DescribeRooms(FMuseeDependencySet& Dependencies)
	{
		FMuseeDependencySet::FScope Reading(Dependencies);
		FSiteRooms Rooms;
		for (const USiteRoom* Room : USiteRoom::AllRooms())
		{
			FSiteDescription Description;
			Room->Describe(Description);
			Rooms.Ports.Add(Room->StageKey(), MoveTemp(Description.Ports));
			Rooms.Footprints.Add(Room->StageKey(), MoveTemp(Description.Footprint));
		}
		return Rooms;
	}
}
