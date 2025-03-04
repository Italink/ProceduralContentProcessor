#pragma once

#include "ProceduralObjectMatrix.h"
#include "IPropertyTypeCustomization.h"

struct FProceduralObjectMatrixRow;

class FPropertyTypeCustomization_ProceduralObjectMatrix
	: public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	FPropertyTypeCustomization_ProceduralObjectMatrix();

	~FPropertyTypeCustomization_ProceduralObjectMatrix();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)override;

	EVisibility GetVisibility() const;
	EColumnSortMode::Type GetColumnSortMode(const FName ColumnId) const;
	void OnSort(EColumnSortPriority::Type InPriorityType, const FName& InName, EColumnSortMode::Type InType);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FProceduralObjectMatrixRow> InInfo, const TSharedRef<STableViewBase>& OwnerTable);
	void OnMouseButtonDoubleClick(TSharedPtr<FProceduralObjectMatrixRow> InInfo);
	void OnSearchBoxTextCommitted(const FText& InNewText, ETextCommit::Type InTextCommit);
	void RebuildListView();
	bool OnTick(float Delta);
private:
	TSharedPtr<IPropertyUtilities> Utils;
	TSharedPtr<IPropertyHandle> ModulesHandle;
	FProceduralObjectMatrix* ProceduralObjectMatrix;
	TArray<TSharedPtr<FProceduralObjectMatrixRow>> SearchInfoList;
	TArray<TSharedPtr<FProceduralObjectMatrixRow>>* CurrInfoList = nullptr;
	FString CurrentSearchKeyword;
	TSharedPtr<SBox> ListViewContainer;
};