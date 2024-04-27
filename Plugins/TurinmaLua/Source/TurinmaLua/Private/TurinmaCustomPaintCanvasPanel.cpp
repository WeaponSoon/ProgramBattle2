#include "TurinmaCustomPaintCanvasPanel.h"

#include "Components/BorderSlot.h"
#include "Components/CanvasPanelSlot.h"


class STurinmaCustomPaintConstraintCanvas : public SConstraintCanvas
{
	using Super = SConstraintCanvas;
public:
	SLATE_BEGIN_ARGS(STurinmaCustomPaintConstraintCanvas)
		{
		}
		SLATE_ARGUMENT(UTurinmaCustomPaintCanvasPanel*, OwnerCanvasPanel)
		SConstraintCanvas::FArguments _SuperParam;
	SLATE_END_ARGS()

	TWeakObjectPtr<UTurinmaCustomPaintCanvasPanel> ParentPanel;

	void Construct(const FArguments& InArgs)
	{
		SConstraintCanvas::Construct(InArgs._SuperParam);
		ParentPanel = InArgs._OwnerCanvasPanel;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		bool bHasLy = ParentPanel.IsValid() && ParentPanel->bShouldCustomDraw;
		int32 RetValue = 0;
		if(bHasLy)
		{
			RetValue = ParentPanel->NativeCustomPaintBeforePaintSlots(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		}
		if(bHasLy)
		{
			RetValue = FMath::Max(SConstraintCanvas::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled), RetValue);
		}
		else
		{
			RetValue = SConstraintCanvas::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		}
		if(bHasLy)
		{
			RetValue = FMath::Max(ParentPanel->NativeCustomPaintAfterPaintSlots(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled), RetValue);
		}
		return RetValue;
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if(ParentPanel.IsValid())
		{
			return	ParentPanel->OnMouseButtonDown(MyGeometry, MouseEvent).NativeReply;
		}

		return FReply::Handled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (ParentPanel.IsValid())
		{
			return	ParentPanel->OnMouseButtonUp(MyGeometry, MouseEvent).NativeReply;
		}
		return FReply::Handled();
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
	{
		if (ParentPanel.IsValid())
		{
			return	ParentPanel->OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent).NativeReply;
		}
		return FReply::Handled();
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (ParentPanel.IsValid())
		{
			return	ParentPanel->OnMouseMove(MyGeometry, MouseEvent).NativeReply;
		}
		return FReply::Handled();
	}

	void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		
	}

	void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		
	}
};

class STurinmaClickableContentPanel : public SBorder
{
	using Super = SBorder;
public:
	SLATE_BEGIN_ARGS(STurinmaClickableContentPanel)
	{
	}
	SLATE_ARGUMENT(UTurinmaClickableContentPanel*, OwnerCanvasPanel)
	SBorder::FArguments _SuperParam;
	WidgetArgsType& SuperParam(const SBorder::FArguments& InSuperParam)
	{
		_SuperParam = InSuperParam;
		return static_cast<WidgetArgsType*>(this)->Me();// this->Me();
	}
	SLATE_END_ARGS()

	TWeakObjectPtr<UTurinmaClickableContentPanel> ParentPanel;


	void Construct(const FArguments& InArgs)
	{
		SBorder::Construct(InArgs._SuperParam);
		ParentPanel = InArgs._OwnerCanvasPanel;
	}

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if(ParentPanel.IsValid())
		{
			ParentPanel->ExecOnHovered(MyGeometry, MouseEvent);
		}
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		if(ParentPanel.IsValid())
		{
			ParentPanel->ExecOnUnhovered(MouseEvent);
		}
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		auto R = Super::OnMouseButtonDown(MyGeometry, MouseEvent);
		if(ParentPanel.IsValid() && ParentPanel->OnMouseDown(MyGeometry, MouseEvent).NativeReply.IsEventHandled())
		{
			return FReply::Handled();
		}
		return R;
	}
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		auto R = Super::OnMouseButtonUp(MyGeometry, MouseEvent);
		if (ParentPanel.IsValid() && ParentPanel->OnMouseUp(MyGeometry, MouseEvent).NativeReply.IsEventHandled())
		{
			return FReply::Handled();
		}
		return R;
	}
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		auto R = Super::OnMouseMove(MyGeometry, MouseEvent);
		if (ParentPanel.IsValid() && ParentPanel->OnMouseMove(MyGeometry, MouseEvent).NativeReply.IsEventHandled())
		{
			return FReply::Handled();
		}
		return R;
		
	}
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
	{
		auto R = Super::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
		if (ParentPanel.IsValid() && ParentPanel->OnMouseDoubleClick(InMyGeometry, InMouseEvent).NativeReply.IsEventHandled())
		{
			return FReply::Handled();
		}
		return R;
	}
};



void UTurinmaCustomPaintCanvasPanel::OnCustomPaintBeforePaintSlots_Implementation(FPaintContext& Context) const
{
}

void UTurinmaCustomPaintCanvasPanel::OnCustomPaintAfterPaintSlots_Implementation(FPaintContext& Context) const
{
}

FEventReply UTurinmaCustomPaintCanvasPanel::OnMouseButtonDown_Implementation(const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	return true;
}

FEventReply UTurinmaCustomPaintCanvasPanel::OnMouseButtonUp_Implementation(const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	return true;
}

FEventReply UTurinmaCustomPaintCanvasPanel::OnMouseButtonDoubleClick_Implementation(const FGeometry& InMyGeometry,
	const FPointerEvent& InMouseEvent)
{
	return true;
}

FEventReply UTurinmaCustomPaintCanvasPanel::OnMouseMove_Implementation(const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	return true;
}

void UTurinmaCustomPaintCanvasPanel::OnMouseEnter_Implementation(const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
}

void UTurinmaCustomPaintCanvasPanel::OnMouseLeave_Implementation(const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{

}

TSharedRef<SWidget> UTurinmaCustomPaintCanvasPanel::RebuildWidget()
{
	MyCanvas = SNew(STurinmaCustomPaintConstraintCanvas).OwnerCanvasPanel(this);
	for (UPanelSlot* PanelSlot : Slots)
	{
		if (UCanvasPanelSlot* TypedSlot = Cast<UCanvasPanelSlot>(PanelSlot))
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyCanvas.ToSharedRef());
		}
	}
	return MyCanvas.ToSharedRef();
}

#define TURINMA_HANDLE_MOUSE_EVENT(DelegateName)\
{\
	FEventReply Ret = false;\
	if (!DelegateName.IsBound() && !DelegateName##Native.IsBound())\
	{\
		Ret = true;\
	}\
	else\
	{\
		if (DelegateName.IsBound())\
		{\
			auto Temp = DelegateName.Execute(this, MyGeometry, MouseEvent);\
			if (Temp.NativeReply.IsEventHandled())\
			{\
				Ret = true;\
			}\
		}\
		if (DelegateName##Native.IsBound())\
		{\
			auto Temp = DelegateName##Native.Execute(this, MyGeometry, MouseEvent);\
			if (Temp.NativeReply.IsEventHandled())\
			{\
				Ret = true;\
			}\
		}\
	}\
	return Ret;\
}\

FEventReply UTurinmaClickableContentPanel::OnMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
TURINMA_HANDLE_MOUSE_EVENT(OnMouseButtonDownEventWithWidget)


FEventReply UTurinmaClickableContentPanel::OnMouseUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
TURINMA_HANDLE_MOUSE_EVENT(OnMouseButtonUpEventWithWidget)

FEventReply UTurinmaClickableContentPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
TURINMA_HANDLE_MOUSE_EVENT(OnMouseMoveEventWithWidget)


FEventReply UTurinmaClickableContentPanel::OnMouseDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
TURINMA_HANDLE_MOUSE_EVENT(OnMouseDoubleClickEventWithWidget)




UTurinmaClickableContentPanel::UTurinmaClickableContentPanel()
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	Background.DrawAs = ESlateBrushDrawType::NoDrawType;
	Padding = {};
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}


TSharedRef<SWidget> UTurinmaClickableContentPanel::RebuildWidget()
{
	MyBorder = SNew(STurinmaClickableContentPanel)
	.OwnerCanvasPanel(this).SuperParam(SBorder::FArguments().FlipForRightToLeftFlowDirection(bFlipForRightToLeftFlowDirection));
	if (GetChildrenCount() > 0)
	{
		Cast<UBorderSlot>(GetContentSlot())->BuildSlot(MyBorder.ToSharedRef());
	}
	return MyBorder.ToSharedRef();
}
