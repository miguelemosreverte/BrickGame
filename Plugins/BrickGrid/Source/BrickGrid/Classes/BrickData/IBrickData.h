

#pragma once

#include <DynamicMeshBuilder.h>
#include "IBrickData.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UBrickData : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IBrickData
{
	GENERATED_IINTERFACE_BODY()

	/** Use for collision */
	virtual bool IsSolid();
	/** Use to determine rendering method (maybe replace by GetRenderType() to add support of Static Mesh) */
	virtual bool IsComplexRender();
    /** Use to render your brick*/
	virtual void Render(TArray<uint16> &VertexIndexMap, TArray<FDynamicMeshVertex> &Vertices, TArray<FMaterialBatch> &MaterialBatches, TArray<uint16> NewIndices, int FaceIndex);
};