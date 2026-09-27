#include "Stages/StageMake.h"

#include "HAL/FileManager.h"
#include "Levels/MuseumLevel.h"
#include "Levels/StageLevel.h"
#include "Make/InputHash.h"
#include "Make/MakeMeasures.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "MuseeRecords.h"
#include "MuseeStageDresser.h"
#include "SiteBlind.h"
#include "SiteGameMode.h"
#include "SitePlan.h"
#include "SitePortClass.h"
#include "SiteRoom.h"
#include "Stages/MakeFinding.h"
#include "Stages/StageCast.h"
#include "Stages/StageFabric.h"

namespace Musee::Studio
{
	namespace
	{
		/** Where a stage's schedules live under Data: Stages/<stage>. */
		const TCHAR* const StagesRecords = TEXT("Stages");

		/** The names MakeByName takes besides a stage's key. */
		const TCHAR* const SiteName = TEXT("site");
		const TCHAR* const AllName = TEXT("all");

		/** Times a step of a make into its report. */
		class FStepTimer
		{
		public:
			FStepTimer(FMakeReport& InReport, FString InName) : Report(InReport), Name(MoveTemp(InName)), Start(FPlatformTime::Seconds()) {}
			~FStepTimer() { Report.Steps.Add({Name, FPlatformTime::Seconds() - Start}); }

		private:
			FMakeReport& Report;
			FString Name;
			double Start;
		};

		FString ScheduleDirOf(const FMuseeKey& Stage, const FStageMakeOptions& Options)
		{
			return Options.ScheduleDir.IsEmpty() ? Musee::Records::DataRoot() / StagesRecords / Stage.ToString() : Options.ScheduleDir;
		}

		/**
		 * The hash of everything a stage's level is made from besides its placements: the records the making read, every
		 * schedule in the stage's folder (the dressers read their own), and the dressers that run, in order.
		 */
		FString LevelDependencyHash(const FMuseeDependencySet& Dependencies, const FString& ScheduleDir)
		{
			FInputHasher Hasher;
			Hasher.Add(TEXT("stage-dependencies")).Add(Dependencies.Hash());
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
		: PlanRecord(Musee::Site::PlanRecord)
		, PortClassesRecord(Musee::Site::PortClassesRecord)
		, BlindsRecord(Musee::Site::BlindsRecord)
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
			FStepTimer Timer(Report, Folder + TEXT(": describe and make"));
			Fabric = MakeFabric(Room, Options, ScheduleDir, Report.Findings);
		}
		FStageCast Cast;
		{
			FStepTimer Timer(Report, Folder + TEXT(": cast"));
			Cast = CastStage(Fabric, Options, ScheduleDir, Report);
		}
		Report.DerivedData = MeasureDerivedData() - Before;
		{
			FStepTimer Timer(Report, Folder + TEXT(": level"));
			FStageLevel Level;
			Level.StageName = Folder;
			Level.PackageName = Options.Paths.StageLevel(Folder);
			Level.Bodies = MoveTemp(Cast.Bodies);
			Level.Placements = MoveTemp(Cast.Placements);
			Level.DependencyHash = LevelDependencyHash(Fabric.Dependencies, ScheduleDir);
			const TArray<FMuseeStageAnchor>& Anchors = Fabric.Description.Anchors;
			Level.Dress = [&Report, &Stage, &ScheduleDir, &Anchors](AActor& Actor, FString&)
			{
				const FMuseeStageContext Context{Stage, ScheduleDir, Anchors, &Actor};
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
		FStepTimer Timer(Report, TEXT("museum level"));
		const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan(Options.PlanRecord);
		if (Plan.HasError())
		{
			Report.Findings.Add(MakeFinding(TEXT("Site.PlanResolves"), TEXT("Site"), Plan.GetError()));
			return Report;
		}
		FMuseeDependencySet Dependencies;
		const FSiteLayout Layout = Musee::Site::Solve(Plan.GetValue(), PortsOfRooms(Dependencies), Report.Findings);
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
