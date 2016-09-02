

#pragma once

#include "Object.h"
#include "IBrickRenderer.h"
#include "BrickRendererRegistry.generated.h"

UCLASS()
class BRICKGRID_API UBrickRendererRegistry : public UObject
{
	GENERATED_BODY()

public:
	UBrickRendererRegistry();
	TScriptInterface<IBrickRenderer> GetBrickRenderer(int32 BrickID);

	UPROPERTY()
	TArray<TScriptInterface<IBrickRenderer>> BrickRenderers;
};