#include "LuaUEValue.h"

void FLuaUEValue::AddReferencedObject(UObject* Oter, FReferenceCollector& Collector, bool bStrong)
{
    if (bStrong)
    {
        Marked = true;
    }
    if (TypeDesc.IsValid() && !Owner)
    {
        TypeDesc->AddReferencedObject(Oter, GetData(), Collector, bStrong);
    }
}