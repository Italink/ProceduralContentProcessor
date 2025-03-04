#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSuggestionTextBox.h"

class UProceduralContentProcessor;

struct FProcessorOutlinerField
{
	TArray<TSharedPtr<FProcessorOutlinerField>> Children;

	virtual FText GetName() = 0;
	virtual FText GetTooltip() = 0;
	virtual UClass* GetProcessorClass() = 0;
	virtual ~FProcessorOutlinerField(){};
};

struct FProcessorEditorField_Category: public FProcessorOutlinerField
{
	FText CategoryText;
	FText GetName() override { return CategoryText; }
	FText GetTooltip() override { return CategoryText; }
	UClass* GetProcessorClass() override { return nullptr; }
};

struct FProcessorOutlinerField_Processor : public FProcessorOutlinerField
{
	TObjectPtr<UClass> ProcessorClass;

	FText GetName() override { return ProcessorClass->GetDisplayNameText(); }
	FText GetTooltip() override { return ProcessorClass->GetToolTipText(); }
	UClass* GetProcessorClass() override { return ProcessorClass; }
};

class SProceduralContentProcessorEditorOutliner : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SProceduralContentProcessorEditorOutliner) {}
	SLATE_END_ARGS()
public:
	~SProceduralContentProcessorEditorOutliner();
	void Construct(const FArguments& InArgs);
	void SetCurrentProcessor(UClass* InProcessorClass, bool bSaveConfig = true);
	void RequestRefreshProcessorList();
protected:
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("ProceduralContentProcessor"); }

	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void RefreshProcessorList();

	void OnMapChanged(uint32 MapChangeFlags);
	void OnMapCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void OnPreSaveWorld(UWorld* InWorld, FObjectPreSaveContext ObjectSaveContext);
	void OnBlueprintPreCompile(UBlueprint* InBlueprint);
	void OnBlueprintCompiled(UBlueprint* InBlueprint);
	void OnSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnAssetAdded(const FAssetData& InData);
	void OnAssetRemoved(const FAssetData& InData);
	void OnAssetUpdated(const FAssetData& InData);
	void OnBrowseToCurrentProcessorBlueprint();

	void OnProcessorTextChanged(const FText& InNewText);
	FText OnGetCurrentProcessorText() const;

	FText OnGetCurrentProcessorTooltipText() const;
	TSharedRef<SWidget> OnGetProcessorSelectorMenu();

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FProcessorOutlinerField> InField, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildren(TSharedPtr<FProcessorOutlinerField> InField, TArray< TSharedPtr<FProcessorOutlinerField>> & OutChildren);
	void OnSelectionChanged(TSharedPtr<FProcessorOutlinerField> InField, ESelectInfo::Type SelectInfo);
	bool IsSelectableOrNavigable(TSharedPtr<FProcessorOutlinerField> InField) const;
private:
	TArray<UClass*> ProcessorList;
	TObjectPtr<UProceduralContentProcessor> CurrentProcessor;
	TArray<UBlueprint*> BlueprintCompileWatcher;
	TSharedPtr<STextBlock> CurrentProcessorBox;
	FText CurrentProcessorText;
	TArray<TSharedPtr<FProcessorOutlinerField>> TopLevelProcessorField;
	TSharedPtr<SComboButton> ProcessorComboButton;
	TSharedPtr<STreeView<TSharedPtr<FProcessorOutlinerField>>> ProcessorTreeView;

	TSharedPtr<SBox> ProcessorDocumentationContainter;
	TSharedPtr<SBox> ProcessorWidgetContainter;
	TSharedPtr<SBox> ProcessorToolBarContainter;

	bool bNeedRefreshProcessorList = false;

};