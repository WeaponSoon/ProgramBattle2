// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuaSource.h"
#include "lua.hpp"

#include <string>

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
FOnLuaLoadFile FLuaSourceModule::OnLuaLoadFile;

static int CustomLoader(lua_State* L)
{
    const char* ModuleName = luaL_checkstring(L, 1);
    FString ModuleNameStr = UTF8_TO_TCHAR(ModuleName);
    if (ModuleNameStr.IsEmpty())
    {
        lua_pushstring(L, "module name is empty");
        return 1; // File not found
    }

    FString Result;
    bool bLoadSuc = false;
    if(FLuaSourceModule::OnLuaLoadFile.IsBound())
    {
        bLoadSuc = FLuaSourceModule::OnLuaLoadFile.Execute(ModuleNameStr, Result);
        if(!bLoadSuc)
        {
            lua_pushstring(L, "module load failed");
            return 1; // File not found
        }
    }
    else
    {
        ModuleNameStr.ReplaceCharInline(TEXT('.'), TEXT('/'));
        if(!ModuleNameStr.StartsWith(TEXT("/" )))
        {
            ModuleNameStr.InsertAt(0, TEXT('/'));
        }
        FString FileName;
        if(!FPackageName::TryConvertLongPackageNameToFilename(ModuleNameStr, FileName, TEXT(".lua")))
        {
            ModuleNameStr.InsertAt(0, TEXT("/Game"));
	        if(!FPackageName::TryConvertLongPackageNameToFilename(ModuleNameStr, FileName, TEXT(".lua")))
	        {
                lua_pushstring(L, "module name is invalid");
                return 1; // File not found
	        }
        }
        
        if(!FFileHelper::LoadFileToString(Result, *FileName))
        {
            lua_pushstring(L, "module load failed");
            return 1; // File not found
        }
    }

    std::string utf8Res = TCHAR_TO_UTF8(*Result);

    int status = luaL_loadbuffer(L, utf8Res.c_str(), utf8Res.size(), ModuleName);
    if (status != LUA_OK)
    {
        lua_error(L);
        lua_pushstring(L, "module load failed");
        return 1; // File not found
    }

    lua_pushstring(L, ModuleName); // Push the file path onto the stack
    return 2;
}

void RegisterCustomLoader(lua_State* L)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");

    lua_pushinteger(L, 2);
    lua_pushcfunction(L, CustomLoader);
    lua_settable(L, -3);

    lua_pop(L, 2); // Pop 'searchers' and 'package' tables
}


void FLuaSourceModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    FOnLuaLoadFile Temp = OnLuaLoadFile;
    OnLuaLoadFile.Unbind();

    GLuaState.L = lua_newstate(&FLuaSourceModule::LuaMalloc, nullptr);
    lua_gc(GLuaState.L, LUA_GCSTOP);
    luaL_openlibs(GLuaState.L);

    RegisterCustomLoader(GLuaState.L);

    lua_getglobal(GLuaState.L, "require");
    lua_pushstring(GLuaState.L, "TurinmaLua.Core.Init");
    lua_call(GLuaState.L, 1, LUA_MULTRET);

    lua_pop(GLuaState.L, lua_gettop(GLuaState.L));


    AddUsefulLuaTableWithMetatableToTable("InternalUseOnly___*___UObjectExtensionWeakTable", LUA_REGISTRYINDEX, true);
    AddUsefulLuaTableWithMetatableToTable("InternalUseOnly___*___UObjectExtensionStrongTable", LUA_REGISTRYINDEX, false);
    lua_pop(GLuaState.L, lua_gettop(GLuaState.L));

    
    FCoreUObjectDelegates::GetPostGarbageCollect();

    OnLuaLoadFile = Temp;
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