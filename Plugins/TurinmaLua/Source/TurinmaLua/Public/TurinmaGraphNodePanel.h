#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "TurinmaProgram.h"
#include "TurinmaGraphNodePanel.generated.h"


USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphItem
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	UTurinmaProgram* Program = nullptr;

	UPROPERTY(Transient)
	int32 GraphIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeItem
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FTurinmaGraphItem Graph;

	UPROPERTY(Transient)
	int32 NodeIndex = INDEX_NONE;


};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeBrush
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateBrush Brush;
};

enum class ETurinmaPinKind
{
	None,
	ExecInput,
	ExecOutput,
	ParamInput,
	ParamOutput,
};


class TURINMALUA_API STurinmaGraphNodePin : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STurinmaGraphNodePin)
		:_PinKind(ETurinmaPinKind::None), _ValueType(ETurinmaValueType::Nil)
	{}
	SLATE_ARGUMENT(ETurinmaPinKind, PinKind)
	SLATE_ARGUMENT(ETurinmaValueType, ValueType)
	SLATE_ARGUMENT(TWeakPtr<class STurinmaGraphNodeSlate>, Node)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

class TURINMALUA_API STurinmaGraphNodeSlate : public SCompoundWidget
{
	friend class UTurinmaGraphNodeBaseWidget;

	SLATE_BEGIN_ARGS(STurinmaGraphNodeSlate)
	: _UMG(nullptr)
	{}
	SLATE_ARGUMENT(class UTurinmaGraphNodeBaseWidget*, UMG)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	class UTurinmaGraphNodeBaseWidget* UMG = nullptr;

	TSharedPtr<SImage> Background;
	TSharedPtr<STextBlock> Title;
	TSharedPtr<STextBlock> EditableTitle;

	TSharedPtr<STurinmaGraphNodePin> ExecOutputPin;
	TSharedPtr<STurinmaGraphNodePin> ExecInputPin;

	TArray<TSharedPtr<STurinmaGraphNodePin>> InputPins;
	TArray<TSharedPtr<STurinmaGraphNodePin>> OutputPins;


};


UCLASS(BlueprintType, Blueprintable)
class TURINMALUA_API UTurinmaGraphNodeBaseWidget : public UWidget
{
	GENERATED_BODY()

	friend class STurinmaGraphNodeSlate;

	TSharedPtr<STurinmaGraphNodeSlate> MySlate;

	UPROPERTY(EditAnywhere, Category = Appearance)
	FTurinmaGraphNodeBrush Brush;

	FTurinmaGraphNodeItem TurinmaGraphNode;
public:
	virtual void BeginDestroy() override;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
};