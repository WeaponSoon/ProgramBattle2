#include "TurinmaGraphNodePanel.h"

#include "Blueprint/WidgetTree.h"

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

void UTurinmaGraphNodeBaseWidget::InitData()
{
	if (NodeItem.GetGraphNodeData())
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
			ITurinmaParamTitleWidgetInterface::Execute_SetTitle(TitleWidget, NodeItem.GetGraphNodeData()->GetNodeName().ToString());
		}

		if (ExecInputContainer)
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
		if (ExecOutputContainer)
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
			auto&& InputParams = NodeItem.GetGraphNodeData()->GetInputParamDescs();
			SetParamNode(InputList, ParamInputInterface, InputParams, ParamInputWidget, ETurinmaPinKind::ParamInput);
		}
		if(OutputList)
		{
			auto&& OutputParams = NodeItem.GetGraphNodeData()->GetOutputParamDescs();
			SetParamNode(OutputList, ParamOutputInterface, OutputParams, ParamOutputWidget, ETurinmaPinKind::ParamOutput);
		}
		
	}
}
