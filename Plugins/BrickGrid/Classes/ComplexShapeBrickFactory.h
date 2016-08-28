
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
			int NewVertexIndex = Vertices.Num();
			new(Vertices) FDynamicMeshVertex(Position);
			FaceVertexIndices[FaceIndex][FaceVertexIndex] = NewVertexIndex;


			/* THE CREATION OF THE STAIR*/
			if (FaceIndex == 0 || FaceIndex == 1 || FaceIndex == 5 || FaceIndex == 2)
			{

				int NewVertexIndex = Vertices.Num();
				new(Vertices) FDynamicMeshVertex(Position);
				FaceVertexIndices[FaceIndex][4 + FaceVertexIndex] = NewVertexIndex;
				if (FaceIndex == 5)
				{
					Vertices[NewVertexIndex].Position.Z -= 0.5 * (0.01 / 2.5);
				}
				if (FaceIndex == 0 || FaceIndex == 1 || FaceIndex == 2 && (FaceVertexIndex == 2 || FaceVertexIndex == 1))
				{
					Vertices[NewVertexIndex].Position.Z -= 0.5 * (0.01 / 2.5);
				}
			}





			NewVertexIndex = Vertices.Num();
			new(Vertices) FDynamicMeshVertex(Position);
			FaceVertexIndices[FaceIndex][FaceVertexIndex] = NewVertexIndex;

			if (FaceIndex == 2)
			{
				Vertices[NewVertexIndex].Position.Y += 0.5 * (0.01 / 2.5);
			}
			if ((FaceIndex == 5 && (FaceVertexIndex == 0 || FaceVertexIndex == 3))
				|| (FaceIndex == 0 && (FaceVertexIndex == 3 || FaceVertexIndex == 2))
				|| (FaceIndex == 1 && (FaceVertexIndex == 1 || FaceVertexIndex == 0))
				)
				Vertices[NewVertexIndex].Position.Y += 0.5 * (0.01 / 2.5);




		}//inside FaceVertexIndex for loop

		// Write the indices for the brick face.
		FFaceBatch& FaceBatch = MaterialBatches[9].FaceBatches[FaceIndex];
		uint16* FaceVertexIndex = &FaceBatch.Indices[FaceBatch.Indices.AddUninitialized(12)];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][0];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][1];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][2];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][0];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][2];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][3];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][4 + 0];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][4 + 1];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][4 + 2];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][4 + 0];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][4 + 2];
		*FaceVertexIndex++ = FaceVertexIndices[FaceIndex][4 + 3];

	}

};