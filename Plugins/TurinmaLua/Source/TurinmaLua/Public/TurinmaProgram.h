#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TurinmaProgram.generated.h"

enum class ETurinmaValueType : uint8
{
	Nil = 0,
	Bool,
	Int,
	Real,
	Vector,
	Quat,
	Matrix,
	Transform,
	String,

	EndOfSimpleValue,

	Array,
	Table,
	Function
};

struct FTurinmaValue
{
	union
	{
		bool BooleanValue;
		int64 IntValue = 0;
		double RealValue;
	};
	FVector VectorValue;
	FQuat QuatValue;
	FMatrix MatrixValue;
	FTransform TransformValue;
	FString StringValue;

	TSharedPtr<struct FTurinmaHeapValue> HeapValue;

	ETurinmaValueType ValueType = ETurinmaValueType::Nil;

	uint32 GetHash() const;
	bool Equals(const FTurinmaValue& Other) const;

	bool operator==(const FTurinmaValue& Other) const
	{
		return Equals(Other);
	}

};

inline uint32 GetTypeHash(const FTurinmaValue& Id)
{
	return Id.GetHash();
}

enum class EHeapValueKind
{
	None,
	Function,
	Array,
	Table
};

struct FTurinmaHeapValue : TSharedFromThis<FTurinmaHeapValue>
{
	bool bReached = false;

	template<typename T>
	T* GetTyped()
	{
		static_assert(std::is_base_of<FTurinmaHeapValue, T>::value, "");
		if(T::StaticHeapValueKind() == GetHeapValueKind())
		{
			return static_cast<T*>(this);
		}
		return nullptr;
	}

	template<typename T>
	const T* GetTyped() const
	{
		static_assert(std::is_base_of<FTurinmaHeapValue, T>::value, "");
		if (T::StaticHeapValueKind() == GetHeapValueKind())
		{
			return static_cast<const T*>(this);
		}
		return nullptr;
	}

	virtual EHeapValueKind GetHeapValueKind() const { return EHeapValueKind::None; }

	static EHeapValueKind StaticHeapValueKind() { return EHeapValueKind::None; }

	virtual uint32 GetHash() const { return 0; }
	virtual bool Equals(const FTurinmaHeapValue& Other) const { return false; }

	virtual ~FTurinmaHeapValue() = default;
};

struct FTurinmaFunctionValue : FTurinmaHeapValue
{

	virtual EHeapValueKind GetHeapValueKind() const override { return EHeapValueKind::Function; }

	static EHeapValueKind StaticHeapValueKind() { return EHeapValueKind::Function; }

	virtual uint32 GetHash() const override { return GetTypeHash(this); }
	virtual bool Equals(const FTurinmaHeapValue& Other) const override { return this == &Other; }
};


struct FTurinmaArrayValue : FTurinmaHeapValue
{
	TArray<FTurinmaValue> Values;

	virtual EHeapValueKind GetHeapValueKind() const override { return EHeapValueKind::Array; }

	static EHeapValueKind StaticHeapValueKind() { return EHeapValueKind::Array; }

	virtual uint32 GetHash() const override;
	virtual bool Equals(const FTurinmaHeapValue& Other) const override;
};

struct FTurinmaTableValue : FTurinmaHeapValue
{

	TMap<FTurinmaValue, FTurinmaValue> InnerMap;

	virtual EHeapValueKind GetHeapValueKind() const override { return EHeapValueKind::Table; }

	static EHeapValueKind StaticHeapValueKind() { return EHeapValueKind::Table; }

	virtual uint32 GetHash() const override;
	virtual bool Equals(const FTurinmaHeapValue& Other) const override;
};




#define DECLARE_TURINMA_GRAPH_NODE(NodeType)\
	typedef NodeType MyNodeType;\
	FORCENOINLINE static int64 StaticTypeId();\
	virtual int64 GetTypeId() const override\
	{\
		return StaticTypeId();\
	}\
	virtual FName GetNodeTypeName() const override\
	{\
		return TEXT(#NodeType);\
	}

#define DEFINE_TURINMA_GRAPH_NODE(NodeType)\
	int64 NodeType::StaticTypeId()\
	{\
		static int64 Inner = FTurinmaGraphNodeBase::GraphNodeTypeId();\
		return Inner;\
	}

struct TURINMALUA_API FTurinmaGraphNodeBase
{
protected:
	FORCENOINLINE static int64 GraphNodeTypeId();
public:

	FVector2D Position;

public:
	template<typename T>
	T* GetTyped()
	{
		static_assert(std::is_base_of<FTurinmaGraphNodeBase, T>::value, "");
		static_assert(std::is_same<T, typename T::MyNodeType>::value, "");

		if(T::StaticTypeId() == GetTypeId())
		{
			return static_cast<T*>(this);
		}
		return nullptr;
	}

	template<typename T>
	const T* GetTyped() const
	{
		static_assert(std::is_base_of<FTurinmaGraphNodeBase, T>::value, "");
		static_assert(std::is_same<T, typename T::MyNodeType>::value, "");

		if (T::StaticTypeId() == GetTypeId())
		{
			return static_cast<const T*>(this);
		}
		return nullptr;
	}

	

	FORCENOINLINE static int64 StaticTypeId();

	virtual int64 GetTypeId() const
	{
		return StaticTypeId();
	}

	virtual FName GetNodeTypeName() const
	{
		return TEXT("FTurinmaGraphNodeBase");
	}
};

struct FTurinmaGraphNodeTest : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaGraphNodeTest)
};




UCLASS(BlueprintType)
class TURINMALUA_API UTurinmaProgram : public UDataAsset
{
	GENERATED_BODY()




};
