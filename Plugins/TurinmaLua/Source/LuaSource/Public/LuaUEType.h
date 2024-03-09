#pragma once
#include "CoreMinimal.h"
#include "lua.hpp"
#include "Modules/ModuleManager.h"
#include "CustomMemoryHandle.h"
#include "LuaUEType.generated.h"

EXTERN_C
{
	struct GCObject;
}
namespace LuaCPPAPI
{
	void luaC_foreachgcobj(lua_State* L, TFunction<void(GCObject*, bool, lua_State*)> cb);

}


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
	Enum,
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

	//lua operators
	LUASOURCE_API int ToLuaValue(lua_State* L, void* ValueAddress, struct FLuaUEValue& ValueProxy) const; //lua stakc -0 +1
	//LUASOURCE_API int IndexLuaByStr(lua_State* L, void* ValueAddress, struct FLuaUEValue& ValueProxy, const char* Str) const; //lua stakc -0 +1
	//LUASOURCE_API int IndexLuaByIndex(lua_State* L, void* ValueAddress, struct FLuaUEValue& ValueProxy, const char* Index) const; //lua stakc -0 +1
	//LUASOURCE_API int IndexLuaByValue(lua_State* L, void* ValueAddress, struct FLuaUEValue& ValueProxy, const char* KeyValueProxy) const; //lua stakc -0 +1

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