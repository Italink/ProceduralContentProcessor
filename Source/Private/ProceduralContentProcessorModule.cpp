#include "ProceduralContentProcessorModule.h"
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "BlueprintEditorTabs.h"
#include "EditorModeRegistry.h"
#include "FileHelpers.h"
#include "Interfaces/IPluginManager.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Landscape.h"
#include "LandscapeStreamingProxy.h"
#include "LevelEditor.h"
#include "Misc/FileHelper.h"
#include "ProceduralContentProcessor.h"
#include "ProceduralContentProcessorAssetActions.h"
#include "ProceduralContentProcessorEdMode.h"
#include "ProceduralObjectMatrixCustomization.h"
#include "SProceduralContentProcessorEditorOutliner.h"
#include "Toolkits/AssetEditorToolkitMenuContext.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "WorldPartition/HLOD/HLODLayer.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionEditorSpatialHash.h"

#define LOCTEXT_NAMESPACE "ProceduralContentProcessor"

const FName LevelEditorProceduralContentProcessorBrowser(TEXT("LevelEditorProceduralContentProcessorBrowser"));

void FProceduralContentProcessorModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ProceduralContentProcessor"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugins/ProceduralContentProcessor"), PluginShaderDir);

	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float Delta) -> bool{
		FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");
		if(AssetToolsModule == nullptr)
			return true;

		IAssetTools& AssetTools = AssetToolsModule->Get();
		AssetTools.RegisterAdvancedAssetCategory("ProceduralContentProcessor", LOCTEXT("ProceduralContentProcessor", "Procedural Content Processor"));

		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout(FProceduralObjectMatrix::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPropertyTypeCustomization_ProceduralObjectMatrix::MakeInstance));

		ProceduralAssetProcessorAssetActions = MakeShared<FProceduralAssetProcessorAssetActions>();
		AssetTools.RegisterAssetTypeActions(ProceduralAssetProcessorAssetActions.ToSharedRef());
		ProceduralWorldProcessorAssetActions = MakeShared<FProceduralWorldProcessorAssetActions>();
		AssetTools.RegisterAssetTypeActions(ProceduralWorldProcessorAssetActions.ToSharedRef());
		ProceduralActorColorationProcessorAssetActions = MakeShared<FProceduralActorColorationProcessorAssetActions>();
		AssetTools.RegisterAssetTypeActions(ProceduralActorColorationProcessorAssetActions.ToSharedRef());

		FEditorModeRegistry::Get().RegisterMode<FProceduralContentProcessorEdMode>(
			FProceduralContentProcessorEdMode::EdID,
			LOCTEXT("ProceduralContentProcessor", "Procedural Content Processor"),
			FSlateIcon(),
			true, 5000000
		);

		return false;
	}));


	//FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	//LevelEditorModule.OnTabManagerChanged().AddLambda([this]() {
	//	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	//	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
	//	const FSlateIcon ProceduralContentProcessorsIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.DataLayers");
	//	LevelEditorTabManager->RegisterTabSpawner(LevelEditorProceduralContentProcessorBrowser, FOnSpawnTab::CreateRaw(this, &FProceduralContentProcessorModule::SpawnProceduralContentProcessorTab))
	//		.SetDisplayName(LOCTEXT("LevelEditorProceduralContentProcessorBrowser", "ProceduralContentProcessor"))
	//		.SetTooltipText(LOCTEXT("LevelEditorProceduralContentProcessorBrowserTooltipText", "Open the Procedural Content Processor Outliner."))
	//		.SetIcon(ProceduralContentProcessorsIcon)
	//		.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory());
	//});


}

void FProceduralContentProcessorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools")) {
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(ProceduralAssetProcessorAssetActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(ProceduralWorldProcessorAssetActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(ProceduralActorColorationProcessorAssetActions.ToSharedRef());
		ProceduralAssetProcessorAssetActions.Reset();
		ProceduralWorldProcessorAssetActions.Reset();
		ProceduralActorColorationProcessorAssetActions.Reset();
	}
	FEditorModeRegistry::Get().UnregisterMode(FProceduralContentProcessorEdMode::EdID);
}

//TSharedRef<SDockTab> FProceduralContentProcessorModule::SpawnProceduralContentProcessorTab(const FSpawnTabArgs& Args)
//{
//	return SNew(SDockTab)
//		.Label(LOCTEXT("ProceduralContentProcessor", "ProceduralContentProcessor"))
//		[
//			SNew(SProceduralContentProcessorEditorOutliner)
//		];
//}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProceduralContentProcessorModule, ProceduralContentProcessor)