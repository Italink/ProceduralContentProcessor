#include "ProceduralObjectMatrixCustomization.h"
#include "Kismet/GameplayStatics.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Kismet2/CompilerResultsLog.h"
#include "PropertyCustomizationHelpers.h"
#include "IPropertyUtilities.h"
#include "Selection.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SSearchBox.h"
#include "ISinglePropertyView.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "ProceduralContentProcessor"

class SProceduralObjectMatrixInfoViewRow
	: public SMultiColumnTableRow<TSharedPtr<FProceduralObjectMatrixRow>> {
public:
	SLATE_BEGIN_ARGS(SProceduralObjectMatrixInfoViewRow) {}
	SLATE_ARGUMENT(TSharedPtr<FProceduralObjectMatrixRow>, MatrixInfo)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		MatrixInfo = InArgs._MatrixInfo;
		SMultiColumnTableRow<TSharedPtr<FProceduralObjectMatrixRow>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override {
		if (ColumnName == "Name" && MatrixInfo->Owner.IsValid()) {
			return SNew(SBox)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(4)
				[
					SNew(STextBlock)
					.Text(FText::FromString(MatrixInfo->Owner->GetName()))
				];
		}		
		if (auto Field = MatrixInfo->Find(ColumnName)) {
			return Field->BuildWidget().ToSharedRef();;
		}
		return SNew(SSpacer);
	}
private:
	TSharedPtr<FProceduralObjectMatrixRow> MatrixInfo;
};

TSharedRef<IPropertyTypeCustomization> FPropertyTypeCustomization_ProceduralObjectMatrix::MakeInstance()
{
	return MakeShared<FPropertyTypeCustomization_ProceduralObjectMatrix>();
}

FPropertyTypeCustomization_ProceduralObjectMatrix::FPropertyTypeCustomization_ProceduralObjectMatrix()
{
}

FPropertyTypeCustomization_ProceduralObjectMatrix::~FPropertyTypeCustomization_ProceduralObjectMatrix()
{
}

void FPropertyTypeCustomization_ProceduralObjectMatrix::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	void* Ptr = nullptr;
	InPropertyHandle->GetValueData(Ptr);
	ProceduralObjectMatrix = (FProceduralObjectMatrix*)Ptr;
	InHeaderRow
	.WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.Padding(5)
			[
				SNew(SSearchBox)
				.OnTextCommitted(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::OnSearchBoxTextCommitted)
				.HintText(LOCTEXT("Search", "Search..."))
			]
	];
}

void FPropertyTypeCustomization_ProceduralObjectMatrix::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	ChildBuilder.AddCustomRow(FText::FromString("List"))
	.Visibility(TAttribute<EVisibility>(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::GetVisibility))
	.ShouldAutoExpand()
	.WholeRowContent()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top)
	[
		SAssignNew(ListViewContainer, SBox)
		.MaxDesiredHeight(800)
	];
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::OnTick));
}

EVisibility FPropertyTypeCustomization_ProceduralObjectMatrix::GetVisibility() const
{
	return (ProceduralObjectMatrix&&ProceduralObjectMatrix->ObjectInfoList.IsEmpty())? EVisibility::Collapsed : EVisibility::Visible;
}

EColumnSortMode::Type FPropertyTypeCustomization_ProceduralObjectMatrix::GetColumnSortMode(const FName ColumnId) const
{
	if (ColumnId == ProceduralObjectMatrix->SortedColumnName)
	{
		return ProceduralObjectMatrix->SortMode;
	}
	return EColumnSortMode::None;
}

void FPropertyTypeCustomization_ProceduralObjectMatrix::OnSort(EColumnSortPriority::Type InPriorityType, const FName& InName, EColumnSortMode::Type InType)
{
	ProceduralObjectMatrix->SortedColumnName = InName;
	ProceduralObjectMatrix->SortMode = InType;

	TFunction<bool(bool)> SortType = [](bool Var) {return Var; };
	if (InType == EColumnSortMode::Descending) {
		SortType = [](bool Var) {return !Var; };
	}
	ProceduralObjectMatrix->ObjectInfoList.StableSort([&](const TSharedPtr<FProceduralObjectMatrixRow>& Lhs, const TSharedPtr<FProceduralObjectMatrixRow>& Rhs) {
		FString LhsValue;
		if (auto LhsItem = Lhs->Find(InName)) {
			LhsValue = LhsItem->GetText();
		}
		FString RhsValue;
		if (auto RhsItem = Rhs->Find(InName)) {
			RhsValue = RhsItem->GetText();
		}
		if (LhsValue.IsNumeric() && RhsValue.IsNumeric())
			return SortType(FCString::Atod(*LhsValue) < FCString::Atod(*RhsValue));
		return SortType(LhsValue < RhsValue);
	});

	ProceduralObjectMatrix->ObjectInfoListView->RequestListRefresh();
}

TSharedRef<ITableRow> FPropertyTypeCustomization_ProceduralObjectMatrix::OnGenerateRow(TSharedPtr<FProceduralObjectMatrixRow> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SProceduralObjectMatrixInfoViewRow, OwnerTable)
		.MatrixInfo(InInfo);
}

void FPropertyTypeCustomization_ProceduralObjectMatrix::OnMouseButtonDoubleClick(TSharedPtr<FProceduralObjectMatrixRow> InInfo)
{
	if (InInfo) {
		if (auto Actor = Cast<AActor>(InInfo->Owner)) {
			GEditor->GetSelectedActors()->Modify();
			GEditor->GetSelectedActors()->BeginBatchSelectOperation();
			GEditor->SelectNone(false, true, true);
			GEditor->SelectActor(Actor, true, false, true);
			GEditor->MoveViewportCamerasToActor({ Actor }, true);
			GEditor->GetSelectedActors()->EndBatchSelectOperation(/*bNotify*/false);
			GEditor->NoteSelectionChange();
		}
		else if (InInfo->Owner->IsAsset()) {
			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			TArray<UObject*> SyncObjects;
			SyncObjects.Add(InInfo->Owner.Get());
			ContentBrowserModule.Get().SyncBrowserToAssets(SyncObjects, true);
		}
	}
}

void FPropertyTypeCustomization_ProceduralObjectMatrix::OnSearchBoxTextCommitted(const FText& InNewText, ETextCommit::Type InTextCommit) {
	if (ProceduralObjectMatrix == nullptr || ProceduralObjectMatrix->ObjectInfoListView == nullptr)
		return;
	if (InNewText.IsEmpty()) {
		ProceduralObjectMatrix->ObjectInfoListView->SetListItemsSource(ProceduralObjectMatrix->ObjectInfoList);
		CurrInfoList = &ProceduralObjectMatrix->ObjectInfoList;
		CurrentSearchKeyword = FString();
	}
	else {
		SearchInfoList.Reset();
		CurrentSearchKeyword = InNewText.ToString();
		for (auto& ObjectInfo : ProceduralObjectMatrix->ObjectInfoList) {
			if (ObjectInfo->Owner.IsValid()) {
				if (ObjectInfo->Owner->GetName().Contains(CurrentSearchKeyword)) {
					SearchInfoList.Add(ObjectInfo);
				}
				else {
					for (const auto& Field : ObjectInfo->Fields) {
						if (Field->GetText().Contains(CurrentSearchKeyword)) {
							SearchInfoList.Add(ObjectInfo);
							break;
						}
					}
				}
			}
		}
		ProceduralObjectMatrix->ObjectInfoListView->SetListItemsSource(SearchInfoList);
		CurrInfoList = &SearchInfoList;
	}
	ProceduralObjectMatrix->ObjectInfoListView->RebuildList();
}

void FPropertyTypeCustomization_ProceduralObjectMatrix::RebuildListView()
{
	TSharedPtr<SHeaderRow> HeaderRow;
	auto View = SAssignNew(ProceduralObjectMatrix->ObjectInfoListView, SListView<TSharedPtr<FProceduralObjectMatrixRow>>)
		.ScrollbarVisibility(EVisibility::Visible)
		.ListItemsSource(&ProceduralObjectMatrix->ObjectInfoList)
		.OnGenerateRow_Raw(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::OnGenerateRow)
		.OnMouseButtonDoubleClick_Raw(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::OnMouseButtonDoubleClick)
		.HeaderRow(
			SAssignNew(HeaderRow, SHeaderRow)
			.ResizeMode(ESplitterResizeMode::FixedPosition)
			.CanSelectGeneratedColumn(true)
		);
	HeaderRow->AddColumn(
		SHeaderRow::Column("Name")
		.HAlignHeader(EHorizontalAlignment::HAlign_Center)
		.DefaultLabel(LOCTEXT("Name","Name"))
		.SortMode_Raw(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::GetColumnSortMode, FName("Name"))
		.SortPriority(EColumnSortPriority::Primary)
		.OnSort_Raw(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::OnSort)
	);
	for (auto FieldKey : ProceduralObjectMatrix->FieldKeys) {
		if (!FieldKey.IsNone()) {
			HeaderRow->AddColumn(
				SHeaderRow::Column(FieldKey)
				.HAlignHeader(EHorizontalAlignment::HAlign_Center)
				.DefaultLabel(FText::FromName(FieldKey))
				.SortMode_Raw(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::GetColumnSortMode, FieldKey)
				.SortPriority(EColumnSortPriority::Primary)
				.OnSort_Raw(this, &FPropertyTypeCustomization_ProceduralObjectMatrix::OnSort)
			);
		}
	}
	ListViewContainer->SetContent(View);
}

bool FPropertyTypeCustomization_ProceduralObjectMatrix::OnTick(float Delta)
{
	if (ProceduralObjectMatrix && ProceduralObjectMatrix->bIsDirty) {
		ProceduralObjectMatrix->bIsDirty = false;
		RebuildListView();
	}
	return true;
}

#undef LOCTEXT_NAMESPACE