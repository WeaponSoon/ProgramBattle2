// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "lua.hpp"
#include "Modules/ModuleManager.h"
#include "LuaSource.generated.h"

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
	static FGlobalLuaVM GLuaState;
public:

	static lua_State* GetState();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void* LuaMalloc(void* ud, void* ptr, size_t osize, size_t nsize);

};
