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

FTurinmaGraphNodeBase* FTurinmaGraphNodeDataTest::CreateNode()
{
	return new FTurinmaGraphNodeTest();
}

DEFINE_TURINMA_GRAPH_NODE(FTurinmaGraphNodeTest)

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
