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

FVector2D UTurinmaGraphNodeBaseWidget::GetWidgetLocationInOtherWidget(UWidget* Widget, UWidget* OtherWidget,
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

FVector2D UTurinmaGraphNodeBaseWidget::GetExecInputPositionInPanel(UWidget* RelativeToWidget, FVector2D Center)
{
	if(ExecInputPinWidget)
	{
		return GetWidgetLocationInOtherWidget(ExecInputPinWidget, RelativeToWidget, Center);
	}
	return GetWidgetLocationInOtherWidget(this, RelativeToWidget, Center);
}

FVector2D UTurinmaGraphNodeBaseWidget::GetExecOutputPositionInPanel(UWidget* RelativeToWidget, int32 Index,
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

FVector2D UTurinmaGraphNodeBaseWidget::GetParamInputPositionInPanel(UWidget* RelativeToWidget, int32 Index,
	FVector2D Center)
{
	if(ParamInputWidget.IsValidIndex(Index))
	{
		return GetWidgetLocationInOtherWidget(ParamInputWidget[Index], RelativeToWidget, Center);
	}
	return GetWidgetLocationInOtherWidget(this, RelativeToWidget, Center);
}

FVector2D UTurinmaGraphNodeBaseWidget::GetParamOutputPositionInPanel(UWidget* RelativeToWidget, int32 Index,
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

int32 UTurinmaGraphCanvasPanel::NativeCustomPaintBeforePaintSlots(const FPaintArgs& Args,
	const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{

	for (auto&& Item : WirelineDatas)
	{
		FSlateDrawElement::MakeSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			Item.StartPos, Item.StartDir, Item.EndPos, Item.EndDir, Item.LineThickness + 2, ESlateDrawEffect::None, FLinearColor::Black);
		FSlateDrawElement::MakeSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			Item.StartPos, Item.StartDir, Item.EndPos, Item.EndDir, Item.LineThickness, ESlateDrawEffect::None, Item.LineColor);
	}

	return LayerId;
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

		auto&& NodeDatas = CurGraphData->NodeDatas;
		for(int32 NodeIndex = 0; NodeIndex < NodeDatas.Num(); ++NodeIndex)
		{
			auto&& NodeData = NodeDatas[NodeIndex];
			if(NodeData.IsValid())
			{
				auto&& NodeWidgetType = GraphNodeDataToGraphNodeWidgetType->ResolveWidgetTypeByClass(NodeData.NodeType);
				UTurinmaGraphNodeBaseWidget* NodeW = CreateWidget<UTurinmaGraphNodeBaseWidget>(this, NodeWidgetType, NodeData.NodeData->GetNodeName());
				NodeW->NodeItem.NodeIndex = NodeIndex;
				NodeW->NodeItem.Graph.GraphPanel = this;
				NodeW->NodeItem.Graph.GraphName = InName;
				UCanvasPanelSlot* SlotW = GraphPanel->AddChildToCanvas(NodeW);
				NodeWidgets.Add(NodeIndex, NodeW);
				SlotW->SetAlignment(FVector2D(0.5f, 0.5f));
				SlotW->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
				SlotW->SetPosition(NodeData.NodeData->Location);
				SlotW->SetSize(NodeData.NodeData->Size);

				NodeW->InitData();
			}
		}
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
	GraphPanel->WirelineDatas.Reset();
	TSet<FTurinmaNodeLinkItem> Linked;
	auto* CurGraphData = HistoryBuffer.GetGraphDataByName(CurrentPanelName);
	if(CurGraphData)
	{
		for(auto&& Item : NodeWidgets)
		{
			if(Item.Value && CurGraphData->NodeDatas.IsValidIndex(Item.Key))
			{
				auto&& NodeData = CurGraphData->NodeDatas[Item.Key];
				if(!NodeData.NodeData->IsPure)
				{
					for(int NextNodeIndex = 0; NextNodeIndex < NodeData.NodeData->NextNodes.Num(); ++NextNodeIndex)
					{
						auto&& NextNode = NodeData.NodeData->NextNodes[NextNodeIndex];
						if(CurGraphData->NodeDatas.IsValidIndex(NextNode.NextNode)
							&& CurGraphData->NodeDatas[NextNode.NextNode].IsValid()
							&& !CurGraphData->NodeDatas[NextNode.NextNode].NodeData->IsPure
							&& CurGraphData->NodeDatas[NextNode.NextNode].NodeData->HasExecInput())
						{
							auto* Res = NodeWidgets.Find(NextNode.NextNode);
							if(Res && *Res)
							{
								FVector2D NextInputPos = (*Res)->GetExecInputPositionInPanel(GraphPanel);
								FVector2D CurOutputPos = Item.Value->GetExecOutputPositionInPanel(GraphPanel, NextNodeIndex);
								FVector2D StartDir(FMath::Max(NextInputPos.X - CurOutputPos.X, 50.0), 0);
								GraphPanel->WirelineDatas.Emplace(UTurinmaGraphCanvasPanel::FGraphNodeLinkWirelineData{
									CurOutputPos,
									StartDir,NextInputPos, StartDir, FLinearColor(1,1,1), 10 });
							}
						}
					}
				}
				auto&& ParamInputs = NodeData.NodeData->InputParams;
				for (int InputIndex = 0; InputIndex < ParamInputs.Num(); ++InputIndex)
				{
					auto&& ParamInput = ParamInputs[InputIndex];
					if(CurGraphData->NodeDatas.IsValidIndex(ParamInput.ParamNode)
						&& CurGraphData->NodeDatas[ParamInput.ParamNode].IsValid()
						)
					{
						auto&& InputNodeOutputPins = CurGraphData->NodeDatas[ParamInput.ParamNode].NodeData->GetOutputParamDescs();
						if(InputNodeOutputPins.IsValidIndex(ParamInput.ParamPin))
						{
							auto* Res = NodeWidgets.Find(ParamInput.ParamNode);
							if(Res && *Res)
							{
								FVector2D CurInputPinPos = Item.Value->GetParamInputPositionInPanel(GraphPanel, InputIndex);
								FVector2D InputNodeOutputPinPos = (*Res)->GetParamOutputPositionInPanel(GraphPanel, ParamInput.ParamPin);
								FVector2D StartDir(FMath::Max(CurInputPinPos.X - InputNodeOutputPinPos.X, 50.0), 0);
								GraphPanel->WirelineDatas.Emplace(UTurinmaGraphCanvasPanel::FGraphNodeLinkWirelineData{
									InputNodeOutputPinPos,
									StartDir,CurInputPinPos, StartDir, FLinearColor(0,1,0), 5 });

							}
						}
					}
				}
				
			}
		}
	}

	GraphPanel->bShouldCustomDraw = GraphPanel->WirelineDatas.Num() > 0;
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