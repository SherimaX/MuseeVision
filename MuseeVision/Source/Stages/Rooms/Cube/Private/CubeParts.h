#pragma once

#include "CoreMinimal.h"
#include "CubePlan.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The Cube's parts by trade, each drawn in Form's language into the description's pieces: the concrete box (§3.1), the
 * panel wall (§3.2), the shaft's glass, gilt, plates and rails (§3.3-§3.5), the sleeve, housing, soffit ring and ceiling
 * iris (§3.6, §3.7), and the ground's cut face with its ammonite (§3.8, §7). The site's declarations (footprint, port,
 * openings, anchors, enclosures) are CubeSite's, made after the pieces so the enclosures name what was drawn. Everything
 * round the shaft is set out on the Élan axis by bearing and radius.
 */
namespace Musee::Cube
{
	/** The collision every piece of the Cube takes: all of it is solid, and nothing in it is walked. */
	inline const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

	/** A key under the stage: cube.<Local>. */
	[[nodiscard]] FMuseeKey Key(const FString& Local);

	/** A surface of the stage's finish schedule: cube.<Local>. */
	[[nodiscard]] FName Surface(const TCHAR* Local);

	/** The plan angle Form turns by (degrees from +x towards +y) of a compass bearing: the bearing less 90. */
	[[nodiscard]] double PlanAngle(double Bearing);

	/** The point at a bearing and radius on the plan: (r sin b, -r cos b). */
	[[nodiscard]] FVector2D Polar(double Radius, double Bearing);

	/** The unit vectors at a bearing: out from the axis, and round it towards increasing bearing. */
	[[nodiscard]] FVector2D Outward(double Bearing);
	[[nodiscard]] FVector2D Along(double Bearing);

	/** The stage's frame whose x and y are the given axes (z their cross product), at an origin. */
	[[nodiscard]] FMuseeFrame FrameOfAxes(const FVector& OriginMeters, const FVector& X, const FVector& Y);

	/** The angle in degrees a chord's half-width subtends at a radius: where a notch's side meets a ring. */
	[[nodiscard]] double HalfAngle(double HalfWidth, double Radius);

	/** A band r0 → r1 by z0 → z1 turned about the axis between two bearings (clockwise, the way bearings increase). */
	[[nodiscard]] FFormLathe Band(double R0, double R1, double Z0, double Z1, double FromBearing, double ToBearing);

	/** The same band turned all the way round: a ring. */
	[[nodiscard]] FFormLathe Ring(double R0, double R1, double Z0, double Z1);

	/** A plan extrusion of an outline between two heights, in a drawing frame raised to the lower. */
	[[nodiscard]] FFormExtrusion Prism(FFormProfile Outline, double Bottom, double Top, TArray<FFormProfile> Holes = {});

	/** A rectangle in plan at a bearing: U0 → U1 out from the axis along it, T0 → T1 across it (towards increasing bearing). */
	[[nodiscard]] FFormProfile AtBearing(double Bearing, double U0, double U1, double T0, double T1);

	/** A drawing of the stage, finished in one of its surfaces, colliding as given, standing in a frame. */
	template <typename TShape>
	[[nodiscard]] FFormDrawing Drawn(const FString& Name, const TCHAR* LocalSurface, TShape&& Shape, const Musee::Collision::FClass* Collision,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing Drawing = FFormDrawing::Of(FName(*Name), Surface(LocalSurface), Forward<TShape>(Shape));
		Drawing.Collision = Collision ? Collision->Profile() : NAME_None;
		Drawing.Frame = Frame;
		Drawing.Drawer = FName(Plan::Stage);
		return Drawing;
	}

	/** Glass is cast without Nanite, which can't render it. */
	FFormDrawing& AsGlass(FFormDrawing& Drawing);

	/** The moving body of blade k (0 to 6) of the ceiling iris, and its frame: at its pin, on its top face, axes of the stage. */
	[[nodiscard]] FMuseeKey BladeBody(int32 Blade);
	[[nodiscard]] FMuseeFrame BladePin(int32 Blade);

	/** The names of the drawn pieces that bound the enclosures: the site's list reads them by these prefixes. */
	namespace Pieces
	{
		inline const TCHAR* const Panel = TEXT("panel.");
		inline const TCHAR* const SoffitRing = TEXT("soffit.ring");
		inline const TCHAR* const Blade = TEXT("iris.blade.");
		inline const TCHAR* const HousingInner = TEXT("housing.inner");
		inline const TCHAR* const HousingRoof = TEXT("housing.roof");
		inline const TCHAR* const Sleeve = TEXT("sleeve.course.");
		inline const TCHAR* const Frame = TEXT("shaft.frame.");
		inline const TCHAR* const Glass = TEXT("shaft.glass.");
		inline const TCHAR* const Plate = TEXT("shaft.plate.");
		inline const TCHAR* const Web = TEXT("shaft.web.");
		inline const TCHAR* const Ground = TEXT("ground.");
	}

	void DrawBox(TArray<FFormDrawing>& Out);
	void DrawPanels(TArray<FFormDrawing>& Out);
	void DrawShaft(TArray<FFormDrawing>& Out);
	void DrawRails(TArray<FFormDrawing>& Out);
	void DrawCeiling(TArray<FFormDrawing>& Out);
	void DrawIris(TArray<FFormDrawing>& Out);
	void DrawGround(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);
}
