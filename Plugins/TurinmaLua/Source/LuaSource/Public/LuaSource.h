// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "lua.hpp"
#include "Modules/ModuleManager.h"
#include "CustomMemoryHandle.h"
#include "LuaSource.generated.h"

DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnLuaLoadFile, const FString&, FString&);

#define MAKE_LUA_METATABLE_NAME(ClassName) "*LMN*" #ClassName "*end*"

EXTERN_C
{
	struct GCObject;
}

namespace LuaCPPAPI
{
	void luaC_foreachgcobj(lua_State* L, TFunction<void(GCObject*, bool, lua_State*)> cb);
	
}


UINTERFACE(Blueprintable, MinimalAPI)
class UTestInterface : public UInterface
{
public:
	GENERATED_BODY()
};

class ITestInterface
{
public:
	GENERATED_BODY()

	virtual void T(){};

};

USTRUCT(BlueprintType)
struct FDummyStruct
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FTestStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 TestInt = 99;
};


enum class ETypeKind : uint16
{
	None,
	PreDefinedKindBegin,
	NumericBegin = PreDefinedKindBegin,
	Int8 = NumericBegin,
	Int16,
	Int32,
	Int64,
	Byte,//uint8
	UInt16,
	UInt32,
	UInt64,
	Float,
	Double,
	NumericEnd = Double,
	Bool,
	BitBool,
	String,
	Name,
	Text,
	Vector,
	Rotator,
	Quat,
	Transform,
	Matrix,
	RotationMatrix,
	Color,
	LinearColor,
	PreDefinedKindEnd,

	CombineKindBegin = PreDefinedKindEnd,
	Array = CombineKindBegin,
	Map,
	Set,
	Delegate,
	MulticastDelegate,
	WeakObject,
	CombineKindEnd,


	UserDefinedKindBegin = CombineKindEnd,
	U_Enum = UserDefinedKindBegin,
	U_ScriptStruct,
	U_Class,
	U_Function,
	U_Interface,
	UserDefinedKindEnd
	
};



struct FTypeDesc : FRefCountBase
{
	TArray<TRefCountPtr<FTypeDesc>> CombineKindSubTypes;

	union
	{
		UObject* Pointer;
		UField* BasePointer;

		UEnum* U_Enum;
		UFunction* U_Function;
		UClass* U_Class;
		UScriptStruct* U_ScriptStruct;
	} UserDefinedTypePointer;

	ETypeKind Kind = ETypeKind::None;

	FTypeDesc()
	{
		//static_assert(std::is_standard_layout_v<FTypeDesc> && std::is_trivial_v<FTypeDesc>, "FTypeDesc must be POD");
		FMemory::Memzero(&UserDefinedTypePointer, sizeof(UserDefinedTypePointer));
	}

	LUASOURCE_API void AddReferencedObject(UObject* Oter, void* PtrToValue, FReferenceCollector& Collector, bool bStrong);

	LUASOURCE_API int32 GetSize() const;
	LUASOURCE_API int32 GetAlign() const;
	LUASOURCE_API void InitValue(void* ValueAddress) const;
	LUASOURCE_API void CopyValue(void* Dest, void* Source) const;
	LUASOURCE_API void DestroyValue(void* ValueAddress) const;

	LUASOURCE_API uint32 GetTypeHash(void* ValueAddress) const;
	LUASOURCE_API bool ValueEqual(void* Dest, void* Source) const;


	LUASOURCE_API static int32 GetSizePreDefinedKind(ETypeKind InKind);
	LUASOURCE_API static TRefCountPtr<FTypeDesc> AquireClassDescByUEType(UField* UEType);
	LUASOURCE_API static TRefCountPtr<FTypeDesc> AquireClassDescByPreDefinedKind(ETypeKind InKind);
	LUASOURCE_API static TRefCountPtr<FTypeDesc> AquireClassDescByCombineKind(ETypeKind InKind, const TArray<TRefCountPtr<FTypeDesc>>& InKey);

	LUASOURCE_API static TRefCountPtr<FTypeDesc> AquireClassDescByProperty(FProperty* UEType);
};

typedef TRefCountPtr<FTypeDesc> FTypeDescRefCount;

UCLASS()
class UUETypeDescContainer : public UObject
{
	GENERATED_BODY()

public:
	FCriticalSection CS;

	TMap<UObject*, FTypeDescRefCount> Map;

	LUASOURCE_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
};

struct FLuaUEValue : public FCustomMemoryItemThirdParty
{
	//static constexpr int32 GetMaxInlineSize()
	//{
	//	int32 MaxInlineSize = 0;
	//	#define FIND_MAX_TYPE_GetMaxInlineSize(Type) \
	//	if(sizeof(Type) > MaxInlineSize)\
	//	{\
	//		MaxInlineSize = sizeof(Type);\
	//	}\

	//	FIND_MAX_TYPE_GetMaxInlineSize(FScriptArray);
	//	FIND_MAX_TYPE_GetMaxInlineSize(FScriptMap);
	//	FIND_MAX_TYPE_GetMaxInlineSize(FScriptSet);
	//	FIND_MAX_TYPE_GetMaxInlineSize(FMatrix);
	//	FIND_MAX_TYPE_GetMaxInlineSize(FRotationMatrix);
	//	FIND_MAX_TYPE_GetMaxInlineSize(FTransform);
	//	FIND_MAX_TYPE_GetMaxInlineSize(void*);

	//	return MaxInlineSize;
	//}

	TCustomMemoryHandle<FLuaUEValue> Owner = nullptr; //if onwer is not null this value represent a part of other value

	static constexpr int32 MaxInlineSize = 256;

	int32 AllValueIndex = -1;
	mutable bool Marked = false;

	FTypeDescRefCount TypeDesc;
	bool bHasData = false;
	TAlignedBytes<MaxInlineSize, alignof(std::max_align_t)> InnerData;

	bool bHasDestroyedData = false;

	FLuaUEValue() : TypeDesc(nullptr), InnerData()
	{
		FMemory::Memzero(&InnerData, sizeof(InnerData));
	}

	void DestroyData()
	{
		if(!IsPartOfOthers() && GetData())
		{
			TypeDesc->DestroyValue(GetData());
		}
		bHasDestroyedData = true;
	}

	bool IsHasValidData() const
	{
		if (bHasData && !bHasDestroyedData && TypeDesc.IsValid() && TypeDesc->GetSize() > 0)
		{
			if(IsPartOfOthers())
			{
				return Owner && Owner->IsHasValidData();
			}
			return true;
		}
		return false;
	}

	bool IsPartOfOthers() const
	{
		return Owner.EverHasValueUntilReset();
	}

	void* GetData() 
	{
		if(!IsHasValidData())
		{
			return nullptr;
		}

		if(bHasData && TypeDesc.IsValid() && TypeDesc->GetSize() > 0)
		{
			if(TypeDesc->GetSize() > MaxInlineSize || IsPartOfOthers())
			{
				return *(void**)(&InnerData);
			}
			else
			{
				return &InnerData;
			}
		}
		return nullptr;
	}

	const void* GetData() const
	{
		if (!IsHasValidData())
		{
			return nullptr;
		}

		if (bHasData && TypeDesc.IsValid() && TypeDesc->GetSize() > 0)
		{
			if (TypeDesc->GetSize() > MaxInlineSize || IsPartOfOthers())
			{
				return *(void**)(&InnerData);
			}
			else
			{
				return &InnerData;
			}
		}
		return nullptr;
	}

	void InitDataAsPartOfOther(void* PartOfOther)
	{
		if (!bHasData && TypeDesc.IsValid() && TypeDesc->GetSize() > 0)
		{
			//void* ObjectMem = FMemory::Malloc(sizeof(void*));
			*(void**)(&InnerData) = PartOfOther;
			bHasData = true;
		}
	}

	void InitData()
	{
		if(!bHasData && TypeDesc.IsValid() && TypeDesc->GetSize() > 0)
		{
			if (TypeDesc->GetSize() > MaxInlineSize)
			{
				void* ObjectMem = FMemory::Malloc(TypeDesc->GetSize());
				TypeDesc->InitValue(ObjectMem);
				*(void**)(&InnerData) = ObjectMem;
				bHasData = true;
			}
			else
			{
				TypeDesc->InitValue(&InnerData);
				bHasData = true;
			}
		}
	}

	void CopyFrom(void* Other)
	{
		if(GetData() != nullptr)
		{
			TypeDesc->CopyValue(GetData(), Other);
		}
	}

	void CopyFrom(const FLuaUEValue& Other)
	{
		if(GetData() && Other.GetData() && TypeDesc == Other.TypeDesc)
		{
			if(GetData() == Other.GetData())
			{
				return;
			}
			TypeDesc->CopyValue(GetData(), (void*)Other.GetData());
		}
	}

	void AddReferencedObject(UObject* Oter, FReferenceCollector& Collector, bool bStrong);
	
};





struct FLuaArrayRefData
{
	UStruct* ElementType;
	FScriptArray* Array;

};

struct FLuaArrayData
{
	UStruct* ElementType;
	TAlignedBytes<sizeof(FScriptArray), alignof(FScriptArray)> InnerData;
	bool bValid;

	void AddReferencedObjects(UObject* Owner, FReferenceCollector& Collector, bool bStrong)
	{
		if(ElementType && bValid)
		{
			FScriptArray* Array = (FScriptArray*)&InnerData;
			
		}
	}

};

struct FLuaUStructRefData //can also used to access native static array PROPERTY e.g. class USomeObject{UPROPERTY()FSomeStruct StrArray[100];}
{
	UScriptStruct* StructType;
	void* PtrToData;
	int32 MaxOffsetCount;

	bool IsDataValid() const
	{
		return StructType != nullptr && PtrToData != nullptr && MaxOffsetCount > 0;
	}

	void Clear()
	{
		StructType = nullptr;
		PtrToData = nullptr;
		MaxOffsetCount = 0;
	}

	void SetRef(UScriptStruct* Tye, void* Data, int32 MaxCount)
	{
		StructType = Tye;
		PtrToData = Data;
		MaxOffsetCount = MaxCount;
	}
};

struct FLuaUStructData
{
	static constexpr int32 MaxInlineSize = sizeof(FTransform) > sizeof(void*) ? sizeof(FTransform) : sizeof(void*);

	UScriptStruct* StructType;
	TAlignedBytes<MaxInlineSize, alignof(std::max_align_t)> InnerData;
	bool bValid;

	bool IsDataValid() const
	{
		return bValid;
	}

	void Clear();

	void CopyData(void* Data);

	void SetData(UScriptStruct* Type, void* Data);

	void* GetData();
	
	void AddReferencedObjects(UObject* Owner, FReferenceCollector& Collector, bool bStrong);
};

struct FLuaUObjectRefData ////can also used to access native static array PROPERTY e.g. class USomeObject{UPROPERTY()UObject* ObjArray[100];}
{
	UObject** pObject;
	int32 MaxOffsetCount;

	void Clear()
	{
		pObject = nullptr;
		MaxOffsetCount = 0;
	}

	bool IsDataValid() const
	{
		return pObject != nullptr && *pObject && MaxOffsetCount > 0;
	}

};

struct FLuaUObjectData
{
	UObject* Object;

	bool IsDataValid() const
	{
		return Object != nullptr;
	}

	void AddReferencedObjects(UObject* Owner, FReferenceCollector& Collector, bool bStrong);
};


enum class EUEDataType
{
	None,
	Struct,
	StructRef,
	Object,
	ObjectRef
};

struct FLuaUEData : public FCustomMemoryItemThirdParty
{
	union
	{
		FLuaUStructRefData StructRef;
		FLuaUStructData Struct;
		FLuaUObjectRefData ObjectRef;
		FLuaUObjectData Object;
	} Data;
	EUEDataType DataType = EUEDataType::None;

	TOptional<TCustomMemoryHandle<FLuaUEData>> Oter;

	bool IsDataValid() const
	{
		if(DataType == EUEDataType::None)
		{
			return false;
		}

		if(DataType == EUEDataType::StructRef)
		{
			if(Oter.IsSet())
			{
				return Oter.GetValue() && Oter.GetValue()->IsDataValid() && Data.StructRef.IsDataValid();
			}
			return false;
		}

		if (DataType == EUEDataType::Struct)
		{
			return Data.Struct.IsDataValid();
		}

		if(DataType == EUEDataType::ObjectRef)
		{
			if (Oter.IsSet())
			{
				return Oter.GetValue() && Oter.GetValue()->IsDataValid() && Data.ObjectRef.IsDataValid();
			}
			return false;
		}

		if(DataType == EUEDataType::Object)
		{
			return Data.Object.IsDataValid();
		}



		return true;
	}

	bool Index(class ULuaState* L, const char* Key);

	FLuaUEData()
	{
		FMemory::Memzero(&Data, sizeof(Data));
	}

	~FLuaUEData()
	{
		switch (DataType)
		{
		case EUEDataType::Struct:
			Data.Struct.Clear();
			break;
		case EUEDataType::StructRef:
			Data.StructRef.Clear();
			break;
		case EUEDataType::Object:
			Data.Object.Object = nullptr;
		case EUEDataType::ObjectRef:
			Data.ObjectRef.Clear();
			break;
		default:
			break;
		}
		DataType = EUEDataType::None;
	}

	void AddReferencedObjects(UObject* Owner, FReferenceCollector& Collector, bool bStrong)
	{
		switch (DataType)
		{
		case EUEDataType::Struct:
			Data.Struct.AddReferencedObjects(Owner, Collector, bStrong);
			break;
		case EUEDataType::Object:
			Data.Struct.AddReferencedObjects(Owner, Collector, bStrong);
			break;
		case EUEDataType::StructRef:
			break;
		case EUEDataType::ObjectRef:
			break;
		case EUEDataType::None:
			break;
		}
	}
};


UCLASS(BlueprintType, MinimalAPI)
class UCompleteObject : public UObject
{
	GENERATED_BODY()
};

UCLASS(BlueprintType, MinimalAPI)
class ATestWeakRefPtr : public AActor
{
	GENERATED_BODY()

	UObject* ObjTest;
	UObject* WeakObjTest;
public:
	UPROPERTY(BlueprintReadWrite)
	TArray<UObject*> ObjArray;
	UPROPERTY(BlueprintReadWrite)
	TArray<FTestStruct> StructArray;
	UPROPERTY()
	UObject* OO[10];

	UFUNCTION(CallInEditor)
	void ConstructObjectTest()
	{
		ObjTest = NewObject<UCompleteObject>();
		WeakObjTest = NewObject<UCompleteObject>();
	}
	UFUNCTION(CallInEditor)
	void ConstructObjectTestSame()
	{
		ObjTest = NewObject<UCompleteObject>();
		WeakObjTest = ObjTest;
	}
	UFUNCTION(CallInEditor)
	void ForceGC()
	{
		GEngine->ForceGarbageCollection();
	}
	UFUNCTION(CallInEditor)
	void ClearStrong()
	{
		ObjTest = nullptr;
	}
	UFUNCTION(CallInEditor)
	void ClearWeak()
	{
		WeakObjTest = nullptr;
	}
	UFUNCTION(CallInEditor)
	void PrintPtr()
	{
		UE_LOG(LogTemp, Log, TEXT("weipengsong: Strong: %p, Weak: %p"), ObjTest, WeakObjTest);
	}

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
	{
		auto* This = CastChecked<ATestWeakRefPtr>(InThis);
		Collector.AddReferencedObject(This->ObjTest);
		Collector.MarkWeakObjectReferenceForClearing(&This->WeakObjTest);
	}

};


UCLASS(BlueprintType, MinimalAPI)
class ULuaState : public UObject
{
	GENERATED_BODY()

	friend int OnDestroyUEValueInLua(lua_State*);

	void PostGarbageCollect();

	friend void LuaLock(lua_State*);
	friend void LuaUnLock(lua_State*);
	lua_State* InnerState = nullptr;
	
	static thread_local uint64 LocalThreadId;
	static thread_local uint64 EnterCount;
	
	uint64 LockedThreadId = 0xffffffffffffffff;
	std::atomic_flag LuaAt = ATOMIC_FLAG_INIT;

	void OnCollectLuaRefs(FReferenceCollector& Collector, GCObject* o, bool w, lua_State* l);
	void LockLua();
	void UnlockLua();

	TArray<int32> UnusedAllValueIndex;
	TArray<FLuaUEValue*> AllValues;

	void PushToAllValues(FLuaUEValue* V)
	{
		int32 ID;
		if(UnusedAllValueIndex.Num() > 0)
		{
			ID = UnusedAllValueIndex.Last();
			UnusedAllValueIndex.Pop();
		}
		else
		{
			ID = AllValues.Num();
			AllValues.AddZeroed();
		}
		V->AllValueIndex = ID;
		AllValues[ID] = V;
	}
	void RemoveFormAllValues(FLuaUEValue* V)
	{
		AllValues[V->AllValueIndex] = nullptr;
		UnusedAllValueIndex.Push(V->AllValueIndex);
		V->AllValueIndex = -1;
	}


	LUASOURCE_API void PushLuaUEValue(void* Value, const TRefCountPtr<FTypeDesc>& Type);
	LUASOURCE_API void PushLuaUEValue(void* Value, const FLuaUEValue& PartOfWhom, const TRefCountPtr<FTypeDesc>& Type);
	LUASOURCE_API void PushUStructCopy(void* Value, UScriptStruct* DataType);

	LUASOURCE_API void PushProperty(void* Value, FProperty* Proy);
	LUASOURCE_API void PushProperty(void* Value, const FLuaUEValue& PartOfWhom, FProperty* Proy);
	//LUASOURCE_API void PushLuaUEData(void* Value, UStruct* DataType, EUEDataType Type, TCustomMemoryHandle<FLuaUEData> Oter, int32 MaxCount = 1);
	
public:

	LUASOURCE_API static const char* const LuaUEDataMetatableName;
	LUASOURCE_API static const char* const LuaUEValueMetatableName;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable)
	LUASOURCE_API void Init();

	UFUNCTION(BlueprintCallable)
	LUASOURCE_API void Finalize();

	UFUNCTION(BlueprintCallable)
	LUASOURCE_API void Pop(int32 Num);

	UFUNCTION(BlueprintCallable)
	void PushUObject(UObject* Obj)
	{
		auto&& R = FTypeDesc::AquireClassDescByUEType(Obj->GetClass());
		PushLuaUEValue(Obj, R);
		//PushLuaUEData(Obj, Obj->GetClass(), EUEDataType::Object, nullptr);
	}
	UFUNCTION(BlueprintCallable)
	void PushUStructDefault(UScriptStruct* Struct)
	{
		auto&& R = FTypeDesc::AquireClassDescByUEType(Struct);
		PushLuaUEValue(nullptr, R);
		//PushUStructCopy(nullptr, Struct);
	}
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (CustomStructureParam = "CustomStruct"))
	void PushUStructCopy(const FDummyStruct& CustomStruct);

	DECLARE_FUNCTION(execPushUStructCopy)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(NULL);
		void* StructAddr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		P_NATIVE_BEGIN;
		((ULuaState*)P_THIS)->PushProperty(StructAddr, Stack.MostRecentProperty);
		P_NATIVE_END;
	}


	template<typename T>
	void PushUStructCopy(const T& Value)
	{
		PushUStructCopy(&Value, T::StaticStruct());
	}





	LUASOURCE_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
};


class LUASOURCE_API FLuaSourceModule : public IModuleInterface
{
public:
	static FOnLuaLoadFile OnLuaLoadFile;
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void* LuaMalloc(void* ud, void* ptr, size_t osize, size_t nsize);

};
