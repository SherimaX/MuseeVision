#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The mouldings (§3.3): the trough-lipped cornice (P1) along the long walls between the piers and round both lunettes,
 * hiding the cove light; the slotted skirting (P2), stopped at the doors and dying into the pier bases; the two-fascia
 * architrave (P5) round the east door, the oval's door and both faces of the cabinet's door; and the cabinet's
 * slot-and-cove cornice (P8) round its ceiling. The piers' base and impost (P3, P4) and the arches' archivolt (P7) are
 * drawn with their stones.
 */
namespace Musee::Salon
{
	namespace
	{
		/** A moulding run along the silk planes, walked with the room on the right. */
		void RunMoulding(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, TConstArrayView<Plan::FStep> Steps,
			TConstArrayView<FVector2D> Path, bool bClosed = false)
		{
			FFormSweep Sweep;
			Sweep.Section = Section(Steps);
			Sweep.Path = FFormProfile::Polyline(Path, bClosed);
			Out.Add(Drawn(Name, LocalSurface, MoveTemp(Sweep), Solid));
		}

		/** The cornice of each bay: bay 1 and bay 5 round their end walls, the others along each long wall. */
		void Cornices(TArray<FFormDrawing>& Out)
		{
			const double S = Plan::SilkHalf;
			for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
			{
				const double W = Plan::Bays[Bay].West;
				const double E = Plan::Bays[Bay].East;
				const FString Name = FString::Printf(TEXT("cornice.bay%d"), Bay + 1);
				if (Bay == 0)
				{
					RunMoulding(Out, Name, TEXT("cornice"), Plan::Cornice, {{W, -S}, {E, -S}, {E, S}, {W, S}});
				}
				else if (Bay == Plan::BayCount - 1)
				{
					RunMoulding(Out, Name, TEXT("cornice"), Plan::Cornice, {{E, S}, {W, S}, {W, -S}, {E, -S}});
				}
				else
				{
					RunMoulding(Out, Name + TEXT(".north"), TEXT("cornice"), Plan::Cornice, {{W, -S}, {E, -S}});
					RunMoulding(Out, Name + TEXT(".south"), TEXT("cornice"), Plan::Cornice, {{E, S}, {W, S}});
				}
			}
			RunMoulding(Out, TEXT("cabinet.cornice"), TEXT("cabinet.cornice"), Plan::CabinetCornice,
				{{Plan::CabinetWest, Plan::CabinetNorth}, {Plan::CabinetEast, Plan::CabinetNorth}, {Plan::CabinetEast, Plan::CabinetSouth},
					{Plan::CabinetWest, Plan::CabinetSouth}}, true);
		}

		/** The skirtings, each run between piers or doors; they stop 1.68 from a door's centre, inside its architrave. */
		void Skirtings(TArray<FFormDrawing>& Out)
		{
			const double S = Plan::SilkHalf;
			const double Stop = Plan::SkirtingStop;
			const double DoorX = Plan::CabinetDoorX;
			const double End = Plan::EndSilkX;
			const double Far = Plan::FarSilkX;
			for (int32 Bay = 1; Bay < Plan::BayCount - 1; ++Bay)
			{
				const double W = Plan::Bays[Bay].West;
				const double E = Plan::Bays[Bay].East;
				RunMoulding(Out, FString::Printf(TEXT("skirting.bay%d.north"), Bay + 1), TEXT("skirting"), Plan::Skirting, {{W, -S}, {E, -S}});
				RunMoulding(Out, FString::Printf(TEXT("skirting.bay%d.south"), Bay + 1), TEXT("skirting"), Plan::Skirting, {{E, S}, {W, S}});
			}
			const double Bay1W = Plan::Bays[0].West;
			RunMoulding(Out, TEXT("skirting.bay1.north"), TEXT("skirting"), Plan::Skirting, {{Bay1W, -S}, {DoorX - Stop, -S}});
			RunMoulding(Out, TEXT("skirting.bay1.east.north"), TEXT("skirting"), Plan::Skirting, {{DoorX + Stop, -S}, {End, -S}, {End, -Stop}});
			RunMoulding(Out, TEXT("skirting.bay1.east.south"), TEXT("skirting"), Plan::Skirting, {{End, Stop}, {End, S}, {Bay1W, S}});
			const double Bay5E = Plan::Bays[Plan::BayCount - 1].East;
			RunMoulding(Out, TEXT("skirting.bay5.west.south"), TEXT("skirting"), Plan::Skirting, {{Bay5E, S}, {Far, S}, {Far, Stop}});
			RunMoulding(Out, TEXT("skirting.bay5.west.north"), TEXT("skirting"), Plan::Skirting, {{Far, -Stop}, {Far, -S}, {Bay5E, -S}});

			const double CS = Plan::CabinetSouth;
			RunMoulding(Out, TEXT("cabinet.skirting.west"), TEXT("skirting"), Plan::Skirting,
				{{DoorX - Stop, CS}, {Plan::CabinetWest, CS}, {Plan::CabinetWest, Plan::CabinetNorth}, {Plan::CabinetEast, Plan::CabinetNorth},
					{Plan::CabinetEast, CS}, {DoorX + Stop, CS}});
		}

		/** The architraves on the bay faces of both port doors and on both faces of the cabinet's door. */
		void Architraves(TArray<FFormDrawing>& Out)
		{
			const double Half = 0.5 * Plan::DoorWidth;
			const double Spring = Plan::DoorSpringing;
			ArchedTrim(Out, TEXT("architrave.east"), TEXT("door.architrave"), FVector(Plan::EndSilkX, 0.0, 0.0), FMuseeFrame::West(), Half, Spring,
				Plan::Architrave);
			ArchedTrim(Out, TEXT("architrave.west"), TEXT("door.architrave"), FVector(Plan::FarSilkX, 0.0, 0.0), FMuseeFrame::East(), Half, Spring,
				Plan::Architrave);
			ArchedTrim(Out, TEXT("architrave.cabinet.bay"), TEXT("door.architrave"), FVector(Plan::CabinetDoorX, -Plan::SilkHalf, 0.0),
				FMuseeFrame::South(), Half, Spring, Plan::Architrave);
			ArchedTrim(Out, TEXT("architrave.cabinet.cabinet"), TEXT("door.architrave"), FVector(Plan::CabinetDoorX, Plan::CabinetSouth, 0.0),
				FMuseeFrame::North(), Half, Spring, Plan::Architrave);
		}
	}

	void DrawMouldings(TArray<FFormDrawing>& Out)
	{
		Cornices(Out);
		Skirtings(Out);
		Architraves(Out);
	}
}
