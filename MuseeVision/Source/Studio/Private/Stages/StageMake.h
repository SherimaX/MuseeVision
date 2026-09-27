#pragma once

#include "CoreMinimal.h"
#include "Make/MakePaths.h"
#include "Make/MakeReport.h"
#include "MuseeKey.h"
#include "Stages/MaterialLookup.h"

class USiteRoom;

namespace Musee::Studio
{
	/** Where a stage make reads and writes, and how it finds materials. The defaults make the museum. */
	struct FStageMakeOptions
	{
		/** The root every made asset lives under: /Game/Generated for the museum. */
		FMakePaths Paths{TEXT("/Game/Generated")};

		/** A stage's schedule folder; empty for Data/Stages/<stage>. */
		FString ScheduleDir;

		/** How palette keys become materials; empty for Matter's resolver. */
		FMaterialResolver Resolve;

		/** The package of the master every work's paint layer is composed over: Matter's paint master. */
		FString PaintMaster;

		/** The Site records the make reads (paths under Data, or absolute). */
		FString PlanRecord;
		FString PortClassesRecord;
		FString BlindsRecord;

		FStageMakeOptions();
	};

	/** A stage's folder and asset name: its key in PascalCase (nympheas: Nympheas, classical-hall: ClassicalHall). */
	FString StageFolder(const FMuseeKey& Stage);

	/** A key or piece name as a token an asset or component name may hold: dots become underscores. */
	FString AssetToken(FName Name);

	/**
	 * Makes one stage, in the stage contract's fixed order: describe it, make its drawings (and a blind for every port
	 * the site plan leaves unmated), gather the pieces the systems add, check its shells, make each hung work's paint
	 * layer (its image as a texture, its paint instance), cast every piece in the
	 * material of its finish, write its level with a body component per moving body and a tagged component per piece,
	 * dress it, and save. Only what changed is written. Every piece whose material can't be resolved is a finding and is
	 * left out; the level still holds everything else.
	 */
	FMakeReport MakeStage(const USiteRoom& Room, const FStageMakeOptions& Options = FStageMakeOptions());

	/**
	 * Makes the museum level: every stage whose level exists, placed by the site plan with a Level Instance at its
	 * translation, the arrival marked, and the site's game mode.
	 */
	FMakeReport MakeMuseum(const FStageMakeOptions& Options = FStageMakeOptions());

	/**
	 * Makes by name: a stage's key, `site` for the museum level, or `all` for every stage and then the museum level.
	 * False, with the reason, for a name that is none of these.
	 */
	bool MakeByName(const FString& Name, FMakeReport& OutReport, FString& OutWhy);
}
