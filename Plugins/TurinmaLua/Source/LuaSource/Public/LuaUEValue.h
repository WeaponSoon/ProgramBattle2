#pragma once
#include "CoreMinimal.h"
#include "LuaUEType.h"
#include "Modules/ModuleManager.h"
#include "CustomMemoryHandle.h"

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

	mutable uint64 ValueVersion = 0;

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
		if (!IsPartOfOthers() && GetData())
		{
			TypeDesc->DestroyValue(GetData());
		}
		bHasDestroyedData = true;
	}

	bool IsHasValidData() const
	{
		if (bHasData && !bHasDestroyedData && TypeDesc.IsValid() && TypeDesc->GetSize() > 0)
		{
			if (IsPartOfOthers())
			{
				return Owner && ValueVersion >= Owner->ValueVersion && Owner->IsHasValidData();
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

	void InitDataAsPartOfOther(void* PartOfOther, uint64 OwnerVersion)
	{
		if (!bHasData && TypeDesc.IsValid() && TypeDesc->GetSize() > 0)
		{
			//void* ObjectMem = FMemory::Malloc(sizeof(void*));
			*(void**)(&InnerData) = PartOfOther;
			bHasData = true;
			ValueVersion = OwnerVersion;
		}
	}

	void InitData()
	{
		if (!bHasData && TypeDesc.IsValid() && TypeDesc->GetSize() > 0)
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
		if (GetData() != nullptr)
		{
			TypeDesc->CopyValue(GetData(), Other);
		}
	}

	void CopyFrom(const FLuaUEValue& Other)
	{
		if (GetData() && Other.GetData() && TypeDesc == Other.TypeDesc)
		{
			if (GetData() == Other.GetData())
			{
				return;
			}
			TypeDesc->CopyValue(GetData(), (void*)Other.GetData());
		}
	}

	void ToLua(lua_State* L)
	{
		if (IsHasValidData())
		{
			TypeDesc->ToLuaValue(L, GetData(), *this);
		}
	}

	void AddReferencedObject(UObject* Oter, FReferenceCollector& Collector, bool bStrong);

};