#pragma once

#include <coroutine>
#include <map>
#include "CoreMinimal.h"
#include "TurinmaCommon.h"
#include "Engine/DataAsset.h"
#include "TurinmaProgram.generated.h"
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
UE_DISABLE_OPTIMIZATION
#endif

UENUM(BlueprintType)
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

	Function,
	Array,
	Table,
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

	int32 HeapValueIndex = INDEX_NONE;

	struct FTurinmaHeapValue* HeapValue(class FTurinmaProcess* Process);
	const struct FTurinmaHeapValue* HeapValue(class FTurinmaProcess* Process) const;

	ETurinmaValueType ValueType = ETurinmaValueType::Nil;
	//游戏里的哈希值
	uint32 GetHash(class FTurinmaProcess* Process) const;
	uint32 GetSimpleHash() const
	{
		if (ValueType < ETurinmaValueType::EndOfSimpleValue)
		{
			return GetHash(nullptr);
		}
		uint32 Hash = GetTypeHash(ValueType);
		return HashCombine(Hash, GetTypeHash(HeapValueIndex));
	}

	//游戏里判断相等
	bool Equals(const FTurinmaValue& Other, class FTurinmaProcess* Process) const;
	bool SimpleEquals(const FTurinmaValue& Other) const
	{
		if (ValueType != Other.ValueType)
		{
			return false;
		}
		if (ValueType < ETurinmaValueType::EndOfSimpleValue)
		{
			return Equals(Other, nullptr);
		}
		return HeapValueIndex == Other.HeapValueIndex;
	}
	//仅供底层使用的相等
	bool operator==(const FTurinmaValue& Other) const
	{
		return SimpleEquals(Other);
	}

	
	FString ToLexicalString(class FTurinmaProcess* Process) const;
	FTurinmaValue ToString(class FTurinmaProcess* Process) const
	{
		FTurinmaValue Ret;
		Ret.ValueType = ETurinmaValueType::String;
		Ret.StringValue = ToLexicalString(Process);
		return Ret;
		
	}
};
//仅供底层使用的哈希
inline uint32 GetTypeHash(const FTurinmaValue& Id)
{
	return Id.GetSimpleHash();
}

enum class EHeapValueKind : uint8
{
	None,
	Function,
	Array,
	Table,
	Struct
};
#define HEAPVALUETYPE_CHECKSAME(Type) static_assert((uint8)EHeapValueKind::Type == (uint8)ETurinmaValueType::Type - (uint8)ETurinmaValueType::EndOfSimpleValue);
HEAPVALUETYPE_CHECKSAME(Function)
HEAPVALUETYPE_CHECKSAME(Array)
HEAPVALUETYPE_CHECKSAME(Table)
HEAPVALUETYPE_CHECKSAME(Struct)



struct FTurinmaHeapValue : TSharedFromThis<FTurinmaHeapValue>
{
	bool bReached = true;

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

	virtual FString ToLexicalString(class FTurinmaProcess* Process) const { return TEXT(""); }

	virtual uint32 GetHash(class FTurinmaProcess* Process) const { return 0; }
	virtual bool Equals(const FTurinmaHeapValue& Other, class FTurinmaProcess* Process) const { return false; }
	virtual FTurinmaHeapValue* GetCopy() const
	{
		return new FTurinmaHeapValue(*this);
	}

	virtual ~FTurinmaHeapValue() = default;
};

struct FTurinmaFunctionValue : FTurinmaHeapValue
{

	virtual EHeapValueKind GetHeapValueKind() const override { return EHeapValueKind::Function; }

	static EHeapValueKind StaticHeapValueKind() { return EHeapValueKind::Function; }

	virtual uint32 GetHash(class FTurinmaProcess* Process) const override { return GetTypeHash(this); }
	virtual bool Equals(const FTurinmaHeapValue& Other, class FTurinmaProcess* Process) const override { return this == &Other; }
	virtual FTurinmaHeapValue* GetCopy() const override
	{
		return new FTurinmaFunctionValue(*this);
	}
};


struct FTurinmaArrayValue : FTurinmaHeapValue
{
	TArray<FTurinmaValue> Values;

	virtual EHeapValueKind GetHeapValueKind() const override { return EHeapValueKind::Array; }

	static EHeapValueKind StaticHeapValueKind() { return EHeapValueKind::Array; }

	virtual uint32 GetHash(class FTurinmaProcess* Process) const override;
	virtual bool Equals(const FTurinmaHeapValue& Other, class FTurinmaProcess* Process) const override;
	virtual FTurinmaHeapValue* GetCopy() const override
	{
		return new FTurinmaArrayValue(*this);
	}
};

struct FTurinmaTableValue : FTurinmaHeapValue
{

	TMap<FTurinmaValue, FTurinmaValue> InnerMap;

	virtual EHeapValueKind GetHeapValueKind() const override { return EHeapValueKind::Table; }

	static EHeapValueKind StaticHeapValueKind() { return EHeapValueKind::Table; }

	virtual uint32 GetHash(class FTurinmaProcess* Process) const override;
	virtual bool Equals(const FTurinmaHeapValue& Other, class FTurinmaProcess* Process) const override;
	virtual FTurinmaHeapValue* GetCopy() const override
	{
		return new FTurinmaTableValue(*this);
	}
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

	virtual uint32 GetHash(class FTurinmaProcess* Process) const override;
	virtual bool Equals(const FTurinmaHeapValue& Other, class FTurinmaProcess* Process) const override;
	virtual FTurinmaHeapValue* GetCopy() const override
	{
		return new FTurinmaStructValue(*this);
	}
};



#define DECLARE_TURINMA_GRAPH_NODE_DATA(DataType)\
	virtual UStruct* GetDataType() const override\
	{\
		return StaticStruct();\
	}\
	virtual void* GetThisPtr() const override\
	{\
		return (void*)this;\
	}\
	void CopyForm(const FTurinmaGraphNodeDataBase* Other) override\
	{\
		if(GetDataType() == Other->GetDataType())\
		{\
			*this = *static_cast<const std::remove_reference_t<decltype(*this)>*>(Other);\
		}\
	}\


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
	FTurinmaGraphParamDescVersion Version = FTurinmaGraphParamDescVersion::Last;
	ETurinmaValueType ValueType;
	FName ExtraTypeInfo;
	FName ParamName;

	bool Serialize(FArchive& Ar)
	{
		if (Ar.IsSaving())
		{
			FTurinmaGraphParamDescVersion CurV = FTurinmaGraphParamDescVersion::Last;
			Ar << CurV;

			Ar << ValueType;
			Ar << ExtraTypeInfo;
			Ar << ParamName;
		}
		if (Ar.IsLoading())
		{
			Ar << Version;

			Ar << ValueType;
			Ar << ExtraTypeInfo;
			Ar << ParamName;
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
	int32 NodeIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()

	UPROPERTY()
	class UTurinmaProgram* ProgramIn = nullptr;

	UPROPERTY(EditAnywhere)
	bool IsPure = false;
	UPROPERTY(EditAnywhere)
	FVector2D Location;
	UPROPERTY(EditAnywhere)
	FVector2D Size = FVector2D(200, 160);
	UPROPERTY(VisibleAnywhere)
	TArray<FTurinmaGraphNodeLinkInfo> NextNodes;
	UPROPERTY(VisibleAnywhere)
	TArray<FTurinmaGraphNodeParamLinkInfo> InputParams;
	

	virtual TArray<FTurinmaGraphNodeParamDescInfo> GetOutputParamDescs() const { return {}; }
	virtual TArray<FTurinmaGraphNodeParamDescInfo> GetInputParamDescs() const { return {}; };


	virtual bool IsInputMatch() const
	{
		auto&& InputParamDescs = GetInputParamDescs();
		return InputParamDescs.Num() == InputParams.Num();
	}
	virtual bool CanModifyInputParamsDesc() const
	{
		return false;
	}

	virtual bool CanModifyOutputParamsDesc() const
	{
		return false;
	}

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

	virtual void CopyForm(const FTurinmaGraphNodeDataBase* Other)
	{
		if(GetDataType() == Other->GetDataType())
		{
			*this = *static_cast<const std::remove_reference_t<decltype(*this)>*>(Other);
		}
	}

	virtual TSharedPtr<struct FTurinmaGraphNodeBase> CreateNode(const FTurinmaNodeCreateInfo& CreatInfo) { return nullptr; }

	virtual int32 DesiredNextNodesNumber() const { return 1; }

	virtual bool CanChangeNodeNameTo(const FString& InPendingName) { return false; }

	virtual void ChangeNodeNameTo(FName InName) {};

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
	bool bIsPure = false;
	int32 DataIndex = INDEX_NONE;
	int32 NodeIndex = INDEX_NONE;
	
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

	TArray<FTurinmaGraphNodeParamDescInfo> OutputParamDesc;

	virtual TArray<FTurinmaGraphNodeParamDescInfo> GetOutputParamDescs() const override
	{
		return OutputParamDesc;
	}

	virtual bool CanModifyOutputParamsDesc() const override
	{
		return true;
	}
};


struct FTurinmaGraphInputNode : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaGraphInputNode)

	virtual bool Execute(const FTurinmaNodeExecuteParam& ExecuteParam) override;

};

USTRUCT()
struct FTurinmaGraphOutputNodeData : public FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()

	DECLARE_TURINMA_GRAPH_NODE_DATA(FTurinmaGraphOutputNodeData)

	virtual TSharedPtr<struct FTurinmaGraphNodeBase> CreateNode(const FTurinmaNodeCreateInfo& CreateInfo) override;


	TArray<FTurinmaGraphNodeParamDescInfo> InputParamDesc;

	virtual TArray<FTurinmaGraphNodeParamDescInfo> GetInputParamDescs() const override
	{
		return InputParamDesc;
	}
	virtual int32 DesiredNextNodesNumber() const override { return 0; }
	virtual bool CanModifyInputParamsDesc() const override
	{
		return true;
	}
};


struct FTurinmaGraphOutputNode : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaGraphOutputNode)

	virtual bool Execute(const FTurinmaNodeExecuteParam& ExecuteParam) override;
};


USTRUCT()
struct FTurinmaCallGraphNodeData : public FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()

	DECLARE_TURINMA_GRAPH_NODE_DATA(FTurinmaCallGraphNodeData)

	UPROPERTY()
	FName GraphName = NAME_None;

	virtual TSharedPtr<struct FTurinmaGraphNodeBase> CreateNode(const FTurinmaNodeCreateInfo& CreateInfo) override;
	virtual TArray<FTurinmaGraphNodeParamDescInfo> GetInputParamDescs() const override;
	virtual TArray<FTurinmaGraphNodeParamDescInfo> GetOutputParamDescs() const override;
	
};

struct FTurinmaCallGraphNode : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaCallGraphNode)

	FName GraphName = NAME_None;
	virtual bool Execute(const FTurinmaNodeExecuteParam& ExecuteParam) override;
};


USTRUCT()
struct FTurinmaLexicalIntGraphNodeData : public FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()

	DECLARE_TURINMA_GRAPH_NODE_DATA(FTurinmaLexicalIntGraphNodeData)

	UPROPERTY()
	int64 LexicalInt;



	virtual TSharedPtr<struct FTurinmaGraphNodeBase> CreateNode(const FTurinmaNodeCreateInfo& CreateInfo) override;
	virtual TArray<FTurinmaGraphNodeParamDescInfo> GetOutputParamDescs() const override;

};

struct FTurinmaLexicalIntGraphNode : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaLexicalIntGraphNode)

	int64 LexicalInt;
	virtual bool Execute(const FTurinmaNodeExecuteParam& ExecuteParam) override;
};


USTRUCT()
struct FTurinmaOutputLogGraphNodeData : public FTurinmaGraphNodeDataBase
{
	GENERATED_BODY()

	DECLARE_TURINMA_GRAPH_NODE_DATA(FTurinmaOutputLogGraphNodeData)

	virtual TSharedPtr<struct FTurinmaGraphNodeBase> CreateNode(const FTurinmaNodeCreateInfo& CreateInfo) override;
	virtual TArray<FTurinmaGraphNodeParamDescInfo> GetInputParamDescs() const override;

};

struct FTurinmaOutputLogGraphNode : FTurinmaGraphNodeBase
{
	DECLARE_TURINMA_GRAPH_NODE(FTurinmaOutputLogGraphNode)

	virtual bool Execute(const FTurinmaNodeExecuteParam& ExecuteParam) override;
};




USTRUCT(BlueprintType)
struct TURINMALUA_API FTurinmaGraphData
{
	GENERATED_BODY()

	UPROPERTY()
	FName GraphName;

	UPROPERTY(Transient)
	int32 StartNodeIndex = INDEX_NONE;

	UPROPERTY(Transient)
	int32 EndNodeIndex = INDEX_NONE;

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

		bool IsValid() const
		{
			return !!NodeType && !!NodeData;
		}

		void Reset()
		{
			if (NodeType && NodeData)
			{
				NodeType->DestroyStruct(NodeData->GetThisPtr());
				FMemory::Free(NodeData);
			}
			NodeType = nullptr;
			NodeData = nullptr;
		}

		template<typename T>
		void SetData(const T& Data)
		{
			static_assert(std::is_base_of<FTurinmaGraphNodeDataBase, T>::value, "error");
			SetData(T::StaticStruct(), &Data);
		}

		void SetData(UScriptStruct* Type, const void* Data)
		{
			if(Type == nullptr || !Type->IsChildOf(FTurinmaGraphNodeDataBase::StaticStruct()))
			{
				Reset();
				return;
			}
			if(Type != NodeType)
			{
				Reset();

				NodeType = Type;
				NodeData = (FTurinmaGraphNodeDataBase*)FMemory::Malloc(NodeType->GetStructureSize());
				NodeType->InitializeStruct(NodeData);
				if(Data)
				{
					NodeType->CopyScriptStruct(NodeData, Data);
				}
			}
			else
			{
				if(!NodeData)
				{
					NodeData = (FTurinmaGraphNodeDataBase*)FMemory::Malloc(NodeType->GetStructureSize());
					NodeType->InitializeStruct(NodeData);
				}
				if(Data)
				{
					NodeType->CopyScriptStruct(NodeData, Data);
				}
			}
			
		}

		void AddReferencedObjects(FReferenceCollector& Collector)
		{
			if(NodeType)
			{
				Collector.AddReferencedObject(NodeType);
				if(NodeData)
				{
					check(NodeType == NodeData->GetDataType());
					Collector.AddPropertyReferences(NodeType, NodeData->GetThisPtr());
				}
			}
			
		}

		FTurinmaNodeDataItem() = default;
		FTurinmaNodeDataItem(const FTurinmaNodeDataItem& Other) : NodeType(Other.NodeType)
		{
			UE_LOG(LogTemp, Log, TEXT("SWP:Copy Constructor"));
			if(NodeType)
			{
				void* Data = FMemory::Malloc(NodeType->GetStructureSize());
				NodeType->InitializeStruct(Data);
				if(Other.NodeData)
				{
					NodeType->CopyScriptStruct(Data, Other.NodeData);
				}
				NodeData = static_cast<FTurinmaGraphNodeDataBase*>(Data);
			}
		}

		FTurinmaNodeDataItem(FTurinmaNodeDataItem&& Other) noexcept
		{
			UE_LOG(LogTemp, Log, TEXT("SWP:Move Constructor"));
			NodeType = Other.NodeType;
			NodeData = Other.NodeData;
			Other.NodeType = nullptr;
			Other.NodeData = nullptr;
		}

		~FTurinmaNodeDataItem()
		{
			UE_LOG(LogTemp, Log, TEXT("SWP:Destructor"));
			if(NodeType && NodeData)
			{
				NodeType->DestroyStruct(NodeData->GetThisPtr());
				FMemory::Free(NodeData);
			}
		}

		FTurinmaNodeDataItem& operator=(const FTurinmaNodeDataItem& Other)
		{
			UE_LOG(LogTemp, Log, TEXT("SWP:Copy Assignment"));

			SetData(Other.NodeType, Other.NodeData);

			return *this;
		}

		FTurinmaNodeDataItem& operator=(FTurinmaNodeDataItem&& Other) noexcept
		{
			UE_LOG(LogTemp, Log, TEXT("SWP:Move Assignment"));

			auto* TempNodeType = NodeType;
			auto* TempNodeData = NodeData;

			NodeType = Other.NodeType;
			NodeData = Other.NodeData;
			Other.NodeType = TempNodeType;
			Other.NodeData = TempNodeData;
			return *this;
		}

		bool operator==(const FTurinmaNodeDataItem& Other) const
		{
			if(NodeType == Other.NodeType)
			{
				if(NodeType)
				{
					if(NodeData)
					{
						return NodeType->CompareScriptStruct(NodeData, Other.NodeData, 0);
					}
					return !Other.NodeData;
				}
				return true;//both invalid
			}
			return false;
		}


		void Serialize(FArchive& Ar)
		{
			if(Ar.IsSaving())
			{
				UE_LOG(LogTemp, Log, TEXT("SWP: Saving"));
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
				UE_LOG(LogTemp, Log, TEXT("SWP: Loading"));
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

	void Init(UTurinmaProgram* Program);

	void InitInoutPut();

	bool Serialize(FArchive& Ar);

	void AddStructReferencedObjects(class FReferenceCollector& Collector) const;
};


template<> struct TStructOpsTypeTraits<FTurinmaGraphData> : public TStructOpsTypeTraitsBase2<FTurinmaGraphData>
{
	enum { WithSerializer = true, WithAddStructReferencedObjects = true };
};

struct FTurinmaGraphCreateInfo
{
	int32 GraphIndex = INDEX_NONE;
	int32 DataIndex = INDEX_NONE;
};

struct FTurinmaGraph
{
	int32 GraphIndex = INDEX_NONE;
	int32 DataIndex = INDEX_NONE;

	TArray<TSharedPtr<FTurinmaGraphNodeBase>> Nodes;

	int32 StartNodeIndex = INDEX_NONE;


	bool InitWithDataAndInfo(const FTurinmaGraphData& InData, const FTurinmaGraphCreateInfo& InInfo);
};



UCLASS(BlueprintType)
class TURINMALUA_API UTurinmaProgram : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FTurinmaGraphData> GraphDatas;

	UPROPERTY(Transient)
	TMap<FName, int32> NameToGraph;

	UFUNCTION(BlueprintCallable)
	void CopyFrom(UTurinmaProgram* Other);

	UFUNCTION(BlueprintCallable)
	void InitAllGraphDatas();

#if WITH_EDITOR
	UFUNCTION(CallInEditor)
	void Test_AddSomeNodeInGraph()
	{
		for(auto&& Graph : GraphDatas)
		{
			auto&& Data = Graph.NodeDatas.AddDefaulted_GetRef();
			Data.NodeType = FTurinmaGraphInputNodeData::StaticStruct();
			FTurinmaGraphInputNodeData* NodeData = (FTurinmaGraphInputNodeData*)FMemory::Malloc(Data.NodeType->GetStructureSize());
			Data.NodeType->InitializeStruct(NodeData);
			Data.NodeData = NodeData;
			NodeData->InputParams.AddDefaulted_GetRef().ParamPin = 100;
		}
	}

	UFUNCTION(BlueprintCallable)
	static UTurinmaProgram* GenerateTestTurinmaProgram();

	UFUNCTION(BlueprintCallable)
	static void TestRun(UTurinmaProgram* InProgram);
	UFUNCTION(BlueprintCallable)
	static void TestStop();
#endif

	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

	void RebuildNameToGraphIndex()
	{
		NameToGraph.Empty(GraphDatas.Num());
		for (int32 I = 0; I < GraphDatas.Num(); ++I)
		{
			auto&& Item = GraphDatas[I];
			NameToGraph.Add(Item.GraphName, I);
		}
		InitAllGraphDatas();
	}

	virtual void PostLoad() override
	{
		Super::PostLoad();
		RebuildNameToGraphIndex();
	}
};


struct FTurinmaProcessManagedHeap
{
	TArray<int32> UnusedIndex;
	TArray<TUniquePtr<FTurinmaHeapValue>> TurinmaHeapValues;
	FTurinmaProcessManagedHeap() = default;
	FTurinmaProcessManagedHeap(const FTurinmaProcessManagedHeap& Other) : UnusedIndex(Other.UnusedIndex)
	{
		TurinmaHeapValues.Reserve(TurinmaHeapValues.Num());
		for(auto&& Item : TurinmaHeapValues)
		{
			TurinmaHeapValues.Emplace(Item->GetCopy());
		}
	}
	FTurinmaProcessManagedHeap(FTurinmaProcessManagedHeap&& Other) noexcept : UnusedIndex(MoveTemp(Other.UnusedIndex)), TurinmaHeapValues(MoveTemp(Other.TurinmaHeapValues))
	{
		
	}

	FTurinmaProcessManagedHeap& operator=(const FTurinmaProcessManagedHeap& Other)
	{
		UnusedIndex = Other.UnusedIndex;
		TurinmaHeapValues.Reset(TurinmaHeapValues.Num());
		for (auto&& Item : TurinmaHeapValues)
		{
			TurinmaHeapValues.Emplace(Item->GetCopy());
		}
		return *this;
	}
	FTurinmaProcessManagedHeap& operator=(FTurinmaProcessManagedHeap&& Other) noexcept
	{
		UnusedIndex = MoveTemp(Other.UnusedIndex);
		TurinmaHeapValues = MoveTemp(Other.TurinmaHeapValues);
		return *this;
	}
};

struct FTurinmaProcessGlobal
{
	TMap<FName, FTurinmaValue>  TurinmaGlobals;
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

	int32 JumpIntoNodeIndex = INDEX_NONE;

	int32 GraphIndex = INDEX_NONE;
	//FTurinmaGraph* Graph = nullptr;
	TArray<FLocalNodeIndex> LocalNodeIndex;
	TMap<FName, FTurinmaValue> LocalVariables;
	TMap<int32, TArray<FTurinmaValue>> TempLocalVariables;


	TArray<FTurinmaValue> GraphInputValue;
	TArray<FTurinmaValue> GraphOutputValue;
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
	friend class FTurinmaProcess;
	struct FTurinmaPromise {
		class FTurinmaProcess* Process = nullptr;
		FTurinmaCoroutine get_return_object() {
			return std::coroutine_handle<FTurinmaPromise>::from_promise(*this);
		}
		std::suspend_always initial_suspend() { return {}; }//协程创建之后，协程函数体执行之前的时候执行此函数，等同于co_await initial_suspend();这里返回suspend_always
		//表示创建调用函数创建协程对象后立马返回，不执行协程函数真正的函数体，知道外面调用resume
		std::suspend_always final_suspend() noexcept;
		

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
	FTurinmaCoroutine() = default;
	FTurinmaCoroutine(const FTurinmaCoroutine&) = default;
	FTurinmaCoroutine& operator=(const FTurinmaCoroutine&) = default;
	std::coroutine_handle<FTurinmaPromise> handle;
private:
	
};

struct TURINMALUA_API FTurinmaProcessConsoleItem
{
	FString Log;
};

class TURINMALUA_API FTurinmaProcessConsole
{
public:
	int32 MaxLogCount = 100;
	TTurinmaCircularQueue<FTurinmaProcessConsoleItem> Logs;

	void ClearLog()
	{
		Logs.Reset();
	}

	void PushLog(const FTurinmaProcessConsoleItem& Log)
	{
		if(Logs.GetCount() >= MaxLogCount)
		{
			Logs.PopQueueNoRet();
		}
		Logs.Enqueue(Log);
	}
};

class TURINMALUA_API FTurinmaProcess
{

	friend struct FTurinmaCoroutine;
	FTurinmaCoroutine Coroutine;
	bool bHasStart = false;
	bool bHasFinish = false;
	bool bShouldExit = false;

	TArray<int32> UnusedCallInfoIndex;
	int32 CurCallInfo = INDEX_NONE;

	bool InitProcessByProgram();

public:

	FTurinmaProcessConsole Console;

	int32 MaxNumExecutePerTick = 10;
	int32 MaxGCProcessCount = 10;

	FName EntryGraphName = TEXT("Main");

	~FTurinmaProcess()
	{
		Stop();
	}
	bool Start();
	bool Stop();

	void Tick();

	bool IsRunning() const
	{
		return bHasStart && !bHasFinish;
	}

	TWeakObjectPtr<UTurinmaProgram> Program;
	FTurinmaProcessManagedHeap Heap;
	FTurinmaProcessGlobal Globals;
	TIndirectArray<FTurinmaGraph> Graphs;
	TIndirectArray<FTurinmaProcessCallInfo> CallInfos;
	TMap<FName, int32> GraphNameToGraphIndex;

	FTurinmaErrorInfo ErrorInfo;

	TArray<int32> CurCallInfoStack;
	

	bool IsGraphValid(int32 GraphIndex);
	bool IsNodeValid(int32 GraphIndex, int32 NodeIndex);
	bool IsNodePure(int32 GraphIndex, int32 NodeIndex);
	bool LocalJmp(FTurinmaProcessCallInfoItem& Item, int32 NodeIndex);
	bool LongJmp(FTurinmaProcessCallInfo& CallInfo, int32 GraphIndex);
	bool Return(int32 InCurCallInfo);

	void RecordError(const FTurinmaErrorContent& InErrorContent);

	FTurinmaCoroutine Execute();
};


struct FTestClass
{
	FTestClass();

	static FTestClass TestFFFFF;
};
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
UE_ENABLE_OPTIMIZATION
#endif
