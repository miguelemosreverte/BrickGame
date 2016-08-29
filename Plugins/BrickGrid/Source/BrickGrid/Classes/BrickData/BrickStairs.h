

#pragma once

#include "Object.h"
#include "IBrickData.h"
#include <DynamicMeshBuilder.h>
#include "BrickStairs.generated.h"

UCLASS()
class BRICKGRID_API UBrickStairs : public UObject, public IBrickData
{
	GENERATED_BODY()

public:
	// Begin IBrickData interface.
	virtual bool IsSolid() override;
	virtual bool IsComplexRender() override;
	virtual void Render(TArray<uint16> &VertexIndexMap, TArray<FDynamicMeshVertex> &Vertices, TArray<FMaterialBatch> &MaterialBatches, TArray<uint16> NewIndices, int FaceIndex);
	//End IBrickData interface
};