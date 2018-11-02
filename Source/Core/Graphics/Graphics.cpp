#include "Precomp.h"
#include "Graphics.h"

#include <limits>
#include "Camera.h"

Graphics* Graphics::ourActiveGraphics = NULL;
int Graphics::ourWidth = 800;
int Graphics::ourHeight = 600;

Graphics::Graphics(AssetTracker& anAssetTracker)
	: myAssetTracker(anAssetTracker)
	, myRenderCalls(0)
{
}