#include "TurinmaGraphNodePanel.h"

#include "MotionDelayBuffer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"

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

		if (ExecInputContainer && !NodeData->IsPure)
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
				NodeW->NodeItem.Graph.Program = HistoryBuffer.Program;
				NodeW->NodeItem.Graph.GraphName = InName;
				UCanvasPanelSlot* SlotW = GraphPanel->AddChildToCanvas(NodeW);
				NodeWidgets.Add(NodeIndex, NodeW);
				SlotW->SetAlignment(FVector2D(0.5f, 0.5f));
				SlotW->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
				SlotW->SetPosition(NodeData.NodeData->Location);
				SlotW->SetSize(NodeData.NodeData->Size);
				
			}
		}
		//todo link them all
	}
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
