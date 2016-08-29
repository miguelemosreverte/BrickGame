

#include "BrickGridPluginPrivatePCH.h"
#include "BrickData/BrickDataRegistry.h"
#include "BrickData/BrickStairs.h"

const TArray<IBrickData*> InitBrickDatas()
{
	TArray<IBrickData*> InitBrickDatas;
	InitBrickDatas.Add(NewObject<UBrickStairs>());
	return InitBrickDatas;
}

const TArray<IBrickData*> UBrickDataRegistry::BrickDatas(InitBrickDatas());

IBrickData* UBrickDataRegistry::GetBrickData(int32 BrickID)
{
	if (BrickDatas.IsValidIndex(BrickID)) return BrickDatas[BrickID];

	return nullptr;
}
