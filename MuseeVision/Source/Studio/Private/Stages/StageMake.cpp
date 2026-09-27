#include "Stages/StageMake.h"

#include "HAL/FileManager.h"
#include "Levels/MuseumLevel.h"
#include "Levels/StageLevel.h"
#include "Make/InputHash.h"
#include "Make/MakeMeasures.h"
#include "MatterPaintLayer.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "MuseeRecords.h"
#include "MuseeStageDresser.h"
#include "SiteBlind.h"
#include "SiteGameMode.h"
#include "SiteJoint.h"
#include "SitePlan.h"
#include "SitePortClass.h"
#include "SiteRoom.h"
#include "SiteVolume.h"
#include "Stages/JointsMake.h"
#include "Stages/MakeFinding.h"
#include "Stages/PaintLayers.h"
#include "Stages/StageCast.h"
#include "Stages/StageFabric.h"
#include "StudioLog.h"

namespace Musee::Studio
{
	namespace
	{
		/** Where a stage's schedules live under Data: Stages/<stage>. */
		const TCHAR* const StagesRecords = TEXT("Stages");

		/** The names MakeByName takes besides a stage's key. */
		const TCHAR* const SiteName = TEXT("site");
		const TCHAR* const AllName = TEXT("all");

		/** The joints' Level Instance in the museum's level. */
		const TCHAR* const JointsLevelName = TEXT("Joints");

		FString ScheduleDirOf(const FMuseeKey& Stage, const FStageMakeOptions& Options)
		{
			return Options.ScheduleDir.IsEmpty() ? Musee::Records::DataRoot() / StagesRecords / Stage.ToString() : Options.ScheduleDir;
		}

		/**
		 * The hash of everything a stage's level is made from besides its placements: the records the making read, the
		 * ports the site plan mates (the mechanisms' rides read them), every schedule in the stage's folder (the dressers read
		 * their own), and the dressers that run, in order.
		 */
		FString LevelDependencyHash(const FStageFabric& Fabric, const FString& ScheduleDir)
		{
			FInputHasher Hasher;
			Hasher.Add(TEXT("stage-dependencies")).Add(Fabric.Dependencies.Hash());
			for (const FMuseeKey& Port : Fabric.MatedPorts)
			{
				Hasher.Add(Port.ToString());
			}
			TArray<FString> Files;
			IFileManager::Get().FindFiles(Files, *(ScheduleDir / TEXT("*.json")), true, false);
			Files.Sort();
			for (const FString& File : Files)
			{
				FString Text;
				FFileHelper::LoadFileToString(Text, *(ScheduleDir / File));
				Hasher.Add(File).Add(Text);
			}
			for (const FName Dresser : Musee::StageDressers())
			{
				Hasher.Add(Dresser.ToString());
			}
			return Hasher.Finish();
		}

		/** The stages the site plan names: its anchor and both ends of every mating. */
		TArray<FMuseeKey> StagesOfPlan(const FSitePlan& Plan)
		{
			TArray<FMuseeKey> Stages = {Plan.Anchor};
			for (const FSiteMating& Mating : Plan.Matings)
			{
				Stages.AddUnique(Musee::Site::StageOfPort(Mating.A));
				Stages.AddUnique(Musee::Site::StageOfPort(Mating.B));
			}
			return Stages;
		}
	}

	FStageMakeOptions::FStageMakeOptions()
		: PaintMaster(Musee::Matter::PaintMasterPackage())
		, PlanRecord(Musee::Site::PlanRecord)
		, PortClassesRecord(Musee::Site::PortClassesRecord)
		, BlindsRecord(Musee::Site::BlindsRecord)
		, JointsRecord(Musee::Site::JointsRecord)
	{
	}

	FString StageFolder(const FMuseeKey& Stage)
	{
		FString Folder;
		for (const FString& Segment : Stage.Segments())
		{
			TArray<FString> Words;
			Segment.ParseIntoArray(Words, TEXT("-"));
			for (const FString& Word : Words)
			{
				Folder += Word.Left(1).ToUpper() + Word.RightChop(1);
			}
		}
		return Folder;
	}

	FString AssetToken(FName Name)
	{
		return Name.ToString().Replace(TEXT("."), TEXT("_"));
	}

	FMakeReport MakeStage(const USiteRoom& Room, const FStageMakeOptions& Options)
	{
		FMakeReport Report;
		const FMuseeKey Stage = Room.StageKey();
		const FString Folder = StageFolder(Stage);
		const FString ScheduleDir = ScheduleDirOf(Stage, Options);
		const FDerivedDataTally Before = MeasureDerivedData();
		FStageFabric Fabric;
		{
			FMakeStepTimer Timer(Report, Folder + TEXT(": describe and make"));
			Fabric = MakeFabric(Room, Options, ScheduleDir, Report.Findings);
		}
		{
			FMakeStepTimer Timer(Report, Folder + TEXT(": paint"));
			MakePaintLayers(FMuseeStageContext{Stage, ScheduleDir, Fabric.Description.Anchors, nullptr, Fabric.MatedPorts}, Options, Report);
		}
		FStageCast Cast;
		{
			FMakeStepTimer Timer(Report, Folder + TEXT(": cast"));
			Cast = CastStage(Fabric, Options, ScheduleDir, Report);
			UE_LOG(LogMuseeStudio, Display, TEXT("%s: %d of %d pieces are placed as instances of a mesh identical pieces share."), *Folder,
				Cast.Instanced, Fabric.Pieces.Num());
		}
		Report.DerivedData = MeasureDerivedData() - Before;
		{
			FMakeStepTimer Timer(Report, Folder + TEXT(": level"));
			FStageLevel Level;
			Level.StageName = Folder;
			Level.PackageName = Options.Paths.StageLevel(Folder);
			Level.Bodies = MoveTemp(Cast.Bodies);
			Level.Placements = MoveTemp(Cast.Placements);
			for (const FSiteBarrier& Barrier : Fabric.Description.Barriers)
			{
				Level.Barriers.Add({AssetToken(Barrier.Key.ToName()), Barrier.Key.ToName(), Barrier.Frame.OriginMeters, Barrier.Frame.Rotation,
					Barrier.HalfSizeMeters});
			}
			Level.DependencyHash = LevelDependencyHash(Fabric, ScheduleDir);
			const TArray<FMuseeStageAnchor>& Anchors = Fabric.Description.Anchors;
			const TArray<FMuseeKey>& Mated = Fabric.MatedPorts;
			Level.Dress = [&Report, &Stage, &ScheduleDir, &Anchors, &Mated](AActor& Actor, FString&)
			{
				const FMuseeStageContext Context{Stage, ScheduleDir, Anchors, &Actor, Mated};
				Musee::DressStage(Context, Report.Findings);
				return true;
			};
			Report.Packages.Add(WriteStageLevel(Level));
		}
		Report.PeakPhysMemoryMB = MeasurePeakPhysMemoryMB();
		return Report;
	}

	FMakeReport MakeMuseum(const FStageMakeOptions& Options)
	{
		FMakeReport Report;
		FMakeStepTimer Timer(Report, TEXT("museum level"));
		const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan(Options.PlanRecord);
		if (Plan.HasError())
		{
			Report.Findings.Add(MakeFinding(TEXT("Site.PlanResolves"), TEXT("Site"), Plan.GetError()));
			return Report;
		}
		FMuseeDependencySet Dependencies;
		const FSiteRooms Rooms = DescribeRooms(Dependencies);
		const FSiteLayout Layout = Musee::Site::Solve(Plan.GetValue(), Rooms.Ports, Report.Findings);
		Report.Findings.Append(Musee::Site::CheckOverlaps(Rooms.Footprints, Layout));
		FMuseumLevel Museum{Options.Paths.MuseumLevel(), {}};
		TArray<FMuseeKey> Placed;
		Layout.TranslationsMeters.GetKeys(Placed);
		Placed.Sort([](const FMuseeKey& A, const FMuseeKey& B) { return A.ToString() < B.ToString(); });
		for (const FMuseeKey& Stage : Placed)
		{
			const FString Level = Options.Paths.StageLevel(StageFolder(Stage));
			if (!FPackageName::DoesPackageExist(Level))
			{
				Report.Findings.Add(MakeFinding(TEXT("Site.PlanResolves"), FName(*Stage.ToString()), FString::Printf(
					TEXT("stage %s is placed but has no level yet: make it first"), *Stage.ToString()), Stage.ToString()));
				continue;
			}
			Museum.Stages.Add({StageFolder(Stage), Level, Layout.TranslationsMeters[Stage]});
		}
		if (MakeJoints(Plan.GetValue(), Layout, Rooms.Ports, Options, Report))
		{
			Museum.Stages.Add({JointsLevelName, Options.Paths.JointsLevel(), FVector::ZeroVector});
		}
		if (Layout.bHasArrival)
		{
			Museum.ArrivalTag = Musee::Site::ArrivalTag;
			Museum.ArrivalMeters = Layout.ArrivalMeters;
			Museum.ArrivalFacingDegrees = Layout.ArrivalFacingDegrees;
		}
		else
		{
			Report.Findings.Add(MakeFinding(TEXT("Site.PlanResolves"), TEXT("Site"), FString::Printf(
				TEXT("the arrival's stage %s is not placed"), *Plan.GetValue().Arrival.Stage.ToString())));
		}
		Museum.GameMode = ASiteGameMode::StaticClass();
		Report.Packages.Add(WriteMuseumLevel(Museum));
		return Report;
	}

	bool MakeByName(const FString& Name, FMakeReport& OutReport, FString& OutWhy)
	{
		if (Name == SiteName)
		{
			OutReport = MakeMuseum();
			return true;
		}
		if (Name == AllName)
		{
			const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan();
			if (Plan.HasError())
			{
				OutWhy = Plan.GetError();
				return false;
			}
			for (const FMuseeKey& Stage : StagesOfPlan(Plan.GetValue()))
			{
				if (const USiteRoom* Room = USiteRoom::Find(Stage))
				{
					OutReport.Append(MakeStage(*Room));
				}
			}
			OutReport.Append(MakeMuseum());
			return true;
		}
		const TOptional<FMuseeKey> Stage = FMuseeKey::Parse(Name);
		const USiteRoom* Room = Stage ? USiteRoom::Find(*Stage) : nullptr;
		if (!Room)
		{
			OutWhy = FString::Printf(TEXT("%s is no stage's key (nor site or all); the stages loaded are the rooms' modules"), *Name);
			return false;
		}
		OutReport = MakeStage(*Room);
		return true;
	}
}
