

#pragma once

#include "Object.h"
#include "IBrickData.h"
#include "BrickDataRegistry.generated.h"

UCLASS()
class BRICKGRID_API UBrickDataRegistry : public UObject
{
	GENERATED_BODY()

public:
	static IBrickData* GetBrickData(int32 BrickID);
private:
	const static TArray<IBrickData*> BrickDatas;
};