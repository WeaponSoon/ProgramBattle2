#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "TurinmaProgram.h"
#include "Components/Image.h"
#include "TurinmaGraphNodePanel.generated.h"


UINTERFACE(Blueprintable, BlueprintType)
class TURINMALUA_API UTurinmaParamTitleWidgetInterface : public UInterface
{
	GENERATED_BODY()

};

class TURINMALUA_API ITurinmaParamTitleWidgetInterface
{
	GENERATED_BODY()

};


UINTERFACE(Blueprintable, BlueprintType)
class TURINMALUA_API UTurinmaParamPinWidgetInterface : public UInterface
{
	GENERATED_BODY()

};

class TURINMALUA_API ITurinmaParamPinWidgetInterface
{
	GENERATED_BODY()

};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphItem
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere)
	UTurinmaProgram* Program = nullptr;

	UPROPERTY(Transient, VisibleAnywhere)
	int32 GraphIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeItem
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere)
	FTurinmaGraphItem Graph;

	UPROPERTY(Transient, VisibleAnywhere)
	int32 NodeIndex = INDEX_NONE;
	
};


enum class ETurinmaPinKind
{
	None,
	ExecInput,
	ExecOutput,
	ParamInput,
	ParamOutput,
};


UCLASS(BlueprintType, Blueprintable)
class TURINMALUA_API UTurinmaGraphNodeBaseWidget : public UUserWidget
{
	GENERATED_BODY()

	friend class STurinmaGraphNodeSlate;

	TSharedPtr<STurinmaGraphNodeSlate> MySlate;

	UPROPERTY(VisibleAnywhere, Category = Data)
	FTurinmaGraphNodeItem NodeItem;

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UImage* Background = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MustImplement = "/Script/TurinmaLua.TurinmaParamTitleWidgetInterface"))
	TSubclassOf<UWidget> TitleClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UWidget> ExecOutputPinClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UWidget> ExecInputPinClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MustImplement = "/Script/TurinmaLua.TurinmaParamPinWidgetInterface"))
	TArray<TSubclassOf<UWidget>> ParamOutputInterface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MustImplement = "/Script/TurinmaLua.TurinmaParamPinWidgetInterface"))
	TArray<TSubclassOf<UWidget>> ParamInputInterface;

};