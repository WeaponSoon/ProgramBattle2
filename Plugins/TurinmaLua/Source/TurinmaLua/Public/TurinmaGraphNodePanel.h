#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "TurinmaProgram.h"
#include "TurinmaGraphNodePanel.generated.h"


USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeItem
{
	GENERATED_BODY()

	FTurinmaGraphData::FTurinmaNodeDataItem TurinmaItem;
};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeBrush
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateBrush Brush;
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
};


UCLASS(BlueprintType, Blueprintable)
class TURINMALUA_API UTurinmaGraphNodeBaseWidget : public UWidget
{
	GENERATED_BODY()

	friend class STurinmaGraphNodeSlate;

	TSharedPtr<STurinmaGraphNodeSlate> MySlate;

	UPROPERTY(EditAnywhere, Category = Appearance)
	FTurinmaGraphNodeBrush Brush;
public:

	virtual void BeginDestroy() override;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

};