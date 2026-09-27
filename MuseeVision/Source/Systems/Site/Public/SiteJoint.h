#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "FormProfile.h"
#include "SitePort.h"
#include "SitePortClass.h"
#include "Templates/ValueOrError.h"

#include "SiteJoint.generated.h"

/** A finish a joint's part is made in: a palette key, its working and its history (Data/Matter/Palette.json). */
USTRUCT()
struct MUSEESITE_API FSiteJointFinish
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FName Material;

	UPROPERTY(meta = (MuseeRequired))
	FName Working;

	UPROPERTY(meta = (MuseeRequired))
	FName History;
};

/**
 * The doorcase every joint is made to, as a door schedule's typical detail: what its threshold, strip, linings,
 * surrounds and masonry are made of, how deep the threshold and strip are laid, the surround's section, and the longest
 * stone of an arch's lining. Read from Data/Site/Joints.json.
 */
USTRUCT()
struct MUSEESITE_API FSiteJointRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FSiteJointFinish Threshold;

	UPROPERTY(meta = (MuseeRequired))
	FSiteJointFinish Strip;

	UPROPERTY(meta = (MuseeRequired))
	FSiteJointFinish Lining;

	UPROPERTY(meta = (MuseeRequired))
	FSiteJointFinish Surround;

	UPROPERTY(meta = (MuseeRequired))
	FSiteJointFinish Masonry;

	/** The threshold's stone and the strip's bar, laid flush with the floor on a mortar bed over the rough sill, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double ThresholdMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double StripDepthMeters = 0.0;

	/** The surround's section: A out from the opening's edge in the face, B proud of it, drawn with its solid on its left. */
	UPROPERTY(meta = (MuseeRequired))
	FFormProfile SurroundSection;

	/** The longest voussoir of an arched lining, measured round the arch, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double LiningStoneMeters = 0.0;

	UPROPERTY()
	FString Source;
};

/**
 * The joint between two mated ports: the doorcase Site draws once at every seam, in Form's language, in the museum's
 * frame. It holds the travertine threshold through both walls' depths and any passage, the bronze strip on the mating
 * plane, the linings in front of both rough openings, the surrounds on the faces the stages leave to it, and the masonry
 * that closes the gap between two faces within the class's collar envelope (the wedges between a flat face and a drum).
 * A vertical port's faces touch and its iris is the lower stage's, so its joint draws nothing.
 */
struct FSiteJoint
{
	TArray<FFormDrawing> Drawings;
};

namespace Musee::Site
{
	/** The joints' record's file, under Data. */
	inline constexpr const TCHAR* JointsRecord = TEXT("Site/Joints.json");

	/** The joints' record (the museum's by default). */
	[[nodiscard]] MUSEESITE_API TValueOrError<FSiteJointRecord, FString> ReadJoints(const FString& Path = JointsRecord);

	/** A port as its stage is placed: its frame and its well moved by the stage's translation, metres. */
	[[nodiscard]] MUSEESITE_API FSitePort Placed(const FSitePort& Port, const FVector& TranslationMeters);

	/**
	 * The joint between two ports placed in the museum's frame, A's face PassageMeters from B's along A's facing: drawn in
	 * the museum's frame and named joint.<A's key>.<part>. False, with the reason, when the ports can't be joined: they
	 * differ in class, the class's closure has no joint yet, or a face turns too tight for the opening.
	 */
	[[nodiscard]] MUSEESITE_API bool Joint(const FSitePort& A, const FSitePort& B, double PassageMeters, const FSitePortClass& Class,
		const FSiteJointRecord& Record, FSiteJoint& OutJoint, FString& OutWhy);
}
