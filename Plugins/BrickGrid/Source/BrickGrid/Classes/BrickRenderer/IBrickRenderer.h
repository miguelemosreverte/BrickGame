

#pragma once

#include <DynamicMeshBuilder.h>
#include "IBrickRenderer.generated.h"

UINTERFACE()
class BRICKGRID_API UBrickRenderer : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IBrickRenderer
{
	GENERATED_IINTERFACE_BODY()

	// Use for collision
	virtual bool IsSolid();
	// Use to determine rendering method (maybe replace by GetRenderType() to add support of Static Mesh) 
	virtual bool IsComplexBrick();
	// Use to render your brick
	virtual void Render(TArray<uint16> &VertexIndexMap, TArray<FDynamicMeshVertex> &Vertices, TArray<FMaterialBatch> &MaterialBatches, TArray<uint16> NewIndices, int FaceIndex);
	
	//TODO add BuildCollision() to build box collision of complexe brick.
};