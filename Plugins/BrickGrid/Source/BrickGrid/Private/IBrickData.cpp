

#include "BrickGridPluginPrivatePCH.h"
#include "IBrickData.h"

UBrickData::UBrickData(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

TArray<uint16> IBrickData::InitIndices()
{
	TArray<uint16> InitIndices;
	//populate it
	return InitIndices;
}

TArray<FDynamicMeshVertex> IBrickData::InitVertices()
{
	TArray<FDynamicMeshVertex> InitVertices;
	//populate it
	return InitVertices;
}

const TArray<uint16> IBrickData::Indices(InitIndices());
const TArray<FDynamicMeshVertex> IBrickData::Vertices(InitVertices());

bool IBrickData::IsSolid()
{
	return true;
}

bool IBrickData::IsOpaque()
{
	return true;
}

bool IBrickData::IsComplexRender()
{
	return false;
}

TArray<uint16> IBrickData::GetIndices()
{
	return Indices;
}

TArray<FDynamicMeshVertex> IBrickData::GetVertices()
{
	return Vertices;
}