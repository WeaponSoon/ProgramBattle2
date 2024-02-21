// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "lua.hpp"
#include "Modules/ModuleManager.h"
#include "LuaSource.generated.h"

DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnLuaLoadFile, const FString&, FString&);


EXTERN_C
{
	struct GCObject;
}

namespace LuaCPPAPI
{
	void luaC_foreachgcobj(lua_State* L, TFunction<void(GCObject*, bool, lua_State*)> cb);
	
}




struct FLuaUStructRefData
{
	UScriptStruct* StructType;
	void* PtrToData;

	void Clear()
	{
		StructType = nullptr;
		PtrToData = nullptr;
	}

	void SetRef(UScriptStruct* Tye, void* Data)
	{
		StructType = Tye;
		PtrToData = Data;
	}
};

struct FLuaUStructData
{
	static constexpr int32 MaxInlineSize = sizeof(FTransform) > sizeof(void*) ? sizeof(FTransform) : sizeof(void*);

	UScriptStruct* StructType;
	TAlignedBytes<MaxInlineSize, alignof(std::max_align_t)> InnerData;
	bool bValid;

	void Clear();

	void CopyData(void* Data);

	void SetData(UScriptStruct* Type, void* Data);

	void* GetData();
	
	void AddReferencedObjects(UObject* Owner, FReferenceCollector& Collector, bool bStrong);
};

struct FLuaUObjectData
{
	UObject* Object;

	void AddReferencedObjects(UObject* Owner, FReferenceCollector& Collector, bool bStrong);
};


enum class EUEDataType
{
	None,
	Struct,
	StructRef,
	Object,
};

struct FLuaUEData : public TSharedFromThis<FLuaUEData>
{
	union
	{
		FLuaUStructRefData StructRef;
		FLuaUStructData Struct;
		FLuaUObjectData Object;
	} Data;
	EUEDataType DataType = EUEDataType::None;

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
			break;
		default:
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
public:

	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable)
	LUASOURCE_API void Init();

	UFUNCTION(BlueprintCallable)
	LUASOURCE_API void Finalize();

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
