#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The piers and the transverse arches over them (§3.2): four pairs of travertine piers standing out from the long
 * walls to the jambs, each with its base (P3) and impost (P4) run round its three faces and its side faces silked in
 * the colour of the bay it looks into; over each pair an arch of fifteen voussoirs with a keystone standing proud to a
 * flat top, the coursed spandrel between the arch and the vault, and the archivolt (P7) round the extrados on both
 * faces, stopped against the keystone.
 */
namespace Musee::Salon
{
	namespace
	{
		/**
		 * The pier's three faces as a path walked with the pier on the left, from the wall out along one side, across the
		 * jamb, back along the other, so a moulding swept on it stands out of the stone.
		 */
		FFormProfile PierFaces(double X, double Sign)
		{
			const double W = X - Plan::PierHalf;
			const double E = X + Plan::PierHalf;
			const double Wall = Sign * Plan::MasonryHalf;
			const double Jamb = Sign * Plan::JambHalf;
			const TArray<FVector2D> Path = Sign < 0.0
				? TArray<FVector2D>{{W, Wall}, {W, Jamb}, {E, Jamb}, {E, Wall}}
				: TArray<FVector2D>{{E, Wall}, {E, Jamb}, {W, Jamb}, {W, Wall}};
			return FFormProfile::Polyline(Path, false);
		}

		/** One pier: the stone, its base and impost, and the silk on its two side faces. */
		void Pier(TArray<FFormDrawing>& Out, int32 Index, double Sign)
		{
			const double X = Plan::PierX[Index];
			const FString Name = FString::Printf(TEXT("pier.%d.%s"), Index + 1, Sign < 0.0 ? TEXT("north") : TEXT("south"));
			const double Wall = Sign * Plan::MasonryHalf;
			const double Jamb = Sign * Plan::JambHalf;
			Out.Add(Box(Name, TEXT("pier"), FVector(X - Plan::PierHalf, FMath::Min(Wall, Jamb), Plan::WallBed),
				FVector(X + Plan::PierHalf, FMath::Max(Wall, Jamb), Plan::PierTop), Solid));

			const TPair<const TCHAR*, TConstArrayView<Plan::FStep>> Mouldings[] = {{TEXT("base"), Plan::PierBase}, {TEXT("impost"), Plan::PierImpost}};
			for (const TPair<const TCHAR*, TConstArrayView<Plan::FStep>>& Moulding : Mouldings)
			{
				FFormSweep Run;
				Run.Section = Section(Moulding.Value);
				Run.Path = PierFaces(X, Sign);
				Out.Add(Drawn(Name + TEXT(".") + Moulding.Key, *FString::Printf(TEXT("pier.%s"), Moulding.Key), MoveTemp(Run), Solid));
			}

			// Each side face is silked in the colour of the bay it looks into: the east face bay Index + 1, the west Index + 2.
			const double SilkJamb = Sign * Plan::PierSilkJamb;
			const double Y0 = FMath::Min(Wall, SilkJamb);
			const double Y1 = FMath::Max(Wall, SilkJamb);
			const double East = X + Plan::PierHalf;
			const double West = X - Plan::PierHalf;
			Out.Add(Box(Name + TEXT(".silk.east"), *FString::Printf(TEXT("bay%d.silk"), Index + 1), FVector(East, Y0, Plan::PierSilkBottom),
				FVector(East + Plan::PierSilkProud, Y1, Plan::PierSilkTop), Solid));
			Out.Add(Box(Name + TEXT(".silk.west"), *FString::Printf(TEXT("bay%d.silk"), Index + 2), FVector(West - Plan::PierSilkProud, Y0, Plan::PierSilkBottom),
				FVector(West, Y1, Plan::PierSilkTop), Solid));
		}

		/** A point of the arch's face at an angle (degrees, standard in the face: 0 south, 90 up) and radius. */
		FVector2D OnArch(double Degrees, double Radius)
		{
			const double T = FMath::DegreesToRadians(Degrees);
			return FVector2D(Radius * FMath::Cos(T), Radius * FMath::Sin(T));
		}

		/**
		 * The arch's stones, each its own block between radial joints, drawn on the arch's west face (x south, y up from
		 * the springing's centre) and pushed through its thickness; the keystone is thicker by its proud faces and rises to
		 * its flat top.
		 */
		void Voussoirs(TArray<FFormDrawing>& Out, int32 Index)
		{
			const double X = Plan::PierX[Index];
			const double Step = 180.0 / Plan::Voussoirs;
			for (int32 Stone = 0; Stone < Plan::Voussoirs; ++Stone)
			{
				const double From = 180.0 - Step * (Stone + 1);
				const double To = 180.0 - Step * Stone;
				if (Stone == Plan::Keystone)
				{
					const double Top = Plan::KeystoneTop - Plan::Springing;
					const double RadiusAtTop = Top / FMath::Sin(FMath::DegreesToRadians(To));
					FFormProfile Key = FFormProfile::From(OnArch(From, Plan::Intrados));
					Key.ArcAbout(FVector2D::ZeroVector, Plan::Intrados, Plan::Intrados, From, To, EFormJoint::Crisp)
						.LineTo(OnArch(To, RadiusAtTop), EFormJoint::Crisp).LineTo(OnArch(From, RadiusAtTop), EFormJoint::Crisp);
					const double Proud = Plan::PierHalf + Plan::KeystoneProud;
					Out.Add(FaceSlab(FString::Printf(TEXT("arch.%d.keystone"), Index + 1), TEXT("arch.keystone"),
						FaceFrame(FVector(X - Proud, 0.0, Plan::Springing), FMuseeFrame::East()), Key.Close(), {}, 2.0 * Proud, Solid));
					continue;
				}
				FFormProfile Voussoir = FFormProfile::From(OnArch(From, Plan::Intrados));
				Voussoir.ArcAbout(FVector2D::ZeroVector, Plan::Intrados, Plan::Intrados, From, To, EFormJoint::Crisp)
					.LineTo(OnArch(To, Plan::Extrados), EFormJoint::Crisp)
					.ArcAbout(FVector2D::ZeroVector, Plan::Extrados, Plan::Extrados, To, From, EFormJoint::Crisp);
				Out.Add(FaceSlab(FString::Printf(TEXT("arch.%d.voussoir.%02d"), Index + 1, Stone), TEXT("arch.voussoir"),
					FaceFrame(FVector(X - Plan::PierHalf, 0.0, Plan::Springing), FMuseeFrame::East()), Voussoir.Close(), {}, 2.0 * Plan::PierHalf, Solid));
			}
		}

		/** The spandrel, a half ring from the extrados to behind the vault's face, through the arch's thickness. */
		void Spandrel(TArray<FFormDrawing>& Out, int32 Index)
		{
			const double X = Plan::PierX[Index];
			const double Thick = 2.0 * Plan::PierHalf;
			FFormLathe Ring;
			Ring.Section = FFormProfile::Polygon({{Plan::Extrados, 0.0}, {Plan::SpandrelOuter, 0.0}, {Plan::SpandrelOuter, Thick}, {Plan::Extrados, Thick}});
			Ring.FromDegrees = 0.0;
			Ring.ToDegrees = 180.0;
			Out.Add(Drawn(FString::Printf(TEXT("arch.%d.spandrel"), Index + 1), TEXT("arch.spandrel"), MoveTemp(Ring), Solid,
				FaceFrame(FVector(X - Plan::PierHalf, 0.0, Plan::Springing), FMuseeFrame::East())));
		}

		/** The archivolt round the extrados on both faces, in two runs each, stopped against the keystone's sides. */
		void Archivolts(TArray<FFormDrawing>& Out, int32 Index)
		{
			const double X = Plan::PierX[Index];
			const double Step = 180.0 / Plan::Voussoirs;
			const double KeyFrom = 180.0 - Step * (Plan::Keystone + 1);
			const double KeyTo = 180.0 - Step * Plan::Keystone;
			const TPair<double, double> Runs[2] = {{0.0, KeyFrom}, {KeyTo, 180.0}};
			for (const double Side : {1.0, -1.0})
			{
				const FVector N = Side > 0.0 ? FMuseeFrame::East() : FMuseeFrame::West();
				for (int32 Run = 0; Run < 2; ++Run)
				{
					FFormLathe Moulding;
					Moulding.Section = Section(Plan::Archivolt, FVector2D(Plan::ArchivoltRadius, 0.0));
					Moulding.FromDegrees = Runs[Run].Key;
					Moulding.ToDegrees = Runs[Run].Value;
					Out.Add(Drawn(FString::Printf(TEXT("arch.%d.archivolt.%s.%d"), Index + 1, Side > 0.0 ? TEXT("east") : TEXT("west"), Run + 1),
						TEXT("arch.archivolt"), MoveTemp(Moulding), Solid,
						FaceFrame(FVector(X + Side * Plan::PierHalf, 0.0, Plan::Springing), N)));
				}
			}
		}
	}

	void DrawPiers(TArray<FFormDrawing>& Out)
	{
		for (int32 Index = 0; Index < Plan::PierCount; ++Index)
		{
			Pier(Out, Index, -1.0);
			Pier(Out, Index, 1.0);
			Voussoirs(Out, Index);
			Spandrel(Out, Index);
			Archivolts(Out, Index);
		}
	}
}
