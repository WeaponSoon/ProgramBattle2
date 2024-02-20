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
	void luaC_foreachgcobj(lua_State* L, void(*cb)(GCObject*,bool,lua_State*));
	
}


UCLASS()
class LUASOURCE_API ULuaState : public UObject
{
	GENERATED_BODY()

};



struct FGlobalLuaVM
{
	lua_State* L = nullptr;
};

class LUASOURCE_API FLuaSourceModule : public IModuleInterface
{
public:
	static FGlobalLuaVM GLuaState;
	static FOnLuaLoadFile OnLuaLoadFile;
public:

	static lua_State* GetState();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void* LuaMalloc(void* ud, void* ptr, size_t osize, size_t nsize);

};
