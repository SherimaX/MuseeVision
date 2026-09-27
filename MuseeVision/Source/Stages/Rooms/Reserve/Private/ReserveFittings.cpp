#include "ReservePlan.h"
#include "ReserveParts.h"

/**
 * The fittings (§3.9): the oak viewing easel at the nave's east end with its bronze caps and ledge brackets, on which
 * Le Pont de l'Europe stands; and the four islands of fumed oak plan chests in bays 7 and 8, two chests back to back on a
 * bronze plinth under one top, ten drawers to a face, with the felt the Degas pastels lie on over the two south ones.
 */
namespace Musee::Reserve
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		void Easel(TArray<FFormDrawing>& Out)
		{
			for (const Plan::FEaselPart& Part : Plan::EaselParts)
			{
				const double* B = Part.Box;
				const FString Name = FString::Printf(TEXT("easel.%s"), Part.Name);
				if (!Part.bMirrored)
				{
					Out.Add(Drawn(Name, Part.Surface, Block(FVector(B[0], B[2], B[4]), FVector(B[1], B[3], B[5])), Solid));
					continue;
				}
				Out.Add(Drawn(Name + TEXT(".north"), Part.Surface, Block(FVector(B[0], -B[3], B[4]), FVector(B[1], -B[2], B[5])), Solid));
				Out.Add(Drawn(Name + TEXT(".south"), Part.Surface, Block(FVector(B[0], B[2], B[4]), FVector(B[1], B[3], B[5])), Solid));
			}
		}

		/**
		 * One chest of an island, drawn in the frame of its drawer face (x along the face, y up, z into the chest): the
		 * carcass with a housing sunk for each drawer front, the ten fronts set in them and standing proud, and their pulls.
		 */
		void Chest(TArray<FFormDrawing>& Out, const FString& Name, const FVector2D& Island, double Face)
		{
			const FMuseeFrame Frame = FrameOfAxes(FVector(Island.X, Island.Y + Face * Plan::ChestHalf[1], 0.0), FVector(Face, 0.0, 0.0), FVector::UpVector);
			const double Sunk = Plan::DrawerThickness - Plan::DrawerProud;
			FFormExtrusion Carcass;
			Carcass.Outline = FFormProfile::Rectangle(FVector2D(-Plan::ChestHalf[0], Plan::ChestPlinthTop), FVector2D(Plan::ChestHalf[0], Plan::ChestCarcassTop));
			Carcass.DepthMeters = Plan::ChestHalf[1];
			TArray<FMuseeFrame> Fronts;
			TArray<FMuseeFrame> Pulls;
			for (int32 Column = 0; Column < 2; ++Column)
			{
				for (int32 Row = 0; Row < Plan::DrawerRows; ++Row)
				{
					const double X = Plan::DrawerFromX + Column * (Plan::DrawerWidth + Plan::DrawerGap);
					const double Z = Plan::DrawerFromZ + Row * (Plan::DrawerHeight + Plan::DrawerGap);
					FFormPocket& Housing = Carcass.Pockets.AddDefaulted_GetRef();
					Housing.Outline = FFormProfile::Rectangle(FVector2D(X, Z), FVector2D(X + Plan::DrawerWidth, Z + Plan::DrawerHeight));
					Housing.DepthMeters = Sunk;
					Housing.bNearFace = true;
					Fronts.Add(Frame.FrameToParent(FMuseeFrame(FVector(X, Z, -Plan::DrawerProud))));
					Pulls.Add(Frame.FrameToParent(FMuseeFrame(FVector(X + 0.5 * (Plan::DrawerWidth - Plan::ChestPullLength),
						Z + Plan::ChestPullAt * Plan::DrawerHeight - 0.5 * Plan::ChestPullHeight, -Plan::DrawerProud - Plan::ChestPullProud))));
				}
			}
			Out.Add(Drawn(Name, TEXT("chest"), MoveTemp(Carcass), Solid, Frame));
			FFormDrawing& Drawers = Out.Add_GetRef(Drawn(Name + TEXT(".drawer"), TEXT("chest"), Block(FVector::ZeroVector, FVector(Plan::DrawerWidth,
				Plan::DrawerHeight, Plan::DrawerThickness)), Solid));
			Drawers.Repeats = MoveTemp(Fronts);
			FFormDrawing& Handles = Out.Add_GetRef(Drawn(Name + TEXT(".pull"), TEXT("chest.pull"), Block(FVector::ZeroVector, FVector(Plan::ChestPullLength,
				Plan::ChestPullHeight, Plan::ChestPullProud)), nullptr));
			Handles.Repeats = MoveTemp(Pulls);
		}

		void Island(TArray<FFormDrawing>& Out, int32 Bay, double Side)
		{
			const FVector2D At(Plan::ChestXs[Bay], Side * Plan::ChestY);
			const FString Name = FString::Printf(TEXT("chest.bay%d-%s"), Bay + 7, Side < 0.0 ? TEXT("north") : TEXT("south"));
			Out.Add(Drawn(Name + TEXT(".plinth"), TEXT("chest.plinth"), Block(FVector(At.X - Plan::ChestPlinthHalf[0], At.Y - Plan::ChestPlinthHalf[1], 0.0),
				FVector(At.X + Plan::ChestPlinthHalf[0], At.Y + Plan::ChestPlinthHalf[1], Plan::ChestPlinthTop)), Solid));
			Chest(Out, Name + TEXT(".north"), At, -1.0);
			Chest(Out, Name + TEXT(".south"), At, 1.0);
			Out.Add(Drawn(Name + TEXT(".top"), TEXT("chest"), Block(FVector(At.X - Plan::ChestTopHalf[0], At.Y - Plan::ChestTopHalf[1], Plan::ChestCarcassTop),
				FVector(At.X + Plan::ChestTopHalf[0], At.Y + Plan::ChestTopHalf[1], Plan::ChestTop)), Solid));
			if (Side > 0.0)
			{
				const double HalfX = 0.5 * Plan::FeltSizes[Bay][0];
				const double HalfY = 0.5 * Plan::FeltSizes[Bay][1];
				Out.Add(Drawn(Name + TEXT(".felt"), TEXT("chest.mat"), Block(FVector(At.X - HalfX, At.Y - HalfY, Plan::ChestTop),
					FVector(At.X + HalfX, At.Y + HalfY, Plan::ChestTop + Plan::FeltThickness)), nullptr));
			}
		}
	}

	void DrawFittings(TArray<FFormDrawing>& Out)
	{
		Easel(Out);
		for (int32 Bay = 0; Bay < int32(UE_ARRAY_COUNT(Plan::ChestXs)); ++Bay)
		{
			for (const double Side : {-1.0, 1.0})
			{
				Island(Out, Bay, Side);
			}
		}
	}
}
