

#include "BrickGridPluginPrivatePCH.h"
#include "BrickData/IBrickData.h"

UBrickData::UBrickData(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

bool IBrickData::IsSolid()
{
	return false;
}

bool IBrickData::IsComplexRender()
{
	return false;
}

void IBrickData::Render(TArray<uint16>& VertexIndexMap, TArray<FDynamicMeshVertex>& Vertices, TArray<FMaterialBatch>& MaterialBatches, TArray<uint16> NewIndices, int FaceIndex){}
