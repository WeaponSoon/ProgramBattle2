#pragma once

#include <coroutine>
#include <map>
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
	Function,
	Struct
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
	Table,
	Struct
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

struct FNameFastCompaire
{
	bool operator()(const FName& A, const FName& B) const noexcept
	{
		return A.FastLess(B);
	}
};


struct FNameStableCompaire
{
	bool operator()(const FName& A, const FName& B) const noexcept
	{
		return A.LexicalLess(B);
	}
};

struct FTurinmaStructValue : FTurinmaHeapValue
{
	FName ProtoName;

	std::map<FName, FTurinmaValue, FNameFastCompaire> Fields;

	virtual EHeapValueKind GetHeapValueKind() const override { return EHeapValueKind::Struct; }

	static EHeapValueKind StaticHeapValueKind() { return EHeapValueKind::Struct; }

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

USTRUCT(BlueprintType)
struct FTurinmaGraphNodeLinkInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	int32 NextNode = -1;
};

USTRUCT(BlueprintType)
struct FTurinmaGraphNodeParamLinkInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	int32 ParamNode = -1;

	UPROPERTY(VisibleAnywhere)
	int32 ParamPin = -1;
};

USTRUCT()
struct FTurinmaGraphNodeParamDescInfo
{
	GENERATED_BODY()

	enum class FTurinmaGraphParamDescVersion : uint16
	{
		First,

		Max,
		Last = Max - 1
	};
	FTurinmaGraphParamDescVersion Version;
	ETurinmaValueType ValueType;
	FName ExtraTypeInfo;


	bool Serialize(FArchive& Ar)
	{
		if (Ar.IsSaving())
		{
			FTurinmaGraphParamDescVersion CurV = FTurinmaGraphParamDescVersion::Last;
			Ar << CurV;

			Ar << ValueType;
			Ar << ExtraTypeInfo;
		}
		if (Ar.IsLoading())
		{
			Ar << Version;

			Ar << ValueType;
			Ar << ExtraTypeInfo;
		}
		return true;
	}
};


template<> struct TStructOpsTypeTraits<FTurinmaGraphNodeParamDescInfo> : public TStructOpsTypeTraitsBase2<FTurinmaGraphNodeParamDescInfo>
{
	enum { WithSerializer = true, };
};


struct FTurinmaNodeCreateInfo
{
	int32 DataIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere)
	bool IsPure = false;
	UPROPERTY(EditAnywhere)
	FVector2D Location;
	UPROPERTY(VisibleAnywhere)
	TArray<FTurinmaGraphNodeLinkInfo> NextNodes;
	UPROPERTY(VisibleAnywhere)
	TArray<FTurinmaGraphNodeParamLinkInfo> InputParams;
	UPROPERTY(VisibleAnywhere)
	TArray<FTurinmaGraphNodeParamDescInfo> OutputParams;



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

	virtual TSharedPtr<struct FTurinmaGraphNodeBase> CreateNode(const FTurinmaNodeCreateInfo& CreatInfo) { return nullptr; }

	virtual FName GetNodeName() const { return NAME_None; }

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


struct FTurinmaNodeExecuteParam
{
	class FTurinmaProcess* Process = nullptr;
	int32 CurCallInfo = INDEX_NONE;
	int32 CurCallItem = INDEX_NONE;
	int32 MyIndex = INDEX_NONE;
};

struct TURINMALUA_API FTurinmaGraphNodeBase : TSharedFromThis<FTurinmaGraphNodeBase>
{
protected:
	FORCENOINLINE static int64 GraphNodeTypeId();
public:

	int32 DataIndex = INDEX_NONE;

	
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

	virtual bool Execute(const FTurinmaNodeExecuteParam& ExecuteParam) { return true; }

	virtual ~FTurinmaGraphNodeBase() = default;
};


USTRUCT()
struct FTurinmaGraphNodeDataTest : public FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()

	DECLARE_TURINMA_GRAPH_NODE_DATA(FTurinmaGraphNodeDataTest)

	virtual TSharedPtr<struct FTurinmaGraphNodeBase> CreateNode(const FTurinmaNodeCreateInfo& CreateInfo) override;
	
};

struct FTurinmaGraphNodeTest : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaGraphNodeTest)
};


USTRUCT()
struct FTurinmaGraphInputNodeData : public FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()

	DECLARE_TURINMA_GRAPH_NODE_DATA(FTurinmaGraphInputNodeData)

	virtual TSharedPtr<struct FTurinmaGraphNodeBase> CreateNode(const FTurinmaNodeCreateInfo& CreateInfo) override;
};


struct FTurinmaGraphInputNode : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaGraphInputNode)
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

	void AddStructReferencedObjects(class FReferenceCollector& Collector)
	{
		for (auto&& Item : NodeDatas)
		{
			Item.AddReferencedObjects(Collector);
		}
	}
};


template<> struct TStructOpsTypeTraits<FTurinmaGraphData> : public TStructOpsTypeTraitsBase2<FTurinmaGraphData>
{
	enum { WithSerializer = true, WithAddStructReferencedObjects = true };
};

struct FTurinmaGraphCreateInfo
{
	int32 DataIndex = INDEX_NONE;
};

struct FTurinmaGraph
{
	int32 DataIndex = INDEX_NONE;

	TArray<TSharedPtr<FTurinmaGraphNodeBase>> Nodes;

	


	void InitWithDataAndInfo(const FTurinmaGraphData& InData, const FTurinmaGraphCreateInfo& InInfo);
};



UCLASS(BlueprintType)
class TURINMALUA_API UTurinmaProgram : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FTurinmaGraphData> GraphDatas;

};

struct FTurinmaProcessHeap
{
	TArray<TSharedPtr<FTurinmaHeapValue>> TurinmaHeap;
};
struct FTurinmaProcessGlobal
{
	TMap<FName, TSharedPtr<FTurinmaHeapValue>>  TurinmaGlobals;
};

struct FTurinmaProcessCallInfoItem
{
	struct FLocalNodeIndex
	{
		enum class ELocalNodeIndexStatus
		{
			None,
			PeekingParams,
			Executing,
			PushingResult,
			Finish
		};
		ELocalNodeIndexStatus Status = ELocalNodeIndexStatus::None;
		int32 NodeIndex = INDEX_NONE;
		TArray<FTurinmaValue> NodeInput;
		TArray<FTurinmaValue> NodeOutput;
		int32 WhichNextToGo = INDEX_NONE;
	};
	int32 GraphIndex = INDEX_NONE;
	//FTurinmaGraph* Graph = nullptr;
	TArray<FLocalNodeIndex> LocalNodeIndex;
	TMap<FName, FTurinmaValue> LocalVariables;
	TMap<int32, TArray<FTurinmaValue>> TempLocalVariables;


};
//a call info mean a thread
struct FTurinmaProcessCallInfo
{
	TArray<FTurinmaProcessCallInfoItem> CallStack;
};

struct FTurinmaErrorContent
{
	
};
struct FTurinmaErrorInfo
{
	bool bError = false;
	FTurinmaErrorContent Content;
};



struct FTurinmaCoroutine {
	struct FTurinmaPromise {
		FTurinmaCoroutine get_return_object() {
			return std::coroutine_handle<FTurinmaPromise>::from_promise(*this);
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

	using promise_type = FTurinmaPromise;
	FTurinmaCoroutine(std::coroutine_handle<FTurinmaPromise> h) : handle(h) {}

	std::coroutine_handle<FTurinmaPromise> handle;
};




class TURINMALUA_API FTurinmaProcess
{
	TWeakObjectPtr<UTurinmaProgram> Program;
	FTurinmaProcessHeap Heap;
	FTurinmaProcessGlobal Globals;
	TIndirectArray<FTurinmaGraph> Graphs;
	TIndirectArray<FTurinmaProcessCallInfo> CallInfos;

	bool bShouldExit = false;

	FTurinmaErrorInfo ErrorInfo;

	int32 CurCallInfo = INDEX_NONE;

	int32 NextCallInfo = INDEX_NONE;

	bool IsGraphValid(int32 GraphIndex);
	bool IsNodeValid(int32 GraphIndex, int32 NodeIndex);
	bool IsNodePure(int32 GraphIndex, int32 NodeIndex);
	bool LocalJmp(FTurinmaProcessCallInfoItem& Item, int32 NodeIndex);

	void RecordError(const FTurinmaErrorContent& InErrorContent);

	FTurinmaCoroutine Execute();
};


struct FTestClass
{
	FTestClass();

	static FTestClass TestFFFFF;
};
