#include "TurinmaCustomPaintCanvasPanel.h"

#include "Components/CanvasPanelSlot.h"


class STurinmaCustomPaintConstraintCanvas : public SConstraintCanvas
{
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
};

void UTurinmaCustomPaintCanvasPanel::OnCustomPaintBeforePaintSlots_Implementation(FPaintContext& Context) const
{
}

void UTurinmaCustomPaintCanvasPanel::OnCustomPaintAfterPaintSlots_Implementation(FPaintContext& Context) const
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
