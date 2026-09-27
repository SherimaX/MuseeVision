#include "ChenghuaiHall.h"

#include "ChenghuaiHallTrades.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"

namespace Musee::Chenghuai
{
	namespace Hall
	{
		FString Named(const FHallSpec& Spec, const FString& Part)
		{
			return FString::Printf(TEXT("%s.%s"), *Spec.Name, *Part);
		}

		bool HasGable(const FHallSpec& Spec, bool bAtU1)
		{
			return Spec.GableT > 0.0 && !(bAtU1 ? Spec.bOpenAtU1 : Spec.bOpenAtU0);
		}

		TArray<double> FreeColumns(const FHallSpec& Spec)
		{
			TArray<double> Free;
			for (int32 Index = 0; Index < Spec.ColU.Num(); ++Index)
			{
				const bool bFirst = Index == 0;
				const bool bLast = Index == Spec.ColU.Num() - 1;
				const bool bInGable = (bFirst && HasGable(Spec, false)) || (bLast && HasGable(Spec, true));
				if (!bInGable)
				{
					Free.Add(Spec.ColU[Index]);
				}
			}
			return Free;
		}

		const TCHAR* PaintedOr(const FHallSpec& Spec, const TCHAR* Plain)
		{
			return Spec.bPainted ? TEXT("timber.painted") : Plain;
		}

		void DrawColumns(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section)
		{
			if (!Spec.bColumns)
			{
				return;
			}
			int32 Count = 0;
			auto Column = [&](double U, double V, double Top, double Diameter)
			{
				DrawColumn(Out, Named(Spec, FString::Printf(TEXT("column.%d"), ++Count)), Spec.Frame, U, V, Spec.Floor, Top, Diameter, Spec.Timber);
			};
			const double Thick = Spec.ColD + Plan::InnerColumnThicker;
			for (const double U : FreeColumns(Spec))
			{
				Column(U, 0.0, Section.Zc, Spec.ColD);
				const int32 Facade = Spec.FacadeV > 0.0 ? Section.PurlinAt(Spec.FacadeV) : INDEX_NONE;
				if (Facade != INDEX_NONE)
				{
					Column(U, Spec.FacadeV, Section.InnerColumnTop(Facade), Thick);
				}
				for (const double V : Spec.InnerColV)
				{
					Column(U, V, Section.InnerColumnTop(Section.PurlinAt(V)), Thick);
				}
				if (Spec.bBackColumns)
				{
					const int32 Back = Spec.PurlinV.Num() - 1;
					Column(U, Spec.PurlinV[Back], Section.Seat[Back] - Plan::SeatOver - 2.0 * Section.PurlinR, Spec.ColD);
				}
			}
		}
	}

	void DrawColumn(TArray<FFormDrawing>& Out, const FString& Name, const FBuildingFrame& F, double U, double V, double Floor, double Top,
		double Diameter, const TCHAR* Timber, double BaseSide)
	{
		const double Half = 0.5 * (BaseSide > 0.0 ? BaseSide : Plan::BaseStonePerDiameter * Diameter);
		Block(Out, Name + TEXT(".base"), TEXT("stone.base"), F, U - Half, U + Half, V - Half, V + Half, Floor + Plan::BaseStoneBottom,
			Floor + Plan::BaseStoneTop, EHit::Solid);
		const double D = Diameter;
		Turned(Out, Name + TEXT(".drum"), TEXT("stone.base"), F.At(U, V, Floor + Plan::BaseStoneTop), Section(Plan::DrumSection, false, D),
			EHit::Solid);
		const double Height = Top - (Floor + Plan::ShaftFoot);
		const double Foot = 0.5 * D;
		const double Head = Foot * (1.0 - Plan::ShaftTaper * Height / D);
		Turned(Out, Name + TEXT(".shaft"), Timber, F.At(U, V, Floor + Plan::ShaftFoot),
			Section({{0.0, 0.0}, {Foot - Plan::ShaftArris, 0.0}, {Foot, Plan::ShaftArris}, {Head, Height}, {0.0, Height}}, false), EHit::Solid);
	}

	void DrawHall(TArray<FFormDrawing>& Out, const FHallSpec& Spec)
	{
		const FHallSection Section = FHallSection::Of(Spec);
		if (Spec.bPlatform)
		{
			Hall::DrawPlatform(Out, Spec);
		}
		Hall::DrawColumns(Out, Spec, Section);
		Hall::DrawTimber(Out, Spec, Section);
		Hall::DrawRoof(Out, Spec, Section);
		Hall::DrawGables(Out, Spec, Section);
		Hall::DrawBackWall(Out, Spec, Section);
		Hall::DrawFront(Out, Spec, Section);
	}
}
