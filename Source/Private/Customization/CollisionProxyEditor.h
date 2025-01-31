#pragma once

#include "ProceduralContentProcessor.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_NEWER_THAN(5, 5, 0)
#include "MeshMerge/MeshApproximationSettings.h"
#else
#include "Engine/MeshMerging.h"
#endif
#include "CollisionProxyEditor.generated.h"

UENUM()
enum class ECollisionProxyClearMethod {
	None,
	DisableSourceActorCollision,
	CleanupSourceMeshCollision
};

UCLASS(Abstract, DefaultToInstanced, EditInlineNew, CollapseCategories, Blueprintable, BlueprintType, config = ProceduralContentProcessor, defaultconfig )
class PROCEDURALCONTENTPROCESSOR_API UCollisionProxyGenerateMethodBase : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Config)
	bool bMergeMaterials = true;

	UPROPERTY(EditAnywhere, Config)
	ECollisionProxyClearMethod ClearMethod = ECollisionProxyClearMethod::DisableSourceActorCollision;

	virtual AStaticMeshActor* Generate(TArray<UStaticMeshComponent*> SourceMeshCompList) { return nullptr; };
};

UCLASS(meta = (DisplayName = "Approximate"), CollapseCategories)
class PROCEDURALCONTENTPROCESSOR_API UCollisionProxyGenerateMethod_Approximate : public UCollisionProxyGenerateMethodBase
{
	GENERATED_BODY()
public:
	virtual AStaticMeshActor* Generate(TArray<UStaticMeshComponent*> SourceMeshCompList) override;

	UPROPERTY(EditAnywhere, Config)
	FMeshApproximationSettings ApproximationSettings;
};

UCLASS(meta = (DisplayName = "Proxy"), CollapseCategories)
class PROCEDURALCONTENTPROCESSOR_API UCollisionProxyGenerateMethod_Proxy : public UCollisionProxyGenerateMethodBase
{
	GENERATED_BODY()
public:
	virtual AStaticMeshActor* Generate(TArray<UStaticMeshComponent*> SourceMeshCompList) override;

	UPROPERTY( EditAnywhere, Config, meta = (ClampMin = "1", ClampMax = "1200", UIMin = "1", UIMax = "1200"))
	int32 ScreenSize = 50;

	/** Override when converting multiple meshes for proxy LOD merging. Warning, large geometry with small sampling has very high memory costs*/
	UPROPERTY(EditAnywhere, Config, meta = (EditCondition = "bOverrideVoxelSize", ClampMin = "0.1", DisplayName = "Override Spatial Sampling Distance"))
	float VoxelSize = 3.f;

	UPROPERTY(EditAnywhere, Config)
	float MergeDistance = 0;

	UPROPERTY(EditAnywhere, Config, meta = (DisplayAfter = "ScreenSize"))
	bool bCalculateCorrectLODModel = 0;

	/** If true, Spatial Sampling Distance will not be automatically computed based on geometry and you must set it directly */
	UPROPERTY(EditAnywhere, Config, AdvancedDisplay, meta = (InlineEditConditionToggle))
	bool bOverrideVoxelSize = false;
};

UCLASS(EditInlineNew, CollapseCategories, config = ProceduralContentProcessor, defaultconfig, Category = "WorldPartition", meta = (DisplayName = "Collision Proxy Editor"))
class UCollisionProxyEditor : public UProceduralWorldProcessor {
	GENERATED_BODY()
protected:
	virtual void Activate() override;
	virtual void Deactivate() override;
	void OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh);

	UFUNCTION(CallInEditor)
	void Generate();

	void RefreshCollisionProxyFinder();
private:
	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<AActor>> SourceActors;

	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<UStaticMesh>> SourceMeshes;

	UPROPERTY(EditAnywhere, Instanced, NoClear, meta = (ShowOnlyInnerProperties))
	UCollisionProxyGenerateMethodBase* GenerateMethod = NewObject<UCollisionProxyGenerateMethod_Approximate>();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AStaticMeshActor> CollisionProxyActor;

	UPROPERTY(EditAnywhere, Config)
	FSoftObjectPath DebugMaterial;

	UPROPERTY(VisibleAnywhere)
	int VertexCount;

	UPROPERTY(VisibleAnywhere)
	int TriangleCount;

	FDelegateHandle OnActorSelectionChangedHandle;

	TMap<FGuid, TObjectPtr<AActor>> CollisionProxyFinder;
};

