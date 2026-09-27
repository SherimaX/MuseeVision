#include "ArtWorkImage.h"

#include "ArtPaintLayer.h"
#include "ArtWork.h"
#include "Engine/Texture2D.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "MuseeRecords.h"
#include "Vessel/VesselDecoration.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The revision of a vessel's composition: raise it whenever the same record and views would compose differently. */
		constexpr int32 DecorationRevision = 2;

		void Fill(const FImage& Image, UTexture2D& Texture)
		{
			Texture.Source.Init(Image);
			Texture.SRGB = Image.GetGammaSpace() != EGammaSpace::Linear;
			Texture.LODGroup = TEXTUREGROUP_World;
			Texture.MaxTextureSize = WorkImageMaxPixels;
		}

		/** The hash of a file's bytes, or empty when it can't be read. */
		FString FileHash(const FString& Path)
		{
			TArray<uint8> Bytes;
			return FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent) ? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString() : FString();
		}

		/** The record of the painted vessel a layer is the decoration of; or why there is none. */
		TValueOrError<FArtVessels, FString> PaintedRecord(const FArtPaintLayer& Layer)
		{
			TValueOrError<FArtVessels, FString> Record = ReadRecord<FArtVessels>(FArtVessels::Record);
			if (Record.HasError())
			{
				return Record;
			}
			const FArtVessel* Vessel = Record.GetValue().Find(Layer.Work);
			if (!Vessel || !Vessel->IsPainted())
			{
				return MakeError(FString::Printf(TEXT("Data/%s gives work %s no views to compose its decoration from"), FArtVessels::Record, *Layer.Work));
			}
			return Record;
		}

		/** Every number a painted vessel's decoration is composed from, as text: its record's and the size it is turned to. */
		FString CompositionText(const FArtVessels& Record, const FArtVessel& Vessel, const FVector2D& SizeMeters)
		{
			const FArtVesselComposition& How = Record.Composition;
			FString Text = FString::Printf(TEXT("vessel-decoration;%d;%.9g;%.9g;%.9g;%.9g;%.9g;%.9g;%.9g;%.9g;%.9g;%.9g;%.9g;%.9g;%.9g;%d"),
				DecorationRevision, SizeMeters.X, SizeMeters.Y, How.EdgePixels, How.ClearanceMeters, How.WhiteSquareness, How.WhiteQuantile,
				How.GlazeChroma, How.GlazeLightness, How.ShadeFloor, How.HighlightOverWhite, How.HighlightWeight, How.FullWeight, How.MostPixelsPerMeter,
				How.MostPixels);
			for (const FVector2D& Point : Vessel.Section)
			{
				Text += FString::Printf(TEXT(";%.9g,%.9g"), Point.X, Point.Y);
			}
			for (const int32 Index : Vessel.Outside)
			{
				Text += FString::Printf(TEXT(";o%d"), Index);
			}
			for (const FArtVesselBand& Band : Vessel.Bands)
			{
				Text += FString::Printf(TEXT(";b%.9g,%.9g,%.9g"), Band.FromMeters, Band.ToMeters, Band.PeriodDegrees);
			}
			for (const FArtVesselView& View : Vessel.Views)
			{
				Text += FString::Printf(TEXT(";v%s,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g"), *View.Image, View.AzimuthDegrees, View.DistanceMeters, View.HeightMeters,
					View.PitchDegrees, View.FocalPixels, View.FitPixels);
				for (const double Axis : View.AxisPixels)
				{
					Text += FString::Printf(TEXT(",%.9g"), Axis);
				}
			}
			return Text;
		}
	}

	bool ComposeWorkImage(const FString& ImagePath, UTexture2D& Texture, FString& OutWhy)
	{
		FImage Image;
		if (!FImageUtils::LoadImage(*ImagePath, Image))
		{
			OutWhy = FString::Printf(TEXT("%s can't be read as an image"), *ImagePath);
			return false;
		}
		Fill(Image, Texture);
		return true;
	}

	bool ComposeLayerImage(const FArtPaintLayer& Layer, UTexture2D& Texture, FString& OutWhy)
	{
		if (Layer.VesselMeters.IsNearlyZero())
		{
			return ComposeWorkImage(Layer.ImagePath, Texture, OutWhy);
		}
		const TValueOrError<FArtVessels, FString> Record = PaintedRecord(Layer);
		if (Record.HasError())
		{
			OutWhy = Record.GetError();
			return false;
		}
		const FArtVessel& Vessel = *Record.GetValue().Find(Layer.Work);
		TArray<FVesselPicture> Views;
		for (const FArtVesselView& View : Vessel.Views)
		{
			const FString Path = Musee::Art::SourcePath(View.Image);
			FImage Loaded;
			if (!FImageUtils::LoadImage(*Path, Loaded))
			{
				OutWhy = FString::Printf(TEXT("%s can't be read as an image"), *Path);
				return false;
			}
			FVesselPicture& Picture = Views.AddDefaulted_GetRef();
			Loaded.CopyTo(Picture.Image, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
			Picture.Camera = CameraOf(View);
		}
		const TArray<FVector2D> Sized = SizedSection(Vessel, Layer.VesselMeters);
		const FArtVesselComposition& How = Record.GetValue().Composition;
		FImage Decoration;
		FDecorationStats Stats;
		ComposeDecoration(How, Vessel, Sized, Views, DecorationPixels(How, Vessel, LayoutOf(Sized)), Decoration, Stats);
		Fill(Decoration, Texture);
		return true;
	}

	FString LayerImageIdentity(const FArtPaintLayer& Layer)
	{
		if (Layer.VesselMeters.IsNearlyZero())
		{
			return FileHash(Layer.ImagePath);
		}
		const TValueOrError<FArtVessels, FString> Record = PaintedRecord(Layer);
		if (Record.HasError())
		{
			return FString();
		}
		const FArtVessel& Vessel = *Record.GetValue().Find(Layer.Work);
		FString Text = CompositionText(Record.GetValue(), Vessel, Layer.VesselMeters);
		for (const FArtVesselView& View : Vessel.Views)
		{
			const FString Hash = FileHash(Musee::Art::SourcePath(View.Image));
			if (Hash.IsEmpty())
			{
				return FString();
			}
			Text += TEXT(";") + Hash;
		}
		return FSHA1::HashBuffer(*Text, Text.Len() * sizeof(TCHAR)).ToString();
	}
}
