#include "TurinmaProgram.h"

#include <coroutine>
#if WITH_EDITOR
#include "TickableEditorObject.h"
#endif
FTurinmaHeapValue* FTurinmaValue::HeapValue(FTurinmaProcess* Process)
{
	return const_cast<FTurinmaHeapValue*>((const_cast<const FTurinmaValue*>(this)->HeapValue(Process)));
}

const FTurinmaHeapValue* FTurinmaValue::HeapValue(FTurinmaProcess* Process) const
{
	if (ValueType > ETurinmaValueType::EndOfSimpleValue && Process && Process->Heap.TurinmaHeapValues.IsValidIndex(HeapValueIndex))
	{
		
		auto R = Process->Heap.TurinmaHeapValues[HeapValueIndex].Get();
		if (R && static_cast<std::underlying_type_t<EHeapValueKind>>(R->GetHeapValueKind()) ==
			static_cast<std::underlying_type_t<ETurinmaValueType>>(ValueType) -
			static_cast<std::underlying_type_t<ETurinmaValueType>>(ETurinmaValueType::EndOfSimpleValue))
		{
			return R;
		}
		else
		{
			Process->RecordError({  });
		}
	}
	return nullptr;
}


uint32 FTurinmaValue::GetHash(class FTurinmaProcess* Process) const
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
	case ETurinmaValueType::Function:
	case ETurinmaValueType::Array: 
	case ETurinmaValueType::Table: 
	case ETurinmaValueType::Struct:
		if(HeapValue(Process))
		{
			Hash = HashCombine(Hash, HeapValue(Process)->GetHash(Process));
		}
		break;
	default:;
	}
	return Hash;
}

bool FTurinmaValue::Equals(const FTurinmaValue& Other, class FTurinmaProcess* Process) const
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
		case ETurinmaValueType::Function:
		case ETurinmaValueType::Array:
		case ETurinmaValueType::Table:
		case ETurinmaValueType::Struct:
			if (HeapValue(Process))
			{
				bRet = HeapValue(Process)->Equals(*Other.HeapValue(Process), Process);
			}
			break;
		
		
		
		default:;
		}
	}
	return bRet;
}

FString FTurinmaValue::ToLexicalString(class FTurinmaProcess* Process) const
{
	switch (ValueType)
	{
	case ETurinmaValueType::Nil:
		return TEXT("Nil");
		
	case ETurinmaValueType::Bool:
		return BooleanValue ? TEXT("True") : TEXT("False");
		
	case ETurinmaValueType::Int:
		{
		TCHAR StrBuffer[100];
		FCString::Sprintf(StrBuffer, TEXT("%lld"), IntValue);
		return FString(StrBuffer);
		}
	case ETurinmaValueType::Real:
	{
		TCHAR StrBuffer[100];
		FCString::Sprintf(StrBuffer, TEXT("%lf"), RealValue);
		return FString(StrBuffer);
	}
	case ETurinmaValueType::Vector:
		return VectorValue.ToString();
		
	case ETurinmaValueType::Quat:
		return QuatValue.ToString();
		
	case ETurinmaValueType::Matrix:
		return MatrixValue.ToString();

	case ETurinmaValueType::Transform:
		return TransformValue.ToString();

	case ETurinmaValueType::String:
		return StringValue;

	case ETurinmaValueType::EndOfSimpleValue:
		check(false)
		return TEXT("");
	default:
		return HeapValue(Process) ? HeapValue(Process)->ToLexicalString(Process) : TEXT("Nil");
	}
}

uint32 FTurinmaArrayValue::GetHash(class FTurinmaProcess* Process) const
{
	uint32 Hash = 0;
	for(auto&& Item : Values)
	{
		Hash = HashCombine(Hash, Item.GetHash(Process));
	}
	return Hash;
}

bool FTurinmaArrayValue::Equals(const FTurinmaHeapValue& Other, class FTurinmaProcess* Process) const
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
			if (!Values[i].Equals(TypedOther->Values[i], Process))
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

uint32 FTurinmaTableValue::GetHash(class FTurinmaProcess* Process) const
{
	return GetTypeHash(this);
}

bool FTurinmaTableValue::Equals(const FTurinmaHeapValue& Other, class FTurinmaProcess* Process) const
{
	return this == &Other;
}

uint32 FTurinmaStructValue::GetHash(class FTurinmaProcess* Process) const
{
	uint32 Hash = GetTypeHash(ProtoName);
	for(auto&& Item : Fields)
	{
		Hash = HashCombine(Hash, GetTypeHash(Item.first));
		Hash = HashCombine(Hash, Item.second.GetHash(Process));
	}

	return Hash;
}

bool FTurinmaStructValue::Equals(const FTurinmaHeapValue& Other, class FTurinmaProcess* Process) const
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
			if(!OtherIter->second.Equals(Item.second, Process))
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
	S->NodeIndex = CreateInfo.NodeIndex;
	S->bIsPure = IsPure;
	return S;
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaGraphNodeTest)

TSharedPtr<struct FTurinmaGraphNodeBase> FTurinmaGraphInputNodeData::CreateNode(const FTurinmaNodeCreateInfo& CreateInfo)
{
	auto S = MakeShared<FTurinmaGraphInputNode>();
	S->DataIndex = CreateInfo.DataIndex;
	S->NodeIndex = CreateInfo.NodeIndex;
	S->bIsPure = IsPure;
	return S;
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaGraphInputNode)

bool FTurinmaGraphInputNode::Execute(const FTurinmaNodeExecuteParam& ExecuteParam)
{
	auto&& CallItem = ExecuteParam.Process->CallInfos[ExecuteParam.CurCallInfo].CallStack[ExecuteParam.CurCallItem];
	auto&& NodeItem = CallItem.LocalNodeIndex[ExecuteParam.MyIndex];
	NodeItem.NodeOutput = CallItem.GraphInputValue;
	if(!bIsPure)
	{
		NodeItem.WhichNextToGo = 0;
	}
	return true;
}

TSharedPtr<FTurinmaGraphNodeBase> FTurinmaGraphOutputNodeData::CreateNode(const FTurinmaNodeCreateInfo& CreateInfo)
{
	auto S = MakeShared<FTurinmaGraphOutputNode>();
	S->DataIndex = CreateInfo.DataIndex;
	S->NodeIndex = CreateInfo.NodeIndex;
	S->bIsPure = IsPure;
	return S;
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaGraphOutputNode)

bool FTurinmaGraphOutputNode::Execute(const FTurinmaNodeExecuteParam& ExecuteParam)
{
	auto&& CallItem = ExecuteParam.Process->CallInfos[ExecuteParam.CurCallInfo].CallStack[ExecuteParam.CurCallItem];
	auto&& NodeItem = CallItem.LocalNodeIndex[ExecuteParam.MyIndex];
	CallItem.GraphOutputValue = NodeItem.NodeInput;
	return true;
}

TSharedPtr<FTurinmaGraphNodeBase> FTurinmaCallGraphNodeData::CreateNode(const FTurinmaNodeCreateInfo& CreateInfo)
{
	auto S = MakeShared<FTurinmaCallGraphNode>();
	S->DataIndex = CreateInfo.DataIndex;
	S->NodeIndex = CreateInfo.NodeIndex;
	S->GraphName = GraphName;
	S->bIsPure = IsPure;
	return S;
}

TArray<FTurinmaGraphNodeParamDescInfo> FTurinmaCallGraphNodeData::GetInputParamDescs() const
{
	if(ProgramIn)
	{
		auto* Res = ProgramIn->NameToGraph.Find(GraphName);
		if(Res && ProgramIn->GraphDatas.IsValidIndex(*Res))
		{
			auto&& GraphData = ProgramIn->GraphDatas[*Res];
			if(auto* Node = GraphData.GetNode(GraphData.StartNodeIndex))// NodeDatas.IsValidIndex(GraphData.StartNodeIndex))
			{
				return Node->GetOutputParamDescs();
			}
		}
	}
	return {};
}

TArray<FTurinmaGraphNodeParamDescInfo> FTurinmaCallGraphNodeData::GetOutputParamDescs() const
{
	if (ProgramIn)
	{
		auto* Res = ProgramIn->NameToGraph.Find(GraphName);
		if (Res && ProgramIn->GraphDatas.IsValidIndex(*Res))
		{
			auto&& GraphData = ProgramIn->GraphDatas[*Res];
			if (auto* Node = GraphData.GetNode(GraphData.EndNodeIndex))//.NodeDatas.IsValidIndex(GraphData.EndNodeIndex))
			{
				return Node->GetInputParamDescs();
			}
		}
	}
	return {};
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaCallGraphNode)

bool FTurinmaCallGraphNode::Execute(const FTurinmaNodeExecuteParam& ExecuteParam)
{
	auto&& CallInfo = ExecuteParam.Process->CallInfos[ExecuteParam.CurCallInfo];
	auto&& CallItem = CallInfo.CallStack[ExecuteParam.CurCallItem];
	auto&& NodeItem = CallItem.LocalNodeIndex[ExecuteParam.MyIndex];
	auto NodeInput = NodeItem.NodeInput;

	auto* Res= ExecuteParam.Process->GraphNameToGraphIndex.Find(GraphName);

	if(Res)
	{
		if (!bIsPure)
		{
			NodeItem.WhichNextToGo = 0;
		}
		bool JunpSuc = ExecuteParam.Process->LongJmp(CallInfo, *Res);
		if(JunpSuc)
		{
			auto&& NewCallItem = CallInfo.CallStack.Last();
			NewCallItem.GraphInputValue = NodeInput;
			NewCallItem.JumpIntoNodeIndex = NodeIndex;

			return true;
		}
		return false;
	}
	return false;
}

TSharedPtr<FTurinmaGraphNodeBase> FTurinmaLexicalIntGraphNodeData::CreateNode(const FTurinmaNodeCreateInfo& CreateInfo)
{
	auto S = MakeShared<FTurinmaLexicalIntGraphNode>();
	S->DataIndex = CreateInfo.DataIndex;
	S->NodeIndex = CreateInfo.NodeIndex;
	S->LexicalInt = LexicalInt;
	S->bIsPure = IsPure;
	return S;
}

TArray<FTurinmaGraphNodeParamDescInfo> FTurinmaLexicalIntGraphNodeData::GetOutputParamDescs() const
{
	FTurinmaGraphNodeParamDescInfo Info;
	Info.ValueType = ETurinmaValueType::Int;
	Info.ParamName = TEXT("Value");
	return { Info };
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaLexicalIntGraphNode)

bool FTurinmaLexicalIntGraphNode::Execute(const FTurinmaNodeExecuteParam& ExecuteParam)
{
	auto&& CallInfo = ExecuteParam.Process->CallInfos[ExecuteParam.CurCallInfo];
	auto&& CallItem = CallInfo.CallStack[ExecuteParam.CurCallItem];
	auto&& NodeItem = CallItem.LocalNodeIndex[ExecuteParam.MyIndex];

	auto&& Ret = NodeItem.NodeOutput.AddDefaulted_GetRef();
	Ret.ValueType = ETurinmaValueType::Int;
	Ret.IntValue = LexicalInt;
	if (!bIsPure)
	{
		NodeItem.WhichNextToGo = 0;
	}
	return true;
}

TSharedPtr<FTurinmaGraphNodeBase> FTurinmaOutputLogGraphNodeData::CreateNode(const FTurinmaNodeCreateInfo& CreateInfo)
{
	auto S = MakeShared<FTurinmaOutputLogGraphNode>();
	S->DataIndex = CreateInfo.DataIndex;
	S->NodeIndex = CreateInfo.NodeIndex;
	S->bIsPure = IsPure;
	return S;
}

TArray<FTurinmaGraphNodeParamDescInfo> FTurinmaOutputLogGraphNodeData::GetInputParamDescs() const
{
	FTurinmaGraphNodeParamDescInfo Info;
	Info.ValueType = ETurinmaValueType::Nil;
	Info.ParamName = TEXT("Value");
	return { Info };
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaOutputLogGraphNode)

bool FTurinmaOutputLogGraphNode::Execute(const FTurinmaNodeExecuteParam& ExecuteParam)
{
	auto&& CallInfo = ExecuteParam.Process->CallInfos[ExecuteParam.CurCallInfo];
	auto&& CallItem = CallInfo.CallStack[ExecuteParam.CurCallItem];
	auto&& NodeItem = CallItem.LocalNodeIndex[ExecuteParam.MyIndex];

	if(NodeItem.NodeInput.Num() > 0)
	{
		FString LogString = NodeItem.NodeInput[0].ToLexicalString(ExecuteParam.Process);
		ExecuteParam.Process->Console.PushLog({  LogString});
		UE_LOG(LogTemp, Log, TEXT("%s"), *LogString);
	}
	if (!bIsPure)
	{
		NodeItem.WhichNextToGo = 0;
	}
	return true;
}

UE_DISABLE_OPTIMIZATION


struct HelloCoroutine {
	struct HelloPromise {
		HelloCoroutine get_return_object() {
			return std::coroutine_handle<HelloPromise>::from_promise(*this);
		}
		std::suspend_always initial_suspend() { return {}; }//协程创建之后，协程函数体执行之前的时候执行此函数，等同于co_await initial_suspend();这里返回suspend_always
																//表示创建调用函数创建协程对象后立马返回，不执行协程函数真正的函数体，知道外面调用resume
		std::suspend_never final_suspend() noexcept
		{
			UE_LOG(LogTemp, Log, TEXT("SWP :: Suspend"));
			return {};
		}//协程函数体执行全部完成之后执行此函数，等同于co_await final_suspend();

		// 协程函数要返回某种类型的值的话（即co_return XXX），
		// 需要在promise里声明void return_value(类型 value)函数。
		// 注意这里才是接受协程函数逻辑上的返回值的地方，协程函数的声名中总是返回promise对象
		/*void return_value(int value) {
			UE_LOG(LogTemp, Log, TEXT("SWP :: Return"));
		}*/

		// 协程函数不需要返回值的话（即co_return或不写等协程函数自然结束），需要在promise里声明void return_void()函数。
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


void FTurinmaGraphData::Init(UTurinmaProgram* Program)
{
	for(int32 NodeDataIndex = 0; NodeDataIndex < NodeDatas.Num(); ++NodeDataIndex)
	{
		auto&& Item = NodeDatas[NodeDataIndex];
		if(Item.NodeType && Item.NodeData)
		{
			Item.NodeData->ProgramIn = Program;
		}
	}
	StartNodeIndex = INDEX_NONE;
	EndNodeIndex = INDEX_NONE;
	InitInoutPut();
}

void FTurinmaGraphData::InitInoutPut()
{
	for (int32 NodeDataIndex = 0; NodeDataIndex < NodeDatas.Num(); ++NodeDataIndex)
	{
		auto&& Item = NodeDatas[NodeDataIndex];
		if (StartNodeIndex == INDEX_NONE && Item.NodeType == FTurinmaGraphInputNodeData::StaticStruct())
		{
			StartNodeIndex = NodeDataIndex;
		}
		if (EndNodeIndex == INDEX_NONE && Item.NodeType == FTurinmaGraphOutputNodeData::StaticStruct())
		{
			EndNodeIndex = NodeDataIndex;
		}
	}
}

bool FTurinmaGraphData::Serialize(FArchive& Ar)
{

	if (Ar.IsSaving())
	{
		FTurinmaGraphDataVersion CurV = FTurinmaGraphDataVersion::Last;
		Ar << CurV;

		Ar << GraphName;

		int32 NumOfNode = NodeDatas.Num();
		Ar << NumOfNode;
		for (int i = 0; i < NumOfNode; ++i)
		{
			NodeDatas[i].Serialize(Ar);
		}
	}
	if (Ar.IsLoading())
	{
		Ar << Version;

		Ar << GraphName;

		int32 NumOfNode = 0;
		Ar << NumOfNode;
		NodeDatas.AddDefaulted(NumOfNode);
		for (int i = 0; i < NumOfNode; ++i)
		{
			NodeDatas[i].Serialize(Ar);
		}
		StartNodeIndex = INDEX_NONE;
		EndNodeIndex = INDEX_NONE;
		InitInoutPut();
	}
	return true;
}

void FTurinmaGraphData::AddStructReferencedObjects(FReferenceCollector& Collector) const
{
	for (auto&& Item : NodeDatas)
	{
		const_cast<FTurinmaNodeDataItem&>(Item).AddReferencedObjects(Collector);
	}
}

bool FTurinmaGraph::InitWithDataAndInfo(const FTurinmaGraphData& InData, const FTurinmaGraphCreateInfo& InInfo)
{
	DataIndex = InInfo.DataIndex;
	GraphIndex = InInfo.GraphIndex;

	bool IterRes = InData.ForEachNode([this](int32 NodeDataIndx, const FTurinmaGraphNodeDataBase* Data)->bool
		{
			auto&& Item = *Data;
			//auto&& Item = InData.NodeDatas[NodeDataIndx];
			int32 NodeIndx = Nodes.Num();
			if (!Item.IsInputMatch())
			{
				return false;
			}
			if (Item.GetDataType() == FTurinmaGraphInputNodeData::StaticStruct())
			{
				if (StartNodeIndex == INDEX_NONE)
				{
					StartNodeIndex = NodeIndx;
				}
				else
				{
					return false; //no more than one input node
				}
			}
			Nodes.Add( const_cast<FTurinmaGraphNodeDataBase&>(Item).CreateNode({ NodeDataIndx , NodeIndx }));
			return true;
		});
	if(!IterRes)
	{
		Nodes.Empty();
		return false;
	}
	if(StartNodeIndex != INDEX_NONE)
	{
		return true;
	}
	else
	{
		Nodes.Empty();
		return false; //must have one input node
	}
}

void UTurinmaProgram::CopyFrom(UTurinmaProgram* Other)
{
	if(Other && Other != this)
	{
		GraphDatas = Other->GraphDatas;
		NameToGraph = Other->NameToGraph;
		RebuildNameToGraphIndex();
	}
}

void UTurinmaProgram::InitAllGraphDatas()
{
	for (int32 I = 0; I < GraphDatas.Num(); ++I)
	{
		auto&& Item = GraphDatas[I];
		Item.Init(this);
	}
}

UTurinmaProgram* UTurinmaProgram::GenerateTestTurinmaProgram()
{
	UTurinmaProgram* Ret = NewObject<UTurinmaProgram>();
	{
		auto&& Graph0 = Ret->GraphDatas.AddDefaulted_GetRef();
		Graph0.GraphName = TEXT("Graph0");

		int32 BeginNodeIndex = 0;
		auto&& Graph0Begin = Graph0.AddNode<FTurinmaGraphInputNodeData>(&BeginNodeIndex);//.NodeDatas.AddDefaulted_GetRef();
		auto&& Param = Graph0Begin.OutputParamDesc.AddDefaulted_GetRef();
		Param.ValueType = ETurinmaValueType::Int;
		Param.ParamName = TEXT("Test");

		int32 LogNodeIndex = 0;
		auto&& Graph0LogNode = Graph0.AddNode<FTurinmaOutputLogGraphNodeData>(&LogNodeIndex);//NodeDatas.AddDefaulted_GetRef();
		auto&& LogNodeParam = Graph0LogNode.InputParams.AddDefaulted_GetRef();
		LogNodeParam.ParamNode = BeginNodeIndex;
		LogNodeParam.ParamPin = 0;
		

		Graph0Begin.NextNodes.AddDefaulted_GetRef().NextNode = LogNodeIndex;

	}
	{
		auto&& GraphMain = Ret->GraphDatas.AddDefaulted_GetRef();
		GraphMain.GraphName = TEXT("Main");

		auto&& GraphMainBegin = GraphMain.AddNode<FTurinmaGraphInputNodeData>();// NodeDatas.AddDefaulted_GetRef();
		GraphMainBegin.Location = FVector2D(-400,0);

		int32 CallNodeIndex = 0;
		auto&& GraphMainCallGraph0Node = GraphMain.AddNode<FTurinmaCallGraphNodeData>(&CallNodeIndex);// NodeDatas.AddDefaulted_GetRef();
		GraphMainCallGraph0Node.GraphName = TEXT("Graph0");
		GraphMainCallGraph0Node.Location = FVector2D(400, 400);

		int32 LexicalIntIndex = 0;
		auto&& GraphMainLexicalIntNode = GraphMain.AddNode<FTurinmaLexicalIntGraphNodeData>(&LexicalIntIndex);// .NodeDatas.AddDefaulted_GetRef();
		GraphMainLexicalIntNode.LexicalInt = 100;
		GraphMainLexicalIntNode.IsPure = true;
		GraphMainLexicalIntNode.Location = FVector2D(0, -200);


		auto&& LinkToCallGraph0 = GraphMainBegin.NextNodes.AddDefaulted_GetRef();
		LinkToCallGraph0.NextNode = CallNodeIndex;

		auto&& ParamLinkToCall = GraphMainCallGraph0Node.InputParams.AddDefaulted_GetRef();
		ParamLinkToCall.ParamNode = LexicalIntIndex;
		ParamLinkToCall.ParamPin = 0;
	}
	
	
	Ret->RebuildNameToGraphIndex();
	return Ret;
}

class FTestTurinmaProcessRun : public FTickableEditorObject
{
public:
	FTurinmaProcess Process;
	static FTestTurinmaProcessRun* It;

	virtual void Tick(float DeltaTime) override
	{
		Process.Tick();
	}
	virtual TStatId GetStatId() const override
	{
		return TStatId();
	}
};
FTestTurinmaProcessRun* FTestTurinmaProcessRun::It = nullptr;

void UTurinmaProgram::TestRun(UTurinmaProgram* InProgram)
{
	TestStop();
	FTestTurinmaProcessRun::It = new FTestTurinmaProcessRun();
	FTestTurinmaProcessRun::It->Process.Program = InProgram;
	FTestTurinmaProcessRun::It->Process.Start();
}

void UTurinmaProgram::TestStop()
{
	if (FTestTurinmaProcessRun::It)
	{
		FTestTurinmaProcessRun::It->Process.Stop();
		delete FTestTurinmaProcessRun::It;

	}
}


void UTurinmaProgram::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
	RebuildNameToGraphIndex();
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

std::suspend_always FTurinmaCoroutine::FTurinmaPromise::final_suspend() noexcept
{
	UE_LOG(LogTemp, Log, TEXT("SWP :: Suspend"));
	Process->bHasFinish = true;
	Process = nullptr;
	return {};
}

bool FTurinmaProcess::InitProcessByProgram()
{
	if(!Program.IsValid() || Program->GraphDatas.Num() == 0)
	{
		return false;
	}

	for(int32 DataIndex = 0; DataIndex < Program->GraphDatas.Num(); ++DataIndex)
	{
		FTurinmaGraphCreateInfo GraphCreateInfo;
		GraphCreateInfo.GraphIndex = Graphs.Num();
		GraphCreateInfo.DataIndex = DataIndex;

		auto* TurinmaGraph = new FTurinmaGraph();
		if(!TurinmaGraph->InitWithDataAndInfo(Program->GraphDatas[DataIndex], GraphCreateInfo))
		{
			Graphs.Empty();
			return false;
		}
		Graphs.Add(TurinmaGraph);
		if(GraphNameToGraphIndex.Find(Program->GraphDatas[DataIndex].GraphName))
		{
			Graphs.Empty();
			return false;
		}
		GraphNameToGraphIndex.Add(Program->GraphDatas[DataIndex].GraphName, GraphCreateInfo.GraphIndex);
	}

	return true;
}

bool FTurinmaProcess::Start()
{
	if(bHasStart)
	{
		return false;
	}
	if(InitProcessByProgram())
	{
		int* Entry = GraphNameToGraphIndex.Find(EntryGraphName);
		if(Entry)
		{
			check(CallInfos.Num() == 0);
			auto* CallInfo = new FTurinmaProcessCallInfo();
			CallInfos.Add(CallInfo);
			auto&& CallStack = CallInfo->CallStack.AddDefaulted_GetRef();
			CallStack.GraphIndex = *Entry;
			auto&& IndexItem = CallStack.LocalNodeIndex.AddDefaulted_GetRef();
			IndexItem.NodeIndex = Graphs[*Entry].StartNodeIndex;
			check(IndexItem.NodeIndex > INDEX_NONE)
			CurCallInfo = 0;

			Coroutine = Execute();
			Coroutine.handle.promise().Process = this;
			bHasStart = true;
			Coroutine.handle.resume();
			return true;
		}
	}
	return false;
}

bool FTurinmaProcess::Stop()
{
	if(bHasFinish)
	{
		return false;
	}
	if(bHasStart)
	{
		bShouldExit = true;
		Coroutine.handle.resume();
		check(Coroutine.handle.done());
		Coroutine.handle.destroy();
		CallInfos.Empty();
	}
	return false;
}

void FTurinmaProcess::Tick()
{
	if(IsRunning())
	{
		Coroutine.handle.resume();
	}
}

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
				if(auto* Node = Program->GraphDatas[Graphs[GraphIndex].DataIndex].GetNode(Graphs[GraphIndex].Nodes[NodeIndex]->DataIndex))
				{
					return !!Node;
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
		return Program->GraphDatas[Graphs[GraphIndex].DataIndex].GetNode(Graphs[GraphIndex].Nodes[NodeIndex]->DataIndex)->IsPure;
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

bool FTurinmaProcess::LongJmp(FTurinmaProcessCallInfo& CallInfo, int32 GraphIndex)
{
	if(IsGraphValid(GraphIndex))
	{
		auto&& CallItem = CallInfo.CallStack.AddDefaulted_GetRef();
		CallItem.GraphIndex = GraphIndex;
		if(Graphs[GraphIndex].StartNodeIndex != INDEX_NONE)
		{
			return LocalJmp(CallItem, Graphs[GraphIndex].StartNodeIndex);
		}
		return false;
	}
	return false;
}

bool FTurinmaProcess::Return(int32 InCurCallInfo)
{
	if(CallInfos.IsValidIndex(InCurCallInfo))
	{
		auto&& CallInfo = CallInfos[InCurCallInfo];
		if(CallInfo.CallStack.Num() > 0)
		{
			int32 JumpIntoNodeIndex = CallInfo.CallStack.Last().JumpIntoNodeIndex;
			auto GraphOutput = CallInfo.CallStack.Last().GraphOutputValue;

			CallInfo.CallStack.Pop();
			if(CallInfo.CallStack.Num() > 0)
			{
				CallInfo.CallStack.Last().TempLocalVariables.FindOrAdd(JumpIntoNodeIndex) = GraphOutput;
			}
			if(CallInfo.CallStack.Num() == 0)
			{
				UnusedCallInfoIndex.Add(InCurCallInfo);
			}
			return true;
		}
	}
	
	return true;
}

void FTurinmaProcess::RecordError(const FTurinmaErrorContent& InErrorContent)
{
	ErrorInfo.bError = true;
	ErrorInfo.Content = InErrorContent;
}

FTurinmaCoroutine FTurinmaProcess::Execute()
{
	int32 CurLeftExecuteNum = MaxNumExecutePerTick;
	int32 CurLeftCountBeforeGC = MaxGCProcessCount;
	while (!bShouldExit && !ErrorInfo.bError && Program.IsValid())
	{
		if (CallInfos.IsValidIndex(CurCallInfo) && CallInfos[CurCallInfo].CallStack.Num() > 0)
		{
			int32 LocalCurCallInfo = CurCallInfo;
			auto&& CallInfo = CallInfos[LocalCurCallInfo];
			auto&& CallItem = CallInfo.CallStack.Last();
			if (CallItem.LocalNodeIndex.Num() > 0)
			{
				auto&& NodeItem = CallItem.LocalNodeIndex.Last();

				auto&& Node = Graphs[CallItem.GraphIndex].Nodes[NodeItem.NodeIndex];
				auto&& NodeData = *Program->GraphDatas[Graphs[CallItem.GraphIndex].DataIndex].GetNode(NodeItem.NodeIndex);

				{
					switch (NodeItem.Status)
					{
					case FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::None:
						NodeItem.Status = FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::PeekingParams;
						[[fallthrough]];
					case FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::PeekingParams:
					{
						bool bNeedFutureExecuteOrError = false;
						while (NodeItem.NodeInput.Num() < NodeData.InputParams.Num())
						{
							auto&& Param = NodeData.InputParams[NodeItem.NodeInput.Num()];
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
						Param.CurCallInfo = LocalCurCallInfo;
						Param.CurCallItem = CallInfo.CallStack.Num() - 1;
						Param.MyIndex = CallItem.LocalNodeIndex.Num() - 1;
						int32 NumOfStack = CallInfo.CallStack.Num();
						int32 NumOfCallNode = CallItem.LocalNodeIndex.Num();
						if (!Node->Execute(Param))
						{
							RecordError(FTurinmaErrorContent());
							break;
						}
						else
						{
							NodeItem.Status = FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::PushingResult;
							if (NumOfStack != CallInfo.CallStack.Num() || NumOfCallNode != CallItem.LocalNodeIndex.Num()) //jump happened 
							{
								break;
							}
						}
					};
					[[fallthrough]];
					case FTurinmaProcessCallInfoItem::FLocalNodeIndex::ELocalNodeIndexStatus::PushingResult:
					{
						auto&& V = CallItem.TempLocalVariables.FindOrAdd(NodeItem.NodeIndex);
						auto&& OutputParams = NodeData.GetOutputParamDescs();
						V.Reset(OutputParams.Num());
						if (NodeItem.NodeOutput.Num() != OutputParams.Num())
						{
							RecordError(FTurinmaErrorContent());
							break;
						}
						bool bAnyError = false;
						for (int OutI = 0; OutI < NodeItem.NodeOutput.Num(); ++OutI)
						{
							if (NodeItem.NodeOutput[OutI].ValueType != ETurinmaValueType::Nil
								&& OutputParams[OutI].ValueType != ETurinmaValueType::Nil
								&& NodeItem.NodeOutput[OutI].ValueType != OutputParams[OutI].ValueType)
							{
								bAnyError = true;
								RecordError(FTurinmaErrorContent());
								break;
							}
							V.Add(NodeItem.NodeOutput[OutI]);
						}
						if (bAnyError)
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
						if (NodeItem.WhichNextToGo != INDEX_NONE)
						{
							if (NodeData.NextNodes.IsValidIndex(NodeItem.WhichNextToGo))
							{
								auto NextNode = NodeData.NextNodes[NodeItem.WhichNextToGo];
								if (IsNodeValid(CallItem.GraphIndex, NextNode.NextNode))
								{
									if (IsNodePure(CallItem.GraphIndex, NextNode.NextNode))
									{
										RecordError(FTurinmaErrorContent());
										break;
									}
									CallItem.LocalNodeIndex.RemoveAt(CallItem.LocalNodeIndex.Num() - 1);
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
								CallItem.LocalNodeIndex.RemoveAt(CallItem.LocalNodeIndex.Num() - 1);
								if (CallItem.LocalNodeIndex.Num() == 0)
								{
									Return(LocalCurCallInfo);
								}
							}
						}
						else
						{
							CallItem.LocalNodeIndex.RemoveAt(CallItem.LocalNodeIndex.Num() - 1);
							if (CallItem.LocalNodeIndex.Num() == 0)
							{
								Return(LocalCurCallInfo);
							}
						}
					};
					break;
					default:;
					}
				}
			}
		}
		else
		{
			bool bFound = false;
			while(CurCallInfoStack.Num() > 0)
			{
				int32 LastCurCallInfoIndex = CurCallInfoStack.Pop();
				if(CallInfos.IsValidIndex(LastCurCallInfoIndex) && CallInfos[LastCurCallInfoIndex].CallStack.Num() > 0)
				{
					bFound = true;
					CurCallInfo = LastCurCallInfoIndex;
					break;
				}
			}
			if(!bFound)
			{
				for(int CI = 0; CI < CallInfos.Num(); ++CI)
				{
					if(CallInfos[CI].CallStack.Num() > 0)
					{
						bFound = true;
						CurCallInfo = CI;
						break;
					}
				}
			}
			if(!bFound)
			{
				CurCallInfo = INDEX_NONE;
			}

		}
		--CurLeftExecuteNum;
		if(CurLeftExecuteNum <= 0)
		{
			--CurLeftCountBeforeGC;
			if(CurLeftCountBeforeGC <= 0)
			{
				CurLeftCountBeforeGC = MaxGCProcessCount;

				auto&& DoGC = [&]()->bool
					{
						for(auto&& HeapValue : Heap.TurinmaHeapValues)
						{
							if(HeapValue)
							{
								HeapValue->bReached = false;
							}
						}
						//todo do gc
						for (auto&& Item : Globals.TurinmaGlobals)
						{
							if(Item.Value.ValueType > ETurinmaValueType::EndOfSimpleValue)
							{
								auto* HeapValue = Item.Value.HeapValue(this);
								if (!HeapValue && Heap.TurinmaHeapValues.IsValidIndex(Item.Value.HeapValueIndex))
								{
									return false;
								}
								if(HeapValue)
								{
									HeapValue->bReached = true;
								}
							}
							
						}
						for (auto&& CallInfo : CallInfos)
						{
							for (auto&& CallItem : CallInfo.CallStack)
							{
								for (auto&& Item : CallItem.GraphInputValue)
								{
									auto* HeapValue = Item.HeapValue(this);
									if (!HeapValue && Heap.TurinmaHeapValues.IsValidIndex(Item.HeapValueIndex))
									{
										return false;
									}
									if (HeapValue)
									{
										HeapValue->bReached = true;
									}
								}
								for (auto&& Item : CallItem.GraphOutputValue)
								{
									auto* HeapValue = Item.HeapValue(this);
									if (!HeapValue && Heap.TurinmaHeapValues.IsValidIndex(Item.HeapValueIndex))
									{
										return false;
									}
									if (HeapValue)
									{
										HeapValue->bReached = true;
									}
								}
								for (auto&& Item : CallItem.LocalVariables)
								{
									auto* HeapValue = Item.Value.HeapValue(this);
									if (!HeapValue && Heap.TurinmaHeapValues.IsValidIndex(Item.Value.HeapValueIndex))
									{
										return false;
									}
									if (HeapValue)
									{
										HeapValue->bReached = true;
									}
								}
								for (auto&& TempVar : CallItem.TempLocalVariables)
								{
									for (auto&& Item : TempVar.Value)
									{
										auto* HeapValue = Item.HeapValue(this);
										if (!HeapValue && Heap.TurinmaHeapValues.IsValidIndex(Item.HeapValueIndex))
										{
											return false;
										}
										if (HeapValue)
										{
											HeapValue->bReached = true;
										}
									}
								}
								for (auto&& LocalNode : CallItem.LocalNodeIndex)
								{
									for (auto&& Item : LocalNode.NodeInput)
									{
										auto* HeapValue = Item.HeapValue(this);
										if (!HeapValue && Heap.TurinmaHeapValues.IsValidIndex(Item.HeapValueIndex))
										{
											return false;
										}
										if (HeapValue)
										{
											HeapValue->bReached = true;
										}
									}
									for (auto&& Item : LocalNode.NodeOutput)
									{
										auto* HeapValue = Item.HeapValue(this);
										if (!HeapValue && Heap.TurinmaHeapValues.IsValidIndex(Item.HeapValueIndex))
										{
											return false;
										}
										if (HeapValue)
										{
											HeapValue->bReached = true;
										}
									}
								}
							}
						}
						for(int32 HI = 0; HI < Heap.TurinmaHeapValues.Num(); ++HI)
						{
							auto&& HeapValue = Heap.TurinmaHeapValues[HI];
							if (HeapValue && !HeapValue->bReached)
							{
								HeapValue.Reset();
								Heap.UnusedIndex.Add(HI);
							}
						}
						return true;
					};

				if(!DoGC())
				{
					RecordError({});
				}
			}
			CurLeftExecuteNum = MaxNumExecutePerTick;
			co_await std::suspend_always{};//todo coroutine not finish yet
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
