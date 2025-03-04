#include "ProceduralContentProcessor.h"
#include "LevelEditor.h"
#include "Kismet/GameplayStatics.h"
#include "ProceduralContentProcessorAssetActions.h"
#include "Engine/StaticMeshActor.h"
#include "InstancedFoliageActor.h"
#include "Layers/LayersSubsystem.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Selection.h"
#include "DataLayer/DataLayerEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "SLevelViewport.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"
#include "StaticMeshAttributes.h"
#include "Engine/TextureRenderTarget.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralMeshConversion.h"
#include "StaticMeshCompiler.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PropertyCustomizationHelpers.h"

#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 4
#include "GameFramework/ActorPrimitiveColorHandler.h"
#endif

#define LOCTEXT_NAMESPACE "ProceduralContentProcessor"

void UProceduralContentProcessor::Activate()
{
	ReceiveActivate();
}

void UProceduralContentProcessor::Tick(const float InDeltaTime)
{
	ReceiveTick(InDeltaTime);
}

void UProceduralContentProcessor::Deactivate()
{
	ReceiveDeactivate();
}

void UProceduralContentProcessor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	ReceivePostEditChangeProperty(PropertyChangedEvent.GetPropertyName(), EObjectPropertyChangeType(PropertyChangedEvent.ChangeType));
	TryUpdateDefaultConfigFile();
}

TSharedPtr<SWidget> UProceduralContentProcessor::BuildWidget()
{
	if (UProceduralContentProcessorBlueprint* BP = Cast<UProceduralContentProcessorBlueprint>(GetClass()->ClassGeneratedBy)) {
		if (BP->OverrideUMGClass) {
			if(!BP->OverrideUMG)
				BP->OverrideUMG = NewObject<UUserWidget>(GetTransientPackage(), BP->OverrideUMGClass);
			return BP->OverrideUMG->TakeWidget();
		}
	}

	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	//DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
	DetailsViewArgs.bShowObjectLabel = false;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bAllowFavoriteSystem = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
	DetailsViewArgs.ViewIdentifier = FName("BlueprintDefaults");
	auto DetailView = EditModule.CreateDetailView(DetailsViewArgs);
	DetailView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& Node) {
		return !(Node.Property.HasAnyPropertyFlags(EPropertyFlags::CPF_Transient) || Node.Property.HasAnyPropertyFlags(EPropertyFlags::CPF_DisableEditOnInstance));
	}));
	DetailView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateLambda([](const FPropertyAndParent& Node) {
		return Node.Property.HasAnyPropertyFlags(EPropertyFlags::CPF_DisableEditOnInstance) || Node.Property.HasAnyPropertyFlags(EPropertyFlags::CPF_BlueprintReadOnly);
	}));
	DetailView->SetObject(this);
	return DetailView;
}

TSharedPtr<SWidget> UProceduralContentProcessor::BuildToolBar()
{
	return SNullWidget::NullWidget;
}

TScriptInterface<IAssetRegistry> UProceduralAssetProcessor::GetAllAssetRegistry()
{
	UClass* Class = LoadObject<UClass>(nullptr, TEXT("/Script/AssetRegistry.AssetRegistryHelpers"));
	UFunction* Func = Class->FindFunctionByName("GetAssetRegistry");
	TScriptInterface<IAssetRegistry> ReturnParam;
	FFrame Frame(Class->GetDefaultObject(), Func, nullptr, 0, Func->ChildProperties);
	Func->Invoke(Class->GetDefaultObject(), Frame, &ReturnParam);
	return ReturnParam;
}

TArray<AActor*> UProceduralWorldProcessor::GetAllActorsByName(FString InName, bool bCompleteMatching /*= false*/)
{
	TArray<AActor*> OutActors;
	if (!GEditor)
		return OutActors;
	UWorld* World = GetWorld();
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
	if (bCompleteMatching) {
		for (auto Actor : AllActors) {
			if (Actor->GetActorNameOrLabel() == InName) {
				OutActors.Add(Actor);
			}
		}
	}
	else {
		for (auto Actor : AllActors) {
			if (Actor->GetActorNameOrLabel().Contains(InName)) {
				OutActors.Add(Actor);
			}
		}
	}
	return OutActors;
}

UWorld* UProceduralWorldProcessor::GetWorld() const
{
	if (GEditor && GEditor->PlayWorld != nullptr) {
		return GEditor->PlayWorld;
	}
	return GEditor->GetEditorWorldContext().World();
}

void UProceduralActorColorationProcessor::Activate()
{
	Super::Activate();
	bVisible = true;
	RefreshVisibility();
}

void UProceduralActorColorationProcessor::Deactivate()
{
	Super::Deactivate();
	bVisible = false;
	RefreshVisibility();
}

FReply UProceduralActorColorationProcessor::OnVisibilityClicked()
{
	bVisible = !bVisible;
	RefreshVisibility();
	return FReply::Handled();
}

void UProceduralActorColorationProcessor::RefreshVisibility()
{
	if (bVisible) {
#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 4
		auto GetColorFunc = [this](const UPrimitiveComponent* PrimitiveComponent) {
			return this->Colour(PrimitiveComponent);
		};
		FActorPrimitiveColorHandler::Get().RegisterPrimitiveColorHandler(*GetClass()->GetDisplayNameText().ToString(), GetClass()->GetDisplayNameText(), GetColorFunc);
		FActorPrimitiveColorHandler::Get().SetActivePrimitiveColorHandler(*GetClass()->GetDisplayNameText().ToString(), GetWorld());

		FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		SLevelViewport* LevelViewport = LevelEditor.GetFirstActiveLevelViewport().Get();
		FLevelEditorViewportClient& LevelViewportClient = LevelViewport->GetLevelViewportClient();
		LevelViewportClient.EngineShowFlags.SetSingleFlag(FEngineShowFlags::SF_ActorColoration, true);
		LevelViewportClient.Invalidate();
#endif
	}
	else {
#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 4
		FActorPrimitiveColorHandler::Get().UnregisterPrimitiveColorHandler(*GetClass()->GetDisplayNameText().ToString());

		FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		if (SLevelViewport* LevelViewport = LevelEditor.GetFirstActiveLevelViewport().Get()) {
			FLevelEditorViewportClient& LevelViewportClient = LevelViewport->GetLevelViewportClient();
			LevelViewportClient.EngineShowFlags.SetSingleFlag(FEngineShowFlags::SF_ActorColoration, false);
			LevelViewportClient.Invalidate();
		}
#endif
	}
}

TSharedPtr<SWidget> UProceduralActorColorationProcessor::BuildToolBar()
{
	return PropertyCustomizationHelpers::MakeVisibilityButton(
		FOnClicked::CreateUObject(this, &UProceduralActorColorationProcessor::OnVisibilityClicked),
		FText(),
		TAttribute<bool>::CreateLambda([this]() { return bVisible; })
	).ToSharedPtr();
}

FLinearColor UProceduralActorColorationProcessor::Colour_Implementation(const UPrimitiveComponent* PrimitiveComponent)
{
	return FLinearColor::White;
}

#undef LOCTEXT_NAMESPACE

