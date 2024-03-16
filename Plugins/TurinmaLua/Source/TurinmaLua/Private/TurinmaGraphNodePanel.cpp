#include "TurinmaGraphNodePanel.h"

void STurinmaGraphNodePin::Construct(const FArguments& InArgs)
{
}

void STurinmaGraphNodeSlate::Construct(const FArguments& InArgs)
{
	UMG = InArgs._UMG;

	if(UMG)
	{
		
	}

}

int32 STurinmaGraphNodeSlate::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if(UMG)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &UMG->Brush.Brush);
	}
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

void UTurinmaGraphNodeBaseWidget::BeginDestroy()
{
	if(MySlate)
	{
		MySlate->UMG = nullptr;
	}
	Super::BeginDestroy();
}

TSharedRef<SWidget> UTurinmaGraphNodeBaseWidget::RebuildWidget()
{
	MySlate = SNew(STurinmaGraphNodeSlate).UMG(this);
	return MySlate.ToSharedRef();
}

void UTurinmaGraphNodeBaseWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	if (MySlate)
	{
		MySlate->UMG = nullptr;
	}
	MySlate.Reset();
}
