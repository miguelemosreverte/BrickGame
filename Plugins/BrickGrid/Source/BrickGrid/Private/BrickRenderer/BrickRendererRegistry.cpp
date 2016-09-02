

#include "BrickGridPluginPrivatePCH.h"
#include "BrickRenderer/BrickRendererRegistry.h"
#include "BrickRenderer/BrickStairsRenderer.h"


UBrickRendererRegistry::UBrickRendererRegistry()
{
	TScriptInterface<IBrickRenderer> BrickStairs(NewObject<UBrickStairsRenderer>());
	BrickRenderers.Add(BrickStairs);
}

TScriptInterface<IBrickRenderer> UBrickRendererRegistry::GetBrickRenderer(int32 BrickID)
{
	if (BrickRenderers.IsValidIndex(BrickID)) return BrickRenderers[BrickID];

	return nullptr;
}
