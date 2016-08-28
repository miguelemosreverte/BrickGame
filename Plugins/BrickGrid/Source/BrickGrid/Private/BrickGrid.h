#pragma once

#include "BrickGridComponent.h"

extern const FInt3 FaceNormals[6];

struct FFaceBatch
{
	TArray<uint16> Indices;
};
struct FMaterialBatch
{
	FFaceBatch FaceBatches[6];
};