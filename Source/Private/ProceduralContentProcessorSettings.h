#pragma once

#include "ProceduralContentProcessorSettings.generated.h"

UCLASS(config = ProceduralContentProcessor, defaultconfig, meta = (DisplayName = "ProceduralContentProcessor"))
class UProceduralContentProcessorSettings : public UObject {
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere)
	FSoftClassPath LastProcessorClass;
}; 
