
#pragma once

#include <DynamicMeshBuilder.h>
#include "BrickGridComponent.h"

class ComplexShapeBrickFactory
{

public:
	ComplexShapeBrickFactory()
	{}

	void a(TArray<uint16> &VertexIndexMap, TArray<FDynamicMeshVertex> &Vertices, TArray<FMaterialBatch> &MaterialBatches, uint16 FaceVertexIndices[6][8], int FaceIndex)
	{

		//uint16 FaceVertexIndices[8] = { 0 };
		const int VertexIndexOffset = MaterialBatches[9].FaceBatches[FaceIndex].Indices.Num() - 6;
		for (uint32 FaceVertexIndex = 0; FaceVertexIndex < 4; ++FaceVertexIndex)
		{
			const int VertexIndex = FaceVertexIndices[FaceIndex][FaceVertexIndex];
			const FVector Position = Vertices[VertexIndex].Position;
			const int NewVertexIndex = Vertices.Num();
			new(Vertices) FDynamicMeshVertex(Position);
			FaceVertexIndices[FaceIndex][FaceVertexIndex] = NewVertexIndex;

		}//inside FaceVertexIndex for loop

		// Write the indices for the brick face.
		FFaceBatch& FaceBatch = MaterialBatches[9].FaceBatches[FaceIndex];
		uint16* FaceVertexIndex = &FaceBatch.Indices[FaceBatch.Indices.AddUninitialized(6)];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][0];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][1];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][2];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][0];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][2];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][3];

	}

};