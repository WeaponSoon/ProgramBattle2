#include "TurinmaGraphNodePanel.h"

#include "MotionDelayBuffer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"

UE_DISABLE_OPTIMIZATION

FTurinmaGraphData* FTurinmaGraphItem::GetGraphData()
{
	if (!GraphPanel)
	{
		return nullptr;
	}
	return GraphPanel->HistoryBuffer.GetGraphDataByName(GraphName);

}

bool UTurinmaGraphNodeBaseWidget::Initialize()
{
	bool SuperRet = Super::Initialize();

	//if (BasePanel)
	//{
	//	if ((TitleClass) && (!TitleWidget || TitleWidget->GetClass() != TitleClass))
	//	{
	//		TitleWidget = WidgetTree->ConstructWidget<UWidget>(TitleClass);
	//		if (TitleWidget)
	//		{
	//			BasePanel->AddChildToCanvas(TitleWidget);
	//		}
	//	}
	//}

	return SuperRet;
}

void UTurinmaGraphNodeBaseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitData();
}

void UTurinmaGraphNodeBaseWidget::OnNodeTitleChanged(const FText& Text)
{
}

void UTurinmaGraphNodeBaseWidget::OnNodeTitleCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if(TitleWidget)
	{
		if(NodeItem.GetGraphNodeData() && NodeItem.GetGraphNodeData()->CanChangeNodeNameTo(Text.ToString()))
		{
			ITurinmaParamTitleWidgetInterface::Execute_SetTitle(TitleWidget, Text.ToString());
			ITurinmaParamTitleWidgetInterface::Execute_SetEditable(TitleWidget, false);
		}
	}
}

void UTurinmaGraphNodeBaseWidget::OnInitData_Implementation()
{
}

void UTurinmaGraphNodeBaseWidget::InitData()
{
	ResetUI();
	if (auto* NodeData = NodeItem.GetGraphNodeData())
	{
		if (TitleContainer)
		{
			if (TitleClass)
			{
				TitleWidget = WidgetTree->ConstructWidget<UWidget>(TitleClass);
				if (TitleWidget)
				{
					TitleContainer->AddChild(TitleWidget);
				}
			}
		}
		if(TitleWidget)
		{
			ITurinmaParamTitleWidgetInterface::Execute_OnSetTurinmaGraphNodeWidget(TitleWidget, this);
			ITurinmaParamTitleWidgetInterface::Execute_SetTitle(TitleWidget, NodeData->GetNodeName().ToString());
		}

		if (ExecInputContainer && !NodeData->IsPure && NodeData->HasExecInput())
		{
			if (ExecInputPinClass)
			{
				ExecInputPinWidget = WidgetTree->ConstructWidget<UWidget>(ExecInputPinClass);
				if (ExecInputPinWidget)
				{
					ExecInputContainer->AddChild(ExecInputPinWidget);
				}
			}
		}
		if (ExecOutputContainer && !NodeData->IsPure && NodeData->DesiredNextNodesNumber() > 0)
		{
			if (ExecOutputPinClass)
			{
				ExecOutputPinWidget = WidgetTree->ConstructWidget<UWidget>(ExecOutputPinClass);
				if (ExecOutputPinWidget)
				{
					ExecOutputContainer->AddChild(ExecOutputPinWidget);
				}
			}
		}
		if(ExtraExecOutputContainer && !NodeData->IsPure && NodeData->DesiredNextNodesNumber() > 1)
		{
			for(int NOI = 1; NOI < NodeData->DesiredNextNodesNumber(); ++NOI)
			{
				auto* ExtraExecOutputPinWidget = WidgetTree->ConstructWidget<UWidget>(ExecOutputPinClass);
				if (ExtraExecOutputPinWidget)
				{
					ExtraExecOutputContainer->AddChild(ExtraExecOutputPinWidget);
					ExtraExecOutputPinWidgets.Add(ExtraExecOutputPinWidget);
				}
			}
		}

		auto&& SetParamNode = [this](UVerticalBox* Container, TSubclassOf<UWidget> InClass, const TArray<FTurinmaGraphNodeParamDescInfo>& InDesc, TArray<UWidget*>& InWidgts, ETurinmaPinKind InPinKind)->void
		{
			if (InWidgts.Num() != InDesc.Num())
			{
				for (auto&& IW : InWidgts)
				{
					IW->RemoveFromParent();
				}
				InWidgts.Reset(InDesc.Num());
				for (auto&& Item : InDesc)
				{
					auto* IW = WidgetTree->ConstructWidget<UWidget>(InClass);
					Container->AddChildToVerticalBox(IW);
					InWidgts.Add(IW);
				}
			}
			for (int IPa = 0; IPa < InWidgts.Num(); ++IPa)
			{
				auto&& IW = InWidgts[IPa];
				auto&& ParamDesc = InDesc[IPa];
				ITurinmaParamPinWidgetInterface::Execute_OnSetTurinmaGraphNodeWidgetForPin(IW, this);
				ITurinmaParamPinWidgetInterface::Execute_SetPinValueKind(IW, ParamDesc.ValueType);
				ITurinmaParamPinWidgetInterface::Execute_SetPinKind(IW, InPinKind);
				ITurinmaParamPinWidgetInterface::Execute_SetPinText(IW, ParamDesc.ParamName.ToString());
			}
		};

		

		if(InputList)
		{
			auto&& InputParams = NodeData->GetInputParamDescs();
			SetParamNode(InputList, ParamInputInterface, InputParams, ParamInputWidget, ETurinmaPinKind::ParamInput);
		}
		if(OutputList)
		{
			auto&& OutputParams = NodeData->GetOutputParamDescs();
			SetParamNode(OutputList, ParamOutputInterface, OutputParams, ParamOutputWidget, ETurinmaPinKind::ParamOutput);
		}

		OnInitData();
	}
}

void UTurinmaGraphNodeBaseWidget::ResetUI()
{
	if(TitleContainer)
	{
		TitleContainer->ClearChildren();
		TitleWidget = nullptr;

	}

	if(ExecInputContainer)
	{
		ExecInputContainer->ClearChildren();
		ExecInputPinWidget = nullptr;
	}
	
	if(ExecOutputContainer)
	{
		ExecOutputContainer->ClearChildren();
		ExecOutputPinWidget = nullptr;
	}

	if(ExtraExecOutputContainer)
	{
		ExtraExecOutputContainer->ClearChildren();
		ExtraExecOutputPinWidgets.Empty();
	}

	if(InputList)
	{
		InputList->ClearChildren();
		ParamInputWidget.Empty();
	}

	if(OutputList)
	{
		OutputList->ClearChildren();
		ParamOutputWidget.Empty();
	}
}

FVector2D UTurinmaGraphNodeBaseWidget::GetWidgetLocationInOtherWidget(const UWidget* Widget, const UWidget* OtherWidget,
	FVector2D Center)
{
	if(Widget)
	{
		auto&& LocalCenter = Widget->GetCachedGeometry().GetLocalPositionAtCoordinates(Center);
		auto&& AbsPos = Widget->GetCachedGeometry().LocalToAbsolute(LocalCenter);
		return OtherWidget ? OtherWidget->GetCachedGeometry().AbsoluteToLocal(AbsPos) : AbsPos;
	}
	return {};
}

FVector2D UTurinmaGraphNodeBaseWidget::GetExecInputPositionInPanel(const UWidget* RelativeToWidget, FVector2D Center)
{
	if(ExecInputPinWidget)
	{
		return GetWidgetLocationInOtherWidget(ExecInputPinWidget, RelativeToWidget, Center);
	}
	return GetWidgetLocationInOtherWidget(this, RelativeToWidget, Center);
}

FVector2D UTurinmaGraphNodeBaseWidget::GetExecOutputPositionInPanel(const UWidget* RelativeToWidget, int32 Index,
	FVector2D Center)
{
	if(Index == 0)
	{
		if(ExecOutputPinWidget)
		{
			return GetWidgetLocationInOtherWidget(ExecOutputPinWidget, RelativeToWidget, Center);
		}
	}
	else if(ExtraExecOutputPinWidgets.IsValidIndex(Index - 1) && ExtraExecOutputPinWidgets[Index - 1])
	{
		return GetWidgetLocationInOtherWidget(ExtraExecOutputPinWidgets[Index - 1], RelativeToWidget, Center);
	}
	return GetWidgetLocationInOtherWidget(this, RelativeToWidget, Center);
}

FVector2D UTurinmaGraphNodeBaseWidget::GetParamInputPositionInPanel(const UWidget* RelativeToWidget, int32 Index,
	FVector2D Center)
{
	if(ParamInputWidget.IsValidIndex(Index))
	{
		return GetWidgetLocationInOtherWidget(ParamInputWidget[Index], RelativeToWidget, Center);
	}
	return GetWidgetLocationInOtherWidget(this, RelativeToWidget, Center);
}

FVector2D UTurinmaGraphNodeBaseWidget::GetParamOutputPositionInPanel(const UWidget* RelativeToWidget, int32 Index,
	FVector2D Center)
{
	if (ParamOutputWidget.IsValidIndex(Index))
	{
		return GetWidgetLocationInOtherWidget(ParamOutputWidget[Index], RelativeToWidget, Center);
	}
	return GetWidgetLocationInOtherWidget(this, RelativeToWidget, Center);
}

void FTurinmaGraphDataRedoUndoItem::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	for (auto&& Item : History)
	{
		Item.AddStructReferencedObjects(Collector);
	}
	for (auto&& Item : UndoHistory)
	{
		Item.AddStructReferencedObjects(Collector);
	}
}

void UTurinmaGraphCanvasPanel::UpdateLink() const
{
	auto&& CurDS = GetCachedGeometry().GetAbsoluteSize();

	if(!bShouldRecalculateLinks && CurDS == DesiredSizeCache)
	{
		return;
	}
	DesiredSizeCache = CurDS;
	bShouldRecalculateLinks = false;
	WirelineDatas.Reset();
	if(!ParentWidget)
	{
		return;
	}
	auto* CurGraphData = ParentWidget->HistoryBuffer.GetGraphDataByName(ParentWidget->CurrentPanelName);
	if (CurGraphData)
	{
		for (auto&& Item : ParentWidget->NodeWidgets)
		{
			if (Item.Value && CurGraphData->GetNode(Item.Key))
			{
				auto&& NodeData = CurGraphData->GetNode(Item.Key);
				if (!NodeData->IsPure)
				{
					for (int NextNodeIndex = 0; NextNodeIndex < NodeData->NextNodes.Num(); ++NextNodeIndex)
					{
						auto&& NextNode = NodeData->NextNodes[NextNodeIndex];
						auto* NextNodeNode = CurGraphData->GetNode(NextNode.NextNode);
						if (NextNodeNode 
							&& !NextNodeNode->IsPure
							&& NextNodeNode->HasExecInput())
						{
							auto* Res = ParentWidget->NodeWidgets.Find(NextNode.NextNode);
							if (Res && *Res)
							{
								FVector2D NextInputPos = (*Res)->GetExecInputPositionInPanel(this);
								FVector2D CurOutputPos = Item.Value->GetExecOutputPositionInPanel(this, NextNodeIndex);
								FVector2D StartDir(FMath::Max(NextInputPos.X - CurOutputPos.X, 100.0) * 2.0, 0);
								WirelineDatas.Emplace(UTurinmaGraphCanvasPanel::FGraphNodeLinkWirelineData{
									CurOutputPos,
									StartDir,NextInputPos, StartDir, FLinearColor(1,1,1), 10 });
							}
						}
					}
				}
				auto&& ParamInputs = NodeData->InputParams;
				for (int InputIndex = 0; InputIndex < ParamInputs.Num(); ++InputIndex)
				{
					auto&& ParamInput = ParamInputs[InputIndex];
					auto* ParamNode = CurGraphData->GetNode(ParamInput.ParamNode);
					if (ParamNode
						)
					{
						auto&& InputNodeOutputPins = ParamNode->GetOutputParamDescs();
						if (InputNodeOutputPins.IsValidIndex(ParamInput.ParamPin))
						{
							auto* Res = ParentWidget->NodeWidgets.Find(ParamInput.ParamNode);
							if (Res && *Res)
							{
								FVector2D CurInputPinPos = Item.Value->GetParamInputPositionInPanel(this, InputIndex);
								FVector2D InputNodeOutputPinPos = (*Res)->GetParamOutputPositionInPanel(this, ParamInput.ParamPin);
								FVector2D StartDir(FMath::Max(CurInputPinPos.X - InputNodeOutputPinPos.X, 100.0) * 2.0, 0);
								WirelineDatas.Emplace(UTurinmaGraphCanvasPanel::FGraphNodeLinkWirelineData{
									InputNodeOutputPinPos,
									StartDir,CurInputPinPos, StartDir, FLinearColor(0,1,0), 5 });

							}
						}
					}
				}

			}
		}
	}
}

int32 UTurinmaGraphCanvasPanel::NativeCustomPaintAfterPaintSlots(const FPaintArgs& Args,
                                                                  const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                                                  int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	UpdateLink();
	for (auto&& Item : WirelineDatas)
	{
		FSlateDrawElement::MakeSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			Item.StartPos, Item.StartDir, Item.EndPos, Item.EndDir, Item.LineThickness + 2, ESlateDrawEffect::None, FLinearColor::Black);
		FSlateDrawElement::MakeSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			Item.StartPos, Item.StartDir, Item.EndPos, Item.EndDir, Item.LineThickness, ESlateDrawEffect::None, Item.LineColor);
	}

	return LayerId;
}


void UTurinmaGraphPanelBaseWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if(GraphPanel)
	{
		GraphPanel->ParentWidget = this;
	}
}

UTurinmaGraphPanelBaseWidget::UTurinmaGraphPanelBaseWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTurinmaGraphNodeWidgetRegister>
		R(TEXT("/Script/TurinmaLua.TurinmaGraphNodeWidgetRegister'/TurinmaLua/TurinmaNodeWidgets/DefaultTurinmaGraphNodeWidgetRegister.DefaultTurinmaGraphNodeWidgetRegister'"));
	if(R.Succeeded())
	{
		GraphNodeDataToGraphNodeWidgetType = R.Object;
	}
}

void UTurinmaGraphPanelBaseWidget::BuildGraphPanel(FName InName)
{
	ResetGraphPanel();

	auto* CurGraphData = HistoryBuffer.GetGraphDataByName(InName);
	if(CurGraphData)
	{
		CurrentPanelName = InName;

		CurGraphData->ForEachNode([this, InName](int32 NodeIndex, const FTurinmaGraphNodeDataBase* NodeData)->bool
		{
				if (NodeData)
				{
					auto&& NodeWidgetType = GraphNodeDataToGraphNodeWidgetType->ResolveWidgetTypeByClass(NodeData->GetDataType());
					UTurinmaGraphNodeBaseWidget* NodeW = CreateWidget<UTurinmaGraphNodeBaseWidget>(this, NodeWidgetType, NodeData->GetNodeName());
					NodeW->NodeItem.NodeIndex = NodeIndex;
					NodeW->NodeItem.Graph.GraphPanel = this;
					NodeW->NodeItem.Graph.GraphName = InName;
					UCanvasPanelSlot* SlotW = GraphPanel->AddChildToCanvas(NodeW);
					NodeWidgets.Add(NodeIndex, NodeW);
					SlotW->SetAlignment(FVector2D(0.5f, 0.5f));
					SlotW->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
					SlotW->SetPosition(NodeData->Location);
					SlotW->SetSize(NodeData->Size);

					NodeW->InitData();
				}
				return true;
		});

		UpdateLink();
	}
}

struct FTurinmaNodeLinkItem
{
	int32 NodeIndex = INDEX_NONE;
	ETurinmaPinKind PinType = ETurinmaPinKind::None;
	int32 PinIndexInType = INDEX_NONE;
};

uint32 GetTypeHash(const FTurinmaNodeLinkItem& Item)
{
	uint32 NodeIndexHash = GetTypeHash(Item.NodeIndex);
	uint32 Temp = HashCombine(NodeIndexHash, GetTypeHash(Item.PinType));
	Temp = HashCombine(Temp, GetTypeHash(Item.PinIndexInType));
	return Temp;
}

void UTurinmaGraphPanelBaseWidget::UpdateLink()
{
	GraphPanel->bShouldRecalculateLinks = true;
}


void UTurinmaGraphPanelBaseWidget::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	/*UTurinmaGraphPanelBaseWidget* This = CastChecked<UTurinmaGraphPanelBaseWidget>(InThis);
	for(int i = 0; i < This->HistoryBuffer.Count(); ++i)
	{
		auto&& Item = This->HistoryBuffer.PokeAtOffset(i);
		Collector.AddReferencedObject(Item, This);
	}*/
}

UE_ENABLE_OPTIMIZATION