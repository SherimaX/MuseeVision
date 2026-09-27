#include "Frame/Hooks.h"

#include "Algo/Find.h"
#include "Canvas/CanvasMesh.h"
#include "FormSweep.h"
#include "MuseeRecords.h"
#include "Objects/ObjectMaking.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** How close a place may come to a wire run's end, or a height to a wire, and still count as on it (metres). */
		constexpr double PlaceTolerance = 1e-9;

		/**
		 * The hooks' section, in the wall's (out, up), from the anchor's plane out: the eye's ring touches the frame's back
		 * and lies level; its far wire runs through the hook's lower bend, with play all round; the shank rises straight
		 * from that bend to the upper bend, which rests over the level wire with play at its top and turns down behind it.
		 */
		struct FHookSection
		{
			/** The wires' radii: the hook's, the eye's, the mesh's; and the play where they link. */
			double Hook = 0.0;
			double Eye = 0.0;
			double Wire = 0.0;
			double Play = 0.0;
			/** The eye's ring: its centreline's radius, and its centre out of the plane. */
			double RingRadius = 0.0;
			double RingOut = 0.0;
			/** The lower bend's centreline radius and its centre out of the plane (the ring's far wire's). */
			double LowerRadius = 0.0;
			double LowerOut = 0.0;
			/** The shank's centreline out of the plane, and the level wire's centre (behind it). */
			double ShankOut = 0.0;
			double WireOut = 0.0;
			/** The upper bend's centreline radius, about the level wire's line. */
			double UpperRadius = 0.0;

			FHookSection(const FArtHooks& Hooks, const FArtMesh& Mesh)
				: Hook(0.5 * Hooks.HookWireMeters)
				, Eye(0.5 * Hooks.EyeWireMeters)
				, Wire(0.5 * Mesh.Wire)
				, Play(Hooks.PlayMeters)
			{
				RingRadius = 0.5 * Hooks.EyeInsideMeters + Eye;
				RingOut = HangingGapMeters - Eye - RingRadius;
				LowerOut = RingOut - RingRadius;
				LowerRadius = Eye + Hook + Play;
				ShankOut = LowerOut - LowerRadius;
				WireOut = -Mesh.Behind;
				UpperRadius = ShankOut - WireOut;
			}

			/** The least the upper bend's radius may be: round the level wire, with play. */
			[[nodiscard]] double LeastUpperRadius() const { return Wire + Hook + Play; }

			/** The upper bend's centre's height over a level wire at this height: its inside rests over the wire, with play. */
			[[nodiscard]] double UpperCentre(double WireHeight) const { return WireHeight + Wire + Hook + Play - UpperRadius; }
		};

		/** The place halfway between two uprights nearest a place along the anchor. */
		double BetweenUprights(const FArtWireRun& Uprights, double Along)
		{
			const double Pitch = Uprights.Pitch;
			return Uprights.From + (FMath::RoundToDouble((Along - Uprights.From) / Pitch - 0.5) + 0.5) * Pitch;
		}

		FFormProfile RoundWire(double Radius)
		{
			return FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, 0.0, true);
		}

		FFormDrawing HookDrawing(FName Name, FName Surface, FFormSweep&& Shape, const FMuseeFrame& Frame, TArray<FMuseeFrame> Repeats)
		{
			FFormDrawing Drawing = FFormDrawing::Of(Name, Surface, MoveTemp(Shape));
			Drawing.Frame = Frame;
			Drawing.Repeats = MoveTemp(Repeats);
			Drawing.Drawer = TEXT("Art");
			return Drawing;
		}
	}

	const TValueOrError<FArtHooks, FString>& FObjectMaking::Hooks()
	{
		if (!HookRecord.IsSet())
		{
			HookRecord.Emplace(ReadRecord<FArtHooks>(FArtHooks::Record));
		}
		return HookRecord.GetValue();
	}

	const FArtMesh* MeshOf(TConstArrayView<FArtMesh> Meshes, const FMuseeKey& Anchor)
	{
		return Algo::FindByPredicate(Meshes, [&Anchor](const FArtMesh& Mesh) { return Mesh.Anchor == Anchor; });
	}

	TArray<FString> MeshFaults(const FArtMesh& Mesh)
	{
		TArray<FString> Out;
		for (const FArtWireRun* Run : { &Mesh.Levels, &Mesh.Uprights })
		{
			if (Run->Pitch <= 0.0 || Run->To < Run->From)
			{
				Out.Add(TEXT("its mesh's wires run from the first to the last at a pitch of more than zero"));
				break;
			}
		}
		if (Mesh.Wire <= 0.0 || Mesh.Behind < 0.5 * Mesh.Wire)
		{
			Out.Add(TEXT("its mesh's wire has a diameter, its level wires behind the anchor's plane"));
		}
		return Out;
	}

	TValueOrError<FHookPlan, FString> PlanHooks(const FArtHooks& Hooks, const FArtMesh& Mesh, const FArtMoulding& Moulding, const FArtHangEntry& Entry)
	{
		const FHookSection Section(Hooks, Mesh);
		if (Section.UpperRadius < Section.LeastUpperRadius())
		{
			return MakeError(FString::Printf(TEXT("its mesh's level wires lie %.1f mm behind its anchor's plane, too close behind its frame for its "
				"hooks to pass round them (they need %.1f)"), 1000.0 * Mesh.Behind, 1000.0 * (Section.LeastUpperRadius() - Section.ShankOut)));
		}
		const FVector2D Size = Entry.SizeMeters();
		const double Outside = Moulding.Face.StartPoint().X * Moulding.WidthFor(Size);
		const double Top = Entry.Height + Size.Y + Outside;
		FHookPlan Plan;
		Plan.EyeHeight = Top - Hooks.EyeDropShare * (Size.Y + 2.0 * Outside);

		// The lowest level wire whose upper bend leaves the hook its least shank above the eye.
		const double Least = Plan.EyeHeight + Hooks.LeastShankMeters - Section.UpperCentre(0.0);
		const FArtWireRun& Levels = Mesh.Levels;
		const double Steps = FMath::Max(0.0, FMath::CeilToDouble((Least - Levels.From) / Levels.Pitch - PlaceTolerance));
		Plan.WireHeight = Levels.From + Steps * Levels.Pitch;
		if (Plan.WireHeight > Levels.To + PlaceTolerance)
		{
			return MakeError(FString::Printf(TEXT("no level wire of its mesh is %.3f m up or more, over its frame's eyes"), Least));
		}

		// Each eye on the middle of its side's back, from the rebate to the outside edge, moved to the nearest gap between
		// uprights; the ring stays on the back.
		const double Ring = Section.RingRadius + Section.Eye;
		const double Middle = 0.5 * (Moulding.RebateClearanceMeters + Outside);
		const double Edges[2] = { Entry.Along, Entry.Along + Size.X };
		const double Outward[2] = { -1.0, 1.0 };
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double Along = BetweenUprights(Mesh.Uprights, Edges[Side] + Outward[Side] * Middle);
			const double In = Outward[Side] * (Along - Edges[Side]);
			const bool bOnBack = In - Ring >= Moulding.RebateClearanceMeters - PlaceTolerance && In + Ring <= Outside + PlaceTolerance;
			const bool bOnMesh = Along >= Mesh.Uprights.From - PlaceTolerance && Along <= Mesh.Uprights.To + PlaceTolerance;
			if (!bOnBack || !bOnMesh)
			{
				return MakeError(FString::Printf(TEXT("its frame's %s eye finds no gap between its mesh's uprights on the frame's back"),
					Side == 0 ? TEXT("left") : TEXT("right")));
			}
			Plan.Along[Side] = Along;
		}
		return MakeValue(Plan);
	}

	TArray<FFormDrawing> HookDrawings(const FHookPlan& Plan, const FArtHooks& Hooks, const FArtMesh& Mesh, const FMuseeFrame& Anchor, FName HookName,
		FName EyeName)
	{
		const FHookSection Section(Hooks, Mesh);
		const FVector Along = Anchor.DirectionToParent(FVector::ForwardVector);
		const FVector Out = Anchor.DirectionToParent(FVector::RightVector);
		const FVector Up = Anchor.DirectionToParent(FVector::UpVector);
		const double Upper = Section.UpperCentre(Plan.WireHeight);
		const double Eye = Plan.EyeHeight;

		// The S-hook in the wall's (out, up), its drawing's x and y; its z runs along the wall, where the two stand.
		FFormSweep S;
		S.Section = RoundWire(Section.Hook);
		S.Path = FFormProfile::From(FVector2D(Section.WireOut - Section.UpperRadius, Upper - Hooks.EndMeters));
		S.Path.LineTo(FVector2D(Section.WireOut - Section.UpperRadius, Upper), EFormJoint::Smooth)
			.ArcAbout(FVector2D(Section.WireOut, Upper), Section.UpperRadius, Section.UpperRadius, 180.0, 0.0, EFormJoint::Smooth)
			.LineTo(FVector2D(Section.ShankOut, Eye), EFormJoint::Smooth)
			.ArcAbout(FVector2D(Section.LowerOut, Eye), Section.LowerRadius, Section.LowerRadius, 180.0, 360.0, EFormJoint::Smooth)
			.LineTo(FVector2D(Section.LowerOut + Section.LowerRadius, Eye + Hooks.EndMeters), EFormJoint::Smooth);
		const FMuseeFrame Hung = FrameAlong(Anchor.OriginMeters, Out, Up);

		// The eye's ring, level, in the wall's (along, out).
		FFormSweep Ring;
		Ring.Section = RoundWire(Section.Eye);
		Ring.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, Section.RingRadius, Section.RingRadius, 0.0, true);
		const FMuseeFrame Level = FrameAlong(Anchor.OriginMeters + Out * Section.RingOut + Up * Eye, Along, Out);

		TArray<FMuseeFrame> Hooked, Eyed;
		for (const double At : Plan.Along)
		{
			Hooked.Add(FMuseeFrame(FVector(0.0, 0.0, At)));
			Eyed.Add(FMuseeFrame(FVector(At, 0.0, 0.0)));
		}
		TArray<FFormDrawing> Drawings;
		Drawings.Add(HookDrawing(HookName, Mesh.Hooks, MoveTemp(S), Hung, MoveTemp(Hooked)));
		Drawings.Add(HookDrawing(EyeName, Mesh.Hooks, MoveTemp(Ring), Level, MoveTemp(Eyed)));
		return Drawings;
	}
}
