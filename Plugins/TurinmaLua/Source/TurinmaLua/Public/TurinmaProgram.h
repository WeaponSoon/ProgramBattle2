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



#define DECLARE_TURINMA_GRAPH_NODE_DATA(DataType)\
	virtual UStruct* GetDataType() const override\
	{\
		return StaticStruct();\
	}\
	virtual void* GetThisPtr() const override\
	{\
		return (void*)this;\
	}

USTRUCT(BlueprintTYpe)
struct TURINMALUA_API FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere)
	FVector2D Location;
	UPROPERTY(VisibleAnywhere)
	TArray<int32> NextNodes;

	template<typename T>
	T* GetTyped()
	{
		static_assert(std::is_base_of<FTurinmaGraphNodeDataBase, T>::value, "");

		if(GetDataType() == T::StaticStruct() || GetDataType()->IsChildOf(T::StaticStruct()))
		{
			return static_cast<T*>(this);
		}
		return nullptr;
	}

	template<typename T>
	const T* GetTyped() const
	{
		static_assert(std::is_base_of<FTurinmaGraphNodeDataBase, T>::value, "");

		if (GetDataType() == T::StaticStruct() || GetDataType()->IsChildOf(T::StaticStruct()))
		{
			return static_cast<const T*>(this);
		}
		return nullptr;
	}

	virtual UStruct* GetDataType() const
	{
		return StaticStruct();
	}

	virtual void* GetThisPtr() const
	{
		return (void*)this;
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector)
	{
		Collector.AddPropertyReferences(GetDataType(), GetThisPtr());
	}

	virtual struct FTurinmaGraphNodeBase* CreateNode() { return nullptr; }

	virtual ~FTurinmaGraphNodeDataBase() = default;
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


struct TURINMALUA_API FTurinmaGraphNodeBase : TSharedFromThis<FTurinmaGraphNodeBase>
{
protected:
	FORCENOINLINE static int64 GraphNodeTypeId();
public:
	FVector2D Position;
	TWeakPtr<FTurinmaGraphNodeBase> Next;
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


	virtual ~FTurinmaGraphNodeBase() = default;
};


USTRUCT()
struct FTurinmaGraphNodeDataTest : public FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()

	DECLARE_TURINMA_GRAPH_NODE_DATA(FTurinmaGraphNodeDataTest)

	virtual FTurinmaGraphNodeBase* CreateNode() override;
	
};

struct FTurinmaGraphNodeTest : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaGraphNodeTest)
};


USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphData
{
	GENERATED_BODY()

		enum class FTurinmaGraphDataVersion : uint16
	{
		First,

		Max,
		Last = Max - 1
	};

	struct FTurinmaNodeDataItem
	{

		enum class FTurinmaNodeDataItemVersion : uint16
		{
			First,

			Max,
			Last = Max - 1
		};

		FTurinmaNodeDataItemVersion Version = FTurinmaNodeDataItemVersion::Last;
		FName NodeName;
		UScriptStruct* NodeType = nullptr;
		FTurinmaGraphNodeDataBase* NodeData = nullptr;

		void AddReferencedObjects(FReferenceCollector& Collector)
		{
			check(NodeType == NodeData->GetDataType());
			Collector.AddReferencedObject(NodeType);
			Collector.AddPropertyReferences(NodeType, NodeData->GetThisPtr());
		}

		~FTurinmaNodeDataItem()
		{
			NodeType->DestroyStruct(NodeData->GetThisPtr());
			FMemory::Free(NodeData);
		}

		void Serialize(FArchive& Ar)
		{
			if(Ar.IsSaving())
			{
				FTurinmaNodeDataItemVersion CurV = FTurinmaNodeDataItemVersion::Last;
				Ar << CurV;

				Ar << NodeName;
				Ar << NodeType;
				if(NodeType)
				{
					void* Default = FMemory::Malloc(NodeType->GetStructureSize());
					NodeType->InitializeStruct(Default);
					NodeType->SerializeItem(Ar, NodeData, Default);
					NodeType->DestroyStruct(Default);
					FMemory::Free(Default);
				}
			}
			if(Ar.IsLoading())
			{
				Ar << Version;

				Ar << NodeName;
				Ar << NodeType;
				if (NodeType)
				{
					void* Value = FMemory::Malloc(NodeType->GetStructureSize());
					NodeType->InitializeStruct(Value);
					NodeType->SerializeItem(Ar, Value, nullptr);
					NodeData = static_cast<FTurinmaGraphNodeDataBase*>(Value);
				}
			}
		}
	};

	FTurinmaGraphDataVersion Version = FTurinmaGraphDataVersion::Last;

	TArray<FTurinmaNodeDataItem> NodeDatas;

	bool Serialize(FArchive& Ar)
	{
		
		if(Ar.IsSaving())
		{
			FTurinmaGraphDataVersion CurV = FTurinmaGraphDataVersion::Last;
			Ar << CurV;

			int32 NumOfNode = NodeDatas.Num();
			Ar << NumOfNode;
			for(int i = 0; i < NumOfNode; ++i)
			{
				NodeDatas[i].Serialize(Ar);
			}
		}
		if(Ar.IsLoading())
		{
			Ar << Version;

			int32 NumOfNode = 0;
			Ar << NumOfNode;
			NodeDatas.AddDefaulted(NumOfNode);
			for (int i = 0; i < NumOfNode; ++i)
			{
				NodeDatas[i].Serialize(Ar);
			}
		}
		return true;
	}

	void AddReferencedObjects(FReferenceCollector& Collector)
	{
		for(auto&& Item : NodeDatas)
		{
			Item.AddReferencedObjects(Collector);
		}
	}
};


template<> struct TStructOpsTypeTraits<FTurinmaGraphData> : public TStructOpsTypeTraitsBase2<FTurinmaGraphData>
{
	enum { WithSerializer = true };
};


struct FTurinmaGraph
{
	
};



UCLASS(BlueprintType)
class TURINMALUA_API UTurinmaProgram : public UDataAsset
{
	GENERATED_BODY()




};


struct FTestClass
{
	FTestClass();

	static FTestClass TestFFFFF;
};
