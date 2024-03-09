#include "TurinmaProgram.h"

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

DEFINE_TURINMA_GRAPH_NODE(FTurinmaGraphNodeTest)