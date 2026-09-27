#include "Plan/EntryContext.h"

#include "Algo/Find.h"
#include "ArtWork.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "MuseeRecords.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The window of a photograph that is the whole work. */
		constexpr double WholeFrom = 0.0;
		constexpr double WholeTo = 1.0;

		/** A share written as a percentage. */
		constexpr double PercentPerShare = 100.0;

		/** Metres written as millimetres. */
		constexpr double MillimetersPerMeter = 1000.0;

		/** A photograph's size in pixels from its file's header, without decoding it; false when it can't be read. */
		bool ReadPixels(const FString& Path, FIntPoint& OutPixels)
		{
			TArray64<uint8> Bytes;
			if (!FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent))
			{
				return false;
			}
			IImageWrapperModule& Wrappers = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
			const EImageFormat Format = Wrappers.DetectImageFormat(Bytes.GetData(), Bytes.Num());
			const TSharedPtr<IImageWrapper> Wrapper = Format == EImageFormat::Invalid ? nullptr : Wrappers.CreateImageWrapper(Format);
			if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Bytes.GetData(), Bytes.Num()))
			{
				return false;
			}
			OutPixels = FIntPoint(int32(Wrapper->GetWidth()), int32(Wrapper->GetHeight()));
			return OutPixels.X > 0 && OutPixels.Y > 0;
		}

		/** Whether an order names each of the photographs 1 to Count once. */
		bool NamesEachOnce(const TArray<int32>& Order, int32 Count)
		{
			TArray<int32> Sorted = Order;
			Sorted.Sort();
			for (int32 Index = 0; Index < Sorted.Num(); ++Index)
			{
				if (Sorted[Index] != Index + 1)
				{
					return false;
				}
			}
			return Sorted.Num() == Count;
		}

		/** One photograph to lay: its number (1 for a work's one photograph), its file and its layer. */
		struct FNumbered
		{
			int32 Number = 1;
			FString Path;
		};

		/**
		 * The work's photographs, left to right as the scroll lies before its reader: its numbered ones in the reverse of
		 * their reading order, or its one photograph. Faults when they can't be ordered or found.
		 */
		TArray<FNumbered> LeftToRight(const FArtWork& Work, const FArtHangEntry& Entry, const FArtHandscroll* Record, TArray<FString>& OutFaults)
		{
			const TArray<FString> Sections = Musee::Art::FindSectionImages(Entry);
			if (Sections.IsEmpty())
			{
				if (Work.ImagePath.IsEmpty())
				{
					OutFaults.Add(TEXT("no image of it is in the collection"));
				}
				else if (Record && !Record->ReadingOrder.IsEmpty())
				{
					OutFaults.Add(FString::Printf(TEXT("Data/%s orders %d photographs of it, and the collection holds its one"), FArtHandscroll::Record,
						Record->ReadingOrder.Num()));
				}
				return { FNumbered{ 1, Work.ImagePath } };
			}
			if (!Record || !NamesEachOnce(Record->ReadingOrder, Sections.Num()))
			{
				OutFaults.Add(FString::Printf(TEXT("Data/%s gives no reading order naming each of its %d photographs once"), FArtHandscroll::Record,
					Sections.Num()));
				return {};
			}
			TArray<FNumbered> Out;
			for (int32 Index = Record->ReadingOrder.Num() - 1; Index >= 0; --Index)
			{
				const int32 Number = Record->ReadingOrder[Index];
				Out.Add(FNumbered{ Number, Sections[Number - 1] });
			}
			return Out;
		}

		/** A photograph's size and the share of it that is the work; a fault when either is wrong. */
		FScrollImage ImageOf(const FNumbered& Photograph, const FArtHandscroll* Record, TArray<FString>& OutFaults)
		{
			FScrollImage Image{ FIntPoint::ZeroValue, WholeFrom, WholeTo };
			if (!ReadPixels(Photograph.Path, Image.Pixels))
			{
				OutFaults.Add(FString::Printf(TEXT("its photograph %s can't be read as an image"), *Photograph.Path));
			}
			const FArtHandscrollWindow* Window = Record ? Algo::FindBy(Record->Windows, Photograph.Number, &FArtHandscrollWindow::Image) : nullptr;
			if (Window)
			{
				Image.From = Window->From;
				Image.To = Window->To;
			}
			if (Image.From < WholeFrom || Image.To > WholeTo || Image.To <= Image.From)
			{
				OutFaults.Add(FString::Printf(TEXT("Data/%s: photograph %d's window must run from 0 or more to 1 or less"), FArtHandscroll::Record,
					Photograph.Number));
			}
			return Image;
		}

		/** The note of photographs whose proportion strays from the sheet's: by how much, and what laying them cuts. */
		FString StrayNote(const FArtHangEntry& Entry, const FScrollLayout& Layout, double Tolerance)
		{
			return FString::Printf(TEXT("its photographs, joined at their own proportion, are %.1f %% %s than its %.3f x %.3f m (the tolerance is %.1f %%): "
				"they are laid to cover it, %.1f mm cut from %s, never stretched"), FMath::Abs(Layout.Stray) * PercentPerShare,
				Layout.Stray > 0.0 ? TEXT("longer") : TEXT("shorter"), Entry.SizeMeters().X, Entry.SizeMeters().Y, Tolerance * PercentPerShare,
				Layout.CutMeters * MillimetersPerMeter, Layout.Stray > 0.0 ? TEXT("each end") : TEXT("its head and its foot"));
		}
	}

	void ReadHandscrollsInto(const FArtHang& Hang, FEntryContext& Context)
	{
		if (!Hang.Works.ContainsByPredicate([](const FArtHangEntry& Entry) { return !Entry.IsBuilt() && Entry.Kind == Kinds::Handscroll; }))
		{
			return;
		}
		TValueOrError<FArtHandscrollFittings, FString> Fittings = ReadRecord<FArtHandscrollFittings>(FArtHandscrollFittings::Record);
		TValueOrError<TArray<FArtHandscroll>, FString> Records = ReadRecords<FArtHandscroll>(FArtHandscroll::Record);
		if (Fittings.HasError() || Records.HasError())
		{
			Context.HandscrollWhy = Fittings.HasError() ? Fittings.GetError() : Records.GetError();
			return;
		}
		Context.Handscroll.Emplace(Fittings.StealValue());
		Context.Handscrolls = Records.StealValue();
	}

	void PlanHandscroll(const FArtWork& Work, const FArtHangEntry& Entry, const FEntryContext& Context, FCanvasPlan& OutCanvas,
		TArray<FString>& OutFaults, TArray<FString>& OutNotes)
	{
		if (!Context.Handscroll.IsSet())
		{
			OutFaults.Add(Context.HandscrollWhy);
			return;
		}
		const FArtHandscroll* Record = Algo::FindBy(Context.Handscrolls, Entry.Work, &FArtHandscroll::Work);
		const TArray<FNumbered> Photographs = LeftToRight(Work, Entry, Record, OutFaults);
		TArray<FScrollImage> Images;
		for (const FNumbered& Photograph : Photographs)
		{
			Images.Add(ImageOf(Photograph, Record, OutFaults));
		}
		if (!OutFaults.IsEmpty())
		{
			return;
		}
		const FScrollLayout Layout = LayScroll(Images, Entry.SizeMeters());
		if (Layout.Panels.IsEmpty())
		{
			OutFaults.Add(TEXT("its photographs can't be laid across its sheet"));
			return;
		}
		const bool bSections = Photographs.Num() > 1;
		for (const FScrollPanel& Panel : Layout.Panels)
		{
			const FNumbered& Photograph = Photographs[Panel.Image];
			FScrollSection& Section = OutCanvas.Sections.AddDefaulted_GetRef();
			Section.Work = bSections ? FString::Printf(TEXT("%s.%d"), *Work.Id, Photograph.Number) : Work.Id;
			Section.ImagePath = Photograph.Path;
			Section.Layer = bSections ? Musee::Art::PaintMaterial(Work.Id, Photograph.Number) : Musee::Art::PaintMaterial(Work.Id);
			Section.Panel = Panel;
			Section.Number = Photograph.Number;
		}
		OutCanvas.ImageTopMeters = Layout.ImageTopMeters;
		const double Tolerance = Context.Handscroll->ProportionTolerance;
		if (FMath::Abs(Layout.Stray) > Tolerance)
		{
			OutNotes.Add(StrayNote(Entry, Layout, Tolerance));
		}
	}
}
