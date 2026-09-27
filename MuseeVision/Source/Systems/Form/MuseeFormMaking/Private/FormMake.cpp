#include "FormMake.h"

#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Drawing/Draw.h"
#include "Kernel/DynamicMeshOut.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/PlaneCut.h"
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

		bool DrawShape(const FFormDrawing& Drawing, FMeshBuilder& Mesh, FMeshBuilder& Pane, Musee::Form::Drawing::FLaidTally& Tally, FString& OutWhy)
		{
			using namespace Musee::Form::Drawing;
			const FFormShape& Shape = Drawing.Shape;
			if (const FFormLoft* S = Shape.TryGet<FFormLoft>()) { return DrawLoft(*S, Mesh, OutWhy); }
			if (const FFormLaid* S = Shape.TryGet<FFormLaid>()) { return DrawLaid(*S, Mesh, Pane, Tally, OutWhy); }
			if (const FFormExtrusion* S = Shape.TryGet<FFormExtrusion>()) { return DrawExtrusion(*S, Mesh, OutWhy); }
			if (const FFormSweep* S = Shape.TryGet<FFormSweep>()) { return DrawSweep(*S, Mesh, OutWhy); }
			if (const FFormLathe* S = Shape.TryGet<FFormLathe>()) { return DrawLathe(*S, Mesh, OutWhy); }
			if (const FFormFace* S = Shape.TryGet<FFormFace>()) { return DrawFace(*S, Mesh, OutWhy); }
			if (const FFormWall* S = Shape.TryGet<FFormWall>()) { return DrawWall(*S, Mesh, OutWhy); }
			if (const FFormBasin* S = Shape.TryGet<FFormBasin>()) { return DrawBasin(*S, Mesh, OutWhy); }
			if (const FFormStair* S = Shape.TryGet<FFormStair>()) { return DrawStair(*S, Mesh, OutWhy); }
			if (const FFormCeiling* S = Shape.TryGet<FFormCeiling>()) { return DrawCeiling(*S, Mesh, Pane, OutWhy); }
			if (const FFormRelief* S = Shape.TryGet<FFormRelief>()) { return DrawRelief(*S, Mesh, OutWhy); }
			if (const FFormDome* S = Shape.TryGet<FFormDome>()) { return DrawDome(*S, Mesh, OutWhy); }
			if (const FFormHeightfield* S = Shape.TryGet<FFormHeightfield>()) { return DrawHeightfield(*S, Mesh, OutWhy); }
			if (const FFormLettering* S = Shape.TryGet<FFormLettering>()) { return DrawLettering(*S, Mesh, OutWhy); }
			if (const FFormCloth* S = Shape.TryGet<FFormCloth>()) { return DrawCloth(*S, Mesh, OutWhy); }
			OutWhy = TEXT("its shape is none of Form's words");
			return false;
		}

		/** A shape drawn in its ground's development, carried onto the ground in place. */
		bool CarryToGround(const FFormDrawing& Drawing, FMeshBuilder& Mesh, FString& OutWhy)
		{
			if (Drawing.Ground.IsFlat() || Mesh.NumTriangles() == 0)
			{
				return true;
			}
			FMeshBuilder Carried(Mesh.NumUVChannels());
			if (!Musee::Form::Drawing::SetOnGround(Drawing.Ground, Mesh, Carried, OutWhy))
			{
				return false;
			}
			Mesh = MoveTemp(Carried);
			return true;
		}
	}

	namespace
	{
		/**
		 * A dome names its own surfaces: its parts take them, the outer face's standing in for any it leaves unnamed (and
		 * the drawing's for an outer face it leaves unnamed).
		 */
		FFormDrawing WithDomeSurfaces(const FFormDrawing& Drawing)
		{
			FFormDrawing Out = Drawing;
			if (const FFormDome* Dome = Drawing.Shape.TryGet<FFormDome>())
			{
				using namespace Musee::Form::Drawing;
				const FName Outer = Dome->Surface.IsNone() ? Drawing.Surface : Dome->Surface;
				Out.PartSurfaces.Add(DomePart::Outer, Outer);
				Out.PartSurfaces.Add(DomePart::Inner, Dome->InnerSurface.IsNone() ? Outer : Dome->InnerSurface);
				Out.PartSurfaces.Add(DomePart::Oculus, Dome->OculusSurface.IsNone() ? Outer : Dome->OculusSurface);
			}
			return Out;
		}
	}

	namespace
	{
		/** A laid surface's units stay within the stock they are cut from. */
		void CheckStock(const FFormDrawing& Drawing, const Musee::Form::Drawing::FLaidTally& Tally, TArray<FMuseeFinding>& OutFindings)
		{
			if (Tally.BeyondStock > 0)
			{
				const FFormLaid& Laid = Drawing.Shape.Get<FFormLaid>();
				OutFindings.Add(Finding(Drawing, TEXT("Form.WithinStock"), FString::Printf(
					TEXT("%s lays %d of its %d units larger than their stock gives (largest %.3f x %.3f m, stock %.3f x %.3f m)"),
					*Drawing.Name.ToString(), Tally.BeyondStock, Tally.Units, Tally.LargestMeters.X, Tally.LargestMeters.Y,
					Laid.StockMeters.GetMax(), Laid.StockMeters.GetMin()), TEXT("units beyond their stock"), Tally.BeyondStock, 0.0, 0.0));
			}
		}

		/** Cuts every made surface of a drawing on its planes. */
		bool CutAll(const FFormDrawing& Drawing, FMeshBuilder& Mesh, FMeshBuilder& Second, FString& OutWhy)
		{
			for (const FFormCut& Cut : Drawing.Cuts)
			{
				if (!Musee::Form::CutMesh(Mesh, Cut.PointMeters, Cut.Normal, FFormDrawing::CutPart, OutWhy)
					|| (Second.NumTriangles() > 0 && !Musee::Form::CutMesh(Second, Cut.PointMeters, Cut.Normal, FFormDrawing::CutPart, OutWhy)))
				{
					return false;
				}
			}
			return true;
		}

		/** The second piece a drawing makes: a ceiling's glazing, a laid surface's filled joints. */
		void AddSecond(const FFormDrawing& Drawing, const FMeshBuilder& Second, TArray<FFormMadePiece>& OutPieces)
		{
			if (const FFormCeiling* Ceiling = Drawing.Shape.TryGet<FFormCeiling>())
			{
				FFormMadePiece& Glass = OutPieces.Add_GetRef(Piece(Drawing, FName(Drawing.Name.ToString() + TEXT(".glazing")), Second,
					Ceiling->GlazingSurface.IsNone() ? Drawing.Surface : Ceiling->GlazingSurface));
				Glass.bNanite = false;
				return;
			}
			OutPieces.Add(Piece(Drawing, FName(Drawing.Name.ToString() + TEXT(".joints")), Second, Drawing.SurfaceOf(FFormLaid::JointPart)));
		}

		/** Identical members: the made pieces set again at each repeat, named for it; none without repeats. */
		void Repeat(const FFormDrawing& Drawing, TArray<FFormMadePiece>& Made, TArray<FFormMadePiece>& OutPieces)
		{
			if (Drawing.Repeats.IsEmpty())
			{
				OutPieces.Append(MoveTemp(Made));
				return;
			}
			const FString Name = Drawing.Name.ToString();
			for (int32 Index = 0; Index < Drawing.Repeats.Num(); ++Index)
			{
				for (const FFormMadePiece& Each : Made)
				{
					FFormMadePiece& Member = OutPieces.Add_GetRef(Each);
					Member.Name = FName(FString::Printf(TEXT("%s.%d%s"), *Name, Index + 1, *Each.Name.ToString().RightChop(Name.Len())));
					Member.Frame = Drawing.Frame.FrameToParent(Drawing.Repeats[Index]);
				}
			}
		}
	}

	void Make(const FFormDrawing& Given, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FFormDrawing Drawing = WithDomeSurfaces(Given);
		FMeshBuilder Mesh;
		FMeshBuilder Second;
		Musee::Form::Drawing::FLaidTally Tally;
		FString Why;
		if (Drawing.Name.IsNone() || Drawing.Surface.IsNone())
		{
			OutFindings.Add(Unmade(Drawing, TEXT("a drawing has a name and a surface")));
			return;
		}
		const FFormLaid* Laid = Drawing.Shape.TryGet<FFormLaid>();
		if (Laid && Laid->FillMeters >= 0.0 && !Drawing.PartSurfaces.Contains(FFormLaid::JointPart))
		{
			OutFindings.Add(Unmade(Drawing, TEXT("filled joints are finished in their own surface (the part joint)")));
			return;
		}
		if (!DrawShape(Drawing, Mesh, Second, Tally, Why) || !CutAll(Drawing, Mesh, Second, Why)
			|| !CarryToGround(Drawing, Mesh, Why) || !CarryToGround(Drawing, Second, Why))
		{
			OutFindings.Add(Unmade(Drawing, Why));
			return;
		}
		if (Mesh.NumTriangles() == 0)
		{
			OutFindings.Add(Unmade(Drawing, TEXT("it draws no surface")));
			return;
		}
		if (const int32 Unfaced = Mesh.NumUnfacedVertices() + Second.NumUnfacedVertices())
		{
			OutFindings.Add(Unmade(Drawing, FString::Printf(
				TEXT("%d of its vertices face no side: an edge of its outline or section doubles back on itself or has no length"), Unfaced)));
			return;
		}
		if (const FFormStair* Stair = Drawing.Shape.TryGet<FFormStair>())
		{
			CheckSteps(Drawing, *Stair, OutFindings);
		}
		if (Laid)
		{
			CheckStock(Drawing, Tally, OutFindings);
		}
		TArray<FFormMadePiece> Made;
		Made.Add(Piece(Drawing, Drawing.Name, Mesh, NAME_None));
		if (Second.NumTriangles() > 0)
		{
			AddSecond(Drawing, Second, Made);
		}
		Repeat(Drawing, Made, OutPieces);
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
