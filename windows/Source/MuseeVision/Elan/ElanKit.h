#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Geometry/MuseeMesh.h"
#include "Plan/MuseePlan.h"
#include "UObject/UObjectGlobals.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPrimitiveComponent;
class UProceduralMeshComponent;

/**
 * The Élan kit shared by the car, the landings, the neck and the platform: dimensions, materials,
 * the curved panes and bronze bands, and the collision. Positions in metres in a component's frame
 * (X east, Y south, Z up; FMuseeMesh writes centimetres); plan angles run from east towards south.
 * A named namespace: the module builds in unity files, where other files' helpers share the unit.
 */
namespace ElanGlassKit
{
	namespace E = MuseePlan::Elan;
	constexpr double Cm = MuseePlan::Cm;
	constexpr double Deg = UE_DOUBLE_PI / 180.0;
	constexpr double Turn = 2.0 * UE_DOUBLE_PI;
	constexpr double DoorTheta = UE_DOUBLE_PI;              // the door faces west
	constexpr double DoorDeg = E::CarDoorHalfAngleDegrees;  // 24°: half the doorway, and how far each leaf slides
	constexpr double LeafDeg = DoorDeg + 1.0;               // a closed leaf tucks a degree behind its pocket pane
	constexpr int32 Round = 192;                            // segments round a full circle: smooth curves at 4K
	// Neighbouring panes share their edge exactly (the same angle, so the same vertices): no gap, no overlap.
	constexpr double Joint = 0.0;
	constexpr double FinWidth = 0.012, FinDepth = 0.025;    // a leaf's bronze leading edge
	// The glass's edges: 15 mm low-iron glass seen edge on, deep green; 14 mm wide where two panes meet.
	constexpr double EdgeWidth = 0.014, EdgeDepth = 0.015;
	constexpr double BlockerThick = 0.04;                   // the collision boxes of walls and leaves
	constexpr double BlockerStepDeg = 12.0;                 // … each this long round the circle

	// The car, in its frame (the level of its floor on the axis).
	constexpr double CarR = E::CarRadius;                   // 2.2 m: the glass
	constexpr double CarLeafR = CarR - 0.05;                // the leaves slide 5 cm inside it
	constexpr double CarRingIn = 2.13, CarRingOut = 2.235;  // the bronze rings, 7 cm high
	constexpr double BaseRingBottom = -0.07, BaseRingTop = 0.025;
	constexpr double TopRingBottom = E::CarHeight - 0.07, TopRingTop = E::CarHeight;
	constexpr double FloorTop = 0.02;                       // 2 cm over the level: clear of the Square's glass floor
	constexpr double FloorBottom = -0.03;
	constexpr double GlassBottom = 0.02, GlassTop = TopRingBottom + 0.005;   // the glass runs into the rings
	constexpr double CanopyZ = E::CarHeight - 0.025;        // the clear glass roof, inside the top ring
	// The slim luminous ring tucked up into the top ring, clear of the leaves (2.15 m).
	constexpr double LightRingIn = 2.06, LightRingOut = 2.14;
	constexpr double LightRingBottom = TopRingBottom - 0.035, LightRingTop = TopRingBottom + 0.01;
	constexpr double RailR = 2.0, RailTube = 0.018, RailZ = FloorTop + 0.95;
	constexpr double RailFromDeg = 30, RailToDeg = 330;     // clear of the doorway and the leaves' pockets
	constexpr double CollarR = 0.2, CollarBottom = -0.16;   // under the floor, where the mast meets the car

	// A landing, in its frame (the landing floor on the axis).
	constexpr double ShaftR = 2.45;                         // 25 cm outside the car's glass
	constexpr double ShaftLeafR = ShaftR - 0.06;            // the leaves slide 6 cm inside it
	constexpr double ShaftRingIn = 2.36, ShaftRingOut = 2.49;
	constexpr double ShoeTop = 0.07;
	constexpr double SillIn = 2.245, SillOut = 2.515;       // from the car's base ring to outside the glass
	constexpr double SillTop = 0.008, SillBottom = -0.03, SillDeg = DoorDeg + 2.0;
	constexpr double TubeRingEvery = 2.6;                   // the tube's thin bronze rings, about this far apart
	constexpr double TubeRingIn = 2.41, TubeRingOut = 2.48;
	constexpr double TubeCollarDepth = 0.12;                // the gilt collar under the ring round the opening

	// The neck and the platform, in the elevator's frame (the Atrium floor on the axis). The neck is the
	// airlock between the building and the Sphere: from the tube's collar (≈ 7.72 m) up through the
	// opening to a bronze floor ring inside the Sphere, sealed below and above by irises (and the platform by a
	// third): each a camera's diaphragm of seven blades in the horizontal plane. Open, the blades lie in the seat
	// ring round the car's way; closing, each turns 52 degrees on its pin and sweeps in, and the seven shut in a
	// spiral. A blade is a 110-degree arc of the ring's middle circle, 1.25 m wide, its ends round; its pin lies
	// 0.65 m outside that circle, in the ring. Seven leave no gap, with 1.5 degrees to spare either way (the exact
	// union, shapely); on the way they reach r 4.01 m, so the rings that hold them run out to IrisStoreR.
	constexpr int32 Petals = 16;                            // the throat's ribs
	constexpr int32 IrisBlades = 7;
	constexpr double IrisSeatR = 2.46;                      // the seat rings' inner edge: the car's way (its glass at 2.2)
	constexpr double IrisWidth = 1.25;
	constexpr double IrisMidR = IrisSeatR + 0.01 + IrisWidth / 2;   // open, the blades' inner edge 1 cm inside the ring
	constexpr double IrisPinOutset = 0.65;                  // the pin, outside the middle circle
	constexpr double IrisArcDegrees = 110, IrisSwingDegrees = 52;
	constexpr double IrisStoreR = 4.1;                      // the rings that hold the open (and moving) blades
	constexpr double IrisStep = 0.003, IrisBladeThickness = 0.005;
	constexpr double NeckTop = 11.40;                       // the upper iris, flush with the flange: the Sphere's floor opening
	constexpr double NeckStop = 8.40;                       // the car's floor while it waits in the neck, both irises shut
	constexpr double NeckInR = 2.55, NeckOutR = 2.98;       // the throat's wall, inside the ring round the opening (3.0 m)
	constexpr double FlangeDepth = 0.25, FlangeOutR = IrisStoreR + 0.1;
	constexpr double NeckLightZ = 9.6;                      // the ring of light, at the eyes of a visitor waiting in the neck
	constexpr double PlatformIrisR = IrisSeatR;             // the platform's opening (the guard round the car's way)
	constexpr double PlatformTop = E::TopFloor + FloorTop;  // 20.42 m: level with the car's floor at the stop
	constexpr double PlatformOutR = 5.5, PlatformDepth = 0.25;
	constexpr double BalustradeR = 5.32, BalustradeHeight = 1.05;
	// The car's crown (the Cube, Cube/CubePlan.h): a gilt drum on four arms over the glass roof, the housing of the
	// spiral-band mast the car hangs from in the Cube; the mast's head rides on it, clamped by the Cube's ceiling iris
	// when the car goes down. Roller guides on the rings, north and south, run on the Cube shaft's rails.
	constexpr double CrownR = 0.34, CrownTop = E::CarHeight + 0.18;   // 2.78
	constexpr double HeadGrooveBottom = 0.07, HeadGrooveTop = 0.11;   // the head: a collar, the groove, a cap
	constexpr double HeadTop = CrownTop + 0.16;                       // 2.94: the car's highest point (the neck's upper iris is 34 mm over it)
	constexpr double ShoeOut = 2.29;                                  // the roller guides reach this far from the axis
	/** The car's floor may rise past this only with the platform's iris open (its crown then reaches it). */
	constexpr double PlatformGate = PlatformTop - HeadTop - 0.08;
	/** The top of the tube's gilt collar under the ring round the Sphere's opening (≈ 7.72 m). */
	double CollarTop();

	// Colours and materials.
	inline const FLinearColor LowIron(0.02f, 0.032f, 0.03f); // M_Glass's Diffuse is 0: this only shows if it is raised
	inline const FLinearColor WarmWhite(1.0f, 0.74f, 0.47f); // about 3000 K
	extern const TCHAR* const GlassPath;
	extern const TCHAR* const GiltPath;
	extern const TCHAR* const MetalPath;
	extern const TCHAR* const PearlPath;
	extern const TCHAR* const TravertinePath;

	/** Panes round a door (degrees from the door's centre line): two pockets, three large panes. */
	const TArray<FVector2D>& DoorSpans();
	/** The car's panes: four of 78°, their joints half a landing pane away from the enclosures'. */
	const TArray<FVector2D>& CarSpans();
	/** The tube over an enclosure: a pane over the door, then the enclosure's panes, joint over joint. */
	const TArray<FVector2D>& TubeSpans();

	struct FLook
	{
		UMaterialInterface* Clear = nullptr;
		UMaterialInterface* Bronze = nullptr;
		UMaterialInterface* Edge = nullptr;
	};

	// Geometry.
	FVector Polar(double R, double A, double Z);
	FVector Outward(double A);
	/** The direction of increasing plan angle at A. */
	FVector Along(double A);
	int32 Segments(double A0, double A1);
	/** A curved pane of the cylinder R from angle A0 to A1, Z0 to Z1: smooth radial normals. */
	void Pane(FMuseeMesh& M, double R, double A0, double A1, double Z0, double Z1);
	/** A flat ring (or a disc, R0 = 0) at height Z, facing N. */
	void Flat(FMuseeMesh& M, double R0, double R1, double Z, const FVector& N);
	/** A band of rectangular section, R0 to R1 and Z0 to Z1, from A0 to A1 (A0 < A1), capped if bCaps. */
	void Band(FMuseeMesh& M, double R0, double R1, double Z0, double Z1, double A0, double A1, bool bCaps);
	/** A round tube of radius Minor on the circle Major at height Zc, from A0 to A1. */
	void Tube(FMuseeMesh& M, double Major, double Minor, double Zc, double A0, double A1);
	/** A small ball. */
	void Ball(FMuseeMesh& M, const FVector& C, double R);
	/** A small box: centre C, half-sizes Half along the unit axes X, Y and Z. */
	void Block(FMuseeMesh& M, const FVector& C, const FVector& X, const FVector& Y, const FVector& Z, const FVector& Half);
	/** A round rod from A to B (a strut), capped. */
	void Rod(FMuseeMesh& M, const FVector& A, const FVector& B, double R);
	/**
	 * A flat ring whose inner edge is the polygon of N sides of circumradius RIn (an iris's seat) and
	 * whose outer edge is the circle ROut, Z0 to Z1: top, bottom, inner faces, and the outer face.
	 */
	void PolygonRing(FMuseeMesh& Top, FMuseeMesh& Sides, int32 N, double RIn, double ROut, double Z0, double Z1);

	// Materials.
	UMaterialInterface* LoadMaterial(const TCHAR* Path);
	/** M_Glass (thin, two-sided, no diffuse: reflections and what shows through). */
	UMaterialInstanceDynamic* GlassInstance(UObject* Outer, float Opacity, float Roughness, float Specular);
	/** A steady warm glow: M_Daylit on a white image, its night floor at 1. */
	UMaterialInstanceDynamic* GlowInstance(UObject* Outer, const FLinearColor& Tint, float Nits);
	/** M_Metal (opaque, lit) in a colour: dark bronze, or the deep green of glass seen edge on. */
	UMaterialInstanceDynamic* MetalInstance(UObject* Outer, const FLinearColor& Colour, float Metallic, float Roughness);
	/** The glass's edges: deep green, glossy, opaque (so the renderer keeps them steady). */
	UMaterialInstanceDynamic* EdgeInstance(UObject* Outer);

	// Components, made at runtime and owned by the elevator actor.
	template <class T>
	T* NewPart(USceneComponent* Parent, const TCHAR* Name)
	{
		AActor* Actor = Parent->GetOwner();
		T* Part = NewObject<T>(Actor, MakeUniqueObjectName(Actor, T::StaticClass(), FName(Name)));
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetupAttachment(Parent);
		return Part;
	}
	void Register(UActorComponent* Part);
	UProceduralMeshComponent* NewMesh(USceneComponent* Parent, const TCHAR* Name, bool bShadow);
	void Section(UProceduralMeshComponent* PM, int32 Index, const FMuseeMesh& Geometry, UMaterialInterface* Material);
	/** Keep a thin bright or emissive part out of the ray-traced scene: its reflections would only be noise. */
	void NoRayTracing(UPrimitiveComponent* Part);
	/** Solid to the visitor, invisible to the look trace and the camera; floors can be stepped on. */
	void Solid(UPrimitiveComponent* Part, bool bWalkable);
	/** A curved wall's collision: thin boxes along the circle R from A0 to A1, Z0 to Z1. */
	void Wall(USceneComponent* Parent, const TCHAR* Name, double R, double A0, double A1, double Z0, double Z1, TArray<TObjectPtr<UBoxComponent>>& Out);
	/** The deep green edge of thick glass at angle A on the cylinder R, Z0 to Z1 (a joint, a jamb, a leaf's end). */
	void EdgeStrip(FMuseeMesh& M, double R, double A, double Z0, double Z1);
	/** The fixed glass: one primitive per pane (sorted pane by pane), with the glass's green edges at every joint. */
	void FixedGlass(USceneComponent* Parent, const TCHAR* Name, double R, double Z0, double Z1, const TArray<FVector2D>& SpansDeg,
					const FLook& Look, TArray<TObjectPtr<UPrimitiveComponent>>& Out);
	/** One door leaf on its own pivot at the axis (see ElanKit.cpp). */
	USceneComponent* Leaf(USceneComponent* Parent, const TCHAR* Name, double R, double Z0, double Z1, double Side, const FLook& Look,
						  TArray<TObjectPtr<UPrimitiveComponent>>& Glass, TArray<TObjectPtr<UBoxComponent>>& Boxes);
	/** Open (0 … 1) turns each leaf round the axis by up to the door's half angle, into its pocket. */
	void TurnLeaves(const TArray<TObjectPtr<USceneComponent>>& Pivots, float Open);
}

namespace GK = ElanGlassKit;
