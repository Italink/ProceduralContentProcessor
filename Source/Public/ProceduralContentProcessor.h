#pragma once

#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "ProceduralContentProcessor.generated.h"

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum EObjectPropertyChangeType : uint64
{
	None = 0, 
	Unspecified = 1 << 0,
	//Array Add
	ArrayAdd = 1 << 1,
	//Array Remove
	ArrayRemove = 1 << 2,
	//Array Clear
	ArrayClear = 1 << 3,
	//Value Set
	ValueSet = 1 << 4,
	//Duplicate
	Duplicate = 1 << 5,
	//Interactive, e.g. dragging a slider. Will be followed by a ValueSet when finished.
	Interactive = 1 << 6,
	//Redirected.  Used when property references are updated due to content hot-reloading, or an asset being replaced during asset deletion (aka, asset consolidation).
	Redirected = 1 << 7,
	// Array Item Moved Within the Array
	ArrayMove = 1 << 8,
	// Edit Condition State has changed
	ToggleEditable = 1 << 9,
};

UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories, config = ProceduralContentProcessor, defaultconfig)
class PROCEDURALCONTENTPROCESSOR_API UProceduralContentProcessor: public UObject {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "ProceduralContentProcessor", meta = (DisplayName = "Activate"))
	void ReceiveActivate();

	UFUNCTION(BlueprintImplementableEvent, Category = "ProceduralContentProcessor", meta = (DisplayName = "Tick"))
	void ReceiveTick(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProceduralContentProcessor", meta = (DisplayName = "Deactivate"))
	void ReceiveDeactivate();

	UFUNCTION(BlueprintImplementableEvent, Category = "ProceduralContentProcessor", meta = (DisplayName = "PostEditChangeProperty"))
	void ReceivePostEditChangeProperty(FName PropertyName, EObjectPropertyChangeType PropertyChangeType);

	virtual void Activate();

	virtual void Tick(const float InDeltaTime);

	virtual void Deactivate();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override{
		ReceivePostEditChangeProperty(PropertyChangedEvent.GetPropertyName(), EObjectPropertyChangeType(PropertyChangedEvent.ChangeType));
		TryUpdateDefaultConfigFile();
	}

	virtual TSharedPtr<SWidget> BuildWidget();

	virtual TSharedPtr<SWidget> BuildToolBar();
};

UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories, config = ProceduralContentProcessor, defaultconfig)
class PROCEDURALCONTENTPROCESSOR_API UProceduralAssetProcessor: public UProceduralContentProcessor {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "ProceduralContentProcessor")
	static TScriptInterface<IAssetRegistry> GetAssetRegistry();
};

UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories, config = ProceduralContentProcessor, defaultconfig)
class PROCEDURALCONTENTPROCESSOR_API UProceduralWorldProcessor: public UProceduralContentProcessor {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static TArray<AActor*> GetAllActorsByName(FString InName, bool bCompleteMatching = false);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static void DisableInstancedFoliageMeshShadow(TArray<UStaticMesh*> InMeshes);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static TArray<AActor*> BreakISM(AActor* InISMActor, bool bDestorySourceActor = true);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static AActor* MergeISM(TArray<AActor*> InSourceActors, TSubclassOf<UInstancedStaticMeshComponent> InISMClass, bool bDestorySourceActor = true);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static void SetHLODLayer(AActor* InActor, UHLODLayer *InHLODLayer);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static void SelectActor(AActor* InActor);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static void LookAtActor(AActor* InActor);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor", meta = (WorldContext = "WorldContextObject"))
	static AActor* SpawnTransientActor(UObject* WorldContextObject, TSubclassOf<AActor> Class, FTransform Transform);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static AActor* ReplaceActor(AActor* InSrc, TSubclassOf<AActor> InDst, bool bNoteSelectionChange = false);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static void ReplaceActors(TMap<AActor*, TSubclassOf<AActor>> ActorMap, bool bNoteSelectionChange);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static void ActorSetIsSpatiallyLoaded(AActor* Actor, bool bIsSpatiallyLoaded);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static bool ActorAddDataLayer(AActor* Actor, UDataLayerAsset* DataLayerAsset);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static bool ActorRemoveDataLayer(AActor* Actor, UDataLayerAsset* DataLayerAsset);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static bool ActorContainsDataLayer(AActor* Actor, UDataLayerAsset* DataLayerAsset);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static FName ActorGetRuntimeGrid(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "ProceduralContentProcessor")
	static void ActorSetRuntimeGrid(AActor* Actor, FName GridName);
protected:
	virtual UWorld* GetWorld() const override;
};

UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories, config = ProceduralContentProcessor, defaultconfig)
class PROCEDURALCONTENTPROCESSOR_API UProceduralActorColorationProcessor: public UProceduralWorldProcessor {
	GENERATED_BODY()
public:
	virtual void Activate() override;
	virtual void Deactivate() override;

	UFUNCTION(BlueprintNativeEvent, Category = "ProceduralContentProcessor", meta = (DisplayName = "Colour"))
	FLinearColor Colour(const UPrimitiveComponent* PrimitiveComponent);

	FReply OnVisibilityClicked();

	void RefreshVisibility();

	virtual TSharedPtr<SWidget> BuildToolBar() override;
private:
	bool bVisible = true;
};