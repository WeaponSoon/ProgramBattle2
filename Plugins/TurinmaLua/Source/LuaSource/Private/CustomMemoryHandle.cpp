#include "CustomMemoryHandle.h"



//PRAGMA_DISABLE_OPTIMIZATION

FCustomMemoryItemBase ItemBaseDummyForSafty;
//
//FCustomMemoryItemBase* FCustomMemoryHandleBase::GetPtr()
//{
//	if (NewInnerIdPair.PtrPair == nullptr)
//	{
//		return nullptr;
//	}
//#if !KF_VALID_SERVER
//	while(NewCurrentOperateIndex() == NewInnerIdPair.PtrPair){} //slot is operating, spin and wait
//#endif
//	auto& ItemPtr = *NewInnerIdPair.PtrPair;// ItemPtrs[InnerIdPair.Index];
//	if(ItemPtr.CheckIndex != NewInnerIdPair.CheckIndex)
//	{
//		return nullptr;
//	}
//	auto& ItemBasePtr = ItemPtr.ItemBase;
//#if	!UE_BUILD_SHIPPING && !KF_VALID_SERVER
//	checkf(ItemBasePtr, TEXT("ItemBasePtr should not be null"));
//	checkf(ItemBasePtr->checkid == 0xffffffff, TEXT("ItemBasePtr Object: invalid checkid"));
//#endif
//
//#if KF_VALID_SERVER
//	return ItemBasePtr;
//#else 
//	return (ItemBasePtr->IsCustomMemoryItemValid()) ? ItemBasePtr : nullptr;
//#endif
//}

//const FCustomMemoryItemBase* FCustomMemoryHandleBase::GetPtr() const
//{
//	if (NewInnerIdPair.PtrPair == nullptr)
//	{
//		return nullptr;
//	}
//#if !KF_VALID_SERVER
//	while (NewCurrentOperateIndex() == NewInnerIdPair.PtrPair) {} //slot is operating, spin and wait
//#endif
//	auto& ItemPtr = *NewInnerIdPair.PtrPair;// ItemPtrs[InnerIdPair.Index];
//	if (ItemPtr.CheckIndex != NewInnerIdPair.CheckIndex)
//	{
//		return nullptr;
//	}
//	auto& ItemBasePtr = ItemPtr.ItemBase;
//#if	!UE_BUILD_SHIPPING && !KF_VALID_SERVER
//	checkf(ItemBasePtr, TEXT("ItemBasePtr should not be null"));
//	checkf(ItemBasePtr->checkid == 0xffffffff, TEXT("ItemBasePtr Object: invalid checkid %d"), ItemBasePtr->checkid);
//#endif
//	
//#if KF_VALID_SERVER
//	return ItemBasePtr;
//#else 
//	return (ItemBasePtr->IsCustomMemoryItemValid()) ? ItemBasePtr : nullptr;
//#endif
//}
// 

#if !KF_VALID_SERVER
KFSpinMutex& FCustomMemoryHandleBase::ItemMutex()
{
	static KFSpinMutex InnerItemMutex;
	return InnerItemMutex;
}
#endif

long long& FCustomMemoryHandleBase::CurrentCheckId()
{
	static long long InnerCurrentCheckId = 0;
	return InnerCurrentCheckId;
}

std::unordered_map<const FCustomMemoryItemBase*, FNewCustormMemoryIdPair>& FCustomMemoryHandleBase::NewCustomMemoryItemMap()
{
	static std::unordered_map<const FCustomMemoryItemBase*, FNewCustormMemoryIdPair> InnerMap;
	return InnerMap;
}

std::list<FCustomMemoryHandleBase::FCustormMemoryPtrPair>& FCustomMemoryHandleBase::NewCustomMemoryItemPtrs()
{
	static std::list<FCustomMemoryHandleBase::FCustormMemoryPtrPair> InnerList;
	return InnerList;
}

std::vector<FCustomMemoryHandleBase::FCustormMemoryPtrPair*>& FCustomMemoryHandleBase::NewUnusedIndices()
{
	static std::vector<FCustomMemoryHandleBase::FCustormMemoryPtrPair*> InnerVector;
	return InnerVector;
}

#if !KF_VALID_SERVER

FCustomMemoryHandleBase::FCustormMemoryPtrPair*& FCustomMemoryHandleBase::NewCurrentOperateIndex()
{
	static FCustomMemoryHandleBase::FCustormMemoryPtrPair* Inner = nullptr;
	return Inner;
}

#endif

void FCustomMemoryHandleBase::Assign(const FCustomMemoryItemBase* InItemBase)
{
#if !KF_VALID_SERVER
	ItemMutex().lock();
#endif
	CurrentCheckId()++;
	FCustormMemoryPtrPair* UnusedId = nullptr;
	if(NewUnusedIndices().size() > 0)
	{
		UnusedId = NewUnusedIndices().back();
#if !KF_VALID_SERVER
		NewCurrentOperateIndex() = UnusedId;
#endif
		NewUnusedIndices().pop_back();
	}
	else
	{
		NewCustomMemoryItemPtrs().emplace_back();
		UnusedId = &NewCustomMemoryItemPtrs().back();
#if !KF_VALID_SERVER
		NewCurrentOperateIndex() = UnusedId;
#endif
		//CustomMemoryItemPtrs().push_back(FCustormMemoryPtrPair());
	}
	
#if CHECK_ITEM_PAIR
	FNewCustormMemoryIdPair& IdPair = NewCustomMemoryItemMap()[InItemBase];
#else
	FNewCustormMemoryIdPair IdPair;
#endif
	IdPair.PtrPair = UnusedId;
	IdPair.CheckIndex = CurrentCheckId();
	InItemBase->NewCustomMemoryPair.CopyFrom(IdPair);
	InItemBase->checkid = 0xffffffff;
	UnusedId->CheckIndex = CurrentCheckId();
	UnusedId->ItemBase = const_cast<FCustomMemoryItemBase*>(InItemBase);
#if !KF_VALID_SERVER
	NewCurrentOperateIndex() = nullptr;
	ItemMutex().unlock();
#endif
}

void FCustomMemoryHandleBase::Unsign(const FCustomMemoryItemBase* InItemBase)
{
#if !KF_VALID_SERVER
	ItemMutex().lock();
#endif
	
#if !UE_BUILD_SHIPPING && !KF_VALID_SERVER
	checkf(InItemBase, TEXT("InItemBase should not be null"));
	checkf(InItemBase->checkid == 0xffffffff, TEXT("InItemBase Object: invalid checkid %d"), InItemBase->checkid);
#endif

#if CHECK_ITEM_PAIR
	auto It = NewCustomMemoryItemMap().find(InItemBase);
#endif

#if !UE_BUILD_SHIPPING && !KF_VALID_SERVER
#if CHECK_ITEM_PAIR
	checkf(It != NewCustomMemoryItemMap().end(), TEXT("InItemBase must exsist in map"))
#endif
#endif
	
	InItemBase->checkid = 0x00000000;

	FNewCustormMemoryIdPair& IdPair = InItemBase->NewCustomMemoryPair;
#if !KF_VALID_SERVER
	NewCurrentOperateIndex() = reinterpret_cast<FCustormMemoryPtrPair*>(IdPair.PtrPair);
#endif
	
#if CHECK_ITEM_PAIR
	checkf(It->second.CheckIndex == IdPair.CheckIndex && It->second.PtrPair == IdPair.PtrPair, TEXT("ItemPair not match"));
#endif
	auto* PairPtr = reinterpret_cast<FCustormMemoryPtrPair*>(IdPair.PtrPair);
	PairPtr->CheckIndex = -1;
	PairPtr->ItemBase = nullptr;
	NewUnusedIndices().push_back(PairPtr);
#if CHECK_ITEM_PAIR
	NewCustomMemoryItemMap().erase(It);
#endif
	IdPair.CheckIndex = -1;
	IdPair.PtrPair = nullptr;
#if !KF_VALID_SERVER
	NewCurrentOperateIndex() = nullptr;
	ItemMutex().unlock();
#endif
}

FCustomMemoryItemBase::FCustomMemoryItemBase()
{
	Assign();
}

FCustomMemoryItemBase::FCustomMemoryItemBase(const FCustomMemoryItemBase& Other)
{
	Assign();
}

FCustomMemoryItemBase::FCustomMemoryItemBase(FCustomMemoryItemBase&& OtherTemp)
{
	Assign();
}

FCustomMemoryItemBase::~FCustomMemoryItemBase()
{
	Unsign();
}

void FCustomMemoryItemBase::Assign()
{
	FCustomMemoryHandleBase::Assign(static_cast<FCustomMemoryItemBase*>(this));
}

void FCustomMemoryItemBase::Unsign()
{
	FCustomMemoryHandleBase::Unsign(static_cast<FCustomMemoryItemBase*>(this));
}
//PRAGMA_ENABLE_OPTIMIZATION