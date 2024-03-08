// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuaSource.h"
#include "lua.hpp"
#include <string>
#include <thread>
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





static void travelluaobj(global_State* g, GCObject* o, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong);



/*
** Traverse a table with weak values and link it to proper list. During
** propagate phase, keep it in 'grayagain' list, to be revisited in the
** atomic phase. In the atomic phase, if table has any white value,
** put it in 'weak' list, to be cleared.
*/
static void traverseweakvalue(global_State* g, Table* h, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {


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


static void traverseallweaktable(global_State* g, Table* h, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {


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


static void traverseweakkey(global_State* g, Table* h, int inv, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {


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


static void traversestrongtable(global_State* g, Table* h, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {

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


static void traversetable(global_State* g, Table* h, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {


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


static void traverseudata(global_State* g, Udata* u, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {

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
static void traverseproto(global_State* g, Proto* f, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {


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


static void traverseCclosure(global_State* g, CClosure* cl, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {

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
static void traverseLclosure(global_State* g, LClosure* cl, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {

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
static void traversethread(global_State* g, lua_State* th, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {


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
static void travelluaobj(global_State* g, GCObject* o, TFunction<void(GCObject*, bool, lua_State*)>& cb, std::unordered_map<GCObject*, bool>& visited, bool strong) {
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


void LuaCPPAPI::luaC_foreachgcobj(lua_State* L, TFunction<void(GCObject*, bool, lua_State*)> cb)
{
    if(cb)
    {
        lua_lock(L);
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
        lua_unlock(L);
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


void AddUsefulLuaTableWithMetatableToTable(lua_State* L, const char* tableId, int32 Index, bool Weak, const char* metatableName = nullptr)
{
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

static int CustomLoaderInternal(lua_State* L, bool bForceDefaultLoader)
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
    if (!bForceDefaultLoader && FLuaSourceModule::OnLuaLoadFile.IsBound())
    {
        bLoadSuc = FLuaSourceModule::OnLuaLoadFile.Execute(ModuleNameStr, Result);
        if (!bLoadSuc)
        {
            lua_pushstring(L, "module load failed");
            return 1; // File not found
        }
    }
    else
    {
        ModuleNameStr.ReplaceCharInline(TEXT('.'), TEXT('/'));
        if (!ModuleNameStr.StartsWith(TEXT("/")))
        {
            ModuleNameStr.InsertAt(0, TEXT('/'));
        }
        FString FileName;
        if (!FPackageName::TryConvertLongPackageNameToFilename(ModuleNameStr, FileName, TEXT(".lua")))
        {
            ModuleNameStr.InsertAt(0, TEXT("/Game"));
            if (!FPackageName::TryConvertLongPackageNameToFilename(ModuleNameStr, FileName, TEXT(".lua")))
            {
                lua_pushstring(L, "module name is invalid");
                return 1; // File not found
            }
        }

        if (!FFileHelper::LoadFileToString(Result, *FileName))
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

static int CustomLoader(lua_State* L)
{
    return CustomLoaderInternal(L, false);
}

static int CustomDefaultLoader(lua_State* L)
{
    return CustomLoaderInternal(L, true);
}

void RegisterCustomLoader(lua_State* L, bool bDefualt)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");

    lua_pushinteger(L, 2);
    lua_pushcfunction(L, bDefualt ? CustomDefaultLoader : CustomLoader);
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


void UUETypeDescContainer::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
    auto* This = CastChecked<UUETypeDescContainer>(InThis);
    FScopeLock Lock(&This->CS);
    for(auto&& It = This->Map.CreateIterator(); It; ++It)
    {
	    if(It->Value.GetRefCount() == 1)
	    {
            Collector.MarkWeakObjectReferenceForClearing(&It->Value->UserDefinedTypePointer.Pointer);
	    }
        else
        {
            Collector.AddReferencedObject(It->Value->UserDefinedTypePointer.Pointer);
        }
    }
}

void FLuaUEValue::AddReferencedObject(UObject* Oter, FReferenceCollector& Collector, bool bStrong)
{
    if(bStrong)
    {
        Marked = true;
    }
	if(TypeDesc.IsValid() && !Owner)
	{
        TypeDesc->AddReferencedObject(Oter, GetData(), Collector, bStrong);
	}
}

void FTypeDesc::AddReferencedObject(UObject* Oter, void* PtrToValue, FReferenceCollector& Collector, bool bStrong)
{
    switch (Kind)
    {
    case ETypeKind::None: return;


    case ETypeKind::Int8:;
    case ETypeKind::Int16:;
    case ETypeKind::Int32:
    case ETypeKind::Int64:
    case ETypeKind::Byte:
    case ETypeKind::UInt16:
    case ETypeKind::UInt32:
    case ETypeKind::UInt64:
    case ETypeKind::Float:
    case ETypeKind::Double:

    case ETypeKind::Bool:
    case ETypeKind::BitBool:
        return;
    case ETypeKind::String: return;
    case ETypeKind::Name: return;
    case ETypeKind::Text: return;
    case ETypeKind::Vector: return;
    case ETypeKind::Rotator: return;
    case ETypeKind::Quat:return;
    case ETypeKind::Transform: return;
    case ETypeKind::Matrix: return;
    case ETypeKind::RotationMatrix: return;
    case ETypeKind::Color: return;
    case ETypeKind::LinearColor: return;


    case ETypeKind::Array:
	    {
			FScriptArray* ArrayPtr = (FScriptArray*)PtrToValue;
            if(CombineKindSubTypes.Num() == 1 && CombineKindSubTypes[0].IsValid())
            {
	            for(int Ai = 0; Ai < ArrayPtr->Num(); ++Ai)
	            {
                    CombineKindSubTypes[0]->AddReferencedObject(Oter, (uint8*)ArrayPtr->GetData() + CombineKindSubTypes[0]->GetSize() * Ai, Collector, bStrong);
	            }
            }
	    }
        return;
    case ETypeKind::Map:
	    {
        FScriptMap* MapPtr = (FScriptMap*)PtrToValue;
        if (CombineKindSubTypes.Num() == 2 && CombineKindSubTypes[0].IsValid() && CombineKindSubTypes[1].IsValid())
        {
            auto Layout = FScriptMap::GetScriptLayout(CombineKindSubTypes[0]->GetSize(), CombineKindSubTypes[0]->GetAlign(),
                CombineKindSubTypes[1]->GetSize(), CombineKindSubTypes[1]->GetAlign());
            for (int Ai = 0; Ai < MapPtr->Num(); ++Ai)
            {
                auto PairPtr = MapPtr->GetData(Ai, Layout);

                CombineKindSubTypes[0]->AddReferencedObject(Oter, (uint8*)PairPtr, Collector, bStrong);
                CombineKindSubTypes[1]->AddReferencedObject(Oter, (uint8*)PairPtr + Layout.ValueOffset, Collector, bStrong);
            }
        }
	    }
        return;
    case ETypeKind::Set:
	    {
    	FScriptSet* SetPtr = (FScriptSet*)PtrToValue;
        if (CombineKindSubTypes.Num() == 1 && CombineKindSubTypes[0].IsValid())
        {
            auto Layout = FScriptSet::GetScriptLayout(CombineKindSubTypes[0]->GetSize(), CombineKindSubTypes[0]->GetAlign());
            for (int Ai = 0; Ai < SetPtr->Num(); ++Ai)
            {
                auto PairPtr = SetPtr->GetData(Ai, Layout);

                CombineKindSubTypes[0]->AddReferencedObject(Oter, (uint8*)PairPtr, Collector, bStrong);
            }
        }
	    }
        return;
    case ETypeKind::Delegate: return;
    case ETypeKind::MulticastDelegate: return;
    case ETypeKind::WeakObject: return;

    case ETypeKind::U_Enum: return;
    case ETypeKind::U_ScriptStruct:
        if (UserDefinedTypePointer.U_ScriptStruct)
        {
            Collector.AddPropertyReferences(UserDefinedTypePointer.U_ScriptStruct, PtrToValue, Oter);
        }
    	return;
    case ETypeKind::U_Class:
	    {
		    if(bStrong)
		    {
                Collector.AddReferencedObject(*(UObject**)PtrToValue, Oter);
		    }
            else
            {
                Collector.MarkWeakObjectReferenceForClearing((UObject**)PtrToValue);
            }
	    }
        return;
    case ETypeKind::U_Function:
	    {
			Collector.AddReferencedObject(UserDefinedTypePointer.U_Function, Oter);
	    }
        return;
    case ETypeKind::U_Interface:
	    {
			Collector.AddReferencedObject(((FScriptInterface*)PtrToValue)->GetObjectRef(), Oter);
	    }
        return;
    default: return;
    }
}

int32 FTypeDesc::GetSize() const
{
    switch(Kind)
    {
    case ETypeKind::U_ScriptStruct: return UserDefinedTypePointer.U_ScriptStruct ? UserDefinedTypePointer.U_ScriptStruct->GetStructureSize() : 0;
    default: return GetSizePreDefinedKind(Kind);
    }
}

int32 FTypeDesc::GetAlign() const
{
    switch (Kind)
    {
    case ETypeKind::None: return 0;


    case ETypeKind::Int8: return alignof(int8);
    case ETypeKind::Int16: return alignof(int16);
    case ETypeKind::Int32: return alignof(int32);
    case ETypeKind::Int64: return alignof(int64);
    case ETypeKind::Byte: return alignof(uint8);
    case ETypeKind::UInt16: return alignof(uint16);
    case ETypeKind::UInt32: return alignof(uint32);
    case ETypeKind::UInt64: return alignof(uint64);
    case ETypeKind::Float: return alignof(float);
    case ETypeKind::Double: return alignof(double);

    case ETypeKind::Bool: return alignof(bool);
    case ETypeKind::BitBool: return 0;
    case ETypeKind::String: return alignof(FString);
    case ETypeKind::Name: return alignof(FName);
    case ETypeKind::Text: return alignof(FText);
    case ETypeKind::Vector: return alignof(FVector);
    case ETypeKind::Rotator: return alignof(FRotator);
    case ETypeKind::Quat: return alignof(FQuat);
    case ETypeKind::Transform: return alignof(FTransform);
    case ETypeKind::Matrix: return alignof(FMatrix);
    case ETypeKind::RotationMatrix: return alignof(FRotationMatrix);
    case ETypeKind::Color: return alignof(FColor);
    case ETypeKind::LinearColor: return alignof(FLinearColor);


    case ETypeKind::Array: return alignof(FScriptArray);
    case ETypeKind::Map: return alignof(FScriptMap);
    case ETypeKind::Set: return alignof(FScriptSet);
    case ETypeKind::Delegate: return alignof(FScriptDelegate);
    case ETypeKind::MulticastDelegate: return alignof(FMulticastScriptDelegate);
    case ETypeKind::WeakObject: return alignof(FWeakObjectPtr);

    case ETypeKind::U_Enum: return alignof(uint8);
    case ETypeKind::U_ScriptStruct: return UserDefinedTypePointer.U_ScriptStruct ? UserDefinedTypePointer.U_ScriptStruct->GetMinAlignment() : 0;
    case ETypeKind::U_Class: return alignof(UObject*);
    case ETypeKind::U_Function: return alignof(UObject*);
    case ETypeKind::U_Interface:return alignof(FScriptInterface);
    default: return alignof(max_align_t);
    }
}

void FTypeDesc::InitValue(void* ValueAddress) const
{
    switch (Kind)
    {
    case ETypeKind::None: return;


    case ETypeKind::Int8: new (ValueAddress) int8(0); return;
    case ETypeKind::Int16: new (ValueAddress) int16(0); return;
    case ETypeKind::Int32: new (ValueAddress) int32(0); return;
    case ETypeKind::Int64: new (ValueAddress) int64(0); return;
    case ETypeKind::Byte: new (ValueAddress) uint8(0); return;
    case ETypeKind::UInt16: new (ValueAddress) uint16(0); return;
    case ETypeKind::UInt32: new (ValueAddress) uint32(0); return;
    case ETypeKind::UInt64: new (ValueAddress) uint64(0); return;
    case ETypeKind::Float: new (ValueAddress) float(0); return;
    case ETypeKind::Double: new (ValueAddress) double(0); return;

    case ETypeKind::Bool: new (ValueAddress) bool(false); return;
    case ETypeKind::BitBool: return;
    case ETypeKind::String: new(ValueAddress) FString(); return;
    case ETypeKind::Name: new (ValueAddress) FName(); return;
    case ETypeKind::Text: new (ValueAddress) FText(); return;
    case ETypeKind::Vector: new (ValueAddress) FVector(); return;
    case ETypeKind::Rotator: new (ValueAddress) FRotator(); return;
    case ETypeKind::Quat: new (ValueAddress) FQuat(); return;
    case ETypeKind::Transform: new (ValueAddress) FTransform(); return;
    case ETypeKind::Matrix: new (ValueAddress) FMatrix(); return;
    case ETypeKind::RotationMatrix: new (ValueAddress) FRotationMatrix(FRotator()); return;
    case ETypeKind::Color: new (ValueAddress) FColor(0); return;
    case ETypeKind::LinearColor:new (ValueAddress) FLinearColor(); return;


    case ETypeKind::Array: new (ValueAddress) FScriptArray(); return;
    case ETypeKind::Map: new (ValueAddress) FScriptMap(); return;
    case ETypeKind::Set: new (ValueAddress) FScriptSet(); return;
    case ETypeKind::Delegate: new (ValueAddress) FScriptDelegate(); return;
    case ETypeKind::MulticastDelegate: new (ValueAddress) FMulticastScriptDelegate(); return;
    case ETypeKind::WeakObject: new (ValueAddress) FWeakObjectPtr(); return;

    case ETypeKind::U_Enum: new (ValueAddress) int8(0); return;
    case ETypeKind::U_ScriptStruct: 
        if (UserDefinedTypePointer.U_ScriptStruct) { UserDefinedTypePointer.U_ScriptStruct->InitializeDefaultValue((uint8*)ValueAddress); } return;
    case ETypeKind::U_Class: new (ValueAddress) UObject* (nullptr); return;
    case ETypeKind::U_Function: return;
    case ETypeKind::U_Interface: new (ValueAddress) FScriptInterface();
    default: return;
    }
}

template<typename T>
uint32 RawGetValueHash(void* Address)
{
    return GetTypeHash(*(T*)Address);
}

template<typename T>
bool RawCompairValue(void* Dest, void* Source)
{
    return *(T*)Dest == *(T*)Source;
}
template<>
bool RawCompairValue<FTransform>(void* Dest, void* Source)
{
    return ((FTransform*)Dest)->Identical((FTransform*)Source, 0);
}
//todo not finish yet
bool RawCompairArray(void* Dest, void* Source, TRefCountPtr<FTypeDesc> ElementType)
{
    auto ADe = (FScriptArray*)Dest;
    auto ASo = (FScriptArray*)Source;

    if(ADe->Num() != ASo->Num())
    {
        return false;
    }
    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        if(!ElementType->ValueEqual((uint8*)ADe->GetData() + ElementType->GetSize() * Di, (uint8*)ASo->GetData() + ElementType->GetSize() * Di))
        {
            return false;
        }
    }
    return true;
}
//todo not finish yet
bool RawCompairSet(void* Dest, void* Source, TRefCountPtr<FTypeDesc> ElementType)
{
    auto ADe = (FScriptSet*)Dest;
    auto ASo = (FScriptSet*)Source;
    if (ADe->Num() != ASo->Num())
    {
        return false;
    }

    auto Layout = FScriptSet::GetScriptLayout(ElementType->GetSize(), ElementType->GetAlign());

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        auto PairPtr1 = ADe->GetData(Di, Layout);
        auto PairPtr2 = ASo->GetData(Di, Layout);
        if(!ElementType->ValueEqual(PairPtr1, PairPtr2))
        {
            return false;
        }
    }
    return true;
}
//todo not finish yet
bool RawCompairMap(void* Dest, void* Source, TRefCountPtr<FTypeDesc> ElementType, TRefCountPtr<FTypeDesc> ValueType)
{
    auto ADe = (FScriptMap*)Dest;
    auto ASo = (FScriptMap*)Source;

    auto Layout = FScriptMap::GetScriptLayout(ElementType->GetSize(), ElementType->GetAlign(), ValueType->GetSize(), ValueType->GetAlign());

    if(ADe->Num() != ASo->Num())
    {
        return false;
    }

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        auto PairPtr1 = ADe->GetData(Di, Layout);
        auto PairPtr2 = ASo->GetData(Di, Layout);
        if(!ElementType->ValueEqual(PairPtr1, PairPtr2))
        {
            return false;
        }
        if(!ValueType->ValueEqual((uint8*)PairPtr1 + Layout.ValueOffset, (uint8*)PairPtr2 + Layout.ValueOffset))
        {
            return false;
        }
    }

    return true;
}

template<typename T>
void RawDctorValue(void* Dest)
{
    ((T*)Dest) ->~T();
}
//todo not finish yet
void RawDctorArray(void* Array, TRefCountPtr<FTypeDesc> ElementType)
{
    auto ADe = (FScriptArray*)Array;

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        ElementType->DestroyValue((uint8*)ADe->GetData() + ElementType->GetSize() * Di);
    }
    ADe->Empty(0, ElementType->GetSize(), ElementType->GetAlign());
    ADe->~FScriptArray();
}
//todo not finish yet
void RawDctorMap(void* Array, TRefCountPtr<FTypeDesc> KeyType, TRefCountPtr<FTypeDesc> ValueType)
{
    auto ADe = (FScriptMap*)Array;

    auto Layout = FScriptMap::GetScriptLayout(KeyType->GetSize(), KeyType->GetAlign(), ValueType->GetSize(), ValueType->GetAlign());

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        auto PairPtr = ADe->GetData(Di, Layout);
        KeyType->DestroyValue(PairPtr);
        ValueType->DestroyValue((uint8*)PairPtr + Layout.ValueOffset);
    }
    ADe->Empty(0, Layout);
    ADe->~FScriptMap();
}
//todo not finish yet
void RawDctorSet(void* Array, TRefCountPtr<FTypeDesc> ElementType)
{
    auto ADe = (FScriptSet*)Array;

    auto Layout = FScriptSet::GetScriptLayout(ElementType->GetSize(), ElementType->GetAlign());

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        auto PairPtr = ADe->GetData(Di, Layout);
        ElementType->DestroyValue(PairPtr);
    }
    ADe->Empty(0, Layout);
    ADe->~FScriptSet();
}

template<typename T>
void RawSetValue(void* Dest, void* Source)
{
    *(T*)Dest = *(T*)Source;
}
void RawSetValueScriptArray(void* Dest, void* Source, TRefCountPtr<FTypeDesc> ElementType)
{
    auto ADe = (FScriptArray*)Dest;
    auto ASo = (FScriptArray*)Source;

    for(int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        ElementType->DestroyValue((uint8*)ADe->GetData() + ElementType->GetSize() * Di);
    }
    ADe->Empty(0, ElementType->GetSize(), ElementType->GetAlign());

    for(int32 Si = 0; Si < ASo->Num(); ++Si)
    {
        ADe->Add(1, ElementType->GetSize(), ElementType->GetAlign());
        ElementType->InitValue((uint8*)ADe->GetData() + ElementType->GetSize() * Si);
        ElementType->CopyValue((uint8*)ADe->GetData() + ElementType->GetSize() * Si, (uint8*)ASo->GetData() + ElementType->GetSize() * Si);
    }
}
//todo not finish yet
void RawSetValueScriptMap(void* Dest, void* Source, TRefCountPtr<FTypeDesc> KeyType, TRefCountPtr<FTypeDesc> ValueType)
{
    auto ADe = (FScriptMap*)Dest;
    auto ASo = (FScriptMap*)Source;

    auto Layout = FScriptMap::GetScriptLayout(KeyType->GetSize(), KeyType->GetAlign(), ValueType->GetSize(), ValueType->GetAlign());

    for(int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        auto PairPtr = ADe->GetData(Di, Layout);
        KeyType->DestroyValue(PairPtr);
        ValueType->DestroyValue((uint8*)PairPtr + Layout.ValueOffset);
    }
    ADe->Empty(0, Layout);

    for(int32 Si = 0; Si < ASo->Num(); ++Si)
    {
        auto SiPairPtr = ASo->GetData(Si, Layout);
        ADe->Add(SiPairPtr, (uint8*)SiPairPtr + Layout.ValueOffset, Layout,
            [KeyType](const void* KeyV)->uint32
            {
                return KeyType->GetTypeHash((void*)KeyV);
            },
            [KeyType](const void* A, const void* B)->bool
            {
                return KeyType->ValueEqual((void*)A, (void*)B);
            },
            [KeyType, SiPairPtr, Layout](void* Key)->void
            {
                KeyType->InitValue(Key);
                KeyType->CopyValue(Key, SiPairPtr);
            },
            [ValueType, SiPairPtr, Layout](void* Value)->void
            {
                ValueType->InitValue(Value);
                ValueType->CopyValue(Value, (uint8*)SiPairPtr + Layout.ValueOffset);
            },
            [ValueType, SiPairPtr, Layout](void* Value)->void
            {
                ValueType->CopyValue(Value, (uint8*)SiPairPtr + Layout.ValueOffset);
            },
            [KeyType, Layout](void* Key)->void
            {
                KeyType->DestroyValue(Key);
            },
            [ValueType, Layout](void* Value)->void
            {
                ValueType->DestroyValue(Value);
            }
        );
    }
}
//todo not finish yet
void RawSetValueScriptSet(void* Dest, void* Source, TRefCountPtr<FTypeDesc> KeyType)
{
    auto ADe = (FScriptSet*)Dest;
    auto ASo = (FScriptSet*)Source;

    auto Layout = FScriptSet::GetScriptLayout(KeyType->GetSize(), KeyType->GetAlign());// , ValueType->GetSize(), ValueType->GetAlign());

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        auto PairPtr = ADe->GetData(Di, Layout);
        KeyType->DestroyValue(PairPtr);
    }
    ADe->Empty(0, Layout);

    for (int32 Si = 0; Si < ASo->Num(); ++Si)
    {
        auto SiPairPtr = ASo->GetData(Si, Layout);

        ADe->Add(SiPairPtr, Layout,
            [KeyType](const void* KeyV)->uint32
            {
                return KeyType->GetTypeHash((void*)KeyV);
            },
            [KeyType](const void* A, const void* B)->bool
            {
                return KeyType->ValueEqual((void*)A, (void*)B);
            },
            [KeyType, SiPairPtr, Layout](void* Key)->void
            {
                KeyType->InitValue(Key);
                KeyType->CopyValue(Key, SiPairPtr);
            },
            [KeyType, Layout](void* Key)->void
            {
                KeyType->DestroyValue(Key);
            }
        );

    }
}


void FTypeDesc::CopyValue(void* Dest, void* Source) const
{

    switch (Kind)
    {
    case ETypeKind::None: return;


    case ETypeKind::Int8: RawSetValue<int8>(Dest, Source); return;
    case ETypeKind::Int16: RawSetValue<int16>(Dest, Source); return;
    case ETypeKind::Int32:  RawSetValue<int32>(Dest, Source); return;
    case ETypeKind::Int64:  RawSetValue<int64>(Dest, Source); return;
    case ETypeKind::Byte:  RawSetValue<uint8>(Dest, Source); return;
    case ETypeKind::UInt16:  RawSetValue<uint16>(Dest, Source); return;
    case ETypeKind::UInt32:  RawSetValue<uint32>(Dest, Source); return;
    case ETypeKind::UInt64:  RawSetValue<uint64>(Dest, Source); return;
    case ETypeKind::Float:  RawSetValue<float>(Dest, Source); return;
    case ETypeKind::Double:  RawSetValue<double>(Dest, Source); return;

    case ETypeKind::Bool:  RawSetValue<bool>(Dest, Source); return;
    case ETypeKind::BitBool: return;
    case ETypeKind::String: RawSetValue<FString>(Dest, Source); return;
    case ETypeKind::Name:  RawSetValue<FName>(Dest, Source); return;
    case ETypeKind::Text:  RawSetValue<FText>(Dest, Source); return;
    case ETypeKind::Vector:  RawSetValue<FVector>(Dest, Source); return;
    case ETypeKind::Rotator:  RawSetValue<FRotator>(Dest, Source); return;
    case ETypeKind::Quat:  RawSetValue<FQuat>(Dest, Source); return;
    case ETypeKind::Transform:  RawSetValue<FTransform>(Dest, Source); return;
    case ETypeKind::Matrix:  RawSetValue<FMatrix>(Dest, Source); return;
    case ETypeKind::RotationMatrix:  RawSetValue<FRotationMatrix>(Dest, Source); return;
    case ETypeKind::Color:  RawSetValue<FColor>(Dest, Source); return;
    case ETypeKind::LinearColor: RawSetValue<FLinearColor>(Dest, Source); return;


    case ETypeKind::Array:  RawSetValueScriptArray(Dest, Source, CombineKindSubTypes[0]); return;
    case ETypeKind::Map:  RawSetValueScriptMap(Dest, Source, CombineKindSubTypes[0], CombineKindSubTypes[1]); return;
    case ETypeKind::Set:  RawSetValueScriptSet(Dest, Source, CombineKindSubTypes[0]); return;
    case ETypeKind::Delegate: RawSetValue<FScriptDelegate>(Dest, Source); return;
    case ETypeKind::MulticastDelegate: RawSetValue<FMulticastScriptDelegate>(Dest, Source); return;
    case ETypeKind::WeakObject: RawSetValue<FWeakObjectPtr>(Dest, Source); return;

    case ETypeKind::U_Enum:  RawSetValue<int8>(Dest, Source); return;
    case ETypeKind::U_ScriptStruct:
        if (UserDefinedTypePointer.U_ScriptStruct) { UserDefinedTypePointer.U_ScriptStruct->CopyScriptStruct(Dest, Source); } return;
    case ETypeKind::U_Class: RawSetValue<UObject*>(Dest, Source); return;
    case ETypeKind::U_Function: return;
    case ETypeKind::U_Interface: RawSetValue<FScriptInterface>(Dest, Source);
    default: return;
    }
}


void FTypeDesc::DestroyValue(void* ValueAddress) const
{
    switch (Kind)
    {
    case ETypeKind::None: return;


    case ETypeKind::Int8:  ;
    case ETypeKind::Int16:;
    case ETypeKind::Int32: 
    case ETypeKind::Int64: 
    case ETypeKind::Byte:
    case ETypeKind::UInt16:
    case ETypeKind::UInt32:
    case ETypeKind::UInt64:
    case ETypeKind::Float:
    case ETypeKind::Double:

    case ETypeKind::Bool: 
    case ETypeKind::BitBool:
    	return;
    case ETypeKind::String: RawDctorValue<FString>(ValueAddress); return;
    case ETypeKind::Name:  RawDctorValue<FName>(ValueAddress); return;
    case ETypeKind::Text:  RawDctorValue<FText>(ValueAddress); return;
    case ETypeKind::Vector:  RawDctorValue<FVector>(ValueAddress); return;
    case ETypeKind::Rotator:  RawDctorValue<FRotator>(ValueAddress);; return;
    case ETypeKind::Quat:  RawDctorValue<FQuat>(ValueAddress); return;
    case ETypeKind::Transform:  RawDctorValue<FTransform>(ValueAddress); return;
    case ETypeKind::Matrix:  RawDctorValue<FMatrix>(ValueAddress);; return;
    case ETypeKind::RotationMatrix:  RawDctorValue<FRotationMatrix>(ValueAddress);; return;
    case ETypeKind::Color:  RawDctorValue<FColor>(ValueAddress); return;
    case ETypeKind::LinearColor: RawDctorValue<FLinearColor>(ValueAddress); return;


    case ETypeKind::Array:  RawDctorArray(ValueAddress, CombineKindSubTypes[0]); return;
    case ETypeKind::Map:  RawDctorMap(ValueAddress, CombineKindSubTypes[0], CombineKindSubTypes[1]); return;
    case ETypeKind::Set:  RawDctorSet(ValueAddress, CombineKindSubTypes[0]); return;
    case ETypeKind::Delegate: RawDctorValue<FScriptDelegate>(ValueAddress); return;
    case ETypeKind::MulticastDelegate: RawDctorValue<FMulticastScriptDelegate>(ValueAddress); return;
    case ETypeKind::WeakObject: RawDctorValue<FWeakObjectPtr>(ValueAddress); return;

    case ETypeKind::U_Enum: return;
    case ETypeKind::U_ScriptStruct:
        if (UserDefinedTypePointer.U_ScriptStruct) { UserDefinedTypePointer.U_ScriptStruct->DestroyStruct(ValueAddress); } return;
    case ETypeKind::U_Class: *(UObject**)ValueAddress = nullptr; return;
    case ETypeKind::U_Function: return;
    case ETypeKind::U_Interface: RawDctorValue<FScriptInterface>(ValueAddress);
    default: return;
    }
}

uint32 FTypeDesc::GetTypeHash(void* ValueAddress) const
{
    switch (Kind)
    {
    case ETypeKind::None: return 0;


    case ETypeKind::Int8:return RawGetValueHash<int8>(ValueAddress);
    case ETypeKind::Int16:return RawGetValueHash<int16>(ValueAddress);
    case ETypeKind::Int32:return RawGetValueHash<int32>(ValueAddress);
    case ETypeKind::Int64:return RawGetValueHash<int64>(ValueAddress);
    case ETypeKind::Byte:return RawGetValueHash<uint8>(ValueAddress);
    case ETypeKind::UInt16:return RawGetValueHash<uint16>(ValueAddress);
    case ETypeKind::UInt32:return RawGetValueHash<uint32>(ValueAddress);
    case ETypeKind::UInt64:return RawGetValueHash<uint64>(ValueAddress);
    case ETypeKind::Float:return RawGetValueHash<float>(ValueAddress);
    case ETypeKind::Double:return RawGetValueHash<double>(ValueAddress);

    case ETypeKind::Bool:return RawGetValueHash<bool>(ValueAddress);
    case ETypeKind::BitBool:
        return 0;
    case ETypeKind::String: return RawGetValueHash<FString>(ValueAddress);
    case ETypeKind::Name:  return RawGetValueHash<FName>(ValueAddress);
    case ETypeKind::Text:  return 0;
    case ETypeKind::Vector:  return RawGetValueHash<FVector>(ValueAddress);
    case ETypeKind::Rotator:  return 0;
    case ETypeKind::Quat:  return RawGetValueHash<FQuat>(ValueAddress);
    case ETypeKind::Transform:  return RawGetValueHash<FTransform>(ValueAddress);
    case ETypeKind::Matrix:  return 0;
    case ETypeKind::RotationMatrix: return 0;
    case ETypeKind::Color:  return RawGetValueHash<FColor>(ValueAddress);
    case ETypeKind::LinearColor: return RawGetValueHash<FLinearColor>(ValueAddress);


    case ETypeKind::Array:  return 0;
    case ETypeKind::Map:  return 0;
    case ETypeKind::Set:  return 0;
    case ETypeKind::Delegate: return 0;
    case ETypeKind::MulticastDelegate: return 0;
    case ETypeKind::WeakObject: return RawGetValueHash<FWeakObjectPtr>(ValueAddress);

    case ETypeKind::U_Enum: return RawGetValueHash<uint8>(ValueAddress);
    case ETypeKind::U_ScriptStruct:
        if (UserDefinedTypePointer.U_ScriptStruct) { return UserDefinedTypePointer.U_ScriptStruct->GetStructTypeHash(ValueAddress); } return 0;
    case ETypeKind::U_Class: return RawGetValueHash<UObject*>(ValueAddress);
    case ETypeKind::U_Function: return RawGetValueHash<UFunction*>(UserDefinedTypePointer.U_Function);
    case ETypeKind::U_Interface: return RawGetValueHash<FScriptInterface>(ValueAddress);
    default: return 0;
    }
}

bool FTypeDesc::ValueEqual(void* Dest, void* Source) const
{
    switch (Kind)
    {
    case ETypeKind::None: return false;


    case ETypeKind::Int8: return RawCompairValue<int8>(Dest, Source); ;
    case ETypeKind::Int16:return RawCompairValue<int16>(Dest, Source); ;
    case ETypeKind::Int32: return RawCompairValue<int32>(Dest, Source); ;
    case ETypeKind::Int64:return  RawCompairValue<int64>(Dest, Source); ;
    case ETypeKind::Byte: return RawCompairValue<uint8>(Dest, Source); ;
    case ETypeKind::UInt16:return  RawCompairValue<uint16>(Dest, Source); ;
    case ETypeKind::UInt32:return  RawCompairValue<uint32>(Dest, Source); ;
    case ETypeKind::UInt64:return  RawCompairValue<uint64>(Dest, Source); ;
    case ETypeKind::Float:return  RawCompairValue<float>(Dest, Source); ;
    case ETypeKind::Double:return  RawCompairValue<double>(Dest, Source); ;

    case ETypeKind::Bool: return RawCompairValue<bool>(Dest, Source); ;
    case ETypeKind::BitBool: return false;
    case ETypeKind::String:return RawCompairValue<FString>(Dest, Source); ;
    case ETypeKind::Name:return  RawCompairValue<FName>(Dest, Source); ;
    case ETypeKind::Text:return  false; ;
    case ETypeKind::Vector: return RawCompairValue<FVector>(Dest, Source); ;
    case ETypeKind::Rotator:return  RawCompairValue<FRotator>(Dest, Source); ;
    case ETypeKind::Quat:return  RawCompairValue<FQuat>(Dest, Source); ;
    case ETypeKind::Transform: return RawCompairValue<FTransform>(Dest, Source); ;
    case ETypeKind::Matrix:return  RawCompairValue<FMatrix>(Dest, Source); ;
    case ETypeKind::RotationMatrix:return  RawCompairValue<FRotationMatrix>(Dest, Source); ;
    case ETypeKind::Color: return RawCompairValue<FColor>(Dest, Source); ;
    case ETypeKind::LinearColor:return RawCompairValue<FLinearColor>(Dest, Source); ;


    case ETypeKind::Array:  return RawCompairArray(Dest, Source, CombineKindSubTypes[0]); ; //todo
    case ETypeKind::Map: return RawCompairMap(Dest, Source, CombineKindSubTypes[0], CombineKindSubTypes[1]); //todo
    case ETypeKind::Set: return RawCompairSet(Dest, Source, CombineKindSubTypes[0]); //todo
    case ETypeKind::Delegate: return RawCompairValue<FScriptDelegate>(Dest, Source); 
    case ETypeKind::MulticastDelegate: return false;
    case ETypeKind::WeakObject: return RawCompairValue<FWeakObjectPtr>(Dest, Source);

    case ETypeKind::U_Enum: return RawCompairValue<int8>(Dest, Source); ;
    case ETypeKind::U_ScriptStruct:
        if (UserDefinedTypePointer.U_ScriptStruct) { return
        	UserDefinedTypePointer.U_ScriptStruct->CompareScriptStruct(Dest, Source, EPropertyPortFlags::PPF_None);
        }
    	return false;
    case ETypeKind::U_Class:return RawCompairValue<UObject*>(Dest, Source); ;
    case ETypeKind::U_Function: return true;
    case ETypeKind::U_Interface:return RawCompairValue<FScriptInterface>(Dest, Source);
    default: return false;
    }
}

int32 FTypeDesc::GetSizePreDefinedKind(ETypeKind InKind)
{
    switch (InKind)
    {
    case ETypeKind::None: return 0;


    case ETypeKind::Int8: return sizeof(int8);
    case ETypeKind::Int16: return sizeof(int16);
    case ETypeKind::Int32: return sizeof(int32);
    case ETypeKind::Int64: return sizeof(int64);
    case ETypeKind::Byte: return sizeof(uint8);
    case ETypeKind::UInt16: return sizeof(uint16);
    case ETypeKind::UInt32: return sizeof(uint32);
    case ETypeKind::UInt64: return sizeof(uint64);
    case ETypeKind::Float: return sizeof(float);
    case ETypeKind::Double: return sizeof(double);

    case ETypeKind::Bool: return sizeof(bool);
    case ETypeKind::BitBool: return 0;
    case ETypeKind::String: return sizeof(FString);
    case ETypeKind::Name: return sizeof(FName);
    case ETypeKind::Text: return sizeof(FText);
    case ETypeKind::Vector: return sizeof(FVector);
    case ETypeKind::Rotator: return sizeof(FRotator);
    case ETypeKind::Quat: return sizeof(FQuat);
    case ETypeKind::Transform: return sizeof(FTransform);
    case ETypeKind::Matrix: return sizeof(FMatrix);
    case ETypeKind::RotationMatrix: return sizeof(FRotationMatrix);
    case ETypeKind::Color: return sizeof(FColor);
    case ETypeKind::LinearColor: return sizeof(FLinearColor);


    case ETypeKind::Array: return sizeof(FScriptArray);
    case ETypeKind::Map: return sizeof(FScriptMap);
    case ETypeKind::Set: return sizeof(FScriptSet);
    case ETypeKind::Delegate: return sizeof(FScriptDelegate);
    case ETypeKind::MulticastDelegate: return sizeof(FMulticastScriptDelegate);
    case ETypeKind::WeakObject: return sizeof(FWeakObjectPtr);

    case ETypeKind::U_Enum: return sizeof(uint8);
    case ETypeKind::U_ScriptStruct: return 0;
    case ETypeKind::U_Class: return sizeof(UObject*);
    case ETypeKind::U_Function: return sizeof(UObject*);
    case ETypeKind::U_Interface: return sizeof(FScriptInterface);
    default: return 0;
    }
}



TRefCountPtr<FTypeDesc> FTypeDesc::AquireClassDescByUEType(UField* UEType)
{
    struct FContainerInit
    {
        UUETypeDescContainer* Container;
        void PostGC()
        {
	        for(auto&& It = Container->Map.CreateIterator(); It; ++It)
	        {
		        if(It->Value->UserDefinedTypePointer.Pointer == nullptr)
		        {
                    It.RemoveCurrent();
		        }
	        }
        }
        FContainerInit()
        {
            Container = NewObject<UUETypeDescContainer>();
            Container->AddToRoot();
            FCoreUObjectDelegates::GetPostGarbageCollect().AddRaw(this, &FContainerInit::PostGC);
        }
        UUETypeDescContainer* operator->() { return Container; }
    };
    static FContainerInit Container;
    FScopeLock Lock(&Container->CS);
    if(UEType)
    {
        ETypeKind Kind = ETypeKind::None;
        if(UClass* Class = Cast<UClass>(UEType))
        {
            if(Class->HasAnyClassFlags(CLASS_Interface))
            {
                Kind = ETypeKind::U_Interface;
            }
            else
            {
                Kind = ETypeKind::U_Class;
            }
        }
        if(UScriptStruct* ScriptStruct = Cast<UScriptStruct>(UEType))
        {
            Kind = ETypeKind::U_ScriptStruct;
        }
        if(UEnum* Enum = Cast<UEnum>(UEType))
        {
            Kind = ETypeKind::U_Enum;
        }
        if(UFunction* Function = Cast<UFunction>(UEType))
        {
            Kind = ETypeKind::U_Function;
        }
        
        if(Kind != ETypeKind::None)
        {
            auto&& Item = Container->Map.FindOrAdd(UEType);
            if (!Item.GetReference())
            {
                Item = new FTypeDesc();
            }
            check(Item->Kind == ETypeKind::None ||
                (Item->Kind >= ETypeKind::UserDefinedKindBegin && Item->Kind < ETypeKind::UserDefinedKindEnd));
            Item->Kind = Kind;
            if(!Item->UserDefinedTypePointer.Pointer)
            {
                Item->UserDefinedTypePointer.Pointer = UEType;
            }
            else
            {
                check(Item->UserDefinedTypePointer.Pointer == UEType);
            }
            return Item;
        }
    }
    return nullptr;
}

TRefCountPtr<FTypeDesc> FTypeDesc::AquireClassDescByPreDefinedKind(ETypeKind InKind)
{
    static FCriticalSection CS;
    FScopeLock Lock(&CS);
    if(InKind >= ETypeKind::PreDefinedKindBegin && InKind < ETypeKind::PreDefinedKindEnd)
    {
        static TMap<ETypeKind, FTypeDescRefCount> Map;
        auto&& Item = Map.FindOrAdd(InKind);
        if(Item->Kind == ETypeKind::None)
        {
            Item->Kind = InKind;
        }
        check(Item->Kind == InKind);
        return Item;
    }
    return nullptr;
}

TRefCountPtr<FTypeDesc> FTypeDesc::AquireClassDescByCombineKind(ETypeKind InKind, const TArray<TRefCountPtr<FTypeDesc>>& InKey)
{
    static FCriticalSection CS;
    FScopeLock Lock(&CS);

    struct FContainer
    {
        TMap<TArray<TRefCountPtr<FTypeDesc>>, FTypeDescRefCount> Map[(uint16)ETypeKind::CombineKindEnd - (uint16)ETypeKind::CombineKindBegin];
        void PostGC()
        {
	        for(auto&& M : Map)
	        {
		        for(auto&& It = M.CreateIterator(); It; ++It)
		        {
			        if(It->Value.GetRefCount() == 1)
			        {
                        It.RemoveCurrent();
			        }
		        }
	        }
        }
        FContainer()
        {
            FCoreUObjectDelegates::GetPostGarbageCollect().AddRaw(this, &FContainer::PostGC);
        }

        
    };
    static FContainer Container;
    //static TMap<TArray<TRefCountPtr<FTypeDesc>>, FTypeDescRefCount> Map[(uint16)ETypeKind::CombineKindEnd - (uint16)ETypeKind::CombineKindBegin];

    if(InKind >= ETypeKind::CombineKindBegin && InKind < ETypeKind::CombineKindEnd)
    {
	    switch(InKind)
	    {
	    case ETypeKind::WeakObject:
        case ETypeKind::Set:
	    case ETypeKind::Array:
            if(InKey.Num() == 1 && InKey[0] != nullptr)
		    {
                auto&& V = Container.Map[(uint16)InKind - (uint16)ETypeKind::CombineKindBegin].FindOrAdd(InKey);
                if(!V.GetReference())
                {
                    V = new FTypeDesc();
                    V->Kind = InKind;
                }
                check(V->Kind == InKind && (V->CombineKindSubTypes.Num() == 1 || V->CombineKindSubTypes.IsEmpty()) );
                if(V->CombineKindSubTypes.IsEmpty())
                {
                    V->CombineKindSubTypes = InKey;
                }
                else
                {
                    check(V->CombineKindSubTypes[0] == InKey[0]);
                }
                return V;
		    }
            break;

	    case ETypeKind::Map:
            if(InKey.Num() == 2 && InKey[0] != nullptr && InKey[1] != nullptr)
            {
                auto&& V = Container.Map[(uint16)InKind - (uint16)ETypeKind::CombineKindBegin].FindOrAdd(InKey);
                if (!V.GetReference())
                {
                    V = new FTypeDesc();
                    V->Kind = InKind;
                    check(V->Kind == InKind && (V->CombineKindSubTypes.Num() == 2 || V->CombineKindSubTypes.IsEmpty()));
                    if (V->CombineKindSubTypes.IsEmpty())
                    {
                        V->CombineKindSubTypes = InKey;
                    }
                    else
                    {
                        check(V->CombineKindSubTypes[0] == InKey[0] && V->CombineKindSubTypes[1] == InKey[1]);
                    }
                    return V;
                }
            }
            break;
	    case ETypeKind::Delegate:
	    case ETypeKind::MulticastDelegate:
            return Container.Map[(uint16)InKind - (uint16)ETypeKind::CombineKindBegin].FindOrAdd(TArray<TRefCountPtr<FTypeDesc>>());
            break;


        default:;
	    }
    }

    return nullptr;
}

TRefCountPtr<FTypeDesc> FTypeDesc::AquireClassDescByProperty(FProperty* Proy)
{
#define  PushProperty_GetTypedProp(PropType) F##PropType##Property* PropType##Property = CastField<F##PropType##Property>(Proy);
    PushProperty_GetTypedProp(Bool);
    PushProperty_GetTypedProp(Int8);
    PushProperty_GetTypedProp(Int16);
    PushProperty_GetTypedProp(Int);
    PushProperty_GetTypedProp(Int64);
    PushProperty_GetTypedProp(Byte);
    PushProperty_GetTypedProp(UInt16);
    PushProperty_GetTypedProp(UInt32);
    PushProperty_GetTypedProp(UInt64);
    PushProperty_GetTypedProp(Float);
    PushProperty_GetTypedProp(Double);
    PushProperty_GetTypedProp(Str);
    PushProperty_GetTypedProp(Name);
    PushProperty_GetTypedProp(Text);



    PushProperty_GetTypedProp(Enum);
    PushProperty_GetTypedProp(Struct);
    PushProperty_GetTypedProp(Object);
    PushProperty_GetTypedProp(Interface);

    PushProperty_GetTypedProp(Delegate);
    PushProperty_GetTypedProp(MulticastDelegate);

    PushProperty_GetTypedProp(Array);
    PushProperty_GetTypedProp(Set);
    PushProperty_GetTypedProp(Map);
    PushProperty_GetTypedProp(WeakObject);


#undef PushProperty_GetTypedProp

#define PushProperty_BaseType(BaseType) if(BaseType##Property) {return AquireClassDescByPreDefinedKind(ETypeKind::BaseType);}
    PushProperty_BaseType(Bool);
    PushProperty_BaseType(Int8);
    PushProperty_BaseType(Int16);
    if (IntProperty) { return AquireClassDescByPreDefinedKind(ETypeKind::Int32); }
    PushProperty_BaseType(Int64);
    PushProperty_BaseType(Byte);
    PushProperty_BaseType(UInt16);
    PushProperty_BaseType(UInt32);
    PushProperty_BaseType(UInt64);
    PushProperty_BaseType(Float);
    PushProperty_BaseType(Double);
    if (StrProperty) { return AquireClassDescByPreDefinedKind(ETypeKind::String); }
    PushProperty_BaseType(Name);
    PushProperty_BaseType(Text);
#undef PushProperty_BaseType

    if(EnumProperty)
    {
        return AquireClassDescByUEType(EnumProperty->GetEnum());
    }
    if(StructProperty)
    {
        return AquireClassDescByUEType(StructProperty->Struct);
    }
    if(ObjectProperty)
    {
        return AquireClassDescByUEType(ObjectProperty->PropertyClass);
    }
    if(InterfaceProperty)
    {
        return AquireClassDescByUEType(InterfaceProperty->InterfaceClass);
    }

    if(ArrayProperty)
    {
        auto InnerProperty = AquireClassDescByProperty(ArrayProperty->Inner);
        return AquireClassDescByCombineKind(ETypeKind::Array, { InnerProperty });
    }
    if (SetProperty)
    {
        auto InnerProperty = AquireClassDescByProperty(SetProperty->ElementProp);
        return AquireClassDescByCombineKind(ETypeKind::Set, { InnerProperty });
    }
    if(MapProperty)
    {
        auto InnerProperty = AquireClassDescByProperty(MapProperty->KeyProp);
        auto ValueProperty = AquireClassDescByProperty(MapProperty->ValueProp);
        return AquireClassDescByCombineKind(ETypeKind::Map, { InnerProperty, ValueProperty });
    }
    if(WeakObjectProperty)
    {
        auto InnerType = AquireClassDescByUEType(WeakObjectProperty->PropertyClass);
        return AquireClassDescByCombineKind(ETypeKind::WeakObject, { InnerType });
    }

    if(DelegateProperty)
    {
        return AquireClassDescByCombineKind(ETypeKind::Delegate, { });
    }
    if (MulticastDelegateProperty)
    {
        return AquireClassDescByCombineKind(ETypeKind::MulticastDelegate, { });
    }

    return nullptr;
}

void FLuaUStructData::Clear()
{
    if (StructType && bValid)
    {
        StructType->DestroyStruct(GetData());
    }
    StructType = nullptr;
    bValid = false;
}

void FLuaUStructData::CopyData(void* Data)
{
    if(StructType && bValid)
    {
        StructType->CopyScriptStruct(GetData(), Data);
    }
}

void FLuaUStructData::SetData(UScriptStruct* Type, void* Data)
{
	if(StructType && bValid)
	{
        StructType->DestroyStruct(GetData());
	}
    StructType = Type;
    bValid = false;

    if(StructType)
    {
        if (StructType->GetStructureSize() > MaxInlineSize)
        {
            void* Mem = operator new(StructType->GetStructureSize());
            StructType->InitializeDefaultValue((uint8*)Mem);
            if(Data)
            {
                StructType->CopyScriptStruct(Mem, Data);
            }
        	*(void**)(&InnerData) = Mem;
        }
        else
        {
            StructType->InitializeDefaultValue((uint8*)&InnerData);
            if(Data)
            {
                StructType->CopyScriptStruct(&InnerData, Data);
            }
        }
        bValid = true;
    }
}

void* FLuaUStructData::GetData()
{
    if(StructType && bValid)
    {
        if (StructType->GetStructureSize() > MaxInlineSize)
        {
            return *(void**)(&InnerData);
        }
        return &InnerData;
    }
    return nullptr;
}

void FLuaUStructData::AddReferencedObjects(UObject* Owner, FReferenceCollector& Collector, bool bStrong)
{
    if(StructType)
    {
        Collector.AddReferencedObject(StructType);
        if(bValid)
        {
            FVerySlowReferenceCollectorArchiveScope Scope(Collector.GetVerySlowReferenceCollectorArchive(), Owner);
            StructType->SerializeBin(Scope.GetArchive(), GetData());
        }

    }
}

void FLuaUObjectData::AddReferencedObjects(UObject* Owner, FReferenceCollector& Collector, bool bStrong)
{
    if(bStrong)
    {
        Collector.AddReferencedObject(Object);
    }
    else
    {
        Collector.MarkWeakObjectReferenceForClearing(&Object);
    }
}

bool FLuaUEData::Index(ULuaState* L, const char* Key)
{
    //switch(DataType)
    //{
    //case EUEDataType::Struct:
	   // {
		  //  if(Data.Struct.IsDataValid())
		  //  {
    //            FProperty* Prop = Data.Struct.StructType->FindPropertyByName(UTF8_TO_TCHAR(Key));
    //            if(Prop)
    //            {
    //                //Prop->ContainerPtrToValuePtr<>()
    //            }
		  //  }
	   // }
    //    break;
    //case EUEDataType::StructRef:
	   // {
		  //  if(Data.StructRef.IsDataValid())
		  //  {
			 //   
		  //  }
	   // }
    //    break;
    //case EUEDataType::Object:
	   // {
		  //  if(Data.Object.IsDataValid())
		  //  {
			 //   
		  //  }
	   // }
    //    break;
    //}
    return false;
}

void ULuaState::PostGarbageCollect()
{
    if(InnerState)
    {
        lua_gc(InnerState, LUA_GCCOLLECT);
        lua_gc(InnerState, LUA_GCSTOP);
    }
}

void ULuaState::OnCollectLuaRefs(FReferenceCollector& Collector, GCObject* o, bool w, lua_State* l)
{
    if(o->tt == LUA_VUSERDATA)
    {
        Udata* u = gco2u(o);
        if (u->nuvalue != 0 && u->metatable)
        {  
            auto metatableName = GetMetatableName(l, u->metatable);
            if(metatableName && strcmp(metatableName, LuaUEDataMetatableName) == 0)
            {
                void* UD = getudatamem(u);
                FLuaUEData* UEData = (FLuaUEData*)UD;
                if(UEData)
                {
                    UEData->AddReferencedObjects(this, Collector, w);
                }
            }
            else if(metatableName && strcmp(metatableName, LuaUEValueMetatableName) == 0)
            {
                void* UD = getudatamem(u);
                FLuaUEValue* UEValue = (FLuaUEValue*)UD;
                if (UEValue)
                {
                    UEValue->AddReferencedObject(this, Collector, w);
                }
            }
        }
    }
}



static std::atomic_uint64_t GlobalThreadId = 0;

thread_local uint64 ULuaState::LocalThreadId = 0xffffffffffffffff;
thread_local uint64 ULuaState::EnterCount = 0;

void ULuaState::LockLua()
{
    if(LocalThreadId == 0xffffffffffffffff)
    {
        LocalThreadId = GlobalThreadId.fetch_add(1);
    }

    if(LocalThreadId == LockedThreadId)
    {
        EnterCount++;
    }
    else
    {
        while (LuaAt.test_and_set()) {}
        LockedThreadId = LocalThreadId;
        EnterCount = 1;
    }
}

void ULuaState::UnlockLua()
{
    if(--EnterCount == 0)
    {
        LockedThreadId = 0xffffffffffffffff;
        LuaAt.clear();
    }
}

void ULuaState::PushLuaUEValue(void* Value, const TRefCountPtr<FTypeDesc>& Type)
{
    if(!InnerState || !Type)
    {
        return;
    }

    FLuaUEValue* LuaUD = (FLuaUEValue*)lua_newuserdata(InnerState, sizeof(FLuaUEValue));//ud
    new (LuaUD) FLuaUEValue();
    PushToAllValues(LuaUD);
    LuaUD->TypeDesc = Type;
    LuaUD->InitData();
    if(Value)
    {
        LuaUD->CopyFrom(Value);
    }
	luaL_newmetatable(InnerState, LuaUEValueMetatableName); //ud, LuaUEValueMetatableName
	lua_setmetatable(InnerState, -2);//ud
}

void ULuaState::PushLuaUEValue(void* Value, const FLuaUEValue& PartOfWhom, const TRefCountPtr<FTypeDesc>& Type)
{
    if (!InnerState || !Type || !PartOfWhom.GetData() || !Value)
    {
        return;
    }
    FLuaUEValue* LuaUD = (FLuaUEValue*)lua_newuserdata(InnerState, sizeof(FLuaUEValue));//ud
    new (LuaUD) FLuaUEValue();
    PushToAllValues(LuaUD);
    LuaUD->Owner = &PartOfWhom;
    LuaUD->TypeDesc = Type;
    LuaUD->InitDataAsPartOfOther(Value);
    luaL_newmetatable(InnerState, LuaUEValueMetatableName); //ud, LuaUEValueMetatableName
    lua_setmetatable(InnerState, -2);//ud
}

//void ULuaState::PushLuaUEData(void* Value, UStruct* DataType, EUEDataType Type, TCustomMemoryHandle<FLuaUEData> Oter, int32 MaxCount)
//{
//    if(!InnerState)
//    {
//        return;
//    }
//    if(Type == EUEDataType::None)
//    {
//        return;
//    }
//    if(ensure((Type != EUEDataType::StructRef && !Oter) || (Type == EUEDataType::StructRef && Oter)))
//    {
//        UScriptStruct* ScriptStruct = nullptr;
//        UClass* Class = nullptr;
//        UObject* Obj = nullptr;
//        UObject** pObj = nullptr;
//        switch (Type)
//        {
//        case EUEDataType::Struct:
//        case EUEDataType::StructRef:
//        {
//            ScriptStruct = Cast<UScriptStruct>(DataType);
//        	if(!ScriptStruct)
//            {
//                return;
//            }
//        }
//        break;
//        case EUEDataType::Object:
//        case EUEDataType::ObjectRef:
//        {
//            Class = Cast<UClass>(DataType);
//			if(Type == EUEDataType::Object)
//			{
//                Obj = (UObject*)Value;
//			}
//            else
//            {
//                pObj = (UObject**)Value;
//                Obj = *pObj;
//            }
//            
//            if (!Class)
//            {
//                return;
//            }
//        }
//        break;
//        default:
//            check(0);
//        }
//
//        if(!ScriptStruct && !Class)
//        {
//            return;
//        }
//
//        if(Obj && !Obj->IsA(Class))
//        {
//            return;
//        }
//
//        FLuaUEData* LuaUD = (FLuaUEData*)lua_newuserdata(InnerState, sizeof(FLuaUEData));//ud
//        new (LuaUD) FLuaUEData();
//        if(!LuaUD)
//        {
//            return;
//        }
//
//        LuaUD->DataType = Type;
//        LuaUD->Oter = Oter;
//
//        luaL_newmetatable(InnerState, LuaUEDataMetatableName); //ud, LuaUEDataMetatableName
//        lua_setmetatable(InnerState, -2);//ud
//
//	    switch (Type)
//	    {
//	    case EUEDataType::StructRef:
//		    {
//            LuaUD->Data.StructRef.SetRef(ScriptStruct, Value, MaxCount);
//		    }
//            break;
//	    case EUEDataType::Object:
//		    {
//            LuaUD->Data.Object.Object = Obj;
//		    }
//            break;
//	    case EUEDataType::ObjectRef:
//		    {
//            LuaUD->Data.ObjectRef.pObject = pObj;
//            LuaUD->Data.ObjectRef.MaxOffsetCount = MaxCount;
//		    }
//            break;
//	    case EUEDataType::Struct:
//		    {
//            LuaUD->Data.Struct.SetData(ScriptStruct, Value);
//		    }
//            break;
//	    default:
//            check(0);
//	    }
//    }
//}

void ULuaState::PushUStructCopy(void* Value, UScriptStruct* DataType)
{
    auto&& R = FTypeDesc::AquireClassDescByUEType(DataType);
    PushLuaUEValue(Value, R);
    //PushLuaUEData(Value, DataType, EUEDataType::Struct, nullptr);
}

void ULuaState::PushProperty(void* Value, FProperty* Proy)
{
    auto Type = FTypeDesc::AquireClassDescByProperty(Proy);
    PushLuaUEValue(Value, Type);
}

void ULuaState::PushProperty(void* Value, const FLuaUEValue& PartOfWhom, FProperty* Proy)
{
    auto Type = FTypeDesc::AquireClassDescByProperty(Proy);
    PushLuaUEValue(Value, PartOfWhom, Type);
}

const char* const ULuaState::LuaUEDataMetatableName = MAKE_LUA_METATABLE_NAME(FLuaUEData);
const char* const ULuaState::LuaUEValueMetatableName = MAKE_LUA_METATABLE_NAME(FLuaUEValue);

void ULuaState::BeginDestroy()
{
	UObject::BeginDestroy();
    Finalize();
}


int TurinmaTableIndex(lua_State* L)
{
    if(lua_isstring(L, 2))
    {
        FString Key = UTF8_TO_TCHAR(lua_tostring(L, 2));
        Key.TrimStartAndEndInline();
        if(!Key.IsEmpty())
        {
          //  FPackageName::IsValidLongPackageName()
        }
    }
    return 0;
}

int TurinmaTableNewIndex(lua_State* L)
{

    return 0;
}


int OnDestroyUEDataInLua(lua_State* L)
{
    lua_getmetatable(L, 1);//obj, metatable
    lua_getfield(L, -1, "__name");//obj, metatable, LuaUEDataMetatableName
    if(ensure(strcmp(lua_tostring(L, -1), ULuaState::LuaUEDataMetatableName) == 0))
    {
        FLuaUEData* LuaUEData = (FLuaUEData*)lua_touserdata(L, 1);
        LuaUEData->~FLuaUEData();
    }
    return 0;
}

int OnDestroyUEValueInLua(lua_State* L)
{
    lua_getmetatable(L, 1);//obj, metatable
    lua_getfield(L, -1, "__name");//obj, metatable, LuaUEDataMetatableName
    if (ensure(strcmp(lua_tostring(L, -1), ULuaState::LuaUEValueMetatableName) == 0))
    {
        FLuaUEValue* LuaUEValue = (FLuaUEValue*)lua_touserdata(L, 1);
        ensure(!LuaUEValue->IsHasValidData() && LuaUEValue->AllValueIndex == -1);
        if(LuaUEValue->AllValueIndex != -1)
        {
            ((ULuaState*)(G(L)->ud))->RemoveFormAllValues(LuaUEValue);
        }
        LuaUEValue->~FLuaUEValue();
    }
    return 0;
}

int LuaUEValueIndex(lua_State* L)
{
    ULuaState* TL = (ULuaState*)G(L)->ud;
    FLuaUEValue* LuaUEValue = (FLuaUEValue*)lua_touserdata(L, 1);

    //todo index lua UE value

    return 0;
}

void ULuaState::Init()
{
    LuaAt.clear();
    InnerState = lua_newstate(&FLuaSourceModule::LuaMalloc, this);
    lua_gc(InnerState, LUA_GCSTOP);
    luaL_openlibs(InnerState);

    RegisterCustomLoader(InnerState, true);

    lua_newtable(InnerState);//TurinmaTable
    lua_newtable(InnerState);//TurinmaTable, metatable
    //lua_pushcfunction(InnerState,)


    //lua_setglobal(InnerState, );

    lua_getglobal(InnerState, "require");
    lua_pushstring(InnerState, "TurinmaLua.Core.Init");

    
    lua_pcall(InnerState, 1, LUA_MULTRET,0);
    lua_pop(InnerState, lua_gettop(InnerState));

    RegisterCustomLoader(InnerState, false);

    AddUsefulLuaTableWithMetatableToTable(InnerState, "InternalUseOnly___*___UObjectExtensionWeakTable", LUA_REGISTRYINDEX, true);
    AddUsefulLuaTableWithMetatableToTable(InnerState, "InternalUseOnly___*___UObjectExtensionStrongTable", LUA_REGISTRYINDEX, false);

    lua_pop(InnerState, lua_gettop(InnerState));

    luaL_newmetatable(InnerState, LuaUEDataMetatableName); // LuaUEDataMetatableName

    lua_pushstring(InnerState, "__gc");
    lua_pushcfunction(InnerState, OnDestroyUEDataInLua);
    lua_rawset(InnerState, -3);
    //todo finalize LuaUEDataMetatableName
    lua_pop(InnerState, 1);



    luaL_newmetatable(InnerState, LuaUEValueMetatableName); // LuaUEValueMetatableName

    lua_pushstring(InnerState, "__gc");
    lua_pushcfunction(InnerState, OnDestroyUEValueInLua);
    lua_rawset(InnerState, -3);
    lua_pushstring(InnerState, "__index");
    lua_pushcfunction(InnerState, LuaUEValueIndex);
    lua_rawset(InnerState, -3);


    //todo finalize LuaUEValueMetatableName
    lua_pop(InnerState, 1);


    FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &ULuaState::PostGarbageCollect);


    LuaCPPAPI::luaC_foreachgcobj(InnerState, OnTravelLuaGCObject);
}

void ULuaState::Finalize()
{
    if(InnerState)
    {
        FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);
        lua_close(InnerState);
        InnerState = nullptr;
    }
}

void ULuaState::Pop(int32 Num)
{
    if (InnerState)
    {
        int32 MaxPop = lua_gettop(InnerState);
        int32 P = FMath::Min(Num, MaxPop);
        if(P > 0)
        {
            lua_pop(InnerState, P);
        }
    }
}

void ULuaState::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
    ULuaState* This = CastChecked<ULuaState>(InThis);

    for(auto* V : This->AllValues)
    {
	    if(V)
	    {
            V->Marked = false;
	    }
    }

    if(This->InnerState)
    {
        LuaCPPAPI::luaC_foreachgcobj(This->InnerState, [This, &Collector](GCObject* o, bool w, lua_State* l)->void
            {
                This->OnCollectLuaRefs(Collector, o, w, l);
            });
    }

    for (auto&& V : This->AllValues)
    {
        if (V)
        {
            if(!V->Marked)
            {
                V->Marked = false;
                V->DestroyData();
                This->RemoveFormAllValues(V);
                check(!V)
            }
            else
            {
                V->Marked = false;
            }
        }
    }

}

void LuaLock(lua_State* L)
{
    if (G(L)->ud)
    {
        ULuaState* LuaState = (ULuaState*)(G(L)->ud);
        LuaState->LockLua();
    }
}

void LuaUnLock(lua_State* L)
{
    if(G(L)->ud)
    {
        ULuaState* LuaState = (ULuaState*)(G(L)->ud);
        LuaState->UnlockLua();
    }
}

void FLuaSourceModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    //FCoreUObjectDelegates::GetPostGarbageCollect();
    glua_lockcb = LuaLock;
    glua_unlockcb = LuaUnLock;
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
