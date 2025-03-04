#pragma once

#include "ProceduralContentProcessor.h"
#include "AssetTypeActions/AssetTypeActions_Blueprint.h"
#include "ProceduralContentProcessorAssetActions.generated.h"

UCLASS(Blueprintable, meta = (ShowWorldContextPin, DisableNativeTick))
class PROCEDURALCONTENTPROCESSOR_API UProceduralContentProcessorBlueprint : public UBlueprint
{
	GENERATED_BODY()
	virtual bool SupportedByDefaultBlueprintFactory() const override {
		return false;
	}
	virtual bool AlwaysCompileOnLoad() const override {
		return false;
	}
#if WITH_EDITOR
	virtual void GetReparentingRules(TSet<const UClass*>& AllowedChildrenOfClasses,
		TSet<const UClass*>& DisallowedChildrenOfClasses) const
	{
		AllowedChildrenOfClasses.Add(UProceduralContentProcessor::StaticClass());
		TArray<UClass*> Children;
		GetDerivedClasses(UProceduralContentProcessor::StaticClass(), Children);
		for (auto Child : Children) {
			if (!Child->HasAnyClassFlags(CLASS_Abstract)) {
				DisallowedChildrenOfClasses.Add(Child);
			}
		}
	}
#endif
public:
	UPROPERTY(EditAnywhere, Category = ProceduralContentProcessor)
	float SortPriority;

	UPROPERTY(EditAnywhere, Category = ProceduralContentProcessor)
	FString DocumentHyperlink = "https://github.com/Italink/ProceduralContentProcessor";

	UPROPERTY(EditAnywhere, Category = ProceduralContentProcessor)
	TSubclassOf<UUserWidget> OverrideUMGClass = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> OverrideUMG;
};

class PROCEDURALCONTENTPROCESSOR_API FProceduralAssetProcessorAssetActions : public FAssetTypeActions_Blueprint
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual bool CanLocalize() const override { return false; };
};

UCLASS()
class PROCEDURALCONTENTPROCESSOR_API UProceduralAssetProcessorFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override { return true; }
protected:
	UPROPERTY(EditAnywhere, Category = WidgetBlueprintFactory, meta = (AllowAbstract = ""))
	TSubclassOf<class UObject> ParentClass;
};

class PROCEDURALCONTENTPROCESSOR_API FProceduralWorldProcessorAssetActions : public FAssetTypeActions_Blueprint
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual bool CanLocalize() const override { return false; };
};

UCLASS()
class PROCEDURALCONTENTPROCESSOR_API UProceduralWorldProcessorFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override { return true; }
protected:
	UPROPERTY(EditAnywhere, Category = WidgetBlueprintFactory, meta = (AllowAbstract = ""))
	TSubclassOf<class UObject> ParentClass;
};

class PROCEDURALCONTENTPROCESSOR_API FProceduralActorColorationProcessorAssetActions : public FAssetTypeActions_Blueprint
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual bool CanLocalize() const override { return false; };
};

UCLASS()
class PROCEDURALCONTENTPROCESSOR_API UProceduralActorColorationProcessorFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override { return true; }
protected:
	UPROPERTY(EditAnywhere, Category = WidgetBlueprintFactory, meta = (AllowAbstract = ""))
	TSubclassOf<class UObject> ParentClass;
};