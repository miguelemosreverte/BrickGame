

#include "BrickGridPluginPrivatePCH.h"
#include "BrickRenderer/IBrickRenderer.h"

UBrickRenderer::UBrickRenderer(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer){}

bool IBrickRenderer::IsSolid()
{
	return false;
}

bool IBrickRenderer::IsComplexBrick()
{
	return false;
}

void IBrickRenderer::Render(TArray<uint16>& VertexIndexMap, TArray<FDynamicMeshVertex>& Vertices, TArray<FMaterialBatch>& MaterialBatches, TArray<uint16> NewIndices, int FaceIndex){}
