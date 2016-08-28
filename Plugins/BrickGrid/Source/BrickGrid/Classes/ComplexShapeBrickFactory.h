
#pragma once

#include <DynamicMeshBuilder.h>
#include "BrickGridComponent.h"

class ComplexShapeBrickFactory
{

public:
	ComplexShapeBrickFactory()
	{}

	void WellThenRenderTheComplexShape(TArray<uint16> &VertexIndexMap, TArray<FDynamicMeshVertex> &Vertices, TArray<FMaterialBatch> &MaterialBatches, TArray<uint8>FacesDrawn)
	{

		for (uint32 FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
		{
			if (FacesDrawn.Contains(FaceIndex))
			{
			uint16 FaceVertexIndices[8] = { 0 };
			const int VertexIndexOffset = MaterialBatches[9].FaceBatches[FaceIndex].Indices.Num() - 6;
			for (uint32 FaceVertexIndex = 0; FaceVertexIndex < 4; ++FaceVertexIndex)
			{
					const int VertexIndex = MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + FaceVertexIndex];
					const FVector Position = Vertices[VertexIndex].Position;
					const int NewVertexIndex = Vertices.Num();
					new(Vertices) FDynamicMeshVertex(Position);
					FaceVertexIndices[FaceVertexIndex] = NewVertexIndex;

			}//inside FaceVertexIndex for loop
			
				// Write the indices for the brick face.
				//this way is alternative to the second way shown below, which uses pointers
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 0] = FaceVertexIndices[0];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 1] = FaceVertexIndices[1];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 2] = FaceVertexIndices[2];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 3] = FaceVertexIndices[0];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 4] = FaceVertexIndices[2];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 5] = FaceVertexIndices[3];
			}
		}//inside FaceIndex for loop


	}

};