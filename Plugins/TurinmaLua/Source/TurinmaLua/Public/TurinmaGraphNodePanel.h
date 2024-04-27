#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "TurinmaProgram.h"
#include "Components/CanvasPanel.h"
#include "Components/ContentWidget.h"
#include "Components/Image.h"
#include "Components/EditableText.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "TurinmaCommon.h"
#include "TurinmaCustomPaintCanvasPanel.h"
#include "Components/InvalidationBox.h"
#include "Slate/SceneViewport.h"
#include "TurinmaGraphNodePanel.generated.h"


UENUM(BlueprintType)
enum class ETurinmaPinKind : uint8
{
	None,
	ExecInput,
	ExecOutput,
	ParamInput,
	ParamOutput,
};



UINTERFACE(Blueprintable, BlueprintType)
class TURINMALUA_API UTurinmaParamTitleWidgetInterface : public UInterface
{
	GENERATED_BODY()

};

class TURINMALUA_API ITurinmaParamTitleWidgetInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent)
	void OnSetTurinmaGraphNodeWidget(class UTurinmaGraphNodeBaseWidget* InWidget);

	UFUNCTION(BlueprintImplementableEvent)
	void SetEditable(bool bEditable);

	UFUNCTION(BlueprintImplementableEvent)
	bool IsEditable() const;

	UFUNCTION(BlueprintImplementableEvent)
	void SetTitle(const FString& InString);

	UFUNCTION(BlueprintImplementableEvent)
	FString GetTitle() const;
};


UINTERFACE(Blueprintable, BlueprintType)
class TURINMALUA_API UTurinmaParamPinWidgetInterface : public UInterface
{
	GENERATED_BODY()



};

class TURINMALUA_API ITurinmaParamPinWidgetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnSetTurinmaGraphNodeWidgetForPin(class UTurinmaGraphNodeBaseWidget* InWidget);

	UFUNCTION(BlueprintImplementableEvent)
	void SetPinKind(ETurinmaPinKind PinKind);

	UFUNCTION(BlueprintImplementableEvent)
	ETurinmaPinKind GetPinKind();

	UFUNCTION(BlueprintImplementableEvent)
	void SetPinValueKind(ETurinmaValueType PinKind);

	UFUNCTION(BlueprintImplementableEvent)
	ETurinmaValueType GetPinValueKind();

	UFUNCTION(BlueprintImplementableEvent)
	void SetPinText(const FString& PinText);

	UFUNCTION(BlueprintImplementableEvent)
	FString GetPinText();
};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	class UTurinmaGraphPanelBaseWidget* GraphPanel = nullptr;

	UPROPERTY(EditAnywhere)
	FName GraphName = NAME_None;

	FTurinmaGraphData* GetGraphData() const;
};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FTurinmaGraphItem Graph;

	UPROPERTY(EditAnywhere)
	int32 NodeIndex = INDEX_NONE;

	UTurinmaGraphPanelBaseWidget* GetPanelWidget() const
	{
		return Graph.GraphPanel;
	}

	FTurinmaGraphNodeDataBase* GetGraphNodeData() const
	{
		auto&& GraphData = Graph.GetGraphData();
		if(GraphData)
		{
			return GraphData->GetNode(NodeIndex);
		}
		return nullptr;
	}
	
};


UCLASS()
class TURINMALUA_API UTurinmaPinClickablePanel : public UTurinmaClickableContentPanel
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETurinmaPinKind PinKind = ETurinmaPinKind::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Index = INDEX_NONE;
};


UCLASS(BlueprintType, Blueprintable)
class TURINMALUA_API UTurinmaGraphNodeBaseWidget : public UUserWidget
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	FTurinmaGraphNodeItem NodeItem;

public:

	bool Initialize() override;

	virtual void NativeConstruct() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* InputList = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* OutputList = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UContentWidget* TitleContainer = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UContentWidget* ExecOutputContainer = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UContentWidget* ExecInputContainer = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget, OptionalWidget))
	UVerticalBox* ExtraExecOutputContainer = nullptr;



	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UCanvasPanel* BasePanel = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UImage* Background = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MustImplement = "/Script/TurinmaLua.TurinmaParamTitleWidgetInterface"))
	TSubclassOf<UWidget> TitleClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UWidget* TitleWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UWidget> ExecOutputPinClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UWidget* ExecOutputPinWidget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<UWidget*> ExtraExecOutputPinWidgets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UWidget> ExecInputPinClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UWidget* ExecInputPinWidget = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MustImplement = "/Script/TurinmaLua.TurinmaParamPinWidgetInterface"))
	TSubclassOf<UWidget> ParamOutputInterface;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<UWidget*> ParamOutputWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MustImplement = "/Script/TurinmaLua.TurinmaParamPinWidgetInterface"))
	TSubclassOf<UWidget> ParamInputInterface;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<UWidget*> ParamInputWidget;

	UFUNCTION(BlueprintCallable)
	void OnNodeTitleChanged(const FText& Text);
	UFUNCTION(BlueprintCallable)
	void OnNodeTitleCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION(BlueprintNativeEvent)
	void OnInitData();


	UFUNCTION(BlueprintCallable, CustomThunk, meta=(CustomStructureParam="OutGraphNodeData"))
	static bool GetNodeData(const FTurinmaGraphNodeItem& InNodeItem, FTurinmaGraphNodeDataBase& OutGraphNodeData);
	DECLARE_FUNCTION(execGetNodeData)
	{
		P_GET_STRUCT_REF(FTurinmaGraphNodeItem, Z_Param_InNodeItem);

		FTurinmaGraphNodeDataBase Z_Param_OutGraphNodeDataTemp;
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		FTurinmaGraphNodeDataBase& Z_Param_OutGraphNodeData = Stack.StepCompiledInRef<FStructProperty, FTurinmaGraphNodeDataBase>(&Z_Param_OutGraphNodeDataTemp);

		P_FINISH;
		P_NATIVE_BEGIN;
		if(Z_Param_InNodeItem.GetGraphNodeData() && 
			Z_Param_OutGraphNodeData.GetDataType() == Z_Param_InNodeItem.GetGraphNodeData()->GetDataType())
		{
			Z_Param_OutGraphNodeData.CopyForm(Z_Param_InNodeItem.GetGraphNodeData());
			(*(bool*)Z_Param__Result) = true;
		}
		else
		{
			(*(bool*)Z_Param__Result) = false;
		}
		P_NATIVE_END;
	}

	UFUNCTION(BlueprintCallable)
	void InitData();
	UFUNCTION(BlueprintCallable)
	void ResetUI();

	UFUNCTION(BlueprintPure)
	static FVector2D GetWidgetLocationInOtherWidget(const UWidget* Widget,const UWidget* OtherWidget, FVector2D Center);

	UFUNCTION(BlueprintPure)
	FVector2D GetExecInputPositionInPanel(const UWidget* RelativeToWidget, FVector2D Center = FVector2D(0.5,0.5));
	UFUNCTION(BlueprintPure)
	FVector2D GetExecOutputPositionInPanel(const UWidget* RelativeToWidget, int32 Index, FVector2D Center = FVector2D(0.5, 0.5));
	UFUNCTION(BlueprintPure)
	FVector2D GetParamInputPositionInPanel(const UWidget* RelativeToWidget, int32 Index, FVector2D Center = FVector2D(0.5, 0.5));
	UFUNCTION(BlueprintPure)
	FVector2D GetParamOutputPositionInPanel(const UWidget* RelativeToWidget, int32 Index, FVector2D Center = FVector2D(0.5, 0.5));

	UFUNCTION()
	void OnPinHovered(UTurinmaClickableContentPanel* Panel, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnPinUnhovered(UTurinmaClickableContentPanel* Panel, const FPointerEvent& MouseEvent);

	UFUNCTION()
	FEventReply OnPinDown(UTurinmaClickableContentPanel* Panel, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION()
	FEventReply OnPinUp(UTurinmaClickableContentPanel* Panel, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
};

UCLASS(BlueprintType)
class TURINMALUA_API UTurinmaGraphNodeWidgetRegister : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, TSubclassOf<UTurinmaGraphNodeBaseWidget>> NodeTypeToWidgetType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UTurinmaGraphNodeBaseWidget> DefaultNodeWidgetType;

	UFUNCTION(BlueprintCallable)
	static TSubclassOf<UTurinmaGraphNodeBaseWidget> GetVeryDefaultGraphNodeWidget()
	{
		UClass* FinalRet = LoadClass<UTurinmaGraphNodeBaseWidget>(nullptr,
			TEXT("/Script/CoreUObject.Class'/TurinmaLua/TurinmaNodeWidgets/Default/DefaultTurinmaGraphNodeWidget.DefaultTurinmaGraphNodeWidget_C'")
		);
		return FinalRet;
	}



	UFUNCTION(BlueprintCallable)
	TSubclassOf<UTurinmaGraphNodeBaseWidget> ResolveWidgetType(FName InName)
	{
		auto* Res = NodeTypeToWidgetType.Find(InName);
		if(Res)
		{
			return *Res;
		}
		if(DefaultNodeWidgetType)
		{
			return DefaultNodeWidgetType;
		}
		return GetVeryDefaultGraphNodeWidget();
	}
	UFUNCTION(BlueprintCallable)
	TSubclassOf<UTurinmaGraphNodeBaseWidget> ResolveWidgetTypeByClass(UScriptStruct* InNodeType)
	{
		if(InNodeType && InNodeType->IsChildOf(FTurinmaGraphNodeDataBase::StaticStruct()))
		{
			FName PathName = *InNodeType->GetPathName();
			return ResolveWidgetType(PathName);
		}
		return nullptr;
	}
};


USTRUCT()
struct TURINMALUA_API FTurinmaGraphDataRedoUndoItem
{
	GENERATED_BODY()

	TTurinmaCircularQueue<FTurinmaGraphData> History;
	TTurinmaCircularQueue<FTurinmaGraphData> UndoHistory;

	void AddStructReferencedObjects(class FReferenceCollector& Collector);
};
template<> struct TStructOpsTypeTraits<FTurinmaGraphDataRedoUndoItem> : public TStructOpsTypeTraitsBase2<FTurinmaGraphDataRedoUndoItem>
{
	enum { WithAddStructReferencedObjects = true };
};

USTRUCT()
struct TURINMALUA_API FTurinmaGraphHistory
{
	GENERATED_BODY()

	UPROPERTY()
	UTurinmaProgram* Program = nullptr;

	UPROPERTY()
	TMap<FName, FTurinmaGraphDataRedoUndoItem> ModifiedTurinmaGraph;

	FTurinmaGraphData* GetGraphDataByName(FName InName)
	{
		if(Program)
		{
			auto* ModifiedRes = ModifiedTurinmaGraph.Find(InName);
			if(ModifiedRes && ModifiedRes->History.GetCount())
			{
				return ModifiedRes->History.PeekLast();
			}
			else
			{
				auto* Res = Program->GraphDatas.FindByPredicate([InName](const FTurinmaGraphData& InData)->bool {return InData.GraphName == InName; });
				if(Res)
				{
					return Res;
				}
			}
		}
		return nullptr;
	}
	//create a new history node and return it to modify
	FTurinmaGraphData* ModifyGraph(FName InName)
	{
		if (Program)
		{
			auto&& ModifyRes = ModifiedTurinmaGraph.FindOrAdd(InName);
			ModifyRes.UndoHistory.Reset();
			if (ModifyRes.History.GetCount())
			{
				return ModifyRes.History.Enqueue(*ModifyRes.History.PeekLast());
			}
			else
			{
				if (auto* Res = Program->GraphDatas.FindByPredicate([InName](const FTurinmaGraphData& InData)->bool {return InData.GraphName == InName; }))
				{
					return ModifyRes.History.Enqueue(*Res);
				}
			}
		}
		return nullptr;
	}
	FTurinmaGraphData* UndoGraph(FName InName)
	{
		if(Program)
		{
			auto* ModifiedRes = ModifiedTurinmaGraph.Find(InName);
			if (ModifiedRes && ModifiedRes->History.GetCount())
			{
				ModifiedRes->UndoHistory.Enqueue(ModifiedRes->History.PopStack());
			}
			return GetGraphDataByName(InName);
		}
		return nullptr;
	}
	FTurinmaGraphData* RedoGraph(FName InName)
	{
		if (Program)
		{
			auto* ModifiedRes = ModifiedTurinmaGraph.Find(InName);
			if (ModifiedRes && ModifiedRes->UndoHistory.GetCount())
			{
				ModifiedRes->History.Enqueue(ModifiedRes->UndoHistory.PopStack());
			}
			return GetGraphDataByName(InName);
		}
		return nullptr;
	}

	void Reset(UTurinmaProgram* InNew = nullptr)
	{
		ModifiedTurinmaGraph.Reset();
		Program = InNew;
	}

	FTurinmaGraphData* ApplyGraph(FName InName)
	{
		if (Program)
		{
			auto* Res = Program->GraphDatas.FindByPredicate([InName](const FTurinmaGraphData& InData)->bool {return InData.GraphName == InName; });
			
			auto* ModifiedRes = ModifiedTurinmaGraph.Find(InName);
			if (ModifiedRes && ModifiedRes->History.GetCount())
			{
				if (!Res)
				{
					Res = &Program->GraphDatas.AddDefaulted_GetRef();
					Res->GraphName = InName;
					Program->RebuildNameToGraphIndex();
				}
				*Res = *ModifiedRes->History.PeekLast();
			}
			return Res;
		}
		return nullptr;
	}
	
};


UCLASS()
class TURINMALUA_API UTurinmaGraphCanvasPanel : public UTurinmaCustomPaintCanvasPanel
{
	GENERATED_BODY()
public:

	struct FGraphNodeLinkWirelineData
	{
		FVector2D StartPos;
		FVector2D StartDir;
		FVector2D EndPos;
		FVector2D EndDir;

		FLinearColor LineColor;
		float LineThickness;
	};
	mutable FVector2D DesiredSizeCache;
	mutable bool bShouldRecalculateLinks = true;

	UTurinmaGraphCanvasPanel()
	{
		bShouldCustomDraw = true;
	}

	UPROPERTY(Transient)
	UTurinmaGraphPanelBaseWidget* ParentWidget = nullptr;

	mutable TArray<FGraphNodeLinkWirelineData> WirelineDatas;


	void UpdateLink() const;

	virtual int32 NativeCustomPaintAfterPaintSlots(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual int32 NativeCustomPaintBeforePaintSlots(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		return LayerId;
	}
};

UCLASS(BlueprintType, Blueprintable)
class TURINMALUA_API UTurinmaGraphPanelBaseWidget : public UUserWidget
{
	GENERATED_BODY()


public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTurinmaGraphCanvasPanel> GraphPanel = nullptr;

	UPROPERTY()
	TMap<int32, TObjectPtr<UTurinmaGraphNodeBaseWidget>> NodeWidgets;
	UPROPERTY()
	FName CurrentPanelName;

	virtual void NativeConstruct() override;

	UTurinmaGraphPanelBaseWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(Transient)
	FTurinmaGraphHistory HistoryBuffer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTurinmaProgram> EditingProgram;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHistoryCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTurinmaGraphNodeWidgetRegister> GraphNodeDataToGraphNodeWidgetType;


	UFUNCTION(BlueprintCallable)
	void ResetGraphPanel()
	{
		GraphPanel->WirelineDatas.Empty();
		GraphPanel->ClearChildren();
		NodeWidgets.Empty();
		CurrentPanelName = NAME_None;
	}

	UFUNCTION(BlueprintCallable)
	void BuildGraphPanel(FName InName);

	UFUNCTION(BlueprintCallable)
	void UpdateLink();

	UFUNCTION(BlueprintCallable)
	void SetProgramForPanel(UTurinmaProgram* Program)
	{
		EditingProgram = Program;
		HistoryBuffer.Reset(Program);
	}


	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);


};