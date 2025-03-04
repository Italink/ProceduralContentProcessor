#include "ProceduralContentProcessorLibrary.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionTime.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "Misc/ScopedSlowTask.h"
#include "LandscapeStreamingProxy.h"
#include <Kismet/GameplayStatics.h>
#include "Selection.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "WorldPartition/HLOD/HLODLayer.h"
#include "InstancedFoliageActor.h"
#include "Components/LightComponentBase.h"
#include "Layers/LayersSubsystem.h"
#include "DataLayer/DataLayerEditorSubsystem.h"
#include "EditPivotTool.h"
#include "ToolTargets/StaticMeshComponentToolTarget.h"
#include "Engine/StaticMeshActor.h"
#include "InteractiveToolManager.h"
#include "InteractiveToolsContext.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "EdModeInteractiveToolsContext.h"
#include "EditorModeManager.h"
#include "ToolTargetManager.h"
#include "Blueprint/UserWidget.h"
#include "ObjectTools.h"
#include "ReferencedAssetsUtils.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "StaticMeshEditorSubsystem.h"
#include "LevelEditor.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/NiagaraEmitterHandleViewModel.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/Stack/NiagaraStackViewModel.h"
#include "ViewModels/Stack/NiagaraStackFunctionInput.h"
#include "ViewModels/Stack/NiagaraStackItemGroup.h"
#include "ViewModels/Stack/NiagaraStackRoot.h"
#include "SLevelViewport.h"
#include "TextureCompiler.h"
#include "ImageUtils.h"

#define LOCTEXT_NAMESPACE "ProceduralContentProcessor"

void UProceduralContentProcessorLibrary::ClearObjectMaterix(FProceduralObjectMatrix& Matrix)
{
	Matrix.ObjectInfoList.Reset();
	Matrix.ObjectInfoMap.Reset();
	Matrix.bIsDirty = true;
}

void UProceduralContentProcessorLibrary::AddPropertyFieldBySecondaryObject(FProceduralObjectMatrix& Matrix, UObject* InOwner, UObject* InObject, FName InPropertyName)
{
	Matrix.FieldKeys.AddUnique(InPropertyName);
	auto InfoPtr = Matrix.ObjectInfoMap.Find(InOwner);
	TSharedPtr<FProceduralObjectMatrixRow> Info;
	if (!InfoPtr) {
		Info = MakeShared<FProceduralObjectMatrixRow>();
		Info->Owner = InOwner;
		Matrix.ObjectInfoMap.Add(InOwner, Info);
		Matrix.ObjectInfoList.Add(Info);
	}
	else {
		Info = *InfoPtr;
	}

	TSharedPtr<FProceduralObjectMatrixPropertyField> Field = MakeShared<FProceduralObjectMatrixPropertyField>();
	Field->Owner = InOwner;
	Field->Object = InObject;
	Field->Name = InPropertyName;
	Field->PropertyName = InPropertyName.ToString();
	Info->AddField(Field);

	Matrix.bIsDirty = true;
}

void UProceduralContentProcessorLibrary::AddPropertyField(FProceduralObjectMatrix& Matrix, UObject* InObject, FName InPropertyName)
{
	Matrix.FieldKeys.AddUnique(InPropertyName);
	auto InfoPtr = Matrix.ObjectInfoMap.Find(InObject);
	TSharedPtr<FProceduralObjectMatrixRow> Info;
	if (!InfoPtr) {
		Info = MakeShared<FProceduralObjectMatrixRow>();
		Info->Owner = InObject;
		Matrix.ObjectInfoMap.Add(InObject, Info);
		Matrix.ObjectInfoList.Add(Info);
	}
	else {
		Info = *InfoPtr;
	}

	TSharedPtr<FProceduralObjectMatrixPropertyField> Field = MakeShared<FProceduralObjectMatrixPropertyField>();
	Field->Owner = InObject;
	Field->Object = InObject;
	Field->Name = InPropertyName;
	Field->PropertyName = InPropertyName.ToString();
	Info->AddField(Field);

	Matrix.bIsDirty = true;
}

void UProceduralContentProcessorLibrary::AddTextField(FProceduralObjectMatrix& Matrix, UObject* InObject, FName InFieldName, FString InFieldValue)
{
	Matrix.FieldKeys.AddUnique(InFieldName);
	auto InfoPtr = Matrix.ObjectInfoMap.Find(InObject);
	TSharedPtr<FProceduralObjectMatrixRow> Info;
	if (!InfoPtr) {
		Info = MakeShared<FProceduralObjectMatrixRow>();
		Info->Owner = InObject;
		Matrix.ObjectInfoMap.Add(InObject, Info);
		Matrix.ObjectInfoList.Add(Info);;
	}
	else {
		Info = *InfoPtr;
	}
	TSharedPtr<FProceduralObjectMatrixTextField> Field = MakeShared<FProceduralObjectMatrixTextField>();
	Field->Owner = InObject;
	Field->Name = InFieldName;
	Field->Text = InFieldValue;
	Info->AddField(Field);
	Matrix.bIsDirty = true;
}

TArray<UObject*> UProceduralContentProcessorLibrary::GetAllObjectsOfClass(UClass* Class, bool bIncludeDerivedClasses)
{
	TArray<UObject*> Results;
	GetObjectsOfClass(Class, Results, bIncludeDerivedClasses);
	return Results;
}

TArray<UObject*> UProceduralContentProcessorLibrary::GetAllObjectsWithOuter(UObject* Outer, bool bIncludeNestedObjects)
{
	TArray<UObject*> Results;
	GetObjectsWithOuter(Outer, Results, bIncludeNestedObjects);
	return Results;
}

bool UProceduralContentProcessorLibrary::ObjectIsAsset(const UObject* InObject)
{
	if(InObject == nullptr)
		return false;
	return InObject->IsAsset();
}

UBlueprint* UProceduralContentProcessorLibrary::GetBlueprint(UObject* InObject)
{
	if (InObject == nullptr)
		return nullptr;
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InObject)) {
		return Blueprint;
	}
	UClass* Class = Cast<UClass>(InObject);
	if (Class == nullptr) {
		Class = InObject->GetClass();
	}
	return Cast<UBlueprint>(Class->ClassGeneratedBy);
}

void UProceduralContentProcessorLibrary::BeginTransaction(FText Text)
{
	GEditor->BeginTransaction(Text);
}

void UProceduralContentProcessorLibrary::ModifyObject(UObject* Object)
{
	if (!Object->HasAnyFlags(RF_Transactional))
		Object->SetFlags(RF_Transactional);
	Object->Modify();
}

void UProceduralContentProcessorLibrary::EndTransaction()
{
	GEditor->EndTransaction();
}

TArray<UClass*> UProceduralContentProcessorLibrary::GetAllDerivedClasses(const UClass* ClassToLookFor, bool bRecursive)
{
	TArray<UClass*> Results;
	::GetDerivedClasses(ClassToLookFor, Results, bRecursive);
	return Results;
}

UObject* UProceduralContentProcessorLibrary::DuplicateObject(UObject* SourceObject, UObject* Outer)
{
	return ::DuplicateObject<UObject>(SourceObject, Outer == nullptr ? GetTransientPackage() : Outer);
}

int32 UProceduralContentProcessorLibrary::DeleteObjects(const TArray< UObject* >& ObjectsToDelete, bool bShowConfirmation /*= true*/, bool bAllowCancelDuringDelete /*= true*/)
{
	return ObjectTools::DeleteObjects(ObjectsToDelete, bShowConfirmation, bAllowCancelDuringDelete ? ObjectTools::EAllowCancelDuringDelete::AllowCancel : ObjectTools::EAllowCancelDuringDelete::CancelNotAllowed);
}

int32 UProceduralContentProcessorLibrary::DeleteObjectsUnchecked(const TArray< UObject* >& ObjectsToDelete)
{
	return ObjectTools::DeleteObjectsUnchecked(ObjectsToDelete);
}

void UProceduralContentProcessorLibrary::ConsolidateObjects(UObject* ObjectToConsolidateTo, TArray<UObject*>& ObjectsToConsolidate, bool bShowDeleteConfirmation /*= true*/)
{
	ObjectTools::ConsolidateObjects(ObjectToConsolidateTo, ObjectsToConsolidate, bShowDeleteConfirmation);
}

TSet<UObject*> UProceduralContentProcessorLibrary::GetAssetReferences(UObject* Object, const TArray<UClass*>& IgnoreClasses, bool bIncludeDefaultRefs /*= false*/)
{
	TSet<UObject*> ReferencedAssets;
	FFindReferencedAssets::BuildAssetList(Object, IgnoreClasses, {}, ReferencedAssets, bIncludeDefaultRefs);
	return ReferencedAssets;
}

UUserWidget* UProceduralContentProcessorLibrary::AddDialog(UObject* Outer, TSubclassOf<UUserWidget> WidgetClass, FText WindowTitle /*= INVTEXT("Dialog")*/, FVector2D DialogSize /*= FVector2D(400, 300)*/, bool IsModalWindow /*= true*/)
{
	UUserWidget* UserWidget = ::NewObject<UUserWidget>(Outer, WidgetClass.Get());
	auto NewWindw = SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(DialogSize);
	if (IsModalWindow) {
		NewWindw->SetAsModalWindow();
	}
	TSharedPtr<SWindow> ParentWindow;
	if (auto Widget = Cast<UUserWidget>(Outer)) {
		if (Widget->GetCachedWidget()) {
			ParentWindow = FSlateApplication::Get().FindWidgetWindow(Widget->GetCachedWidget().ToSharedRef());
		}
	}
	if (ParentWindow) {
		FSlateApplication::Get().AddWindowAsNativeChild(NewWindw, ParentWindow.ToSharedRef())->SetContent
		(
			UserWidget->TakeWidget()
		);
	}
	else {
		FSlateApplication::Get().AddWindow(NewWindw)->SetContent
		(
			UserWidget->TakeWidget()
		);
	}
	return UserWidget;
}

EMsgBoxReturnType UProceduralContentProcessorLibrary::AddMessageBox(EMsgBoxType Type, FText Msg)
{
	EAppReturnType::Type Ret = FMessageDialog::Open((EAppMsgType::Type)Type, Msg);
	return EMsgBoxReturnType(Ret);
}

UUserWidget* UProceduralContentProcessorLibrary::AddContextMenu(UUserWidget* InParentWidget, TSubclassOf<UUserWidget> WidgetClass, FVector2D SummonLocation /*= FVector2D::ZeroVector*/)
{
	if (!WidgetClass)
		return nullptr;
	UUserWidget* UserWidget = ::NewObject<UUserWidget>(InParentWidget, WidgetClass.Get());
	if (SummonLocation == FVector2D::ZeroVector)
		SummonLocation = FSlateApplication::Get().GetCursorPos();
	TSharedPtr<SWidget> ParentWidget = InParentWidget->GetCachedWidget();
	if (ParentWidget.IsValid()) {
		FSlateApplication::Get().PushMenu(
			ParentWidget.ToSharedRef(),
			FWidgetPath(),
			UserWidget->TakeWidget(),
			SummonLocation,
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("PushContextMenu: Parent widget is invaild!"));
	}
	return UserWidget;
}

void UProceduralContentProcessorLibrary::DismissAllMenus()
{
	FSlateApplication::Get().DismissAllMenus();
}

void UProceduralContentProcessorLibrary::AddNotification(FText Notification, float FadeInDuration /*= 2*/, float ExpireDuration /*= 2*/, float FadeOutDuration /*= 2*/)
{
	FNotificationInfo Info(Notification);
	Info.FadeInDuration = FadeInDuration;
	Info.ExpireDuration = ExpireDuration;
	Info.FadeOutDuration = FadeOutDuration;
	FSlateNotificationManager::Get().AddNotification(Info);
}

void UProceduralContentProcessorLibrary::AddNotificationWithButton(FText Notification, UObject* Object, TArray<FName> FunctionNames)
{
	if (Object == nullptr)
		return;
	static TWeakPtr<SNotificationItem> NotificationPtr;
	FNotificationInfo Info(Notification);
	Info.FadeInDuration = 2.0f;
	Info.FadeOutDuration = 2.0f;
	Info.bFireAndForget = false;
	Info.bUseThrobber = false;
	for (auto FunctionName : FunctionNames) {
		FNotificationButtonInfo Button = FNotificationButtonInfo(
			FText::FromName(FunctionName),
			FText::FromName(FunctionName),
			FSimpleDelegate::CreateLambda([Object, FunctionName]() {
				TSharedPtr<SNotificationItem> Notification = NotificationPtr.Pin();
				if (Notification.IsValid()){
					if (UFunction* Function = Object->GetClass()->FindFunctionByName(FunctionName)) {
						Object->ProcessEvent(Function, nullptr);
					}
					Notification->SetEnabled(false);
					Notification->SetExpireDuration(0.0f);
					Notification->ExpireAndFadeout();
					NotificationPtr.Reset();
				}
				}),
			SNotificationItem::ECompletionState::CS_None
		);
		Info.ButtonDetails.Add(Button);
	}
	NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
}

void UProceduralContentProcessorLibrary::PushSlowTask(FText TaskName, float AmountOfWork)
{
	TSharedPtr<FSlowTask> SlowTask = MakeShared<FSlowTask>(AmountOfWork, TaskName);
	SlowTasks.Add(SlowTask);
	SlowTask->Initialize();
	SlowTask->MakeDialog();
}

void UProceduralContentProcessorLibrary::EnterSlowTaskProgressFrame(float ExpectedWorkThisFrame /*= 1.f*/, const FText& Text /*= FText()*/)
{
	if (!SlowTasks.IsEmpty()) {
		SlowTasks[SlowTasks.Num() - 1]->EnterProgressFrame(ExpectedWorkThisFrame, Text);
	}
}

void UProceduralContentProcessorLibrary::PopSlowTask()
{
	if (!SlowTasks.IsEmpty()) {
		SlowTasks[SlowTasks.Num() - 1]->Destroy();
		SlowTasks.SetNum(SlowTasks.Num() - 1);
	}
}

void UProceduralContentProcessorLibrary::ClearAllSlowTask()
{
	for (auto Task : SlowTasks) {
		Task->Destroy();
	}
	SlowTasks.Reset();
}

bool UProceduralContentProcessorLibrary::IsNaniteEnable(UStaticMesh* InMesh)
{
	return InMesh ? (bool )InMesh->NaniteSettings.bEnabled: false;
}

void UProceduralContentProcessorLibrary::SetNaniteMeshEnabled(UStaticMesh* StaticMesh, bool bEnabled)
{
	StaticMesh->Modify();
	StaticMesh->NaniteSettings.bEnabled = bEnabled;
	FProperty* ChangedProperty = FindFProperty<FProperty>(UStaticMesh::StaticClass(), GET_MEMBER_NAME_CHECKED(UStaticMesh, NaniteSettings));
	FPropertyChangedEvent Event(ChangedProperty);
	StaticMesh->PostEditChangeProperty(Event);
}

bool UProceduralContentProcessorLibrary::IsMaterialHasTimeNode(AStaticMeshActor* StaticMeshActor)
{
	if (StaticMeshActor && StaticMeshActor->GetStaticMeshComponent()) {
		TArray<UMaterialInterface*> Materials = StaticMeshActor->GetStaticMeshComponent()->GetMaterials();
		TQueue<UMaterialInterface*> Queue;
		for (auto Material : Materials) {
			Queue.Enqueue(Material);
		}
		while (!Queue.IsEmpty()) {
			UMaterialInterface* Top;
			Queue.Dequeue(Top);
			if (auto MatIns = Cast<UMaterialInstance>(Top)) {
				Queue.Enqueue(MatIns->Parent);
			}
			else if (auto Mat = Cast<UMaterial>(Top)) {
				UObjectBase* TimeNode = FindObjectWithOuter(Mat, UMaterialExpressionTime::StaticClass());
				if (TimeNode) {
					StaticMeshActor->SetIsTemporarilyHiddenInEditor(true);
					return true;
				}
			}
		}
	}
	return false;
}

bool UProceduralContentProcessorLibrary::MatchString(FString InString, const TArray<FString>& IncludeList, const TArray<FString>& ExcludeList)
{
	for (const auto& Inc : IncludeList) {
		if (InString.Contains(Inc) || Inc.IsEmpty()) {
			for (const auto& Exc : ExcludeList) {
				if (InString.Contains(Exc)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

float UProceduralContentProcessorLibrary::GetStaticMeshDiskSize(UStaticMesh* StaticMesh, bool bWithTexture)
{
	float DiskSize = 0.0f;
	if (StaticMesh) {
		if (StaticMesh->GetRenderData()) {
			DiskSize = StaticMesh->NaniteSettings.bEnabled ? StaticMesh->GetRenderData()->EstimatedNaniteTotalCompressedSize : StaticMesh->GetRenderData()->EstimatedCompressedSize;
		}
		if (bWithTexture) {
			TSet<UObject*> ReferencedAssets;
			FFindReferencedAssets::BuildAssetList(StaticMesh, {}, {}, ReferencedAssets, false);
			for (auto Asset : ReferencedAssets) {
				if (UTexture* Texture = Cast<UTexture>(Asset)) {
					const FStreamableRenderResourceState SRRState = Texture->GetStreamableResourceState();
					const int32 ActualMipBias = SRRState.IsValid() ? (SRRState.ResidentFirstLODIdx() + SRRState.AssetLODBias) : Texture->GetCachedLODBias();
					FTexturePlatformData** PlatformDataPtr = Texture->GetRunningPlatformData();
					const int64 ResourceSize = PlatformDataPtr && *PlatformDataPtr ? (*PlatformDataPtr)->GetPayloadSize(ActualMipBias) : Texture->GetResourceSizeBytes(EResourceSizeMode::Exclusive);
					DiskSize += ResourceSize;
				}
			}
		}
	}
	return DiskSize / 1048576.0f;
}

bool UProceduralContentProcessorLibrary::IsGeneratedByBlueprint(UObject* InObject)
{
	return InObject&& InObject->GetClass()&&Cast<UBlueprint>(InObject->GetClass()->ClassGeneratedBy);
}

void UProceduralContentProcessorLibrary::ShowObjectDetailsView(UObject* InObject)
{
	static const FName TabID("ObjectDetails");

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditor.GetLevelEditorTabManager();
	if (LevelEditorTabManager == nullptr) {
		return;
	}
	if (LevelEditorTabManager->FindExistingLiveTab(LevelEditorTabIds::LevelEditorSelectionDetails)) {
		TSharedPtr<SDockTab> LiveTab = LevelEditorTabManager->FindExistingLiveTab(TabID);
		if (LiveTab != nullptr) {
			TSharedRef<IDetailsView> DetailView = StaticCastSharedRef<IDetailsView>(LiveTab->GetContent());
			DetailView->SetObject(InObject);
			return;
		}
		else {
			FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			FDetailsViewArgs DetailsViewArgs;
			DetailsViewArgs.bUpdatesFromSelection = true;
			DetailsViewArgs.bLockable = true;
			DetailsViewArgs.bAllowFavoriteSystem = true;
			//DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea | FDetailsViewArgs::ComponentsAndActorsUseNameArea;
			DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
			DetailsViewArgs.NotifyHook = GUnrealEd;
			DetailsViewArgs.ViewIdentifier = FName("BlueprintDefaults");
			DetailsViewArgs.bCustomNameAreaLocation = true;
			DetailsViewArgs.bCustomFilterAreaLocation = true;
			DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
			DetailsViewArgs.bShowSectionSelector = true;
			//DetailsViewArgs.bShowObjectLabel = false;
			DetailsViewArgs.bAllowSearch = true;
			auto DetailView = EditModule.CreateDetailView(DetailsViewArgs);
			DetailView->SetObject(InObject);

			LevelEditorTabManager->InsertNewDocumentTab(LevelEditorTabIds::LevelEditorSelectionDetails, TabID, FTabManager::FLiveTabSearch(),
				SNew(SDockTab)
				.TabRole(ETabRole::DocumentTab)
				.ContentPadding(FMargin(3.0f))
				.Label(LOCTEXT("ObjectDetails", "Object Details"))
				[
					DetailView
				]
			);
		}
	}
}

void UProceduralContentProcessorLibrary::ShowObjectEditor(UObject* InObject)
{
	GEditor->EditObject(InObject);
}

UObject* UProceduralContentProcessorLibrary::CopyProperties(UObject* SourceObject, UObject* TargetObject)
{
	if(SourceObject == nullptr || TargetObject == nullptr)
		return nullptr;
	UEngine::FCopyPropertiesForUnrelatedObjectsParams Params;
	Params.bNotifyObjectReplacement = true;
	UEngine::CopyPropertiesForUnrelatedObjects(SourceObject, TargetObject);
	return TargetObject;
}

void UProceduralContentProcessorLibrary::ForceReplaceReferences(UObject* SourceObjects, UObject* TargetObject)
{
	TArray<UObject*> ObjectsToReplace(&TargetObject, 1);
	ObjectTools::ForceReplaceReferences(SourceObjects, ObjectsToReplace);
}

DEFINE_FUNCTION(UProceduralContentProcessorLibrary::execSetObjectPropertyByName)
{
	P_GET_PROPERTY(FObjectProperty, Param_Object);
	P_GET_PROPERTY(FNameProperty, Param_PropertyName);
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* SourceProperty = Stack.MostRecentProperty;
	void* SourceValuePtr = Stack.MostRecentPropertyAddress;
	P_FINISH;
	if (Param_Object) {
		if (FProperty* Property = FindFProperty<FProperty>(Param_Object->GetClass(), Param_PropertyName)) {
			Property->SetValue_InContainer(Param_Object, SourceValuePtr);
			return;
		}
	}
}

UStaticMesh* UProceduralContentProcessorLibrary::GetComplexCollisionMesh(UStaticMesh* InMesh)
{
	if (!InMesh)
		return nullptr;
	return InMesh->ComplexCollisionMesh;
}

void UProceduralContentProcessorLibrary::SetStaticMeshPivot(UStaticMesh* InStaticMesh, EStaticMeshPivotType PivotType)
{
	if(InStaticMesh == nullptr)
		return;
	auto InteractiveToolsContext = NewObject<UEditorInteractiveToolsContext>();
	InteractiveToolsContext->InitializeContextWithEditorModeManager(&GLevelEditorModeTools(), nullptr);
	UE::TransformGizmoUtil::RegisterTransformGizmoContextObject(InteractiveToolsContext);

	InteractiveToolsContext->ToolManager->RegisterToolType("EditPivotTool", NewObject<UEditPivotToolBuilder>());
	UStaticMeshComponent* StaticMeshComp = NewObject<UStaticMeshComponent>();
	StaticMeshComp->SetStaticMesh(InStaticMesh);

	InteractiveToolsContext->TargetManager->AddTargetFactory(NewObject<UStaticMeshComponentToolTargetFactory>());
	UToolTarget* Target = InteractiveToolsContext->TargetManager->BuildTarget(StaticMeshComp, FToolTargetTypeRequirements());

	InteractiveToolsContext->ToolManager->SelectActiveToolType(EToolSide::Left, "EditPivotTool");
	GLevelEditorModeTools().SelectNone();
	GLevelEditorModeTools().GetSelectedComponents()->Select(StaticMeshComp);

	InteractiveToolsContext->ToolManager->ActivateTool(EToolSide::Left);
	if (auto EditTool = Cast<UEditPivotTool>(InteractiveToolsContext->ToolManager->ActiveLeftTool))
	{
		EditTool->SetTargets({ Target });
		EditTool->RequestAction((EEditPivotToolActions)PivotType);
		EditTool->Tick(0.1);
		EditTool->Shutdown(EToolShutdownType::Accept);
	}
}

UStaticMeshEditorSubsystem* UProceduralContentProcessorLibrary::GetStaticMeshEditorSubsystem()
{
	return GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();
}

float UProceduralContentProcessorLibrary::GetLodScreenSize(UStaticMesh* InStaticMesh, int32 LODIndex)
{
	if (InStaticMesh == nullptr || LODIndex< 0 || LODIndex >= InStaticMesh->GetNumLODs() || InStaticMesh->GetRenderData() == nullptr)
		return 0;
	return InStaticMesh->GetRenderData()->ScreenSize[LODIndex].GetValue();
}

float UProceduralContentProcessorLibrary::GetLodDistance(UStaticMesh* InStaticMesh, int32 LODIndex)
{
	if(LODIndex == 0)
		return 0;
	float ScreenSize = GetLodScreenSize(InStaticMesh, LODIndex);
	return CalcLodDistance(InStaticMesh->GetBounds().SphereRadius, ScreenSize);
}

float UProceduralContentProcessorLibrary::CalcLodDistance(float ObjectSphereRadius, float ScreenSize)
{
	const float FOV = 90.0f;
	const float FOVRad = FOV * (float)UE_PI / 360.0f;
	const FMatrix ProjectionMatrix = FPerspectiveMatrix(FOVRad, 1920, 1080, 0.01f);
	const float ScreenMultiple = FMath::Max(0.5f * ProjectionMatrix.M[0][0], 0.5f * ProjectionMatrix.M[1][1]);
	const float ScreenRadius = FMath::Max(UE_SMALL_NUMBER, ScreenSize * 0.5f);
	return ComputeBoundsDrawDistance(ScreenSize, ObjectSphereRadius, ProjectionMatrix);
}

float UProceduralContentProcessorLibrary::CalcScreenSize(float ObjectSphereRadius, float Distance)
{
	const float FOV = 90.0f;
	const float FOVRad = FOV * (float)UE_PI / 360.0f;
	const FMatrix ProjectionMatrix = FPerspectiveMatrix(FOVRad, 1920, 1080, 0.01f);
	const float ScreenMultiple = FMath::Max(0.5f * ProjectionMatrix.M[0][0], 0.5f * ProjectionMatrix.M[1][1]);
	if (Distance <= 0.000001f) {
		return 2.0f;
	}
	return  2.0f * ScreenMultiple * ObjectSphereRadius / FMath::Max(1.0f, Distance);
}

float UProceduralContentProcessorLibrary::CalcObjectSphereRadius(float ScreenSize, float Distance)
{
	const float FOV = 90.0f;
	const float FOVRad = FOV * (float)UE_PI / 360.0f;
	const FMatrix ProjectionMatrix = FPerspectiveMatrix(FOVRad, 1920, 1080, 0.01f);
	const float ScreenMultiple = FMath::Max(0.5f * ProjectionMatrix.M[0][0], 0.5f * ProjectionMatrix.M[1][1]);
	return ScreenSize * Distance / ( 2.0f* ScreenMultiple);
}

UTexture2D* UProceduralContentProcessorLibrary::ConstructTexture2D(UTextureRenderTarget2D* TextureRenderTarget2D, UObject* Outer, FString Name /*= NAME_None*/, TextureCompressionSettings CompressionSettings)
{
	if (TextureRenderTarget2D == nullptr)
		return nullptr;
	Outer = Outer ? Outer->GetPackage() : GetTransientPackage();
	UTexture2D* NewObj = TextureRenderTarget2D->ConstructTexture2D(Outer, Name, TextureRenderTarget2D->GetMaskedFlags(),
		static_cast<EConstructTextureFlags>(CTF_Default | CTF_SkipPostEdit), /*InAlphaOverride = */nullptr);
	NewObj->CompressionSettings = CompressionSettings;
	NewObj->MarkPackageDirty();

#if WITH_EDITOR
	NewObj->PostEditChange();
#endif
	return NewObj;
}

UTexture2D* UProceduralContentProcessorLibrary::ConstructTexture2DByRegion(UTextureRenderTarget2D* TextureRenderTarget2D, FBox2D Region, UObject* Outer, FString Name, TextureCompressionSettings CompressionSettings /*= TC_Default*/)
{
	if (TextureRenderTarget2D == nullptr)
		return nullptr;
	ETextureRenderTargetFormat RenderTargetFormat = TextureRenderTarget2D->RenderTargetFormat;
	FTextureRenderTargetResource* RenderTargetResource = TextureRenderTarget2D->GameThread_GetRenderTargetResource();
	TArray<FColor> OutLDR;
	FReadSurfaceDataFlags ReadPixelFlags(RCM_MinMax);
	FIntRect IntRegion(Region.Min.IntPoint(), Region.Max.IntPoint());
	RenderTargetResource->ReadPixels(OutLDR, ReadPixelFlags, IntRegion);
	FCreateTexture2DParameters Params;
	Params.CompressionSettings = CompressionSettings;
	return FImageUtils::CreateTexture2D(IntRegion.Width(), IntRegion.Height(), OutLDR, Outer, Name, EObjectFlags::RF_NoFlags, Params);
}

void UProceduralContentProcessorLibrary::UpdateTexture2D(UTextureRenderTarget2D* TextureRenderTarget2D, UTexture2D* Texture)
{
	if (TextureRenderTarget2D == nullptr || Texture == nullptr)
		return ;
	TextureRenderTarget2D->UpdateTexture2D(Texture, TextureRenderTarget2D->GetTextureFormatForConversionToTexture2D());
}

void ForeachNiagaraEntry(UNiagaraStackEntry* Root, TFunction<void(UNiagaraStackEntry*)> Func)
{
	if (!Root) 
		return;
	Func(Root);
	TArray<UNiagaraStackEntry*> Children;
	Root->GetFilteredChildren(Children);
	for (auto Child : Children) {
		ForeachNiagaraEntry(Child, Func);
	}
}

FNiagaraSystemInfo UProceduralContentProcessorLibrary::GetNiagaraSystemInformation(UNiagaraSystem* InNaigaraSystem)
{
	FNiagaraSystemViewModelOptions SystemViewModelOptions;
	SystemViewModelOptions.bCanSimulate = false;
	SystemViewModelOptions.bCanAutoCompile = false;
	// SystemViewModelOptions.bIsForDataProcessingOnly will affect Win.CMD Operate.
	SystemViewModelOptions.bIsForDataProcessingOnly = true;
	SystemViewModelOptions.MessageLogGuid = InNaigaraSystem->GetAssetGuid();
	TSharedRef<FNiagaraSystemViewModel> SystemViewModel = MakeShared<FNiagaraSystemViewModel>();
	SystemViewModel->Initialize(*InNaigaraSystem, SystemViewModelOptions);
	TArray<TSharedRef<FNiagaraEmitterHandleViewModel>> EmitterViewModels = SystemViewModel->GetEmitterHandleViewModels();
	FNiagaraSystemInfo SystemInfo;
	for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : EmitterViewModels){
		FNiagaraEmitterHandle* EmitterHandle = EmitterHandleViewModel->GetEmitterHandle();
		FNiagaraEmitterInfo EmitterInfo;
		EmitterInfo.Name = EmitterHandle->GetName();
		EmitterInfo.bEnabled = EmitterHandle->GetIsEnabled();
		EmitterInfo.Data = *EmitterHandle->GetEmitterData();
		UNiagaraStackViewModel* StackViewModel = EmitterHandleViewModel->GetEmitterStackViewModel();
		TArray<UNiagaraStackItemGroup*> StackItemGroups;
		StackViewModel->GetRootEntry()->GetUnfilteredChildrenOfType<UNiagaraStackItemGroup>(StackItemGroups);
		UNiagaraStackRoot* StackRoot = CastChecked<UNiagaraStackRoot>(StackViewModel->GetRootEntry());
		ForeachNiagaraEntry(StackRoot,[&EmitterInfo](UNiagaraStackEntry* Entry){
			if (UNiagaraStackFunctionInput* Input = Cast<UNiagaraStackFunctionInput>(Entry)) {
				FString Name = Input->GetDisplayName().ToString();
				FString Value;

				//TODO: value assign

				EmitterInfo.Inputs.Add(Name, Value);
			}
		});
		SystemInfo.Emitters.Emplace(EmitterInfo);
	}

	return SystemInfo;
}

FVector2D UProceduralContentProcessorLibrary::ProjectWorldToScreen(const FVector& InWorldPos, bool bClampToScreenRectangle)
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
	if (LevelEditor) {
		TSharedPtr<SLevelViewport> ActiveLevelViewport = LevelEditor->GetActiveViewportInterface();
		TSharedPtr<FEditorViewportClient> Client = ActiveLevelViewport->GetViewportClient();
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			Client->Viewport,
			Client->GetScene(),
			Client->EngineShowFlags));
		// SceneView is deleted with the ViewFamily
		FSceneView* SceneView = Client->CalcSceneView(&ViewFamily);

		// Compute the MinP/MaxP in pixel coord, relative to View.ViewRect.Min
		const FMatrix& WorldToView = SceneView->ViewMatrices.GetViewMatrix();
		const FMatrix& ViewToProj = SceneView->ViewMatrices.GetProjectionMatrix();
		const float NearClippingDistance = SceneView->NearClippingDistance + SMALL_NUMBER;
		const FIntRect ViewRect = SceneView->UnconstrainedViewRect;

		// Clamp position on the near plane to get valid rect even if bounds' points are behind the camera
		FPlane P_View = WorldToView.TransformFVector4(FVector4(InWorldPos, 1.f));
		if (P_View.Z <= NearClippingDistance)
		{
			P_View.Z = NearClippingDistance;
		}

		// Project from view to projective space
		FVector2D MinP(FLT_MAX, FLT_MAX);
		FVector2D MaxP(-FLT_MAX, -FLT_MAX);
		FVector2D ScreenPos;
		const bool bIsValid = FSceneView::ProjectWorldToScreen(P_View, ViewRect, ViewToProj, ScreenPos);

		// Clamp to pixel border
		ScreenPos = FIntPoint(FMath::FloorToInt(ScreenPos.X), FMath::FloorToInt(ScreenPos.Y));

		// Clamp to screen rect
		if (bClampToScreenRectangle)
		{
			ScreenPos.X = FMath::Clamp(ScreenPos.X, ViewRect.Min.X, ViewRect.Max.X);
			ScreenPos.Y = FMath::Clamp(ScreenPos.Y, ViewRect.Min.Y, ViewRect.Max.Y);
		}

		return FVector2D(ScreenPos.X, ScreenPos.Y);
	}
	return FVector2D::ZeroVector;
}

bool UProceduralContentProcessorLibrary::DeprojectScreenToWorld(const FVector2D& InScreenPos, FVector& OutWorldOrigin, FVector& OutWorldDirection)
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
	if (LevelEditor) {
		TSharedPtr<SLevelViewport> ActiveLevelViewport = LevelEditor->GetActiveViewportInterface();
		TSharedPtr<FEditorViewportClient> Client = ActiveLevelViewport->GetViewportClient();
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			Client->Viewport,
			Client->GetScene(),
			Client->EngineShowFlags));
		// SceneView is deleted with the ViewFamily
		FSceneView* SceneView = Client->CalcSceneView(&ViewFamily);
		const FMatrix& InvViewProjectionMatrix = SceneView->ViewMatrices.GetInvViewProjectionMatrix();
		const FIntRect ViewRect = SceneView->UnconstrainedViewRect;
		FSceneView::DeprojectScreenToWorld(InScreenPos, ViewRect, InvViewProjectionMatrix, OutWorldOrigin, OutWorldDirection);
		return true;
	}
	return false;
}

UPhysicalMaterial* UProceduralContentProcessorLibrary::GetSimplePhysicalMaterial(UPrimitiveComponent* Component)
{
	return Component->BodyInstance.GetSimplePhysicalMaterial();
}

TArray<AActor*> UProceduralContentProcessorLibrary::BreakISM(AActor* InISMActor, bool bDestorySourceActor /*= true*/)
{
	TArray<AActor*> Actors;
	if (!InISMActor)
		return Actors;
	ULayersSubsystem* LayersSubsystem = GEditor->GetEditorSubsystem<ULayersSubsystem>();
	TArray<UInstancedStaticMeshComponent*> InstanceComps;
	InISMActor->GetComponents(InstanceComps, true);
	UWorld* World = InISMActor->GetWorld();
	if (!InstanceComps.IsEmpty()) {
		for (auto& ISMC : InstanceComps) {
			for (int i = 0; i < ISMC->GetInstanceCount(); i++) {
				FActorSpawnParameters SpawnInfo;
				SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				FTransform Transform;
				ISMC->GetInstanceTransform(i, Transform, true);
				auto NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform, SpawnInfo);
				NewActor->GetStaticMeshComponent()->SetStaticMesh(ISMC->GetStaticMesh());
				NewActor->SetActorLabel(InISMActor->GetActorLabel());
				auto Materials = ISMC->GetMaterials();
				for (int j = 0; j < Materials.Num(); j++)
					NewActor->GetStaticMeshComponent()->SetMaterial(j, Materials[j]);
				NewActor->Modify();
				NewActor->SetActorLabel(MakeUniqueObjectName(World, AStaticMeshActor::StaticClass(), *ISMC->GetStaticMesh()->GetName()).ToString());
				Actors.Add(NewActor);
				LayersSubsystem->InitializeNewActorLayers(NewActor);
				const bool bCurrentActorSelected = GUnrealEd->GetSelectedActors()->IsSelected(InISMActor);
				if (bCurrentActorSelected)
				{
					// The source actor was selected, so deselect the old actor and select the new one.
					GUnrealEd->GetSelectedActors()->Modify();
					GUnrealEd->SelectActor(NewActor, bCurrentActorSelected, false);
					GUnrealEd->SelectActor(InISMActor, false, false);
				}
				{
					LayersSubsystem->DisassociateActorFromLayers(NewActor);
					NewActor->Layers.Empty();
					LayersSubsystem->AddActorToLayers(NewActor, InISMActor->Layers);
				}
			}
			ISMC->PerInstanceSMData.Reset();
			if (bDestorySourceActor) {
				LayersSubsystem->DisassociateActorFromLayers(InISMActor);
				InISMActor->GetWorld()->EditorDestroyActor(InISMActor, true);
			}
			GUnrealEd->NoteSelectionChange();
		}
	}
	return Actors;
}

AActor* UProceduralContentProcessorLibrary::MergeISM(TArray<AActor*> InSourceActors, TSubclassOf<UInstancedStaticMeshComponent> InISMClass, bool bDestorySourceActor /*= true*/)
{
	if (InSourceActors.IsEmpty() || !InISMClass)
		return nullptr;
	ULayersSubsystem* LayersSubsystem = GEditor->GetEditorSubsystem<ULayersSubsystem>();
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UWorld* World = InSourceActors[0]->GetWorld();
	FBoxSphereBounds Bounds;
	TMap<UStaticMesh*, TArray<FTransform>> InstancedMap;
	for (auto Actor : InSourceActors) {
		TArray<UStaticMeshComponent*> MeshComps;
		Actor->GetComponents(MeshComps, true);
		for (auto MeshComp : MeshComps) {
			UStaticMesh* Mesh = MeshComp->GetStaticMesh();
			auto& InstancedInfo = InstancedMap.FindOrAdd(Mesh);
			Bounds = MeshComp->Bounds + Bounds;
			InstancedInfo.Add(MeshComp->K2_GetComponentToWorld());
		}
		TArray<UInstancedStaticMeshComponent*> InstMeshComps;
		Actor->GetComponents(InstMeshComps, true);
		for (auto InstMeshComp : InstMeshComps) {
			UStaticMesh* Mesh = InstMeshComp->GetStaticMesh();
			auto& InstancedInfo = InstancedMap.FindOrAdd(Mesh);
			FTransform Transform;
			Bounds = InstMeshComp->Bounds + Bounds;
			for (int i = 0; i < InstMeshComp->GetInstanceCount(); i++) {
				InstMeshComp->GetInstanceTransform(i, Transform, true);
				InstancedInfo.Add(Transform);
			}
		}
	}
	FTransform Transform;
	Transform.SetLocation(Bounds.Origin);
	auto NewISMActor = World->SpawnActor<AActor>(AActor::StaticClass(), Transform, SpawnInfo);
	USceneComponent* RootComponent = NewObject<USceneComponent>(NewISMActor, USceneComponent::GetDefaultSceneRootVariableName(), RF_Transactional);
	RootComponent->Mobility = EComponentMobility::Static;
	RootComponent->bVisualizeComponent = true;
	NewISMActor->SetRootComponent(RootComponent);
	NewISMActor->AddInstanceComponent(RootComponent);
	RootComponent->OnComponentCreated();
	RootComponent->RegisterComponent();

	for (const auto& InstancedInfo : InstancedMap) {
		UInstancedStaticMeshComponent* ISMComponent = NewObject<UInstancedStaticMeshComponent>(NewISMActor, InISMClass, *InstancedInfo.Key->GetName(), RF_Transactional);
		ISMComponent->Mobility = EComponentMobility::Static;
		NewISMActor->AddInstanceComponent(ISMComponent);
		ISMComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		ISMComponent->OnComponentCreated();
		ISMComponent->RegisterComponent();
		ISMComponent->SetStaticMesh(InstancedInfo.Key);
		for (auto InstanceTransform : InstancedInfo.Value) {
			ISMComponent->AddInstance(InstanceTransform, true);
		}
	}
	NewISMActor->Modify();
	LayersSubsystem->InitializeNewActorLayers(NewISMActor);
	GUnrealEd->GetSelectedActors()->Modify();
	GUnrealEd->SelectActor(NewISMActor, true, false);
	if (bDestorySourceActor) {
		for (auto Actor : InSourceActors) {
			LayersSubsystem->DisassociateActorFromLayers(Actor);
			World->EditorDestroyActor(Actor, true);
		}
	}
	return NewISMActor;
}

void UProceduralContentProcessorLibrary::SetHLODLayer(AActor* InActor, UHLODLayer* InHLODLayer)
{
	if (InActor->GetHLODLayer() != InHLODLayer) {
		InActor->Modify();
		InActor->bEnableAutoLODGeneration = InHLODLayer != nullptr;
		InActor->SetHLODLayer(InHLODLayer);
	}
}

void UProceduralContentProcessorLibrary::SelectActor(AActor* InActor)
{
	GEditor->GetSelectedActors()->Modify();
	GEditor->GetSelectedActors()->BeginBatchSelectOperation();
	GEditor->SelectNone(false, true, true);
	GEditor->SelectActor(InActor, true, false, true);
	GEditor->GetSelectedActors()->EndBatchSelectOperation(/*bNotify*/false);
	GEditor->NoteSelectionChange();
}

void UProceduralContentProcessorLibrary::LookAtActor(AActor* InActor)
{
	GEditor->MoveViewportCamerasToActor(*InActor, true);
}

AActor* UProceduralContentProcessorLibrary::SpawnTransientActor(UObject* WorldContextObject, TSubclassOf<AActor> Class, FTransform Transform)
{
	if (WorldContextObject == nullptr)
		return nullptr;
	UWorld* World = WorldContextObject->GetWorld();
	AActor* Actor = World->SpawnActor(Class, &Transform);
	if (Actor)
		Actor->SetFlags(RF_Transient);
	return Actor;
}

AActor* UProceduralContentProcessorLibrary::ReplaceActor(AActor* InSrc, TSubclassOf<AActor> InDst, bool bNoteSelectionChange /*= false*/)
{
	if (!InSrc || !InDst)
		return nullptr;
	GEditor->BeginTransaction(LOCTEXT("ReplaceActor", "Replace Actor"));
	FTransform Transform = InSrc->GetTransform();
	auto NewActor = GUnrealEd->ReplaceActor(InSrc, InDst, nullptr, bNoteSelectionChange);
	NewActor->SetActorTransform(Transform);
	GEditor->EndTransaction();
	return NewActor;
}

void UProceduralContentProcessorLibrary::ReplaceActors(TMap<AActor*, TSubclassOf<AActor>> ActorMap, bool bNoteSelectionChange)
{
	if (GUnrealEd && !ActorMap.IsEmpty()) {
		GEditor->BeginTransaction(LOCTEXT("ReplaceActors", "Replace Actors"));
		for (auto ActorPair : ActorMap) {
			if (ActorPair.Key && ActorPair.Value) {
				FTransform Transform = ActorPair.Key->GetTransform();
				auto NewActor = GUnrealEd->ReplaceActor(ActorPair.Key, ActorPair.Value, nullptr, bNoteSelectionChange);
				NewActor->SetActorTransform(Transform);
			}
		}
		GEditor->EndTransaction();
	}
}

void UProceduralContentProcessorLibrary::ActorSetIsSpatiallyLoaded(AActor* Actor, bool bIsSpatiallyLoaded)
{
	Actor->SetIsSpatiallyLoaded(bIsSpatiallyLoaded);
}

bool UProceduralContentProcessorLibrary::ActorAddDataLayer(AActor* Actor, UDataLayerAsset* DataLayerAsset)
{
	if (Actor && DataLayerAsset) {
		UDataLayerInstance* Instance = GEditor->GetEditorSubsystem<UDataLayerEditorSubsystem>()->GetDataLayerInstance(DataLayerAsset);
		if (Instance) {
			return Instance->AddActor(Actor);
		}
	}
	return false;
}

bool UProceduralContentProcessorLibrary::ActorRemoveDataLayer(AActor* Actor, UDataLayerAsset* DataLayerAsset)
{
	if (Actor && DataLayerAsset) {
		UDataLayerInstance* Instance = GEditor->GetEditorSubsystem<UDataLayerEditorSubsystem>()->GetDataLayerInstance(DataLayerAsset);
		if (Instance) {
			return Instance->RemoveActor(Actor);
		}
	}
	return false;
}

bool UProceduralContentProcessorLibrary::ActorContainsDataLayer(AActor* Actor, UDataLayerAsset* DataLayerAsset)
{
	if (Actor && DataLayerAsset) {
		UDataLayerInstance* Instance = GEditor->GetEditorSubsystem<UDataLayerEditorSubsystem>()->GetDataLayerInstance(DataLayerAsset);
		if (Instance) {
			return Actor->ContainsDataLayer(Instance);
		}
	}
	return false;
}

FName UProceduralContentProcessorLibrary::ActorGetRuntimeGrid(AActor* Actor)
{
	if (Actor) {
		return Actor->GetRuntimeGrid();
	}
	return FName();
}

void UProceduralContentProcessorLibrary::ActorSetRuntimeGrid(AActor* Actor, FName GridName)
{
	if (Actor && Actor->GetRuntimeGrid() != GridName) {
		Actor->SetRuntimeGrid(GridName);
		Actor->Modify();
	}
}

TArray<TSharedPtr<FSlowTask>> UProceduralContentProcessorLibrary::SlowTasks;

#undef LOCTEXT_NAMESPACE