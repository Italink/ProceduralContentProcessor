#include "ProceduralContentProcessorAssetActions.h"
#include "BlueprintEditorModule.h"
#include "ContentBrowserModule.h"
#include "EditorReimportHandler.h"
#include "IAssetTools.h"
#include "IBlutilityModule.h"
#include "IContentBrowserSingleton.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "LevelViewportTabContent.h"
#include "ToolMenuSection.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SWidget.h"

#define LOCTEXT_NAMESPACE "ProceduralContentProcessor"

FText FProceduralAssetProcessorAssetActions::GetName() const {
	return LOCTEXT("ProceduralAssetProcessor", "Procedural Asset Processor");
}

UClass* FProceduralAssetProcessorAssetActions::GetSupportedClass() const {
	return UProceduralAssetProcessor::StaticClass();
}

FColor FProceduralAssetProcessorAssetActions::GetTypeColor() const {
	return FColor(0, 100, 200);
}

uint32 FProceduralAssetProcessorAssetActions::GetCategories() {
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	return AssetTools.FindAdvancedAssetCategory("ProceduralContentProcessor");
}

void FProceduralAssetProcessorAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/) {
	for (UObject* Object : InObjects)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			bool bLetOpen = true;
			if (!Blueprint->SkeletonGeneratedClass || !Blueprint->GeneratedClass)
			{
				bLetOpen = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("FailedToLoadBlueprintWithContinue", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed! Do you want to continue (it can crash the editor)?"));
			}
			if (bLetOpen)
			{
				FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
				TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor(EToolkitMode::Standalone, nullptr, Blueprint);
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToLoadBlueprint", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

UProceduralAssetProcessorFactory::UProceduralAssetProcessorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {
	SupportedClass = UProceduralAssetProcessor::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UProceduralAssetProcessorFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (!InParent->GetPathName().Contains("/ProceduralContentProcessor")) {
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DirError", "This asset can only be created in the plugin's content directory : /ProceduralContentProcessor/*"));
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToFolders({ "/ProceduralContentProcessor" }, true);
		return NULL;
	}
	UBlueprint* NewBP = CastChecked<UBlueprint>(FKismetEditorUtilities::CreateBlueprint(
				SupportedClass, 
				InParent,
				InName,
				BPTYPE_Normal, 
				UProceduralContentProcessorBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass(), 
				NAME_None));
	int32 NodePositionY = 0;
	return NewBP;
}

FText FProceduralWorldProcessorAssetActions::GetName() const {
	return LOCTEXT("ProceduralWorldProcessor", "Procedural World Processor");
}

UClass* FProceduralWorldProcessorAssetActions::GetSupportedClass() const {
	return UProceduralWorldProcessor::StaticClass();
}

FColor FProceduralWorldProcessorAssetActions::GetTypeColor() const {
	return FColor(255, 156, 0);
}

uint32 FProceduralWorldProcessorAssetActions::GetCategories() {
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	return AssetTools.FindAdvancedAssetCategory("ProceduralContentProcessor");
}

void FProceduralWorldProcessorAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/) {
	for (UObject* Object : InObjects)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			bool bLetOpen = true;
			if (!Blueprint->SkeletonGeneratedClass || !Blueprint->GeneratedClass)
			{
				bLetOpen = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("FailedToLoadBlueprintWithContinue", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed! Do you want to continue (it can crash the editor)?"));
			}
			if (bLetOpen)
			{
				FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
				TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor(EToolkitMode::Standalone, nullptr, Blueprint);
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToLoadBlueprint", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

UProceduralWorldProcessorFactory::UProceduralWorldProcessorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {
	SupportedClass = UProceduralWorldProcessor::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UProceduralWorldProcessorFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (!InParent->GetPathName().Contains("/ProceduralContentProcessor")) {
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DirError", "This asset can only be created in the plugin's content directory : /ProceduralContentProcessor/*"));
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToFolders({ "/ProceduralContentProcessor" }, true);
		return NULL;
	}
	UBlueprint* NewBP = CastChecked<UBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		SupportedClass,
		InParent,
		InName,
		BPTYPE_Normal,
		UProceduralContentProcessorBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None));
	int32 NodePositionY = 0;
	return NewBP;
}

FText FProceduralActorColorationProcessorAssetActions::GetName() const {
	return LOCTEXT("ProceduralActorColorationProcessor", "Procedural Actor Coloration Processor");
}

UClass* FProceduralActorColorationProcessorAssetActions::GetSupportedClass() const {
	return UProceduralActorColorationProcessor::StaticClass();
}

FColor FProceduralActorColorationProcessorAssetActions::GetTypeColor() const {
	return FColor(60, 7, 248);
}

uint32 FProceduralActorColorationProcessorAssetActions::GetCategories() {
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	return AssetTools.FindAdvancedAssetCategory("ProceduralContentProcessor");
}

void FProceduralActorColorationProcessorAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/) {
	for (UObject* Object : InObjects)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			bool bLetOpen = true;
			if (!Blueprint->SkeletonGeneratedClass || !Blueprint->GeneratedClass)
			{
				bLetOpen = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("FailedToLoadBlueprintWithContinue", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed! Do you want to continue (it can crash the editor)?"));
			}
			if (bLetOpen)
			{
				FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
				TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor(EToolkitMode::Standalone, nullptr, Blueprint);
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToLoadBlueprint", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

UProceduralActorColorationProcessorFactory::UProceduralActorColorationProcessorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {
	SupportedClass = UProceduralActorColorationProcessor::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UProceduralActorColorationProcessorFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (!InParent->GetPathName().Contains("/ProceduralContentProcessor")) {
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DirError", "This asset can only be created in the plugin's content directory : /ProceduralContentProcessor/*"));
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToFolders({ "/ProceduralContentProcessor" }, true);
		return NULL;
	}
	UBlueprint* NewBP = CastChecked<UBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		SupportedClass,
		InParent,
		InName,
		BPTYPE_Normal,
		UProceduralContentProcessorBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None));
	int32 NodePositionY = 0;
	return NewBP;
}

#undef LOCTEXT_NAMESPACE