#pragma once
#include "UObject/Object.h"
#include "ISinglePropertyView.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ProceduralObjectMatrix.generated.h"

struct IProceduralPropertyMatrixField
{
	FName Name;
	TWeakObjectPtr<UObject> Owner;
	virtual TSharedPtr<SWidget> BuildWidget() = 0;
	virtual FString GetText() = 0;
	virtual ~IProceduralPropertyMatrixField(){};
};

struct FProceduralObjectMatrixTextField : public IProceduralPropertyMatrixField
{
	FString Text;
	TSharedPtr<SWidget> BuildWidget() override {
		return SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(4)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Text))
			];
	}

	FString GetText() override { return Text; }
};

struct FProceduralObjectMatrixPropertyField : public IProceduralPropertyMatrixField
{
	TWeakObjectPtr<UObject> Object;
	FString PropertyName;
	TSharedPtr<SWidget> BuildWidget() override {
		if (Object.IsValid() && Object->GetClass()->IsValidLowLevel()) {
			FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), *PropertyName);
			if (Property) {
				FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
				FSinglePropertyParams Params;
				Params.NamePlacement = EPropertyNamePlacement::Hidden;
				auto Widget = EditModule.CreateSingleProperty(Object.Get(), *PropertyName, Params);
				Widget->SetEnabled(!Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
				return Widget;
			}
		}
		return SNew(SSpacer);
	}

	FString GetText() override { 
		FString Text;
		if (Object.IsValid() && Object->GetClass()->IsValidLowLevel()) {
			FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), *PropertyName);
			if (Property) {
				FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(Object.GetEvenIfUnreachable()), Text);
			}
		}
		return Text;
	}
};

struct FProceduralObjectMatrixRow {
	TWeakObjectPtr<UObject> Owner;
	TArray<TSharedPtr<IProceduralPropertyMatrixField>> Fields;
	TMap<FName, IProceduralPropertyMatrixField*> FieldMap;

	void AddField(TSharedPtr<IProceduralPropertyMatrixField> Field) {
		Fields.Add(Field);
		FieldMap.Add(Field->Name, Field.Get());
	}

	IProceduralPropertyMatrixField* Find(FName Name) {
		if (auto Field = FieldMap.Find(Name)) {
			return *Field;
		}
		return nullptr;
	}
};


USTRUCT(BlueprintType)
struct FProceduralObjectMatrix{
	GENERATED_BODY()
public:
	TMap<UObject*, TSharedPtr<FProceduralObjectMatrixRow>> ObjectInfoMap;

	TArray<TSharedPtr<FProceduralObjectMatrixRow>> ObjectInfoList;

	UPROPERTY()
	TArray<FName> FieldKeys;

	UPROPERTY()
	bool bIsDirty = false;

	TSharedPtr<SListView<TSharedPtr<FProceduralObjectMatrixRow>>> ObjectInfoListView;

	FName SortedColumnName;

	EColumnSortMode::Type SortMode = EColumnSortMode::None;
};