

#pragma once

#include "IBrickRenderer.h"
#include <DynamicMeshBuilder.h>
#include "BrickStairsRenderer.generated.h"

UCLASS()
class BRICKGRID_API UBrickStairsRenderer : public UObject, public IBrickRenderer
{
	GENERATED_BODY()

public:
	// Begin IBrickRenderer interface.
	virtual bool IsSolid() override;
	virtual bool IsComplexBrick() override;
	virtual void Render(TArray<uint16> &VertexIndexMap, TArray<FDynamicMeshVertex> &Vertices, TArray<FMaterialBatch> &MaterialBatches, TArray<uint16> NewIndices, int FaceIndex) override;
	//End IBrickRenderer interface
};