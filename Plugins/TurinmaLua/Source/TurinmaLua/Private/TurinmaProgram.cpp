#include "TurinmaProgram.h"

#include <coroutine>

uint32 FTurinmaValue::GetHash() const
{
	uint32 Hash = GetTypeHash(ValueType);
	switch (ValueType)
	{
	case ETurinmaValueType::Nil: break;
	case ETurinmaValueType::Bool: 
		Hash = HashCombine(Hash, GetTypeHash(BooleanValue));
		break;
	case ETurinmaValueType::Int:
		Hash = HashCombine(Hash, GetTypeHash(IntValue));
		break;
	case ETurinmaValueType::Real:
		Hash = HashCombine(Hash, GetTypeHash(RealValue));
		break;
	case ETurinmaValueType::Vector: 
		Hash = HashCombine(Hash, GetTypeHash(VectorValue));
		break;
	case ETurinmaValueType::Quat:
		Hash = HashCombine(Hash, GetTypeHash(QuatValue));
		break;
	case ETurinmaValueType::Matrix:
		for (int32 X = 0; X < 4; X++)
		{
			for (int32 Y = 0; Y < 4; Y++)
			{
				Hash = HashCombine(Hash, GetTypeHash(MatrixValue.M[X][Y]));
			}
		}
		break;
	case ETurinmaValueType::Transform: 
		Hash = HashCombine(Hash, GetTypeHash(TransformValue));
		break;
	case ETurinmaValueType::String: 
		Hash = HashCombine(Hash, GetTypeHash(StringValue));
		break;
	case ETurinmaValueType::Array: 
	case ETurinmaValueType::Table: 
	case ETurinmaValueType::Function:
	case ETurinmaValueType::Struct:
		if(HeapValue)
		{
			Hash = HashCombine(Hash, HeapValue->GetHash());
		}
		break;
	default:;
	}
	return Hash;
}

bool FTurinmaValue::Equals(const FTurinmaValue& Other) const
{
	bool bRet = ValueType == Other.ValueType;
	if(bRet)
	{
		switch (ValueType)
		{
		case ETurinmaValueType::Nil: break;
		case ETurinmaValueType::Bool: 
			bRet = BooleanValue == Other.BooleanValue;
			break;
		case ETurinmaValueType::Int:
			bRet = IntValue == Other.IntValue;
			break;
		case ETurinmaValueType::Real:
			bRet = RealValue == Other.RealValue;
			break;
		case ETurinmaValueType::Vector: 
			bRet = VectorValue == Other.VectorValue;
			break;
		case ETurinmaValueType::Quat:
			bRet = QuatValue == Other.QuatValue;
			break;
		case ETurinmaValueType::Matrix:
			bRet = MatrixValue == Other.MatrixValue;
			break;
		case ETurinmaValueType::Transform: 
			bRet = (TransformValue.GetTranslation() == Other.TransformValue.GetTranslation()
				&& TransformValue.GetRotation() == Other.TransformValue.GetRotation()
				&& TransformValue.GetScale3D() == Other.TransformValue.GetScale3D());
			break;
		case ETurinmaValueType::String:
			bRet = StringValue == Other.StringValue;
			break;
		case ETurinmaValueType::Array:
		case ETurinmaValueType::Table:
		case ETurinmaValueType::Function:
		case ETurinmaValueType::Struct:
			if (HeapValue)
			{
				bRet = HeapValue->Equals(*Other.HeapValue);
			}
			break;
		
		
		
		default:;
		}
	}
	return bRet;
}

uint32 FTurinmaArrayValue::GetHash() const
{
	uint32 Hash = 0;
	for(auto&& Item : Values)
	{
		Hash = HashCombine(Hash, Item.GetHash());
	}
	return Hash;
}

bool FTurinmaArrayValue::Equals(const FTurinmaHeapValue& Other) const
{
	if(this == &Other)
	{
		return true;
	}

	auto* TypedOther = Other.GetTyped<FTurinmaArrayValue>();
	if(!TypedOther)
	{
		return false;
	}
	if(Values.Num() == TypedOther->Values.Num())
	{
		for (int i = 0; i < Values.Num(); ++i)
		{
			if (!Values[i].Equals(TypedOther->Values[i]))
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

uint32 FTurinmaTableValue::GetHash() const
{
	return GetTypeHash(this);
}

bool FTurinmaTableValue::Equals(const FTurinmaHeapValue& Other) const
{
	return this == &Other;
}

uint32 FTurinmaStructValue::GetHash() const
{
	uint32 Hash = GetTypeHash(ProtoName);
	for(auto&& Item : Fields)
	{
		Hash = HashCombine(Hash, GetTypeHash(Item.first));
		Hash = HashCombine(Hash, Item.second.GetHash());
	}

	return Hash;
}

bool FTurinmaStructValue::Equals(const FTurinmaHeapValue& Other) const
{
	if (this == &Other)
	{
		return true;
	}
	auto* TypedOther = Other.GetTyped<FTurinmaStructValue>();
	if(!TypedOther)
	{
		return false;
	}

	if(ProtoName != TypedOther->ProtoName)
	{
		return false;
	}

	if(Fields.size() != TypedOther->Fields.size())
	{
		return false;
	}

	for(auto&& Item : Fields)
	{
		auto&& OtherIter = TypedOther->Fields.find(Item.first);
		if(OtherIter != TypedOther->Fields.end())
		{
			if(!OtherIter->second.Equals(Item.second))
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	return true;
}

int64 FTurinmaGraphNodeBase::GraphNodeTypeId()
{
	static int64 Inner = 0;
	return ++Inner;
}

int64 FTurinmaGraphNodeBase::StaticTypeId()
{
	static int64 Inner = GraphNodeTypeId();
	return Inner;
}

TSharedPtr<struct FTurinmaGraphNodeBase> FTurinmaGraphNodeDataTest::CreateNode(const FTurinmaNodeCreateInfo& CreateInfo)
{
	auto S = MakeShared<FTurinmaGraphNodeTest>();
	S->DataIndex = CreateInfo.DataIndex;
	return S;
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaGraphNodeTest)

TSharedPtr<struct FTurinmaGraphNodeBase> FTurinmaGraphInputNodeData::CreateNode(const FTurinmaNodeCreateInfo& CreateInfo)
{
	auto S = MakeShared<FTurinmaGraphInputNode>();
	S->DataIndex = CreateInfo.DataIndex;
	return S;
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaGraphInputNode)

UE_DISABLE_OPTIMIZATION


struct HelloCoroutine {
	struct HelloPromise {
		HelloCoroutine get_return_object() {
			return std::coroutine_handle<HelloPromise>::from_promise(*this);
		}
		std::suspend_always initial_suspend() { return {}; }//Э�̴���֮��Э�̺�����ִ��֮ǰ��ʱ��ִ�д˺�������ͬ��co_await initial_suspend();���ﷵ��suspend_always
																//��ʾ�������ú�������Э�̶���������أ���ִ��Э�̺��������ĺ����壬֪���������resume
		std::suspend_never final_suspend() noexcept
		{
			UE_LOG(LogTemp, Log, TEXT("SWP :: Suspend"));
			return {};
		}//Э�̺�����ִ��ȫ�����֮��ִ�д˺�������ͬ��co_await final_suspend();

		// Э�̺���Ҫ����ĳ�����͵�ֵ�Ļ�����co_return XXX����
		// ��Ҫ��promise������void return_value(���� value)������
		// ע��������ǽ���Э�̺����߼��ϵķ���ֵ�ĵط���Э�̺��������������Ƿ���promise����
		/*void return_value(int value) {
			UE_LOG(LogTemp, Log, TEXT("SWP :: Return"));
		}*/

		// Э�̺�������Ҫ����ֵ�Ļ�����co_return��д��Э�̺�����Ȼ����������Ҫ��promise������void return_void()������
		void return_void() {
			UE_LOG(LogTemp, Log, TEXT("SWP :: Finish Void"));
		}
		void unhandled_exception() {}
	};

	using promise_type = HelloPromise;
	HelloCoroutine(std::coroutine_handle<HelloPromise> h) : handle(h) {}

	std::coroutine_handle<HelloPromise> handle;
};


HelloCoroutine hello() {
	UE_LOG(LogTemp, Log, TEXT("SWP :: Hello"));
	co_await std::suspend_always{};
	UE_LOG(LogTemp, Log, TEXT("SWP :: World"));

	//co_return 20;
}



void FTurinmaGraph::InitWithDataAndInfo(const FTurinmaGraphData& InData, const FTurinmaGraphCreateInfo& InInfo)
{
	DataIndex = InInfo.DataIndex;
	for(int NodeDataIndx = 0; NodeDataIndx < InData.NodeDatas.Num(); ++NodeDataIndx)
	{
		auto&& Item = InData.NodeDatas[NodeDataIndx];
		Nodes.Add(Item.NodeData->CreateNode({ NodeDataIndx }));
	}
}

//bool FTurinmaProcessCallInfoItem::LocalJmp(int32 NodeIndex)
//{
//	if(Graph->IsNodeValid(NodeIndex))
//	{
//		auto&& New = LocalNodeIndex.AddDefaulted_GetRef();
//		New.NodeIndex = NodeIndex;
//	}
//	return false;
//}

bool FTurinmaProcess::IsGraphValid(int32 GraphIndex)
{
	if(!Program.IsValid())
	{
		return false;
	}
	if(Graphs.IsValidIndex(GraphIndex))
	{
		if(Program->GraphDatas.IsValidIndex(Graphs[GraphIndex].DataIndex))
		{
			return true;
		}
	}
	return false;
}

bool FTurinmaProcess::IsNodeValid(int32 GraphIndex, int32 NodeIndex)
{
	if(IsGraphValid(GraphIndex))
	{
		if(Graphs[GraphIndex].Nodes.IsValidIndex(NodeIndex))
		{
			if(Graphs[GraphIndex].Nodes[NodeIndex].IsValid())
			{
				if(Program->GraphDatas[Graphs[GraphIndex].DataIndex].NodeDatas.IsValidIndex(Graphs[GraphIndex].Nodes[NodeIndex]->DataIndex))
				{
					return !!Program->GraphDatas[Graphs[GraphIndex].DataIndex].NodeDatas[Graphs[GraphIndex].Nodes[NodeIndex]->DataIndex].NodeData;
				}
			}
		}
	}
	return false;
}

bool FTurinmaProcess::IsNodePure(int32 GraphIndex, int32 NodeIndex)
{
	if(IsNodeValid(GraphIndex, NodeIndex))
	{
		return Program->GraphDatas[Graphs[GraphIndex].DataIndex].NodeDatas[Graphs[GraphIndex].Nodes[NodeIndex]->DataIndex].NodeData->IsPure;
	}
	return false;
}

bool FTurinmaProcess::LocalJmp(FTurinmaProcessCallInfoItem& Item, int32 NodeIndex)
{
	if(IsNodeValid(Item.GraphIndex, NodeIndex))
	{
		auto&& D = Item.LocalNodeIndex.AddDefaulted_GetRef();
		D.NodeIndex = NodeIndex;
		return true;
	}
	return false;
}

void FTurinmaProcess::RecordError(const FTurinmaErrorContent& InErrorContent)
{
	ErrorInfo.bError = true;
	ErrorInfo.Content = InErrorContent;
}

FTurinmaCoroutine FTurinmaProcess::Execute()
{
	if(CallInfos.IsValidIndex(CurCallInfo))
	{
		while(!bShouldExit && !ErrorInfo.bError && CallInfos.IsValidIndex(CurCallInfo) && CallInfos[CurCallInfo].CallStack.Num() > 0)
		{
			auto&& CallInfo = CallInfos[CurCallInfo];
			auto&& CallItem = CallInfo.CallStack.Last();
			if(CallItem.LocalNodeIndex.Num() > 0)
			{
				auto&& NodeItem = CallItem.LocalNodeIndex.Last();

				auto&& Node = Graphs[CallItem.GraphIndex].Nodes[NodeItem.NodeIndex];
				auto&& NodeData = Program->GraphDatas[Graphs[CallItem.GraphIndex].DataIndex].NodeDatas[NodeItem.NodeIndex];

				{
					switch (NodeItem.Status)
					{
					case FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::None:
						NodeItem.Status = FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::PeekingParams;
						[[fallthrough]];
					case FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::PeekingParams:
					{
						bool bNeedFutureExecuteOrError = false;
						while (NodeItem.NodeInput.Num() < NodeData.NodeData->InputParams.Num())
						{
							auto&& Param = NodeData.NodeData->InputParams[NodeItem.NodeInput.Num()];
							auto Res = CallItem.TempLocalVariables.Find(Param.ParamNode);
							if (!Res)
							{
								bNeedFutureExecuteOrError = true;

								if (!IsNodePure(CallItem.GraphIndex, Param.ParamNode))
								{
									RecordError(FTurinmaErrorContent());
									break;
								}
								if (!LocalJmp(CallItem, Param.ParamNode))
								{
									RecordError(FTurinmaErrorContent());
									break;
								}
								break;
							}
							if (Res->IsValidIndex(Param.ParamPin))
							{
								NodeItem.NodeInput.Add((*Res)[Param.ParamPin]);
							}
							else
							{
								RecordError(FTurinmaErrorContent());
								bNeedFutureExecuteOrError = true;
								break;
							}
						}
						if (bNeedFutureExecuteOrError)
						{
							break;
						}
						else
						{
							NodeItem.Status = FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::Executing;
						}
					};
					[[fallthrough]];
					case FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::Executing:
					{
						FTurinmaNodeExecuteParam Param;
						Param.Process = this;
						Param.CurCallInfo = CurCallInfo;
						Param.CurCallItem = CallInfo.CallStack.Num() - 1;
						Param.MyIndex = CallItem.LocalNodeIndex.Num() - 1;
						if (!Node->Execute(Param))
						{
							RecordError(FTurinmaErrorContent());
							break;
						}
						else
						{
							NodeItem.Status = FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::PushingResult;
						}
					};
					[[fallthrough]];
					case FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::PushingResult:
					{
						auto&& V = CallItem.TempLocalVariables.FindOrAdd(NodeItem.NodeIndex);
						V.Reset(NodeData.NodeData->OutputParams.Num());
						if(NodeItem.NodeOutput.Num() != NodeData.NodeData->OutputParams.Num())
						{
							RecordError(FTurinmaErrorContent());
							break;
						}
						bool bAnyError = false;
						for(int OutI = 0; OutI < NodeItem.NodeOutput.Num(); ++OutI)
						{
							if(NodeItem.NodeOutput[OutI].ValueType != NodeData.NodeData->OutputParams[OutI].ValueType)
							{
								bAnyError = true;
								RecordError(FTurinmaErrorContent());
								break;
							}
							V.Add(NodeItem.NodeOutput[OutI]);
						}
						if(bAnyError)
						{
							break;
						}
						else
						{
							NodeItem.Status = FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::Finish;
						}
					};
					[[fallthrough]];
					case FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::Finish:
					{
						if(NodeItem.WhichNextToGo != INDEX_NONE)
						{
							if(NodeData.NodeData->NextNodes.IsValidIndex(NodeItem.WhichNextToGo))
							{
								auto NextNode = NodeData.NodeData->NextNodes[NodeItem.WhichNextToGo];
								if(IsNodeValid(CallItem.GraphIndex, NextNode.NextNode))
								{
									if(IsNodePure(CallItem.GraphIndex, NextNode.NextNode))
									{
										RecordError(FTurinmaErrorContent());
										break;
									}

									LocalJmp(CallItem, NextNode.NextNode);
									break;
								}
								else
								{
									RecordError(FTurinmaErrorContent());
									break;
								}
							}
							else
							{
								RecordError(FTurinmaErrorContent());
								break;
							}
						}
					};
						break;
					default:;
					}

					co_await std::suspend_always{};//todo coroutine not finish yet
				}
			}
		}
	}

}

FTestClass::FTestClass()
{
	//auto&& R = hello();
	//UE_LOG(LogTemp, Log, TEXT("SWP :: BeforeExecute"));
	//R.handle.resume();
	//UE_LOG(LogTemp, Log, TEXT("SWP :: Resume"));
	//R.handle.resume();

	//FTurinmaGraphNodeBase Base;
	//FTurinmaGraphNodeTest Test;

	//FTurinmaGraphNodeTest* pR = Base.GetTyped<FTurinmaGraphNodeTest>();
	//FTurinmaGraphNodeTest* pR1 = Test.GetTyped<FTurinmaGraphNodeTest>();

}
UE_ENABLE_OPTIMIZATION


FTestClass FTestClass::TestFFFFF;
