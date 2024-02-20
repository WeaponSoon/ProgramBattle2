// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuaSource.h"
#include "lua.hpp"
#include <string>
#include <unordered_map>




EXTERN_C {
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"
#include "lstring.h"
}


#define twoto_l(x)	(1ll<<(x))
#define sizenode_l(t)	(twoto_l((t)->lsizenode))
#define gnodelast(h)	gnode(h, cast_sizet(sizenode_l(h)))

#define eqshrstr(a,b)	check_exp((a)->tt == LUA_VSHRSTR, (a) == (b))

#define hashpow2(t,n)		(gnode(t, lmod((n), sizenode(t))))

#define LOCTEXT_NAMESPACE "FLuaSourceModule"



UE_DISABLE_OPTIMIZATION





static void travelluaobj(global_State* g, GCObject* o, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong);



/*
** Traverse a table with weak values and link it to proper list. During
** propagate phase, keep it in 'grayagain' list, to be revisited in the
** atomic phase. In the atomic phase, if table has any white value,
** put it in 'weak' list, to be cleared.
*/
static void traverseweakvalue(global_State* g, Table* h, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {


    unsigned int asize = luaH_realasize(h);
    
    /* traverse array part */
    for (unsigned int i = 0; i < asize; i++) {
        auto arrayEle = &h->array[i];
        if(iscollectable(arrayEle))
        {
            travelluaobj(g, gcvalue(arrayEle), cb, visited, false);
        }
    }

    Node* n, * limit = gnodelast(h);
    for (n = gnode(h, 0); n < limit; n++) {  /* traverse hash part */
        auto Key = gckeyN(n);
        auto Value = gval(n);
        if(Key)
        {
            travelluaobj(g, Key, cb, visited, strong);
        }
        if (iscollectable(Value))
        {
            travelluaobj(g,gcvalue(Value), cb, visited, false);
           
        }  /* entry is empty? */
    }
}


static void traverseallweaktable(global_State* g, Table* h, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {


    unsigned int asize = luaH_realasize(h);

    /* traverse array part */
    for (unsigned int i = 0; i < asize; i++) {
        auto arrayEle = &h->array[i];
        if (iscollectable(arrayEle))
        {
            travelluaobj(g, gcvalue(arrayEle), cb, visited, false);
        }
    }

    Node* n, * limit = gnodelast(h);
    for (n = gnode(h, 0); n < limit; n++) {  /* traverse hash part */
        auto Key = gckeyN(n);
        auto Value = gval(n);
        if (Key)
        {
            travelluaobj(g, Key, cb, visited, false);
        }
        if (iscollectable(Value))
        {
            travelluaobj(g, gcvalue(Value), cb, visited, false);

        }  /* entry is empty? */
    }
}


static void traverseweakkey(global_State* g, Table* h, int inv, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {


    int marked = 0;  /* true if an object is marked in this traversal */
    unsigned int i;
    unsigned int asize = luaH_realasize(h);
    unsigned int nsize = sizenode(h);
    /* traverse array part */
    for (i = 0; i < asize; i++) {
        auto arrayEle = &h->array[i];
        if (iscollectable(arrayEle))
        {
            travelluaobj(g, gcvalue(arrayEle), cb, visited, strong);
        }
    }
    /* traverse hash part; if 'inv', traverse descending
       (see 'convergeephemerons') */
    for (i = 0; i < nsize; i++) {
        Node* n = inv ? gnode(h, nsize - 1 - i) : gnode(h, i);
        auto Key = gckeyN(n);
        auto Value = gval(n);
        if (Key)
        {
            travelluaobj(g, Key, cb, visited, false);
        }
        if (iscollectable(Value))
        {
            travelluaobj(g, gcvalue(Value), cb, visited, strong);

        }  /* entry is empty? */
    }
}


static void traversestrongtable(global_State* g, Table* h, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {

    Node* n, * limit = gnodelast(h);
    unsigned int i;
    unsigned int asize = luaH_realasize(h);
    for (i = 0; i < asize; i++)  /* traverse array part */
    {
        auto arrayEle = &h->array[i];
        if (iscollectable(arrayEle))
        {
            travelluaobj(g, gcvalue(arrayEle), cb, visited, strong);
        }
    }
    for (n = gnode(h, 0); n < limit; n++) {  /* traverse hash part */
        auto Key = gckeyN(n);
        auto Value = gval(n);
        if (Key)
        {
            travelluaobj(g, Key, cb, visited, strong);
        }
        if (iscollectable(Value))
        {
            travelluaobj(g, gcvalue(Value), cb, visited, strong);

        }  /* entry is empty? */
    }
}


static void traversetable(global_State* g, Table* h, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {


    auto&& res = visited.emplace(obj2gco(h), strong);

    if (!strong)
    {
        if (!res.second)
        {
            return;
        }
    }
    else
    {
        if (!res.second && res.first->second)
        {
            return;
        }
    }

    if (cb)
    {
        cb(obj2gco(h), strong, g->mainthread);
    }


    const char* weakkey, * weakvalue;
    const TValue* mode = gfasttm(g, h->metatable, TM_MODE);
    travelluaobj(g, obj2gco(h->metatable), cb, visited, strong);
    if (mode && ttisstring(mode) &&  /* is there a weak mode? */
        (cast_void(weakkey = strchr(svalue(mode), 'k')),
            cast_void(weakvalue = strchr(svalue(mode), 'v')),
            (weakkey || weakvalue))) {  /* is really weak? */
        if (!weakkey)  /* strong keys? */
            traverseweakvalue(g, h, cb, visited, strong);
        else if (!weakvalue)  /* strong values? */
            traverseweakkey(g, h, 0, cb, visited, strong);
        else  /* all weak */
            traverseallweaktable(g, h, cb, visited, strong);  /* nothing to traverse now */
    }
    else  /* not weak */
        traversestrongtable(g, h, cb, visited, strong);
}


static void traverseudata(global_State* g, Udata* u, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {

    auto&& res = visited.emplace(obj2gco(u), strong);

    if (!strong)
    {
        if (!res.second)
        {
            return;
        }
    }
    else
    {
        if (!res.second && res.first->second)
        {
            return;
        }
    }

    if (cb)
    {
        cb(obj2gco(u), strong, g->mainthread);
    }


	int i;
    travelluaobj(g, obj2gco(u->metatable), cb, visited, strong);
    for (i = 0; i < u->nuvalue; i++)
    {
        auto tv = &u->uv[i].uv;
        if(iscollectable(tv))
        {
            travelluaobj(g, gcvalue(tv), cb, visited, strong);
        }
    }
}


/*
** Traverse a prototype. (While a prototype is being build, its
** arrays can be larger than needed; the extra slots are filled with
** NULL, so the use of 'markobjectN')
*/
static void traverseproto(global_State* g, Proto* f, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {


    auto&& res = visited.emplace(obj2gco(f), strong);

    if (!strong)
    {
        if (!res.second)
        {
            return;
        }
    }
    else
    {
        if (!res.second && res.first->second)
        {
            return;
        }
    }

    if (cb)
    {
        cb(obj2gco(f), strong, g->mainthread);
    }


    int i;
    travelluaobj(g, obj2gco(f->source), cb, visited, strong);
    for (i = 0; i < f->sizek; i++)  /* mark literals */
    {
        auto tv = &f->k[i];
        if (iscollectable(tv))
        {
            travelluaobj(g, gcvalue(tv), cb, visited, strong);
        }
    }

    for (i = 0; i < f->sizeupvalues; i++)  /* mark upvalue names */
    {
        //markobjectN(g, f->upvalues[i].name);
        travelluaobj(g, obj2gco(f->upvalues[i].name), cb, visited, strong);
    }
        
    for (i = 0; i < f->sizep; i++)  /* mark nested protos */
    {
        //markobjectN(g, f->p[i]);
        travelluaobj(g, obj2gco(f->p[i]), cb, visited, strong);
    }
        
    for (i = 0; i < f->sizelocvars; i++)  /* mark local-variable names */
    {
        //markobjectN(g, f->locvars[i].varname);
        travelluaobj(g, obj2gco(f->locvars[i].varname), cb, visited, strong);
    }

}


static void traverseCclosure(global_State* g, CClosure* cl, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {

    auto&& res = visited.emplace(obj2gco(cl), strong);

    if (!strong)
    {
        if (!res.second)
        {
            return;
        }
    }
    else
    {
        if (!res.second && res.first->second)
        {
            return;
        }
    }

    if (cb)
    {
        cb(obj2gco(cl), strong, g->mainthread);
    }

	int i;
    for (i = 0; i < cl->nupvalues; i++)  /* mark its upvalues */
    {
        //markvalue(g, &cl->upvalue[i]);
        auto tv = &cl->upvalue[i];
        if (iscollectable(tv))
        {
            travelluaobj(g, gcvalue(tv), cb, visited, strong);
        }
    }
}

/*
** Traverse a Lua closure, marking its prototype and its upvalues.
** (Both can be NULL while closure is being created.)
*/
static void traverseLclosure(global_State* g, LClosure* cl, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {

    auto&& res = visited.emplace(obj2gco(cl), strong);

    if (!strong)
    {
        if (!res.second)
        {
            return;
        }
    }
    else
    {
        if (!res.second && res.first->second)
        {
            return;
        }
    }

    if (cb)
    {
        cb(obj2gco(cl), strong, g->mainthread);
    }


	int i;
    travelluaobj(g, obj2gco(cl->p), cb, visited, strong);
    for (i = 0; i < cl->nupvalues; i++) {  /* visit its upvalues */
        UpVal* uv = cl->upvals[i];
        travelluaobj(g, obj2gco(uv), cb, visited, strong);
    }
}


/*
** Traverse a thread, marking the elements in the stack up to its top
** and cleaning the rest of the stack in the final traversal. That
** ensures that the entire stack have valid (non-dead) objects.
** Threads have no barriers. In gen. mode, old threads must be visited
** at every cycle, because they might point to young objects.  In inc.
** mode, the thread can still be modified before the end of the cycle,
** and therefore it must be visited again in the atomic phase. To ensure
** these visits, threads must return to a gray list if they are not new
** (which can only happen in generational mode) or if the traverse is in
** the propagate phase (which can only happen in incremental mode).
*/
static void traversethread(global_State* g, lua_State* th, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {


    auto&& res = visited.emplace(obj2gco(th), strong);

    if (!strong)
    {
        if (!res.second)
        {
            return;
        }
    }
    else
    {
        if (!res.second && res.first->second)
        {
            return;
        }
    }

    if (cb)
    {
        cb(obj2gco(th), strong, g->mainthread);
    }

    UpVal* uv;
    StkId o = th->stack.p;
    
    if (o == NULL)
        return;  /* stack not completely built yet */
    
    for (; o < th->top.p; o++)  /* mark live elements in the stack */
    {
        //markvalue(g, s2v(o));
        auto tv = s2v(o);
        if (iscollectable(tv))
        {
            travelluaobj(g, gcvalue(tv), cb, visited, strong);
        }
    }
        
    for (uv = th->openupval; uv != NULL; uv = uv->u.open.next)
    {
        //markobject(g, uv);  /* open upvalues cannot be collected */
        travelluaobj(g, obj2gco(uv), cb, visited, strong);
    }
}


/*
** traverse one gray object, turning it to black.
*/
static void travelluaobj(global_State* g, GCObject* o, void(*cb)(GCObject*, bool, lua_State*), std::unordered_map<GCObject*, bool>& visited, bool strong) {
    if(!o)
    {
        return;
    }

    switch (o->tt) {
    case LUA_VTABLE: return traversetable(g, gco2t(o), cb, visited, strong);
    case LUA_VUSERDATA: return traverseudata(g, gco2u(o), cb, visited, strong);
    case LUA_VLCL: return traverseLclosure(g, gco2lcl(o), cb, visited, strong);
    case LUA_VCCL: return traverseCclosure(g, gco2ccl(o), cb, visited, strong);
    case LUA_VPROTO: return traverseproto(g, gco2p(o), cb, visited, strong);
    case LUA_VTHREAD: return traversethread(g, gco2th(o), cb, visited, strong);
    default:
	    {
        auto&& res = visited.emplace(o, strong);

        if (!strong)
        {
            if (!res.second)
            {
                return;
            }
        }
        else
        {
            if (!res.second && res.first->second)
            {
                return;
            }
        }

        if (cb)
        {
            cb(o, strong, g->mainthread);
        }
	    }
    }
}















void LuaCPPAPI::luaC_foreachgcobj(lua_State* L, void(*cb)(GCObject*,bool, lua_State*))
{
    if(cb)
    {
        global_State* g = L->l_G;
        std::unordered_map<GCObject*, bool> ObjectHasVisit;

        travelluaobj(g, obj2gco(g->mainthread), cb, ObjectHasVisit, true);
        travelluaobj(g, gcvalue(&g->l_registry), cb, ObjectHasVisit, true);
        for (int i = 0; i < LUA_NUMTAGS; i++)
        {
            if (g->mt[i])
            {
                travelluaobj(g, obj2gco(g->mt[i]), cb, ObjectHasVisit, true);
            }
        }
        for (auto o = g->tobefnz; o != NULL; o = o->next) {
            
            travelluaobj(g, o, cb, ObjectHasVisit, true);
        }
    }
}


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

void AddUsefulLuaTableWithMetatableToTable(const char* tableId, int32 Index, bool Weak, const char* metatableName = nullptr)
{
    auto* L = FLuaSourceModule::GetState();

    
    lua_pushstring(L, tableId);// "..."
    
    lua_newtable(L);//"...", table



    lua_newtable(L);//"...", table, metatable
    if(metatableName)
    {
        lua_pushstring(L, metatableName);
        lua_setfield(L, -2, "__name");
    }

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
unsigned int HashLuaString(lua_State* L, const char* str)
{
    auto l = strlen(str);
    //if (l <= LUAI_MAXSHORTLEN)  /* short string? */
    {
        return luaS_hash(str, l, G(L)->seed);
    }
    //else 
    //{
    //    
    //    unsigned int hash = G(L)->seed;
    //        ts->hash = luaS_hash(getstr(ts), len, ts->hash);
    //        ts->extra = 1;  /* now it has its hash */
    //    
    //}

}

const char* GetMetatableName(lua_State* L, Table* metaTable)
{
    auto hash = HashLuaString(L, "__name");
    auto n = hashpow2(metaTable, hash);

    for (;;) {  /* check whether 'key' is somewhere in the chain */
        if (keyisshrstr(n) && strcmp(getstr(keystrval(n)), "__name") == 0)
        {
            auto value = gval(n);
            if (ttisstring(value))
            {
                auto tsv = tsvalue(value);
                return (getstr(tsv));
            }
            break;
        }
        else {
            int nx = gnext(n);
            if (nx == 0)
                break;
            n += nx;
        }
    }
    return nullptr;
}

void OnTravelLuaGCObject(GCObject* o, bool strong, lua_State* L)
{
    switch (o->tt) {
    case LUA_VSHRSTR:
    case LUA_VLNGSTR: {
        auto ts = gco2ts(o);
        FString str = UTF8_TO_TCHAR(getstr(ts));
        UE_LOG(LogTemp, Log, TEXT("Strong: %d, Str: %s"), strong, *str);
        break;
    }
    case LUA_VUPVAL: {
        //UpVal* uv = gco2upv(o);

        break;
    }
    case LUA_VUSERDATA: {
        Udata* u = gco2u(o);
        if (u->nuvalue == 0) {  /* no user values? */
            
        }
        else if (u->metatable)
        {
            auto metatableName = GetMetatableName(L, u->metatable);
            if(metatableName)
            {
                FString str = UTF8_TO_TCHAR(metatableName);
                UE_LOG(LogTemp, Log, TEXT("Strong: %d, type name: %s"), strong, *str);
            }
        }
        break;
    }
    case LUA_VLCL: break; case LUA_VCCL:break;
    case LUA_VTABLE:
    {
        Table* u = gco2t(o);
		if (u->metatable)
        {
            auto metatableName = GetMetatableName(L, u->metatable);
            if (metatableName)
            {
                FString str = UTF8_TO_TCHAR(metatableName);
                UE_LOG(LogTemp, Log, TEXT("Strong: %d, type name: %s"), strong, *str);
            }
        }
        break;
    }
    case LUA_VTHREAD: case LUA_VPROTO: {
        break;
    }
    default: lua_assert(0); break;
    
    }
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
	LuaCPPAPI::luaC_foreachgcobj(GLuaState.L, OnTravelLuaGCObject);

    FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddLambda([]()->void
    {
            LuaCPPAPI::luaC_foreachgcobj(GLuaState.L, OnTravelLuaGCObject);
    });
    
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


UE_ENABLE_OPTIMIZATION
