

#include "BrickGridPluginPrivatePCH.h"
#include "BrickDataRegistry.h"

TArray<IBrickData*> InitBrickDatas()
{
	TArray<IBrickData*> InitBrickDatas;
	//InitBrickDatas.Add(NewObject<UBrickData>());
	return InitBrickDatas;
}

const TArray<IBrickData*> UBrickDataRegistry::BrickDatas(InitBrickDatas());

IBrickData* UBrickDataRegistry::GetBrickData(int32 BrickID)
{
	if (BrickDatas.IsValidIndex(BrickID)) return BrickDatas[BrickID];

	return nullptr;
}

/// BrickStairs

bool UBrickStairs::IsSolid()
{
	return true;
}

bool UBrickStairs::IsOpaque()
{
	return true;
}

bool UBrickStairs::IsComplexRender()
{
	return true;
}

TArray<uint16> UBrickStairs::InitIndices()
{
	TArray<uint16> InitIndices;
	uint16 Values[] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
	InitIndices.Append(Values, ARRAY_COUNT(Values));
	return InitIndices;
}

TArray<FDynamicMeshVertex> UBrickStairs::InitVertices()
{
	TArray<FDynamicMeshVertex> InitVertices;
	//Work in progress thinking about the best method to support FaceIndex.
	return InitVertices;
}

const TArray<uint16> UBrickStairs::Indices(InitIndices());
const TArray<FDynamicMeshVertex> UBrickStairs::Vertices(InitVertices());

TArray<uint16> UBrickStairs::GetIndices()
{
	return Indices;
}

TArray<FDynamicMeshVertex> UBrickStairs::GetVertices()
{
	return Vertices;
}