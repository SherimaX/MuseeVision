#include "ArtWorkImage.h"

#include "Engine/Texture2D.h"
#include "ImageCore.h"
#include "ImageUtils.h"

namespace Musee::ArtMaking
{
	bool ComposeWorkImage(const FString& ImagePath, UTexture2D& Texture, FString& OutWhy)
	{
		FImage Image;
		if (!FImageUtils::LoadImage(*ImagePath, Image))
		{
			OutWhy = FString::Printf(TEXT("%s can't be read as an image"), *ImagePath);
			return false;
		}
		Texture.Source.Init(Image);
		Texture.SRGB = Image.GetGammaSpace() != EGammaSpace::Linear;
		Texture.LODGroup = TEXTUREGROUP_World;
		Texture.MaxTextureSize = WorkImageMaxPixels;
		// A photograph of a bounded work, never a tile: its edges are clamped, so a face ending at the image's edge (a
		// handscroll's panel meeting the next) takes no colour from the opposite edge.
		Texture.AddressX = TA_Clamp;
		Texture.AddressY = TA_Clamp;
		return true;
	}
}
