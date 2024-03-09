#include "LuaUEType.h"
#include "LuaSource.h"


EXTERN_C{
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"
#include "lstring.h"
}

void UUETypeDescContainer::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
    auto* This = CastChecked<UUETypeDescContainer>(InThis);
    FScopeLock Lock(&This->CS);
    for (auto&& It = This->Map.CreateIterator(); It; ++It)
    {
        if (It->Value.GetRefCount() == 1)
        {
            Collector.MarkWeakObjectReferenceForClearing(&It->Value->UserDefinedTypePointer.Pointer);
        }
        else
        {
            Collector.AddReferencedObject(It->Value->UserDefinedTypePointer.Pointer);
        }
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
        if (CombineKindSubTypes.Num() == 1 && CombineKindSubTypes[0].IsValid())
        {
            for (int Ai = 0; Ai < ArrayPtr->Num(); ++Ai)
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
    case ETypeKind::Enum: return;

    case ETypeKind::U_Enum: return;
    case ETypeKind::U_ScriptStruct:
        if (UserDefinedTypePointer.U_ScriptStruct)
        {
            Collector.AddPropertyReferences(UserDefinedTypePointer.U_ScriptStruct, PtrToValue, Oter);
        }
        return;
    case ETypeKind::U_Class:
    {
        if (bStrong)
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
    switch (Kind)
    {
    case ETypeKind::U_ScriptStruct: return UserDefinedTypePointer.U_ScriptStruct ? UserDefinedTypePointer.U_ScriptStruct->GetStructureSize() : 0;
    case ETypeKind::Enum: return CombineKindSubTypes[1] ? CombineKindSubTypes[1]->GetSize() : sizeof(uint64);
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
    case ETypeKind::Enum: return CombineKindSubTypes[1] ? CombineKindSubTypes[1]->GetAlign() : alignof(uint64);

    case ETypeKind::U_Enum: return alignof(uint64);
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
    case ETypeKind::Enum: FMemory::Memzero(ValueAddress, CombineKindSubTypes[1] ? CombineKindSubTypes[1]->GetSize() : sizeof(uint64)); return;

    case ETypeKind::U_Enum: new (ValueAddress) int64(0); return;
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

    if (ADe->Num() != ASo->Num())
    {
        return false;
    }
    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        if (!ElementType->ValueEqual((uint8*)ADe->GetData() + ElementType->GetSize() * Di, (uint8*)ASo->GetData() + ElementType->GetSize() * Di))
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
        if (!ElementType->ValueEqual(PairPtr1, PairPtr2))
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

    if (ADe->Num() != ASo->Num())
    {
        return false;
    }

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        auto PairPtr1 = ADe->GetData(Di, Layout);
        auto PairPtr2 = ASo->GetData(Di, Layout);
        if (!ElementType->ValueEqual(PairPtr1, PairPtr2))
        {
            return false;
        }
        if (!ValueType->ValueEqual((uint8*)PairPtr1 + Layout.ValueOffset, (uint8*)PairPtr2 + Layout.ValueOffset))
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

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        ElementType->DestroyValue((uint8*)ADe->GetData() + ElementType->GetSize() * Di);
    }
    ADe->Empty(0, ElementType->GetSize(), ElementType->GetAlign());

    for (int32 Si = 0; Si < ASo->Num(); ++Si)
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

    for (int32 Di = 0; Di < ADe->Num(); ++Di)
    {
        auto PairPtr = ADe->GetData(Di, Layout);
        KeyType->DestroyValue(PairPtr);
        ValueType->DestroyValue((uint8*)PairPtr + Layout.ValueOffset);
    }
    ADe->Empty(0, Layout);

    for (int32 Si = 0; Si < ASo->Num(); ++Si)
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
    case ETypeKind::Enum: FMemory::Memcpy(Dest, Source, CombineKindSubTypes[1] ? CombineKindSubTypes[1]->GetSize() : sizeof(uint64)); return;

    case ETypeKind::U_Enum:  RawSetValue<int64>(Dest, Source); return;
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
    case ETypeKind::Enum: return;

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
    case ETypeKind::Enum: return CombineKindSubTypes[1] ? CombineKindSubTypes[1]->GetTypeHash(ValueAddress) : RawGetValueHash<uint64>(ValueAddress);

    case ETypeKind::U_Enum: return RawGetValueHash<uint64>(ValueAddress);
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
    case ETypeKind::Enum: return CombineKindSubTypes[1] ? CombineKindSubTypes[1]->ValueEqual(Dest, Source) : RawCompairValue<uint64>(Dest, Source);

    case ETypeKind::U_Enum: return RawCompairValue<int64>(Dest, Source); ;
    case ETypeKind::U_ScriptStruct:
        if (UserDefinedTypePointer.U_ScriptStruct) {
            return
                UserDefinedTypePointer.U_ScriptStruct->CompareScriptStruct(Dest, Source, EPropertyPortFlags::PPF_None);
        }
        return false;
    case ETypeKind::U_Class:return RawCompairValue<UObject*>(Dest, Source); ;
    case ETypeKind::U_Function: return true;
    case ETypeKind::U_Interface:return RawCompairValue<FScriptInterface>(Dest, Source);
    default: return false;
    }
}

int FTypeDesc::ToLuaValue(lua_State* L, void* ValueAddress, FLuaUEValue& ValueProxy) const
{
    int LuaTop = lua_gettop(L);
    switch (Kind)
    {
    case ETypeKind::None:
    {
        lua_pushnil(L);
        return 1;
    }
    case ETypeKind::Int8:
    {
        lua_pushinteger(L, *(int8*)ValueAddress);
        return 1;
    }
    case ETypeKind::Int16:
    {
        lua_pushinteger(L, *(int16*)ValueAddress);
        return 1;
    }
    case ETypeKind::Int32:
    {
        lua_pushinteger(L, *(int32*)ValueAddress);
        return 1;
    }
    case ETypeKind::Int64:
    {
        lua_pushinteger(L, *(int64*)ValueAddress);
        return 1;
    }
    case ETypeKind::Byte: {
        lua_pushinteger(L, *(uint8*)ValueAddress);
        return 1;
    }
    case ETypeKind::UInt16: {
        lua_pushinteger(L, *(uint16*)ValueAddress);
        return 1;
    };
    case ETypeKind::UInt32: {
        lua_pushinteger(L, *(uint32*)ValueAddress);
        return 1;
    }
    case ETypeKind::UInt64: {
        lua_pushinteger(L, *(uint64*)ValueAddress);
        return 1;
    }
    case ETypeKind::Float: {
        lua_pushnumber(L, *(float*)ValueAddress);
        return 1;
    }
    case ETypeKind::Double: {
        lua_pushnumber(L, *(double*)ValueAddress);
        return 1;
    }


    case ETypeKind::Bool: {
        lua_pushboolean(L, *(bool*)ValueAddress);
        return 1;
    }
    case ETypeKind::BitBool: break;
    case ETypeKind::String: {
        FString* Str = (FString*)ValueAddress;
        lua_pushstring(L, TCHAR_TO_UTF8(**Str));
        return 1;
    }
    case ETypeKind::Name: {
        FName* Str = (FName*)ValueAddress;
        lua_pushstring(L, TCHAR_TO_UTF8(*Str->ToString()));
        return 1;
    }
    case ETypeKind::Text: {
        FText* Str = (FText*)ValueAddress;
        lua_pushstring(L, TCHAR_TO_UTF8(*Str->ToString()));
        return 1;
    };
    case ETypeKind::Vector:
    {
        FVector* Vec = (FVector*)ValueAddress;
        lua_newtable(L);
        lua_pushnumber(L, Vec->X);
        lua_setfield(L, -2, "X");
        lua_pushnumber(L, Vec->Y);
        lua_setfield(L, -2, "Y");
        lua_pushnumber(L, Vec->Z);
        lua_setfield(L, -2, "Z");

        return 1;
    };
    case ETypeKind::Rotator: {
        FRotator* Vec = (FRotator*)ValueAddress;
        lua_newtable(L);
        lua_pushnumber(L, Vec->Pitch);
        lua_setfield(L, -2, "Pitch");
        lua_pushnumber(L, Vec->Yaw);
        lua_setfield(L, -2, "Yaw");
        lua_pushnumber(L, Vec->Roll);
        lua_setfield(L, -2, "Roll");

        return 1;
    };
    case ETypeKind::Quat: break;
    case ETypeKind::Transform: break;
    case ETypeKind::Matrix: break;
    case ETypeKind::RotationMatrix: break;
    case ETypeKind::Color: break;
    case ETypeKind::LinearColor: break;


    case ETypeKind::Array:
    {
        lua_newtable(L);//table
        ULuaState* UEStat = (ULuaState*)G(L)->ud;
        FScriptArray* A = (FScriptArray*)ValueAddress;
        for (int32 IA = 0; IA < A->Num(); ++IA)
        {
            void* ElementPtr = (uint8*)A->GetData() + CombineKindSubTypes[0]->GetSize() * IA;
            UEStat->PushLuaUEValue(ElementPtr, ValueProxy, CombineKindSubTypes[0]); //table, value
            lua_rawseti(L, -2, IA);//table
        }
        return 1;
    };
    case ETypeKind::Map: break;
    case ETypeKind::Set: break;
    case ETypeKind::Delegate: break;
    case ETypeKind::MulticastDelegate: break;
    case ETypeKind::WeakObject: break;
    case ETypeKind::Enum:
    {
        int64 EV = 0;
        if (CombineKindSubTypes[1])
        {
            switch (CombineKindSubTypes[1]->Kind)
            {
            case ETypeKind::Byte:
                EV = *(uint8*)ValueAddress;
                break;
            case ETypeKind::UInt16:
                EV = *(uint16*)ValueAddress;
                break;
            case ETypeKind::UInt32:
                EV = *(uint32*)ValueAddress;
                break;
            case ETypeKind::UInt64:
                EV = *(uint64*)ValueAddress;
                break;
            case ETypeKind::Int8:
                EV = *(int8*)ValueAddress;
                break;
            case ETypeKind::Int16:
                EV = *(int16*)ValueAddress;
                break;
            case ETypeKind::Int32:
                EV = *(int32*)ValueAddress;
                break;
            case ETypeKind::Int64:
                EV = *(int64*)ValueAddress;
                break;
            default:
                break;
            }
        }
        else
        {
            EV = *(int64*)ValueAddress;
        }
        lua_pushstring(L, TCHAR_TO_UTF8(*UserDefinedTypePointer.U_Enum->GetNameByValue(EV).ToString()));
        return 1;
    };

    case ETypeKind::U_Enum: break;
    case ETypeKind::U_ScriptStruct:
    {
        lua_newtable(L);//table
        ULuaState* UEStat = (ULuaState*)G(L)->ud;
        for (TFieldIterator<FProperty> It(UserDefinedTypePointer.U_ScriptStruct); It; ++It)
        {
            void* PropPtr = It->ContainerPtrToValuePtr<void>(ValueAddress);
            UEStat->PushLuaUEValue(PropPtr, ValueProxy, AquireClassDescByProperty(*It)); //table, value
            lua_setfield(L, -2, TCHAR_TO_UTF8(*It->GetName()));//table
        }
        return 1;
        //UserDefinedTypePointer.U_ScriptStruct
    };
    case ETypeKind::U_Class: break;
    case ETypeKind::U_Function: break;
    case ETypeKind::U_Interface: break;
    case ETypeKind::UserDefinedKindEnd: break;
    default:;
    }

    if (lua_gettop(L) - LuaTop == 0)
    {
        lua_pushnil(L);
    }
    check(lua_gettop(L) - LuaTop == 1);
    return 1;
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
    case ETypeKind::Enum: return 0;

    case ETypeKind::U_Enum: return sizeof(uint64);
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
            for (auto&& It = Container->Map.CreateIterator(); It; ++It)
            {
                if (It->Value->UserDefinedTypePointer.Pointer == nullptr)
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
    if (UEType)
    {
        ETypeKind Kind = ETypeKind::None;
        if (UClass* Class = Cast<UClass>(UEType))
        {
            if (Class->HasAnyClassFlags(CLASS_Interface))
            {
                Kind = ETypeKind::U_Interface;
            }
            else
            {
                Kind = ETypeKind::U_Class;
            }
        }
        if (UScriptStruct* ScriptStruct = Cast<UScriptStruct>(UEType))
        {
            Kind = ETypeKind::U_ScriptStruct;
        }
        if (UEnum* Enum = Cast<UEnum>(UEType))
        {
            Kind = ETypeKind::U_Enum;
        }
        if (UFunction* Function = Cast<UFunction>(UEType))
        {
            Kind = ETypeKind::U_Function;
        }

        if (Kind != ETypeKind::None)
        {
            auto&& Item = Container->Map.FindOrAdd(UEType);
            if (!Item.GetReference())
            {
                Item = new FTypeDesc();
            }
            check(Item->Kind == ETypeKind::None ||
                (Item->Kind >= ETypeKind::UserDefinedKindBegin && Item->Kind < ETypeKind::UserDefinedKindEnd));
            Item->Kind = Kind;
            if (!Item->UserDefinedTypePointer.Pointer)
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
    if (InKind >= ETypeKind::PreDefinedKindBegin && InKind < ETypeKind::PreDefinedKindEnd)
    {
        static TMap<ETypeKind, FTypeDescRefCount> Map;
        auto&& Item = Map.FindOrAdd(InKind);
        if (!Item)
        {
            Item = new FTypeDesc();
        }
        check(Item->Kind == ETypeKind::None || Item->Kind == InKind)
            if (Item->Kind == ETypeKind::None)
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
            for (auto&& M : Map)
            {
                for (auto&& It = M.CreateIterator(); It; ++It)
                {
                    if (It->Value.GetRefCount() == 1)
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

    if (InKind >= ETypeKind::CombineKindBegin && InKind < ETypeKind::CombineKindEnd)
    {
        switch (InKind)
        {
        case ETypeKind::WeakObject:
        case ETypeKind::Set:
        case ETypeKind::Array:
            if (InKey.Num() == 1 && InKey[0] != nullptr)
            {
                auto&& V = Container.Map[(uint16)InKind - (uint16)ETypeKind::CombineKindBegin].FindOrAdd(InKey);
                if (!V.GetReference())
                {
                    V = new FTypeDesc();
                    V->Kind = InKind;
                }
                check(V->Kind == InKind && (V->CombineKindSubTypes.Num() == 1 || V->CombineKindSubTypes.IsEmpty()));
                if (V->CombineKindSubTypes.IsEmpty())
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
            if (InKey.Num() == 2 && InKey[0] != nullptr && InKey[1] != nullptr)
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
                return V;
            }
            break;
        case ETypeKind::Delegate:
        case ETypeKind::MulticastDelegate:
            return Container.Map[(uint16)InKind - (uint16)ETypeKind::CombineKindBegin].FindOrAdd(TArray<TRefCountPtr<FTypeDesc>>());
            break;
        case ETypeKind::Enum:
            if (InKey.Num() == 2 && InKey[0] != nullptr)
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
                return V;
            }
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


    if (StructProperty)
    {
        return AquireClassDescByUEType(StructProperty->Struct);
    }
    if (ObjectProperty)
    {
        return AquireClassDescByUEType(ObjectProperty->PropertyClass);
    }
    if (InterfaceProperty)
    {
        return AquireClassDescByUEType(InterfaceProperty->InterfaceClass);
    }

    if (ArrayProperty)
    {
        auto InnerProperty = AquireClassDescByProperty(ArrayProperty->Inner);
        return AquireClassDescByCombineKind(ETypeKind::Array, { InnerProperty });
    }
    if (SetProperty)
    {
        auto InnerProperty = AquireClassDescByProperty(SetProperty->ElementProp);
        return AquireClassDescByCombineKind(ETypeKind::Set, { InnerProperty });
    }
    if (MapProperty)
    {
        auto InnerProperty = AquireClassDescByProperty(MapProperty->KeyProp);
        auto ValueProperty = AquireClassDescByProperty(MapProperty->ValueProp);
        return AquireClassDescByCombineKind(ETypeKind::Map, { InnerProperty, ValueProperty });
    }
    if (WeakObjectProperty)
    {
        auto InnerType = AquireClassDescByUEType(WeakObjectProperty->PropertyClass);
        return AquireClassDescByCombineKind(ETypeKind::WeakObject, { InnerType });
    }
    if (EnumProperty)
    {
        FNumericProperty* Under = EnumProperty->GetUnderlyingProperty();
        TRefCountPtr<FTypeDesc> D = nullptr;
        if (Under)
        {
            D = AquireClassDescByProperty(Under);
        }
        auto Inner = AquireClassDescByUEType(EnumProperty->GetEnum());
        return AquireClassDescByCombineKind(ETypeKind::Enum, { Inner, D });
    }

    if (DelegateProperty)
    {
        return AquireClassDescByCombineKind(ETypeKind::Delegate, { });
    }
    if (MulticastDelegateProperty)
    {
        return AquireClassDescByCombineKind(ETypeKind::MulticastDelegate, { });
    }

    return nullptr;
}