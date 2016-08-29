

#pragma once

#include "Object.h"
#include "IBrickData.h"
#include "BrickDataRegistry.generated.h"

UCLASS()
class BRICKGRID_API UBrickDataRegistry : public UObject
{
	GENERATED_BODY()

public:
	static IBrickData* GetBrickData(int32 BrickID);

private:
	const static TArray<IBrickData*> BrickDatas;
};

UCLASS()
class BRICKGRID_API UBrickStairs : public UObject, public IBrickData
{
	GENERATED_BODY()

public:

	virtual bool IsSolid() override;
	virtual bool IsOpaque() override;
	virtual bool IsComplexRender() override;

	virtual TArray<uint16> GetIndices() override;
	virtual TArray<FDynamicMeshVertex> GetVertices() override;

private:
	const static TArray<uint16> Indices;
	const static TArray<FDynamicMeshVertex> Vertices;

	static TArray<uint16> InitIndices();
	static TArray<FDynamicMeshVertex> InitVertices();
};