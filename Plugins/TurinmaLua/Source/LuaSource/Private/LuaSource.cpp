// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuaSource.h"
#include "lua.hpp"

#define LOCTEXT_NAMESPACE "FLuaSourceModule"

//FLuaStateHandle FLuaSourceStateManager::CreateState()
//{
//    //int32 StateSlotID = -1;
//    //if(UnusedStates.Num() == 0)
//    //{
//    //    StateSlotID = UnusedStates.Pop();
//    //}
//    //else
//    //{
//    //    StateSlotID = AllStates.Num();
//    //    AllStates.AddZeroed();
//    //}
//
//    //auto&& State = AllStates[StateSlotID].Get<0>();
//    //AllStates[StateSlotID].Get<1>() = ++CurrentCheckId;
//
//    //State = lua_newstate(&FLuaSourceModule::LuaMalloc, nullptr);
//    //luaL_openlibs(State);
//    ////lua_register(State, "require", Global_Require);
//}

FGlobalLuaVM FLuaSourceModule::GLuaState;

lua_State* FLuaSourceModule::GetState()
{
    return GLuaState.L;
}

void AddUsefulLuaTableWithMetatableToTable(const char* tableId, int32 Index, bool Weak)
{
    auto* L = FLuaSourceModule::GetState();

    
    lua_pushstring(L, tableId);// "..."
    
    lua_newtable(L);//"...", table

    lua_newtable(L);//"...", table, metatable
    if(Weak)
    {
        lua_pushstring(L, "kv"); //"...", table, metatable, "kv"
        lua_setfield(L, -2, "__mode");//"...", table, metatable    
    }

    lua_setmetatable(L, -2);// "...", table

    lua_rawset(L, Index);//

    //lua_pop(L, 1);
}

void FLuaSourceModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
    GLuaState.L = lua_newstate(&FLuaSourceModule::LuaMalloc, nullptr);
    luaL_openlibs(GLuaState.L);
    lua_gc(GLuaState.L, LUA_GCSTOP);
   
    AddUsefulLuaTableWithMetatableToTable("InternalUseOnly___*___UObjectWeakTable", LUA_REGISTRYINDEX, true);
    AddUsefulLuaTableWithMetatableToTable("InternalUseOnly___*___UObjectStrongTable", LUA_REGISTRYINDEX, false);


    lua_gc(GLuaState.L, LUA_GCCOLLECT);

    FCoreUObjectDelegates::GetPostGarbageCollect();



}

void FLuaSourceModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void* FLuaSourceModule::LuaMalloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    if (nsize == 0)
    {
#if STATS
        const uint32 Size = FMemory::GetAllocSize(ptr);
        
#endif
        FMemory::Free(ptr);
        return nullptr;
    }

    void* Buffer = nullptr;
    if (!ptr)
    {
        Buffer = FMemory::Malloc(nsize);
#if STATS
        const uint32 Size = FMemory::GetAllocSize(Buffer);
        
#endif
    }
    else
    {
#if STATS
        const uint32 OldSize = FMemory::GetAllocSize(ptr);
#endif
        Buffer = FMemory::Realloc(ptr, nsize);
#if STATS
        const uint32 NewSize = FMemory::GetAllocSize(Buffer);
        if (NewSize > OldSize)
        {
            
        }
        else
        {
            
        }
#endif
    }
    return Buffer;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLuaSourceModule, LuaSource)