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
	UTurinmaProgram* Program = nullptr;

	UPROPERTY(EditAnywhere)
	int32 GraphIndex = INDEX_NONE;

	FTurinmaGraphData* GetGraphData()
	{
		if(!Program)
		{
			return nullptr;
		}
		if(Program->GraphDatas.IsValidIndex(GraphIndex))
		{
			return &Program->GraphDatas[GraphIndex];
		}
		return nullptr;
	}
};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FTurinmaGraphItem Graph;

	UPROPERTY(EditAnywhere)
	int32 NodeIndex = INDEX_NONE;

	FTurinmaGraphNodeDataBase* GetGraphNodeData()
	{
		auto&& GraphData = Graph.GetGraphData();
		if(GraphData)
		{
			if(GraphData->NodeDatas.IsValidIndex(NodeIndex))
			{
				return GraphData->NodeDatas[NodeIndex].NodeData;
			}
		}
		return nullptr;
	}
	
};



UCLASS(BlueprintType, Blueprintable)
class TURINMALUA_API UTurinmaGraphNodeBaseWidget : public UUserWidget
{
	GENERATED_BODY()

	friend class STurinmaGraphNodeSlate;

	TSharedPtr<STurinmaGraphNodeSlate> MySlate;

	UPROPERTY(EditAnywhere, Category = Data)
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

	void InitData();
};