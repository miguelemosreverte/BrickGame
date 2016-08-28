
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
			uint16 FaceVertexIndices[8] = { 0 };
			const int VertexIndexOffset = MaterialBatches[9].FaceBatches[FaceIndex].Indices.Num() - 6;
			for (uint32 FaceVertexIndex = 0; FaceVertexIndex < 4; ++FaceVertexIndex)
			{
				if (FacesDrawn.Contains(FaceIndex))
				{
					const int VertexIndex = MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + FaceVertexIndex];
					const FVector Position = Vertices[VertexIndex].Position;
					const int NewVertexIndex = Vertices.Num();
					new(Vertices) FDynamicMeshVertex(Position);
					FaceVertexIndices[FaceVertexIndex] = NewVertexIndex;

					/*taller rectangle*/
					if ((FaceIndex == 5 && (FaceVertexIndex == 0 || FaceVertexIndex == 3))
						|| (FaceIndex == 0 && (FaceVertexIndex == 3 || FaceVertexIndex == 2))
						|| (FaceIndex == 1 && (FaceVertexIndex == 1 || FaceVertexIndex == 0))
						|| (FaceIndex == 2))
					{
						Vertices[NewVertexIndex].Position.Y += 0.5 * (0.01 / 2.55);
					}

					/*smaller rectangle
					if (FaceIndex == 0 || FaceIndex == 1 || FaceIndex == 5 || FaceIndex == 2)
					{
						FVector Position = Vertices[VertexIndex].Position;

						int NewVertexIndex = Vertices.Num();
						new(Vertices) FDynamicMeshVertex(Position);
						FaceVertexIndices[4 + FaceVertexIndex] = NewVertexIndex;
						if (FaceIndex == 5)
						{
							Vertices[NewVertexIndex].Position.Z -= 0.5 * (0.01 / 2.55);
						}
						if (FaceIndex == 0 || FaceIndex == 1 || FaceIndex == 2 && (FaceVertexIndex == 2 || FaceVertexIndex == 1))
						{
							Vertices[NewVertexIndex].Position.Z -= 0.5 * (0.01 / 2.55);
						}
					}*/

				}//if face is drawn

			}//inside FaceVertexIndex for loop
			if (FacesDrawn.Contains(FaceIndex))
			{

				// Write the indices for the brick face.
				//this way is alternative to the second way shown below, which uses pointers
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 0] = FaceVertexIndices[0];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 1] = FaceVertexIndices[1];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 2] = FaceVertexIndices[2];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 3] = FaceVertexIndices[0];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 4] = FaceVertexIndices[2];
				MaterialBatches[9].FaceBatches[FaceIndex].Indices[VertexIndexOffset + 5] = FaceVertexIndices[3];

				/*smaller rectangle
				FFaceBatch& FaceBatch = MaterialBatches[9].FaceBatches[FaceIndex];
				uint16* FaceVertexIndex = &FaceBatch.Indices[FaceBatch.Indices.AddUninitialized(6)];
				*FaceVertexIndex++ = FaceVertexIndices[4 + 0];
				*FaceVertexIndex++ = FaceVertexIndices[4 + 1];
				*FaceVertexIndex++ = FaceVertexIndices[4 + 2];
				*FaceVertexIndex++ = FaceVertexIndices[4 + 0];
				*FaceVertexIndex++ = FaceVertexIndices[4 + 2];
				*FaceVertexIndex++ = FaceVertexIndices[4 + 3];*/
			}
		}//inside FaceIndex for loop


	}

};