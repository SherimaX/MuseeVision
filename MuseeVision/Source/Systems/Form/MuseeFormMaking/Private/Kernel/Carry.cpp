#include "Kernel/Carry.h"

#include "Kernel/MeshBuilder.h"
#include "Kernel/Triangulation.h"

namespace Musee::Form
{
	namespace
	{
		/** The step of the central differences that measure the map's Jacobian: a tenth of a millimetre. */
		constexpr double JacobianStepMeters = 1.0e-4;

		/**
		 * The most parts an edge is divided into. A ground that needs more bends within a thousandth of the edge, where
		 * the development and the ground don't belong together; the edge is left at this many.
		 */
		constexpr int32 MostDivisions = 1024;

		/** Lattice points keep at least half a spacing from every edge, so no sliver lies along a boundary. */
		constexpr double LatticeClearance = 0.5;

		/** The order an edge's points are taken in, whichever triangle asks, so the edge is divided alike on both its sides. */
		bool IsLower(const FVector& A, const FVector& B)
		{
			return A.X != B.X ? A.X < B.X : (A.Y != B.Y ? A.Y < B.Y : A.Z < B.Z);
		}

		/** A point of the development with its attributes, as the builder keeps them for a vertex. */
		struct FDevelopedPoint
		{
			FVector Position = FVector::ZeroVector;
			FVector Normal = FVector::ZeroVector;
			TArray<FVector2D, TInlineAllocator<FMeshBuilder::MaxUVChannels>> UVs;
			FLinearColor Color = FLinearColor::White;
		};

		/** A weighted mix of a mesh's vertices: a point along an edge, or within a triangle, with its attributes. */
		FDevelopedPoint Mix(const FMeshBuilder& Mesh, TConstArrayView<int32> Vertices, TConstArrayView<double> Weights)
		{
			FDevelopedPoint Out;
			Out.UVs.SetNumZeroed(Mesh.NumUVChannels());
			FLinearColor Color(0.0f, 0.0f, 0.0f, 0.0f);
			for (int32 Index = 0; Index < Vertices.Num(); ++Index)
			{
				const int32 Vertex = Vertices[Index];
				const double Weight = Weights[Index];
				Out.Position += Mesh.Position(Vertex) * Weight;
				Out.Normal += Mesh.Normal(Vertex) * Weight;
				for (int32 Channel = 0; Channel < Mesh.NumUVChannels(); ++Channel)
				{
					Out.UVs[Channel] += Mesh.UV(Vertex, Channel) * Weight;
				}
				if (Mesh.HasColors())
				{
					Color += Mesh.Color(Vertex) * float(Weight);
				}
			}
			if (!Out.Normal.Normalize())
			{
				Out.Normal = Mesh.Normal(Vertices[0]);
			}
			Out.Color = Mesh.HasColors() ? Color : FLinearColor::White;
			return Out;
		}

		/** Divides and carries a developed mesh, triangle by triangle, sharing what the triangles share. */
		class FCarrier
		{
		public:
			FCarrier(const FMeshBuilder& InDeveloped, TFunctionRef<FVector(const FVector&)> InMap, double InTolerance, FMeshBuilder& InOut)
				: Developed(InDeveloped), Map(InMap), Tolerance(InTolerance), Out(InOut)
			{
			}

			void CarryTriangle(int32 Triangle)
			{
				const FIntVector& T = Developed.Triangle(Triangle);
				const int32 Corners[3] = {T.X, T.Y, T.Z};
				Out.UseSlot(Developed.Slots()[Developed.TriangleSlot(Triangle)]);
				int32 Parts[3];
				bool bDivided = false;
				for (int32 Edge = 0; Edge < 3; ++Edge)
				{
					int32 Low, High;
					Order(Corners[Edge], Corners[(Edge + 1) % 3], Low, High);
					Parts[Edge] = Divisions(At(Low), At(High));
					bDivided |= Parts[Edge] > 1;
				}
				const FVector Centroid = (At(Corners[0]) + At(Corners[1]) + At(Corners[2])) / 3.0;
				const FVector CarriedCentroid = (Map(At(Corners[0])) + Map(At(Corners[1])) + Map(At(Corners[2]))) / 3.0;
				const double MiddleStray = FVector::Distance(Map(Centroid), CarriedCentroid);
				if (!bDivided && MiddleStray <= Tolerance)
				{
					Out.AddTriangle(Carried(Corners[0]), Carried(Corners[1]), Carried(Corners[2]));
					return;
				}
				Refill(Corners, Parts, MiddleStray);
			}

		private:
			const FMeshBuilder& Developed;
			TFunctionRef<FVector(const FVector&)> Map;
			double Tolerance;
			FMeshBuilder& Out;
			TMap<int32, int32> CarriedVertices;
			TMap<FIntVector, int32> EdgePoints;

			const FVector& At(int32 Vertex) const { return Developed.Position(Vertex); }

			void Order(int32 A, int32 B, int32& OutLow, int32& OutHigh) const
			{
				const bool bForward = IsLower(At(A), At(B));
				OutLow = bForward ? A : B;
				OutHigh = bForward ? B : A;
			}

			/** How far the carried middle of a part of an edge strays from its carried chord's middle. */
			double Stray(const FVector& Low, const FVector& High, double From, double To) const
			{
				const FVector A = Map(FMath::Lerp(Low, High, From));
				const FVector B = Map(FMath::Lerp(Low, High, To));
				const FVector Middle = Map(FMath::Lerp(Low, High, 0.5 * (From + To)));
				return FVector::Distance(Middle, 0.5 * (A + B));
			}

			/** The fewest equal parts of an edge, taken lower point first, whose carried chords keep within the tolerance. */
			int32 Divisions(const FVector& Low, const FVector& High) const
			{
				// A chord's stray falls with the square of its length: start from the whole edge's, then check every part.
				int32 Parts = FMath::Clamp(FMath::CeilToInt32(FMath::Sqrt(Stray(Low, High, 0.0, 1.0) / Tolerance)), 1, MostDivisions);
				while (Parts < MostDivisions)
				{
					bool bWithin = true;
					for (int32 Part = 0; Part < Parts && bWithin; ++Part)
					{
						bWithin = Stray(Low, High, double(Part) / Parts, double(Part + 1) / Parts) <= Tolerance;
					}
					if (bWithin)
					{
						break;
					}
					Parts = FMath::Min(MostDivisions, Parts + FMath::Max(1, Parts / 2));
				}
				return Parts;
			}

			/** The normal carried by the map's Jacobian J at a point: J^-T n, turned over where the map turns space over. */
			FVector CarriedNormal(const FVector& Position, const FVector& Normal) const
			{
				const double H = JacobianStepMeters;
				const FVector Xu = (Map(Position + FVector(H, 0.0, 0.0)) - Map(Position - FVector(H, 0.0, 0.0))) / (2.0 * H);
				const FVector Xv = (Map(Position + FVector(0.0, H, 0.0)) - Map(Position - FVector(0.0, H, 0.0))) / (2.0 * H);
				const FVector Xw = (Map(Position + FVector(0.0, 0.0, H)) - Map(Position - FVector(0.0, 0.0, H))) / (2.0 * H);
				const FVector Carried = FVector::CrossProduct(Xv, Xw) * Normal.X + FVector::CrossProduct(Xw, Xu) * Normal.Y
					+ FVector::CrossProduct(Xu, Xv) * Normal.Z;
				const double Turn = FVector::DotProduct(Xu, FVector::CrossProduct(Xv, Xw));
				return (Turn < 0.0 ? -Carried : Carried).GetSafeNormal();
			}

			int32 Add(const FDevelopedPoint& Point)
			{
				const int32 Vertex = Out.AddVertex(Map(Point.Position), CarriedNormal(Point.Position, Point.Normal), Point.UVs[0]);
				for (int32 Channel = 1; Channel < Point.UVs.Num() && Channel < Out.NumUVChannels(); ++Channel)
				{
					Out.SetUV(Vertex, Channel, Point.UVs[Channel]);
				}
				if (Developed.HasColors())
				{
					Out.SetColor(Vertex, Point.Color);
				}
				return Vertex;
			}

			int32 Carried(int32 Vertex)
			{
				if (const int32* Found = CarriedVertices.Find(Vertex))
				{
					return *Found;
				}
				const int32 Corner[1] = {Vertex};
				const double Whole[1] = {1.0};
				return CarriedVertices.Add(Vertex, Add(Mix(Developed, Corner, Whole)));
			}

			/** Part Part of Parts along the edge from Low to High, shared by every triangle on the same two vertices. */
			int32 EdgePoint(int32 Low, int32 High, int32 Part, int32 Parts, FVector& OutPosition)
			{
				const double Along = double(Part) / Parts;
				const int32 Ends[2] = {Low, High};
				const double Weights[2] = {1.0 - Along, Along};
				const FDevelopedPoint Point = Mix(Developed, Ends, Weights);
				OutPosition = Point.Position;
				if (const int32* Found = EdgePoints.Find(FIntVector(Low, High, Part)))
				{
					return *Found;
				}
				return EdgePoints.Add(FIntVector(Low, High, Part), Add(Point));
			}

			/** A divided triangle: its edges' parts round it, a lattice within, triangulated afresh in its own plane. */
			void Refill(const int32 (&Corners)[3], const int32 (&Parts)[3], double MiddleStray)
			{
				TArray<int32> Loop;
				TArray<FVector> LoopAt;
				double Spacing = TNumericLimits<double>::Max();
				double Longest = 0.0;
				for (int32 Edge = 0; Edge < 3; ++Edge)
				{
					const int32 From = Corners[Edge];
					const int32 To = Corners[(Edge + 1) % 3];
					int32 Low, High;
					Order(From, To, Low, High);
					Loop.Add(Carried(From));
					LoopAt.Add(At(From));
					for (int32 Step = 1; Step < Parts[Edge]; ++Step)
					{
						FVector Position;
						Loop.Add(EdgePoint(Low, High, Low == From ? Step : Parts[Edge] - Step, Parts[Edge], Position));
						LoopAt.Add(Position);
					}
					const double Length = FVector::Distance(At(From), At(To));
					Spacing = FMath::Min(Spacing, Length / Parts[Edge]);
					Longest = FMath::Max(Longest, Length);
				}
				if (MiddleStray > Tolerance)
				{
					Spacing = FMath::Min(Spacing, Longest / (FMath::CeilToDouble(FMath::Sqrt(MiddleStray / Tolerance)) + 1.0));
				}
				const FVector Origin = At(Corners[0]);
				const FVector Plane = FVector::CrossProduct(At(Corners[1]) - Origin, At(Corners[2]) - Origin);
				const FVector AxisU = (At(Corners[1]) - Origin).GetSafeNormal();
				const FVector AxisV = FVector::CrossProduct(Plane, AxisU).GetSafeNormal();
				if (AxisU.IsNearlyZero() || AxisV.IsNearlyZero())
				{
					Out.AddTriangle(Carried(Corners[0]), Carried(Corners[1]), Carried(Corners[2]));
					return;
				}
				auto Flat = [&](const FVector& P) { return FVector2D(FVector::DotProduct(P - Origin, AxisU), FVector::DotProduct(P - Origin, AxisV)); };
				TArray<FVector2D> Outline;
				for (const FVector& P : LoopAt)
				{
					Outline.Add(Flat(P));
				}
				const FVector2D Flats[3] = {Flat(At(Corners[0])), Flat(At(Corners[1])), Flat(At(Corners[2]))};
				TArray<FVector2D> Inner;
				TArray<FVector> Barycentric;
				Lattice(Flats, Spacing, Inner, Barycentric);
				TArray<FIntVector> Triangles;
				if (!TriangulatePolygonWithPoints(Outline, {}, Inner, Triangles))
				{
					// A lattice point too near a boundary to triangulate round: the boundary alone still meets its neighbours.
					Inner.Reset();
					Barycentric.Reset();
					if (!TriangulatePolygon(Outline, {}, Triangles))
					{
						Out.AddTriangle(Carried(Corners[0]), Carried(Corners[1]), Carried(Corners[2]));
						return;
					}
				}
				TArray<int32> Vertices = Loop;
				for (const FVector& Weights : Barycentric)
				{
					const double W[3] = {Weights.X, Weights.Y, Weights.Z};
					Vertices.Add(Add(Mix(Developed, Corners, W)));
				}
				for (const FIntVector& Triangle : Triangles)
				{
					Out.AddTriangle(Vertices[Triangle.X], Vertices[Triangle.Y], Vertices[Triangle.Z]);
				}
			}

			/**
			 * A triangular lattice of the spacing within a flat triangle, clear of its edges, with each point's barycentric
			 * weights over the three corners.
			 */
			static void Lattice(const FVector2D (&Corners)[3], double Spacing, TArray<FVector2D>& OutPoints, TArray<FVector>& OutWeights)
			{
				const double Area = FVector2D::CrossProduct(Corners[1] - Corners[0], Corners[2] - Corners[0]);
				if (FMath::Abs(Area) <= 0.0 || Spacing <= 0.0)
				{
					return;
				}
				const FVector2D Low(FMath::Min3(Corners[0].X, Corners[1].X, Corners[2].X), FMath::Min3(Corners[0].Y, Corners[1].Y, Corners[2].Y));
				const FVector2D High(FMath::Max3(Corners[0].X, Corners[1].X, Corners[2].X), FMath::Max3(Corners[0].Y, Corners[1].Y, Corners[2].Y));
				const double Row = Spacing * FMath::Sqrt(3.0) * 0.5;
				const int32 Rows = FMath::FloorToInt32((High.Y - Low.Y) / Row);
				const int32 Columns = FMath::FloorToInt32((High.X - Low.X) / Spacing);
				for (int32 J = 1; J <= Rows; ++J)
				{
					for (int32 I = 0; I <= Columns; ++I)
					{
						const FVector2D P(Low.X + Spacing * (I + ((J % 2) ? 0.5 : 0.0)), Low.Y + Row * J);
						FVector Weights;
						double Clear = TNumericLimits<double>::Max();
						for (int32 Edge = 0; Edge < 3; ++Edge)
						{
							const FVector2D& A = Corners[(Edge + 1) % 3];
							const FVector2D& B = Corners[(Edge + 2) % 3];
							const double Sub = FVector2D::CrossProduct(B - A, P - A) / Area;
							Weights[Edge] = Sub;
							Clear = FMath::Min(Clear, Sub * FMath::Abs(Area) / FVector2D::Distance(A, B));
						}
						if (Clear >= LatticeClearance * Spacing)
						{
							OutPoints.Add(P);
							OutWeights.Add(Weights);
						}
					}
				}
			}
		};
	}

	void CarryOntoGround(const FMeshBuilder& Developed, TFunctionRef<FVector(const FVector&)> Map, double ToleranceMeters, FMeshBuilder& Out)
	{
		FCarrier Carrier(Developed, Map, ToleranceMeters, Out);
		for (int32 Triangle = 0; Triangle < Developed.NumTriangles(); ++Triangle)
		{
			Carrier.CarryTriangle(Triangle);
		}
	}
}
