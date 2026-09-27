#include "ArtWorkImage.h"

#include "Engine/Texture2D.h"
#include "ImageCore.h"
#include "ImageUtils.h"

namespace Musee::ArtMaking
{
	bool FillWorkImage(const FString& ImagePath, UTexture2D& Texture, FString& OutWhy)
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
		return true;
	}
}
