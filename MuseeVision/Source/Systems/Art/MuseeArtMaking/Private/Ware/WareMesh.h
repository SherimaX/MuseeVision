#pragma once

#include "CoreMinimal.h"

#include "WareMesh.generated.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

/**
 * How Art turns ware (`Data/Art/Ware.json`): a dish's section, as fractions of its rim's radius and its height (its face
 * from the rim in to the centre, its underside from the centre out to the rim), how a vessel's open mouth turns in over
 * its lip and down its neck, and how finely a turned form is divided round its axis.
 */
USTRUCT()
struct FArtWareForms
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Ware.json");

	UPROPERTY(meta = (MuseeRequired))
	TArray<FVector2D> DishFace;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FVector2D> DishUnderside;

	/** A vessel's wall at its lip: this, or this fraction of the lip's radius where that is less. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double WallMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	double WallOfLip = 0.0;

	/** How far its neck runs down inside: this, or this fraction of its height where that is less. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double NeckDepthMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	double NeckDepthOfHeight = 0.0;

	/** A mouth whose radius is less than this is drawn closed. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double OpenLipMeters = 0.0;

	/** The most a chord round the axis strays from the true circle. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double ChordToleranceMeters = 0.0;
};

/**
 * A turned piece's form as its photograph gives it (`Data/Art/Turned.json`): where it stands in its image, [left, right,
 * top, bottom] as fractions of the image, and for a vessel its outline, [radius, height] from the foot up as fractions of
 * its half-width and its height.
 */
USTRUCT()
struct FArtTurned
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Turned.json");

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Window;

	UPROPERTY()
	TArray<FVector2D> Profile;
};

namespace Musee::ArtMaking
{
	/** The kinds of turned ware Art makes: a dish, lying on its foot, and a vessel standing on it. */
	inline const FName DishKind(TEXT("dish"));
	inline const FName VesselKind(TEXT("vase"));

	/**
	 * One piece of turned ware, in its own frame: the foot's centre at the origin, x to the viewer's right, y towards the
	 * viewer (its front), z up. Its photograph is laid over it as the photograph was taken: a dish's from above, its top
	 * away from the viewer; a vessel's from the front, projected through it.
	 */
	struct FWareSpec
	{
		FName Kind;
		/** Its width (a dish's diameter) and its height, metres. */
		double WidthMeters = 0.0;
		double HeightMeters = 0.0;
		/** Where it stands in its image, [left, right, top, bottom] as fractions. */
		FVector4 Window = FVector4(0.0, 1.0, 0.0, 1.0);
		/** A vessel's outline, [radius, height] from the foot up as fractions of its half-width and height. */
		TArray<FVector2D> Profile;
	};

	/** The size in metres of the image its photograph spans: the piece's box fills its window. */
	[[nodiscard]] FVector2D WareImageMeters(const FWareSpec& Ware);

	/**
	 * Builds the piece to the Form kernel's mesh contract: its decorated faces (a dish's face; a vessel's outside), their
	 * UV0 in metres from its image's top-left corner, and its ground (a dish's underside; a vessel's mouth, neck and
	 * floor inside), UV0 in metres across it. Together they are one closed surface.
	 */
	void BuildWare(const FArtWareForms& Forms, const FWareSpec& Ware, UE::Geometry::FDynamicMesh3& OutDecorated, UE::Geometry::FDynamicMesh3& OutGround);
}
