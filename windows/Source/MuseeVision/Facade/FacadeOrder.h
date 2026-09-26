#pragma once

#include "CoreMinimal.h"
#include "Facade/FacadeKit.h"

/**
 * The classical elements of the museum's exterior (AMuseeFacadeStructure): an Ionic order after Vignola (the column
 * 9 D with its Attic base 0.5 D and capital, the entablature 2.25 D: architrave 0.625 D in three fasciae, frieze
 * 0.75 D, cornice 0.875 D with dentils), the podium's banded rustication, blind glazed windows with architraves,
 * keystones, sills and aprons, a bronze door, balustrades and urns. Heights are absolute (plan metres above the ground
 * floor); d is measured out from the existing wall face, which the ashlar skin covers to Skin.
 */
namespace FacadeOrder
{
	using FacadeKit::FFaceLine;
	using FacadeKit::FLocal;
	using FacadeKit::FMeshData;
	using FacadeKit::FOpening;
	using FacadeKit::FProfile;

	/** The mesh sections the parts go to. */
	struct FSink
	{
		FMeshData Skin;     // vein-cut ashlar: the wall fields
		FMeshData Stone;    // honed travertine: the podium, steps, floors, roofs, reveals' surrounds
		FMeshData Shafts;   // honed travertine: pilasters' and columns' shafts
		FMeshData Carved;   // carved stone: bases, capitals, entablatures, frames, balustrades, urns
		FMeshData Bronze;   // the door, the glazing bars
		FMeshData Dark;     // the windows' backing behind the glass
		FMeshData Glass;    // the windows' glass
		FMeshData Gravel;   // the strip at the foot of the walls
		FMeshData Base;     // the podium, its edge, the steps and the portico's floor: a darker, greyer stone
		FMeshData Gilt;     // the inscription's gilt bronze letters
	};

	struct FOrder
	{
		double D = 1.0;
		double Base = 2.0;   // the podium's top, where the bases stand
		double Skin = 0.2;   // the ashlar skin's face (d)

		double Proj() const { return D / 6.0; }
		double Face() const { return Skin + Proj(); }            // the pilasters' face and the architrave's
		double EntBottom() const { return Base + 9.0 * D; }
		double EntTop() const { return Base + 11.25 * D; }
		double ShaftTop() const { return Base + 8.55 * D; }      // the capital's echinus starts here
		double FriezeTop() const { return EntBottom() + 1.375 * D; }
	};

	/** The podium's section: a moulded base course, two courses of banded rustication (V joints at 0.6, 1.2, 1.8 m), a cap. */
	FProfile PodiumProfile();

	/** The whole entablature (architrave face at Face), from the wall (A 0) out; open at the back unless bClosed. */
	FProfile EntablatureProfile(const FOrder& O, double Face, bool bClosed = false);

	/** The cornice alone (the entablature's from the frieze's top), A from the frieze's face, B from the frieze's top. */
	FProfile CorniceSection(const FOrder& O, double Back = 0.3);

	/** A small cornice for low walls (the passages, the vestibule): H high from Z0, A out from Face. */
	FProfile SmallCornice(double Face, double Z0, double H);

	/** The dentils under the corona along a face, from U0 to U1 (the entablature on the same face line, architrave at Face). */
	void Dentils(FMeshData& M, const FOrder& O, const FFaceLine& F, double U0, double U1, double Face);

	/** An Ionic pilaster at L (its centre on the face), its base from Bottom (normally the podium's top). */
	void Pilaster(FSink& S, const FOrder& O, const FLocal& L, double Bottom, bool bFluted = true);

	/** A free-standing Ionic column at L (the axis; N its front), fluted, with entasis, on the podium's top. */
	void Column(FSink& S, const FOrder& O, const FLocal& L);

	/** A blind window's dress on a face: architrave, keystone, sill on consoles, apron, glazing bars (the skin makes the opening). */
	struct FWindow
	{
		FOpening Open;
		double Frame = 0.24;          // the architrave's width
		bool bKeystone = true;
		bool bSill = true;
		bool bApron = true;
		int32 Mullions = 2;
		double Transom = 0.9;         // glazing bars' spacing up the window
		double Medallion = 0.0;       // radius of a round tablet above it (0: none)
		double MedallionZ = 0.0;
	};
	void WindowDress(FSink& S, const FFaceLine& F, const FWindow& W, double SkinFace, double Back);

	/** A bronze double door in a rectangular opening, with its architrave and a cornice on consoles over it. */
	void DoorDress(FSink& S, const FFaceLine& F, const FOpening& Door, double SkinFace, double Back);

	/** A balustrade: plinth and rail swept along a path; pedestals and balusters placed on faces. */
	struct FBalustrade
	{
		double Z0 = 0.0;       // stands on this (the cornice's top)
		double H = 1.1;
		double Front = 0.35;   // the front of the rail's die (d)
		double Width = 0.42;   // front to back
		double PedestalWidth = 1.1;
		double Pitch = 0.34;   // balusters' centres
	};
	void BalustradeRails(FSink& S, const FBalustrade& B, const TArray<FVector2D>& Path, bool bClosed);
	void Balusters(FSink& S, const FBalustrade& B, const FFaceLine& F, double U0, double U1);
	void Pedestal(FSink& S, const FBalustrade& B, const FLocal& L, double ExtraHeight = 0.0);

	/** A stone urn (a lidded vase) standing on Foot, Height tall. */
	void Urn(FMeshData& M, const FVector& Foot, double Height);

	/**
	 * An inscription in Roman capitals (thick and thin strokes, serifs) on a face: centred on A = CentreA of L, standing
	 * on Baseline, CapH high, raised from D0 to D1. Letters: those needed here (E, É, I, M, N, O, S, V), the middle dot
	 * and the space; Tracking is the space between letters, in cap heights.
	 */
	void Inscription(FMeshData& M, const FLocal& L, double CentreA, double Baseline, double CapH, double D0, double D1, const TCHAR* Text,
					 double Tracking = 0.4);

	/** A round tablet with a rolled rim on a face (centre at A 0 of L, height Z), R across. */
	void Medallion(FMeshData& M, const FLocal& L, double SkinFace, double Z, double R);
}
