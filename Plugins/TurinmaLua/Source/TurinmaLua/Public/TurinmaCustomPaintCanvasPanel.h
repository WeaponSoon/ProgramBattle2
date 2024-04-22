#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "Components/CanvasPanel.h"

#include "TurinmaCustomPaintCanvasPanel.generated.h"

//FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);


UCLASS()
class TURINMALUA_API UTurinmaCustomPaintCanvasPanel : public UCanvasPanel
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldCustomDraw = false;

	UFUNCTION(BlueprintNativeEvent)
	void OnCustomPaintBeforePaintSlots(UPARAM(ref) FPaintContext& Context) const;
	UFUNCTION(BlueprintNativeEvent)
	void OnCustomPaintAfterPaintSlots(UPARAM(ref) FPaintContext& Context) const;

	virtual int32 NativeCustomPaintBeforePaintSlots(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		OnCustomPaintBeforePaintSlots(Context);
		return FMath::Max(LayerId, Context.MaxLayer);
	};
	virtual int32 NativeCustomPaintAfterPaintSlots(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		OnCustomPaintAfterPaintSlots(Context);
		return FMath::Max(LayerId, Context.MaxLayer);
	};


	virtual TSharedRef<SWidget> RebuildWidget() override;
};