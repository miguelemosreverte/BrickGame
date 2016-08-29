

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
	/** Use for transluency */
	virtual bool IsOpaque();
	/** Use to determine rendering method (maybe replace by GetRenderType() to add support of Static Mesh) */
	virtual bool IsComplexRender();
   
	/** Get Indices of the "brick" */
	virtual TArray<uint16> GetIndices();
	/** Get Vertices of the "brick" */
	virtual TArray<FDynamicMeshVertex> GetVertices();

private:
	const static TArray<uint16> Indices;
	const static TArray<FDynamicMeshVertex> Vertices;

	static TArray<uint16> InitIndices();
	static TArray<FDynamicMeshVertex> InitVertices();
};