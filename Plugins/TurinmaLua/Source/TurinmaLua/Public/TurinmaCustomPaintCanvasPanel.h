#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
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


	UFUNCTION(BlueprintNativeEvent)
	FEventReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual FEventReply OnMouseButtonDown_Implementation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION(BlueprintNativeEvent)
	FEventReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual FEventReply OnMouseButtonUp_Implementation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION(BlueprintNativeEvent)
	FEventReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent);
	virtual FEventReply OnMouseButtonDoubleClick_Implementation(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent);

	UFUNCTION(BlueprintNativeEvent)
	FEventReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual FEventReply OnMouseMove_Implementation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION(BlueprintNativeEvent)
	void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual void OnMouseEnter_Implementation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION(BlueprintNativeEvent)
	void OnMouseLeave(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual void OnMouseLeave_Implementation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);


	virtual TSharedRef<SWidget> RebuildWidget() override;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTurinmaClickableContentHovered, class UTurinmaClickableContentPanel*, Panel, const FGeometry&, MyGeometry, const FPointerEvent&, MouseEvent);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTurinmaClickableContentHoveredNative, class UTurinmaClickableContentPanel*, const FGeometry&, const FPointerEvent&)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTurinmaClickableContentUnhovered, class UTurinmaClickableContentPanel*, Panel, const FPointerEvent&, MouseEvent);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTurinmaClickableContentUnhoveredNative, class UTurinmaClickableContentPanel*, const FPointerEvent&)

DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(FEventReply, FOnPointerEventWithInputWidget, UTurinmaClickableContentPanel*, Panel, const FGeometry&, MyGeometry, const FPointerEvent&, MouseEvent);
DECLARE_DELEGATE_RetVal_ThreeParams(FEventReply, FOnPointerEventWithInputWidgetNative, UTurinmaClickableContentPanel*, const FGeometry&, const FPointerEvent&);


UCLASS()
class TURINMALUA_API UTurinmaClickableContentPanel : public UBorder
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FTurinmaClickableContentHovered OnHovered;
	FTurinmaClickableContentHoveredNative OnHoveredNative;

	UPROPERTY(BlueprintAssignable)
	FTurinmaClickableContentUnhovered OnUnhovered;
	FTurinmaClickableContentUnhoveredNative OnUnhoveredNative;


	UPROPERTY(EditAnywhere, Category = Events, meta = (IsBindableEvent = "True"))
	FOnPointerEventWithInputWidget OnMouseButtonDownEventWithWidget;
	FOnPointerEventWithInputWidgetNative OnMouseButtonDownEventWithWidgetNative;


	UPROPERTY(EditAnywhere, Category = Events, meta = (IsBindableEvent = "True"))
	FOnPointerEventWithInputWidget OnMouseButtonUpEventWithWidget;
	FOnPointerEventWithInputWidgetNative OnMouseButtonUpEventWithWidgetNative;


	UPROPERTY(EditAnywhere, Category = Events, meta = (IsBindableEvent = "True"))
	FOnPointerEventWithInputWidget OnMouseMoveEventWithWidget;
	FOnPointerEventWithInputWidgetNative OnMouseMoveEventWithWidgetNative;


	UPROPERTY(EditAnywhere, Category = Events, meta = (IsBindableEvent = "True"))
	FOnPointerEventWithInputWidget OnMouseDoubleClickEventWithWidget;
	FOnPointerEventWithInputWidgetNative OnMouseDoubleClickEventWithWidgetNative;



	UFUNCTION(BlueprintCallable)
	virtual void ExecOnHovered(const FGeometry& InGeo, const FPointerEvent& InPointEvent)
	{
		OnHovered.Broadcast(this, InGeo, InPointEvent);
		OnHoveredNative.Broadcast(this, InGeo, InPointEvent);
	}

	UFUNCTION(BlueprintCallable)
	virtual void ExecOnUnhovered(const FPointerEvent& InPointEvent)
	{
		OnUnhovered.Broadcast(this, InPointEvent);
		OnUnhoveredNative.Broadcast(this, InPointEvent);
	}

	UFUNCTION(BlueprintCallable)
	virtual FEventReply OnMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION(BlueprintCallable)
	virtual FEventReply OnMouseUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION(BlueprintCallable)
	virtual FEventReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION(BlueprintCallable)
	virtual FEventReply OnMouseDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);


	UTurinmaClickableContentPanel();

	virtual TSharedRef<SWidget> RebuildWidget() override;
};
