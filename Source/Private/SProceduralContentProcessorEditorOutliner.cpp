#include "SProceduralContentProcessorEditorOutliner.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/GameplayStatics.h"
#include "WorldPartition/WorldPartition.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ProceduralContentProcessorModule.h"
#include "BlueprintEditorModule.h"
#include "ProceduralContentProcessorModule.h"
#include "Editor.h"
#include "Selection.h"
#include "Engine/World.h"
#include "EditorSupportDelegates.h"
#include "LevelEditor.h"
#include "ProceduralContentProcessor.h"
#include "ProceduralContentProcessorAssetActions.h"
#include "ProceduralContentProcessorSettings.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/SlateIconFinder.h"
#include "IDocumentation.h"

SProceduralContentProcessorEditorOutliner::~SProceduralContentProcessorEditorOutliner()
{
	if (CurrentProcessor) {
		CurrentProcessor->Deactivate();
		CurrentProcessor = nullptr;
	}
}

void SProceduralContentProcessorEditorOutliner::Construct(const FArguments& InArgs)
{
	ChildSlot[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			[
				SAssignNew(ProcessorComboButton, SComboButton)
				.OnGetMenuContent(this, &SProceduralContentProcessorEditorOutliner::OnGetProcessorSelectorMenu)
				.ToolTipText(this, &SProceduralContentProcessorEditorOutliner::OnGetCurrentProcessorTooltipText)
				.ContentPadding(FMargin(2.0f, 2.0f))
				.ButtonContent()
				[
					
					SAssignNew(CurrentProcessorBox, STextBlock)
					.Text(this, &SProceduralContentProcessorEditorOutliner::OnGetCurrentProcessorText)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2)
			.HAlign(HAlign_Right)
			[
				SAssignNew(ProcessorDocumentationContainter, SBox)
				/*IDocumentation::Get()->CreateAnchor("https://www.baidu.com/", FString(), "Hello")*/
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2)
			.HAlign(HAlign_Right)
			[
				PropertyCustomizationHelpers::MakeBrowseButton(
					FSimpleDelegate::CreateSP(this, &SProceduralContentProcessorEditorOutliner::OnBrowseToCurrentProcessorBlueprint),
					FText()
				)
			]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SAssignNew(ProcessorToolBarContainter, SBox)

				]
		]
		+ SVerticalBox::Slot()
		.Padding(5)
		[
			SAssignNew(ProcessorWidgetContainter, SBox)
		]
	];

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.OnAssetAdded().AddSP(this, &SProceduralContentProcessorEditorOutliner::OnAssetAdded);
	AssetRegistry.OnAssetRemoved().AddSP(this, &SProceduralContentProcessorEditorOutliner::OnAssetRemoved);
	AssetRegistry.OnAssetUpdated().AddSP(this, &SProceduralContentProcessorEditorOutliner::OnAssetUpdated);

	FEditorDelegates::MapChange.AddSP(this, &SProceduralContentProcessorEditorOutliner::OnMapChanged);
	GEditor->OnBlueprintPreCompile().AddSP(this,& SProceduralContentProcessorEditorOutliner::OnBlueprintPreCompile);

	FWorldDelegates::OnWorldCleanup.AddSP(this, &SProceduralContentProcessorEditorOutliner::OnMapCleanup);
	FEditorDelegates::PreSaveWorldWithContext.AddSP(this, &SProceduralContentProcessorEditorOutliner::OnPreSaveWorld);

	RequestRefreshProcessorList();
}

void SProceduralContentProcessorEditorOutliner::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (bNeedRefreshProcessorList) {
		RefreshProcessorList();
		bNeedRefreshProcessorList = false;
	}
	if (CurrentProcessor) {
		CurrentProcessor->Tick(InDeltaTime);
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SProceduralContentProcessorEditorOutliner::RefreshProcessorList()
{
	ProcessorList.Reset();
	for (auto Watcher : BlueprintCompileWatcher) {
		Watcher->OnCompiled().RemoveAll(this);
		Watcher->OnChanged().RemoveAll(this);
	}
	BlueprintCompileWatcher.Reset();
	TopLevelProcessorField.Reset();

	TArray<UClass*> NativeClasses;
	GetDerivedClasses(UProceduralContentProcessor::StaticClass(), NativeClasses);
	for (auto Class : NativeClasses) {
		if (Class->HasAnyClassFlags(EClassFlags::CLASS_Abstract)
			|| Class->GetName().StartsWith(TEXT("SKEL_"))
			|| Class->GetName().StartsWith(TEXT("REINST_"))
			|| Class->HasAnyClassFlags(EClassFlags::CLASS_NewerVersionExists)
			) {
			continue;
		}
		ProcessorList.AddUnique(Class);
	}
	TArray<FAssetData> AssetDataList;
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.GetAssetsByPath("/ProceduralContentProcessor", AssetDataList, true);
	for (auto AssetData : AssetDataList) {
		if (UProceduralContentProcessorBlueprint* Blueprint = Cast<UProceduralContentProcessorBlueprint>(AssetData.GetAsset())) {
			if (Blueprint->GeneratedClass && !Blueprint->GeneratedClass->HasAnyClassFlags(EClassFlags::CLASS_Abstract)) {
				ProcessorList.AddUnique(Blueprint->GeneratedClass);
				Blueprint->OnCompiled().AddSP(this,&SProceduralContentProcessorEditorOutliner::OnBlueprintCompiled);
				BlueprintCompileWatcher.Add(Blueprint);
			}
		}
	}

	ProcessorList.Sort([](const UClass& A, const UClass& B) {
		static auto GetSortPriority = [](const UClass& Class){
			if (UProceduralContentProcessorBlueprint* Blueprint = Cast<UProceduralContentProcessorBlueprint>(Class.ClassGeneratedBy)) 
				return Blueprint->SortPriority;
			if(Class.HasMetaData("SortPriority"))
				return FCString::Atof(*Class.GetMetaData("SortPriority"));
			return 0.0f;
		};
		return GetSortPriority(A) < GetSortPriority(B);
	});

	TSharedPtr<FProcessorEditorField_Category> CommonCategoryField = MakeShared<FProcessorEditorField_Category>();
	CommonCategoryField->CategoryText = FText::FromString("Common");
	TopLevelProcessorField.Add(CommonCategoryField);

	for (auto Processor : ProcessorList) {
		FString CategoryChain = Processor->GetMetaData("Namespace");
		if (CategoryChain.IsEmpty())
			CategoryChain = Processor->GetMetaData("Category");
		if(CategoryChain.IsEmpty())
			CategoryChain = TEXT("Common");
		TArray<FString> CategoryFieldNames;
		CategoryChain.ParseIntoArray(CategoryFieldNames,TEXT("."));
		TArray<TSharedPtr<FProcessorOutlinerField>>* CurrentFields = &TopLevelProcessorField;
		for (int i = 0; i < CategoryFieldNames.Num(); i++) {
			FString FieldName = CategoryFieldNames[i];
			TSharedPtr<FProcessorOutlinerField> CategoryField;
			for (auto Field : *CurrentFields) {
				if (Field->GetName().ToString() == FieldName) {
					CategoryField = Field;
					break;
				}
			}
			if (!CategoryField) {
				TSharedPtr<FProcessorEditorField_Category> NewCategoryField = MakeShared<FProcessorEditorField_Category>();
				NewCategoryField->CategoryText = FText::FromString(FieldName);
				CategoryField = NewCategoryField;
				CurrentFields->Add(NewCategoryField);
			}
			CurrentFields = &CategoryField->Children;
		}
		TSharedPtr<FProcessorOutlinerField_Processor> NewProcessorField = MakeShared<FProcessorOutlinerField_Processor>();
		NewProcessorField->ProcessorClass = Processor;
		CurrentFields->Add(NewProcessorField);
	}
	if (ProcessorTreeView) {
		ProcessorTreeView->RebuildList();
	}
	FSoftClassPath LastSelectedClassPath = GetMutableDefault<UProceduralContentProcessorSettings>()->LastProcessorClass;
	UClass* LastSelectedClass = Cast<UClass>(LastSelectedClassPath.TryLoad());
	if (LastSelectedClass) {
		if (ProcessorList.Contains(LastSelectedClass)) {
			SetCurrentProcessor(LastSelectedClass);
		}
		else {
			UBlueprint* BP = Cast<UBlueprint>(LastSelectedClass->ClassGeneratedBy);
			if (BP && ProcessorList.Contains(BP->GeneratedClass)) {
				SetCurrentProcessor(BP->GeneratedClass);
			}
		}
	}
}

void SProceduralContentProcessorEditorOutliner::SetCurrentProcessor(UClass* InProcessorClass, bool bSaveConfig)
{
	if (CurrentProcessor) {
		CurrentProcessor->SaveConfig(CPF_Config, *CurrentProcessor->GetDefaultConfigFilename());
		CurrentProcessor->Deactivate();
	}
	if (bSaveConfig) {
		GetMutableDefault<UProceduralContentProcessorSettings>()->LastProcessorClass = InProcessorClass;
		GetMutableDefault<UProceduralContentProcessorSettings>()->TryUpdateDefaultConfigFile();
	}
	if (InProcessorClass) {
		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		CurrentProcessor = NewObject<UProceduralContentProcessor>(EditorWorld != nullptr ? EditorWorld->GetPackage() : GetTransientPackage(), InProcessorClass, NAME_None, RF_Transient);
		CurrentProcessor->LoadConfig();
		CurrentProcessor->Activate();
		ProcessorWidgetContainter->SetContent(CurrentProcessor->BuildWidget().ToSharedRef());
		ProcessorToolBarContainter->SetContent(CurrentProcessor->BuildToolBar().ToSharedRef());
		CurrentProcessorText = InProcessorClass->GetDisplayNameText();
		FString DocumentHyperlink;
		if (UProceduralContentProcessorBlueprint* Blueprint = Cast<UProceduralContentProcessorBlueprint>(InProcessorClass->ClassGeneratedBy)){
			DocumentHyperlink = Blueprint->DocumentHyperlink;
		}
		else {
			DocumentHyperlink = InProcessorClass->GetMetaData("DocumentHyperlink");
		}
		if (DocumentHyperlink.IsEmpty()) {
			ProcessorDocumentationContainter->SetContent(SNullWidget::NullWidget);
		}
		else {
			ProcessorDocumentationContainter->SetContent(IDocumentation::Get()->CreateAnchor(DocumentHyperlink, DocumentHyperlink, "Document"));
		}
	}
	else {
		CurrentProcessor = nullptr;
		CurrentProcessorText = FText();
		ProcessorDocumentationContainter->SetContent(SNullWidget::NullWidget);
		ProcessorWidgetContainter->SetContent(SNullWidget::NullWidget);
		ProcessorToolBarContainter->SetContent(SNullWidget::NullWidget);
	}
}

void SProceduralContentProcessorEditorOutliner::RequestRefreshProcessorList()
{
	SetCurrentProcessor(nullptr, false);
	bNeedRefreshProcessorList = true;
}

void SProceduralContentProcessorEditorOutliner::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(CurrentProcessor);
	Collector.AddReferencedObjects(ProcessorList);
}

void SProceduralContentProcessorEditorOutliner::OnMapChanged(uint32 MapChangeFlags)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	RequestRefreshProcessorList();
}

void SProceduralContentProcessorEditorOutliner::OnMapCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World->WorldType == EWorldType::Editor) {
		SetCurrentProcessor(nullptr, false);
	}
}

void SProceduralContentProcessorEditorOutliner::OnPreSaveWorld(UWorld* InWorld, FObjectPreSaveContext ObjectSaveContext)
{
	SetCurrentProcessor(nullptr, false);
	RequestRefreshProcessorList();
}

void SProceduralContentProcessorEditorOutliner::OnBlueprintPreCompile(UBlueprint* InBlueprint)
{
	if (Cast<UProceduralContentProcessorBlueprint>(InBlueprint)) {
		SetCurrentProcessor(nullptr, false);
	}
}

void SProceduralContentProcessorEditorOutliner::OnBlueprintCompiled(UBlueprint* InBlueprint)
{
	RequestRefreshProcessorList();
}

void SProceduralContentProcessorEditorOutliner::OnAssetAdded(const FAssetData& InData)
{
	if (InData.GetClass() && InData.GetClass()->IsChildOf<UProceduralContentProcessorBlueprint>()) {
		RequestRefreshProcessorList();
	}
}

void SProceduralContentProcessorEditorOutliner::OnAssetRemoved(const FAssetData& InData)
{
	if (InData.GetClass() && InData.GetClass()->IsChildOf<UProceduralContentProcessorBlueprint>()) {
		RequestRefreshProcessorList();
	}
}

void SProceduralContentProcessorEditorOutliner::OnAssetUpdated(const FAssetData& InData)
{
	if (InData.GetClass() && InData.GetClass()->IsChildOf<UProceduralContentProcessorBlueprint>()) {
		RequestRefreshProcessorList();
	}
}

void SProceduralContentProcessorEditorOutliner::OnBrowseToCurrentProcessorBlueprint()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	if (CurrentProcessor) {
		ContentBrowserModule.Get().SyncBrowserToAssets({ CurrentProcessor->GetClass()->ClassGeneratedBy });
	}
	else {
		ContentBrowserModule.Get().SyncBrowserToFolders({ "/ProceduralContentProcessor" }, true);
	}
}

void SProceduralContentProcessorEditorOutliner::OnProcessorTextChanged(const FText& InNewText)
{
	CurrentProcessorText = InNewText;
}

FText SProceduralContentProcessorEditorOutliner::OnGetCurrentProcessorText() const
{
	//if (CurrentProcessor) {
	//	return CurrentProcessor->GetClass()->GetDisplayNameText();
	//}
	return CurrentProcessorText;
}

FText SProceduralContentProcessorEditorOutliner::OnGetCurrentProcessorTooltipText() const
{
	if (CurrentProcessor) {
		return CurrentProcessor->GetClass()->GetToolTipText();
	}
	return FText::FromString("Null");
}

TSharedRef<SWidget> SProceduralContentProcessorEditorOutliner::OnGetProcessorSelectorMenu()
{
	SAssignNew(ProcessorTreeView, STreeView<TSharedPtr<FProcessorOutlinerField>>)
		.TreeItemsSource(&TopLevelProcessorField)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SProceduralContentProcessorEditorOutliner::OnGenerateRow)
		.OnGetChildren(this, &SProceduralContentProcessorEditorOutliner::OnGetChildren)
		.OnSelectionChanged(this, &SProceduralContentProcessorEditorOutliner::OnSelectionChanged)
		.OnIsSelectableOrNavigable(this, &SProceduralContentProcessorEditorOutliner::IsSelectableOrNavigable);

	for (auto Field : TopLevelProcessorField) {
		ProcessorTreeView->SetItemExpansion(Field, true);
	}

	return ProcessorTreeView.ToSharedRef();
}

TSharedRef<ITableRow> SProceduralContentProcessorEditorOutliner::OnGenerateRow(TSharedPtr<FProcessorOutlinerField> InField, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FProcessorOutlinerField>>, OwnerTable)
		.Padding(FMargin(2.f, 1.f, 2.f, 1.f))
		.Content()
		[
			SNew(STextBlock)
			.Text(InField->GetName())
			.ToolTipText(InField->GetTooltip())
		];
}

void SProceduralContentProcessorEditorOutliner::OnGetChildren(TSharedPtr<FProcessorOutlinerField> InField, TArray< TSharedPtr<FProcessorOutlinerField>>& OutChildren)
{
	OutChildren = InField->Children;
}

void SProceduralContentProcessorEditorOutliner::OnSelectionChanged(TSharedPtr<FProcessorOutlinerField> InField, ESelectInfo::Type SelectInfo)
{
	if (InField && InField->GetProcessorClass()) {
		SetCurrentProcessor(InField->GetProcessorClass());
		if (ProcessorComboButton) {
			ProcessorComboButton->SetIsOpen(false);
		}
	}
}

bool SProceduralContentProcessorEditorOutliner::IsSelectableOrNavigable(TSharedPtr<FProcessorOutlinerField> InField) const
{
	return InField->GetProcessorClass() != nullptr;
}
