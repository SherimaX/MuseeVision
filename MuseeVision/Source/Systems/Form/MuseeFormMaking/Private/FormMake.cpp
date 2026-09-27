#include "FormMake.h"

#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Drawing/Draw.h"
#include "Kernel/DynamicMeshOut.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/ShellEdges.h"
#include "MuseeHumanMeasure.h"
#include "MuseeUnits.h"

namespace Musee::FormMaking
{
	namespace
	{
		using Musee::Form::FMeshBuilder;

		/**
		 * The range of a comfortable pace on a stair, 0.59 to 0.65 m for two risers and a going (DIN 18065:2020, the
		 * step rule), about the human measure's pace of 0.63 m.
		 */
		constexpr double LeastStairPaceMeters = 0.59;
		constexpr double GreatestStairPaceMeters = 0.65;

		FMuseeFinding Finding(const FFormDrawing& Drawing, const TCHAR* Check, const FString& Summary, const FString& Label,
			double Value, double Least, double Most)
		{
			FMuseeMeasurement Reading;
			Reading.Label = Label;
			Reading.Value = Value;
			Reading.ExpectedMinimum = Least;
			Reading.ExpectedMaximum = Most;
			const TOptional<FMuseeKey> Subject = FMuseeKey::Parse(Drawing.Name.ToString());
			Reading = Reading.At(Subject.Get(FMuseeKey()), FMuseeKey(), Drawing.Frame.OriginMeters);
			FMuseeFinding Result = FMuseeFinding::Make(Drawing.Drawer, Summary, Reading);
			Result.Check = Check;
			Result.Owner = TEXT("Form");
			return Result;
		}

		FMuseeFinding Unmade(const FFormDrawing& Drawing, const FString& Why)
		{
			return Finding(Drawing, TEXT("Form.Drawn"), FString::Printf(TEXT("%s can't be made: %s"), *Drawing.Name.ToString(), *Why),
				TEXT("pieces made"), 0.0, 1.0, TNumericLimits<double>::Max());
		}

		/** The steps of a stair fit the body: no riser above its step, and a pace within the step rule's range. */
		void CheckSteps(const FFormDrawing& Drawing, const FFormStair& Stair, TArray<FMuseeFinding>& OutFindings)
		{
			const FFormStairSteps Steps = Stair.SetOut();
			const double Tallest = FMuseeHumanMeasure::MaxStepHeight.Get();
			if (Steps.RiserMeters > Tallest)
			{
				OutFindings.Add(Finding(Drawing, TEXT("Form.StepsFitTheBody"), FString::Printf(TEXT("%s has risers taller than the body steps"),
					*Drawing.Name.ToString()), TEXT("riser, m"), Steps.RiserMeters, 0.0, Tallest));
			}
			if (Steps.PaceMeters() < LeastStairPaceMeters || Steps.PaceMeters() > GreatestStairPaceMeters)
			{
				OutFindings.Add(Finding(Drawing, TEXT("Form.StepsFitTheBody"), FString::Printf(TEXT("%s breaks the step rule: two risers and a going make %.3f m"),
					*Drawing.Name.ToString(), Steps.PaceMeters()), TEXT("pace, m"), Steps.PaceMeters(), LeastStairPaceMeters, GreatestStairPaceMeters));
			}
		}

		/**
		 * A piece from a drawn mesh: each builder slot (a part) takes the surface the drawing names for it, parts of one
		 * surface share a material slot, and the drawing's own surface is slot 0 whenever a part uses it.
		 */
		FFormMadePiece Piece(const FFormDrawing& Drawing, FName Name, const FMeshBuilder& Mesh, FName OnlySurface)
		{
			FFormMadePiece Made;
			Made.Name = Name;
			Made.Material = Drawing.Material;
			Made.Working = Drawing.Working;
			Made.History = Drawing.History;
			Made.Body = Drawing.Body;
			Made.Collision = Drawing.Collision;
			Made.bNanite = Drawing.bNanite;
			Made.Frame = Drawing.Frame;
			Made.Drawer = Drawing.Drawer;

			TArray<bool> Used;
			Used.Init(false, Mesh.Slots().Num());
			for (int32 Triangle = 0; Triangle < Mesh.NumTriangles(); ++Triangle)
			{
				Used[Mesh.TriangleSlot(Triangle)] = true;
			}
			TArray<FName> Surfaces;
			TArray<int32> SurfaceOfSlot;
			for (int32 Slot = 0; Slot < Mesh.Slots().Num(); ++Slot)
			{
				const FName Part = Mesh.Slots()[Slot];
				const FName Surface = !OnlySurface.IsNone() ? OnlySurface : (Part.IsNone() ? Drawing.Surface : Drawing.SurfaceOf(Part));
				SurfaceOfSlot.Add(Used[Slot] ? Surfaces.AddUnique(Surface) : INDEX_NONE);
			}
			// The drawing's own surface leads when it is used at all, so a one-surface piece is always slot 0.
			const int32 Own = Surfaces.IndexOfByKey(OnlySurface.IsNone() ? Drawing.Surface : OnlySurface);
			if (Own > 0)
			{
				Surfaces.Swap(0, Own);
				for (int32& Slot : SurfaceOfSlot)
				{
					Slot = Slot == Own ? 0 : (Slot == 0 ? Own : Slot);
				}
			}
			Made.Surface = Surfaces.IsEmpty() ? Drawing.Surface : Surfaces[0];
			for (int32 Index = 1; Index < Surfaces.Num(); ++Index)
			{
				Made.SlotSurfaces.Add(Surfaces[Index]);
			}
			Musee::Form::WriteDynamicMesh(Mesh, ToEngine(Meters(1.0)), *Made.Mesh);
			UE::Geometry::FDynamicMeshMaterialAttribute* Materials = Made.Mesh->Attributes()->GetMaterialID();
			for (const int32 Triangle : Made.Mesh->TriangleIndicesItr())
			{
				Materials->SetValue(Triangle, SurfaceOfSlot[Materials->GetValue(Triangle)]);
			}
			return Made;
		}

		bool DrawShape(const FFormDrawing& Drawing, FMeshBuilder& Mesh, FMeshBuilder& Pane, FString& OutWhy)
		{
			using namespace Musee::Form::Drawing;
			const FFormShape& Shape = Drawing.Shape;
			if (const FFormExtrusion* S = Shape.TryGet<FFormExtrusion>()) { return DrawExtrusion(*S, Mesh, OutWhy); }
			if (const FFormSweep* S = Shape.TryGet<FFormSweep>()) { return DrawSweep(*S, Mesh, OutWhy); }
			if (const FFormLathe* S = Shape.TryGet<FFormLathe>()) { return DrawLathe(*S, Mesh, OutWhy); }
			if (const FFormFace* S = Shape.TryGet<FFormFace>()) { return DrawFace(*S, Mesh, OutWhy); }
			if (const FFormWall* S = Shape.TryGet<FFormWall>()) { return DrawWall(*S, Mesh, OutWhy); }
			if (const FFormBasin* S = Shape.TryGet<FFormBasin>()) { return DrawBasin(*S, Mesh, OutWhy); }
			if (const FFormStair* S = Shape.TryGet<FFormStair>()) { return DrawStair(*S, Mesh, OutWhy); }
			if (const FFormCeiling* S = Shape.TryGet<FFormCeiling>()) { return DrawCeiling(*S, Mesh, Pane, OutWhy); }
			if (const FFormRelief* S = Shape.TryGet<FFormRelief>()) { return DrawRelief(*S, Mesh, OutWhy); }
			OutWhy = TEXT("its shape is none of Form's words");
			return false;
		}
	}

	void Make(const FFormDrawing& Drawing, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		FMeshBuilder Mesh;
		FMeshBuilder Pane;
		FString Why;
		if (Drawing.Name.IsNone() || Drawing.Surface.IsNone())
		{
			OutFindings.Add(Unmade(Drawing, TEXT("a drawing has a name and a surface")));
			return;
		}
		if (!DrawShape(Drawing, Mesh, Pane, Why))
		{
			OutFindings.Add(Unmade(Drawing, Why));
			return;
		}
		if (Mesh.NumTriangles() == 0)
		{
			OutFindings.Add(Unmade(Drawing, TEXT("it draws no surface")));
			return;
		}
		if (const FFormStair* Stair = Drawing.Shape.TryGet<FFormStair>())
		{
			CheckSteps(Drawing, *Stair, OutFindings);
		}
		OutPieces.Add(Piece(Drawing, Drawing.Name, Mesh, NAME_None));
		if (Pane.NumTriangles() > 0)
		{
			const FFormCeiling& Ceiling = Drawing.Shape.Get<FFormCeiling>();
			FFormMadePiece& Glass = OutPieces.Add_GetRef(Piece(Drawing, FName(Drawing.Name.ToString() + TEXT(".glazing")), Pane,
				Ceiling.GlazingSurface.IsNone() ? Drawing.Surface : Ceiling.GlazingSurface));
			Glass.bNanite = false;
		}
	}

	void Make(TConstArrayView<FFormDrawing> Drawings, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		for (const FFormDrawing& Drawing : Drawings)
		{
			Make(Drawing, OutPieces, OutFindings);
		}
	}

	FFormPieceShell MeasureShell(const FFormMadePiece& Piece)
	{
		const Musee::Form::FShellEdges Edges = Musee::Form::MeasureShellEdges(*Piece.Mesh);
		FFormPieceShell Shell;
		Shell.Piece = Piece.Name;
		Shell.Drawer = Piece.Drawer;
		Shell.Faces = Edges.Faces;
		Shell.BoundaryEdges = Edges.Boundary;
		Shell.NonManifoldEdges = Edges.NonManifold;
		Shell.MisorientedEdges = Edges.Misoriented;
		return Shell;
	}
}
