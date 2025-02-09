// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FProceduralActorClassifierAssetActions;
class FProceduralAssetProcessorAssetActions;
class FProceduralWorldProcessorAssetActions;
class FProceduralActorColorationProcessorAssetActions;

class FProceduralContentProcessorModule : public IModuleInterface
{
public:
	static FORCEINLINE FProceduralContentProcessorModule& Get(){
		return FModuleManager::LoadModuleChecked<FProceduralContentProcessorModule>("ProceduralContentProcessor");
	}
protected:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//TSharedRef<SDockTab> SpawnProceduralContentProcessorTab(const FSpawnTabArgs& Args);
private:
	TSharedPtr<FProceduralAssetProcessorAssetActions> ProceduralAssetProcessorAssetActions;
	TSharedPtr<FProceduralWorldProcessorAssetActions> ProceduralWorldProcessorAssetActions;
	TSharedPtr<FProceduralActorColorationProcessorAssetActions> ProceduralActorColorationProcessorAssetActions;
};
