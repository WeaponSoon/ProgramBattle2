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

	LUASOURCE_API void PushLuaUEData(void* Value, UStruct* DataType, EUEDataType Type, TCustomMemoryHandle<FLuaUEData> Oter, int32 MaxCount = 1);
	LUASOURCE_API void PushUStructCopy(void* Value, UScriptStruct* DataType);
public:

	static const char* const LuaUEDataMetatableName;

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
		PushLuaUEData(Obj, Obj->GetClass(), EUEDataType::Object, nullptr);
	}
	UFUNCTION(BlueprintCallable)
	void PushUStructDefault(UScriptStruct* Struct)
	{
		PushUStructCopy(nullptr, Struct);
	}
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (CustomStructureParam = "CustomStruct"))
	void PushUStructCopy(const FDummyStruct& CustomStruct);
	void implPushUStructCopy(void* const StructAddr, UScriptStruct* StructType)
	{
		PushUStructCopy(StructAddr, StructType);
	}

	DECLARE_FUNCTION(execPushUStructCopy)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FStructProperty>(NULL);
		void* StructAddr = Stack.MostRecentPropertyAddress;
		FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

		P_FINISH;
		P_NATIVE_BEGIN;
		((ULuaState*)P_THIS)->implPushUStructCopy(StructAddr, StructProperty->Struct);
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
