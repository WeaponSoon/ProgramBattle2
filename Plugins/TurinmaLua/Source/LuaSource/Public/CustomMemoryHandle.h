#pragma once
#include <unordered_map>
#include <vector>
#include <mutex>
#include <map>
#include <tuple>
#include <atomic>
#include <list>


#ifdef KF_IN_UE
#include "AssertionMacros.h"
#else
#ifndef check
#define check(expr)
#endif
#endif

#define check_kf(expr) if(!(expr)) *((int*)13) = 100; //write to a small address  to trigger an SEGV_MAPERR, because clang noticed writing to const NULL and warned about it

#ifdef KF_IN_UE
#if KF_IN_UE
#include "CoreMinimal.h"
#include "AssertionMacros.h"
#else
#define QUICK_SCOPE_CYCLE_COUNTER(Stat)
#define checkf(Exp, Msg, ...)
#endif
#else
#define QUICK_SCOPE_CYCLE_COUNTER(Stat)
#define checkf(Exp, Msg, ...)
#endif
#include <atomic>

#ifndef UE_BUILD_SHIPPING
#define UE_BUILD_SHIPPING 1
#endif

#ifndef UE_BUILD_TEST
#define UE_BUILD_TEST 0
#endif

#ifndef KF_VALID_SERVER
#define KF_VALID_SERVER 0
#endif

#ifndef KF_EDITOR
#define KF_EDITOR 0
#endif

#if UE_BUILD_TEST || UE_BUILD_SHIPPING || KF_VALID_SERVER
#define CHECK_ITEM_PAIR 0
#else
#define CHECK_ITEM_PAIR 1
#endif

#ifndef LUASOURCE_API 
#define  LUASOURCE_API 
#endif



//PRAGMA_DISABLE_OPTIMIZATION

////a lock free and waitless stack impl
//template<typename Element>
//class LUASOURCE_API FCustomLockFreeStack
//{
//private:
//	template<typename T = Element>
//	struct FElementItem
//	{
//		//no value(allow to add): 0,1
//		//operating(block any other op in the same slot): 1,1
//		//has value(allow to remove): 1,0
//		//invalid status: 0,0
//		std::atomic_flag operate_flag = ATOMIC_FLAG_INIT;
//		std::atomic_flag hasvalue_flag = ATOMIC_FLAG_INIT;
//		
//		std::aligned_storage<sizeof(T)> storage;
//
//		
//		FElementItem()
//		{
//			hasvalue_flag.test_and_set();
//		}
//	};
//	std::atomic_flag atomic_size_flag = ATOMIC_FLAG_INIT;
//	volatile int32_t size = 0;
//	
//	std::list<FElementItem<Element>*> inner_list;
//public:
//	template<typename PushT>
//	void push(PushT&& element)
//	{
//		//stack scope atomic: decide which slot to operate, the whole stack will be spin locked
//		while (atomic_size_flag.test_and_set())
//		{}
//		size++;
//		inner_list.push_back(new FElementItem<Element>());
//		FElementItem<Element>* ref = inner_list.back();
//		atomic_size_flag.clear();
//		//end stack scope atomic: decide which slot to operate
//
//		//slot scope operate: operate element, only block operations in the same slot, push or pop operations in other slot will be executed parallel
//		while (ref.operate_flag.test_and_set()) //wait until 0,1 and set to 1,1
//		{}
//
//		new (&ref->storage) Element(std::forward<PushT>(element));
//
//		ref->hasvalue_flag.clear();//set to 1,0
//		//end slot scope operate: operate element
//	}
//	bool pop(Element& Out)
//	{
//		//stack scope atomic: decide which slot to operate
//		while (atomic_size_flag.test_and_set())
//		{}
//		if (size <= 0)
//		{
//			atomic_size_flag.clear();
//			return false;
//		}
//		--size;
//		auto& last_iter = inner_list.back();
//		FElementItem<Element>* last_ref = *last_iter;
//		inner_list.pop_back();
//		atomic_size_flag.clear();
//		//end stack scope atomic: decide which slot to operate
//		
//		//slot scope operate: operate element, only block operations in the same slot, push or pop operations in other slot will be executed parallel
//		while ((*last_ref).hasvalue_flag.test_and_set()) //wait until 1,0 and set to 1,1
//		{}
//
//		Out = *reinterpret_cast<Element*>(&(*last_ref).storage);
//		reinterpret_cast<Element*>(&(*last_ref).storage)->~Element();
//		(*last_ref).operate_flag.clear(); //set to 0,1
//		//end slot scope operate: operate element
//		return true;
//	}
//};

#if !KF_VALID_SERVER
struct KFSpinMutex
{
	std::atomic_flag spin_flag;

	KFSpinMutex()
	{
		spin_flag.clear();
	}

	void lock()
	{
		while (spin_flag.test_and_set())
		{
		}
	}
	void unlock()
	{
		spin_flag.clear();
	}
};
#endif


struct FNewCustormMemoryIdPair
{
	friend class FCustomMemoryHandleBase;
public:
	FNewCustormMemoryIdPair() {}
	FNewCustormMemoryIdPair(const FNewCustormMemoryIdPair&) {}
	FNewCustormMemoryIdPair(FNewCustormMemoryIdPair&&) noexcept {}

	const FNewCustormMemoryIdPair& operator=(const FNewCustormMemoryIdPair&)
	{
		return *this;
	}
	const FNewCustormMemoryIdPair& operator=(FNewCustormMemoryIdPair&&) noexcept
	{
		return *this;
	}

	bool operator==(const FNewCustormMemoryIdPair&) const { return true; }
	bool operator!=(const FNewCustormMemoryIdPair&) const { return false; }
private:
	void CopyFrom(const FNewCustormMemoryIdPair& Other)
	{
		PtrPair = Other.PtrPair;
		CheckIndex = Other.CheckIndex;
	}
	void* PtrPair = nullptr;
	long long int CheckIndex = -1;
};


//KFObject的基类，所有KF类都继承自KFObject，因此不要直接继承这个类
class LUASOURCE_API FCustomMemoryItemBase
{
	friend class FCustomMemoryHandleBase;
public:
	FCustomMemoryItemBase();
	FCustomMemoryItemBase(const FCustomMemoryItemBase& Other);
	FCustomMemoryItemBase(FCustomMemoryItemBase&& OtherTemp);
	virtual ~FCustomMemoryItemBase();

	FCustomMemoryItemBase& operator=(const FCustomMemoryItemBase& InOther) = default;

	virtual bool IsCustomMemoryItemValid() const { return true; }
	virtual void DestroyCustomMomoryItem(){};

	bool IsInterface() const {return bIsInterface;}

	mutable unsigned int checkid = 0xffffffff;
private:
	mutable FNewCustormMemoryIdPair NewCustomMemoryPair;
	void Assign();
	void Unsign();
protected:
	bool bIsInterface = false;
};

//UE中的，需要跟KF交互的类（比如需要把自己的函数绑定到KF层的回调上）继承这个类
class LUASOURCE_API FCustomMemoryItemThirdParty : public FCustomMemoryItemBase
{

};




class LUASOURCE_API FCustomMemoryHandleBase
{
	friend class FCustomMemoryItemBase;
public:
	virtual ~FCustomMemoryHandleBase() = default;

	bool EverHasValueUntilReset() const
	{
		return NewInnerIdPair.PtrPair != nullptr;
	}

protected:
	std::ptrdiff_t ItemDiff = (std::ptrdiff_t)0;
	 
	FCustomMemoryItemBase* GetPtr()
	{
		{
			if (NewInnerIdPair.PtrPair == nullptr)
			{
				return nullptr;
			}
#if !KF_VALID_SERVER
			while (NewCurrentOperateIndex() == NewInnerIdPair.PtrPair) {} //slot is operating, spin and wait
#endif
			auto& ItemPtr = *NewInnerIdPair.PtrPair;// ItemPtrs[InnerIdPair.Index];
			if (ItemPtr.CheckIndex != NewInnerIdPair.CheckIndex)
			{
				return nullptr;
			}
			auto& ItemBasePtr = ItemPtr.ItemBase;
#if	!UE_BUILD_SHIPPING && !KF_VALID_SERVER
			checkf(ItemBasePtr, TEXT("ItemBasePtr should not be null"));
			checkf(ItemBasePtr->checkid == 0xffffffff, TEXT("ItemBasePtr Object: invalid checkid"));
#endif

#if KF_VALID_SERVER
			return ItemBasePtr;
#else 
			return (ItemBasePtr->IsCustomMemoryItemValid()) ? ItemBasePtr : nullptr;
#endif
		}
	}
	const FCustomMemoryItemBase* GetPtr() const
	{
		{
			if (NewInnerIdPair.PtrPair == nullptr)
			{
				return nullptr;
			}
#if !KF_VALID_SERVER
			while (NewCurrentOperateIndex() == NewInnerIdPair.PtrPair) {} //slot is operating, spin and wait
#endif
			auto& ItemPtr = *NewInnerIdPair.PtrPair;// ItemPtrs[InnerIdPair.Index];
			if (ItemPtr.CheckIndex != NewInnerIdPair.CheckIndex)
			{
				return nullptr;
			}
			auto& ItemBasePtr = ItemPtr.ItemBase;
#if	!UE_BUILD_SHIPPING && !KF_VALID_SERVER
			checkf(ItemBasePtr, TEXT("ItemBasePtr should not be null"));
			checkf(ItemBasePtr->checkid == 0xffffffff, TEXT("ItemBasePtr Object: invalid checkid %d"), ItemBasePtr->checkid);
#endif

#if KF_VALID_SERVER
			return ItemBasePtr;
#else 
			return (ItemBasePtr->IsCustomMemoryItemValid()) ? ItemBasePtr : nullptr;
#endif
		}
	}
private:

	struct FCustormMemoryPtrPair
	{
		friend class FCustomMemoryHandleBase;
	private:
		long long int CheckIndex = -1;
		FCustomMemoryItemBase* ItemBase = nullptr;
	};
	
	struct NewInnerFCustormMemoryIdPair
	{
		friend class FCustomMemoryHandleBase;
	public:
		void CopyFrom(const FNewCustormMemoryIdPair& Other)
		{
			PtrPair = reinterpret_cast<FCustormMemoryPtrPair*>(Other.PtrPair);
			CheckIndex = Other.CheckIndex;
		}
		FCustormMemoryPtrPair* PtrPair = nullptr;
		long long int CheckIndex = -1;
	};

	NewInnerFCustormMemoryIdPair NewInnerIdPair;

protected:
#if !KF_VALID_SERVER
	static KFSpinMutex& ItemMutex();
#endif
private:
	static long long& CurrentCheckId();
	static std::unordered_map<const FCustomMemoryItemBase*, FNewCustormMemoryIdPair>& NewCustomMemoryItemMap();
	static std::list<FCustormMemoryPtrPair>& NewCustomMemoryItemPtrs();
	static std::vector<FCustormMemoryPtrPair*>& NewUnusedIndices();


#if !KF_VALID_SERVER
	static FCustormMemoryPtrPair*& NewCurrentOperateIndex();
#endif
protected:
	static void Assign(const FCustomMemoryItemBase* InItemBase);
	static void Unsign(const FCustomMemoryItemBase* InItemBase);

	void AssignToMe(const FCustomMemoryItemBase* InItemBase){
		if (!InItemBase)
		{
			NewInnerIdPair.PtrPair = nullptr;
			NewInnerIdPair.CheckIndex = -1;
			return;
		}
#if !KF_VALID_SERVER
		ItemMutex().lock();
#endif
		FNewCustormMemoryIdPair& IdPair = InItemBase->NewCustomMemoryPair;

#if CHECK_ITEM_PAIR
		{
			auto It = NewCustomMemoryItemMap().find(InItemBase);
			if (It != NewCustomMemoryItemMap().end())
			{
				checkf(It->second.CheckIndex == IdPair.CheckIndex && It->second.PtrPair == IdPair.PtrPair, TEXT("ItemPair not match"));
			}
		}
#endif

		NewInnerIdPair.CopyFrom(IdPair);
#if !KF_VALID_SERVER
		ItemMutex().unlock();
#endif	
	}
public:
	std::tuple<void*, long long int> GetInnerPair() const {
		void* Index = (NewInnerIdPair.PtrPair);
		long long CheckIndex = NewInnerIdPair.CheckIndex;
		return std::make_tuple(Index, CheckIndex);
	}
};

struct TbInterfaceTrue {};
struct TbInterfaceFalse {};

template<typename HandledClass> 
class TCustomMemoryHandle final : public FCustomMemoryHandleBase
{
public:
	//static_assert(std::is_base_of<FCustomMemoryItemBase, HandledClass>::value, "HandledClass must inherit form FCustomMemoryItemBase");
	
	inline HandledClass* Get() const;
private:
	template<typename T = HandledClass>
	inline typename std::enable_if<!std::is_base_of<FCustomMemoryItemBase, T>::value, const HandledClass*>::type
	Get_Internal() const;

	template<typename T = HandledClass>
	inline typename std::enable_if<std::is_base_of<FCustomMemoryItemBase, T>::value, const HandledClass*>::type
	Get_Internal() const;

	template<typename T = HandledClass>
	typename std::enable_if<!std::is_base_of<FCustomMemoryItemBase, T>::value>::type
	AssignInternal(const HandledClass* HandledClassItem);

	template<typename T = HandledClass>
	typename std::enable_if<std::is_base_of<FCustomMemoryItemBase, T>::value>::type
	AssignInternal(const HandledClass* HandledClassItem);

public:

	using HandledType = HandledClass;

	TCustomMemoryHandle() = default;
	TCustomMemoryHandle(decltype(nullptr)) {}
	
	TCustomMemoryHandle(const HandledClass* InOrigin);
	
	TCustomMemoryHandle<HandledClass>& operator=(decltype(nullptr))
	{
		AssignToMe(nullptr);
		return *this;
	}

	TCustomMemoryHandle<HandledClass>& operator=(HandledClass* InOriginPtr);
	

	bool operator==(const TCustomMemoryHandle<HandledClass>& Other) const
	{
		return Get() == Other.Get();
	}

	template<typename ComHandledClass>
	bool operator==(const TCustomMemoryHandle<ComHandledClass>& Other) const
	{
		static_assert(std::is_base_of<HandledClass, ComHandledClass>::value || std::is_base_of<ComHandledClass, HandledClass>::value, "Error");
		return Get() == Other.Get();
	}

	bool operator==(const HandledClass* Other) const
	{
		return Get() == Other;
	}
	
	operator bool() const { return nullptr != Get();}

	HandledClass* operator->() const
	{
		auto* innerPtr = Get();
		check(innerPtr);
		return innerPtr;
	}
};


namespace std
{
template<typename HandleClass>
struct less<TCustomMemoryHandle<HandleClass>>
{
	bool operator()(const TCustomMemoryHandle<HandleClass>& A, const TCustomMemoryHandle<HandleClass>& B)
	{
		auto&& APair = A.GetInnerPair();
		auto&& BPair = B.GetInnerPair();
		if (std::get<0>(APair)  != std::get<0>(BPair))
		{
			return std::get<0>(APair) < std::get<0>(BPair);
		}
		else
		{
			return std::get<1>(APair) < std::get<1>(BPair);
		}
	}
};
}

//KF中的接口继承这个类，不要用虚继承
template<typename HandledClass>
class TCustomMemoryItem
{
	friend class TCustomMemoryHandle<HandledClass>;
public:
	class TCustomMemoryItemInterface : public FCustomMemoryItemBase
	{
	public:
		TCustomMemoryItem<HandledClass>* CustomMemoryItem;
		TCustomMemoryItemInterface(TCustomMemoryItem<HandledClass>* InT) : CustomMemoryItem(InT) { bIsInterface = true; }
		virtual bool IsCustomMemoryItemValid() const override {
			return CustomMemoryItem ? CustomMemoryItem->IsCustomMemoryItemValid() : false;
		}
	};
private:
	TCustomMemoryItemInterface InnerItemBase;
public:
	TCustomMemoryItem() : InnerItemBase(this) {}
	TCustomMemoryItem(const TCustomMemoryItem& Other) : InnerItemBase(this) {}
	TCustomMemoryItem(TCustomMemoryItem&& OtherTemp) : InnerItemBase(this) {}
	TCustomMemoryItem& operator=(const TCustomMemoryItem& Other)
	{
		return *this;
	}
	virtual ~TCustomMemoryItem() = default;
	virtual bool IsCustomMemoryItemValid() const { return true; }
};


template<typename HandledClass>
TCustomMemoryHandle<HandledClass>::TCustomMemoryHandle(const HandledClass* InOrigin)
{
	static_assert(std::is_base_of<FCustomMemoryItemBase, HandledClass>::value || std::is_base_of<TCustomMemoryItem<HandledClass>, HandledClass>::value,
		"HandledClass must inherit form FCustomMemoryItemBase");

	AssignInternal(InOrigin);
}

template<typename HandledClass>
inline TCustomMemoryHandle<HandledClass>& TCustomMemoryHandle<HandledClass>::operator=(HandledClass* InOriginPtr)
{
	static_assert(std::is_base_of<FCustomMemoryItemBase, HandledClass>::value || std::is_base_of<TCustomMemoryItem<HandledClass>, HandledClass>::value,
		"HandledClass must inherit form FCustomMemoryItemBase");


	AssignInternal(InOriginPtr);

	return *this;
}


template<typename HandledClass>
template<typename T>
inline typename std::enable_if<std::is_base_of<FCustomMemoryItemBase, T>::value>::type
TCustomMemoryHandle<HandledClass>::AssignInternal(const HandledClass* HandledClassItem)
{
	const FCustomMemoryItemBase* MemoryItemBase = static_cast<const FCustomMemoryItemBase*>(HandledClassItem);
	if (MemoryItemBase && !MemoryItemBase->IsInterface())
	{
		ItemDiff = reinterpret_cast<std::ptrdiff_t>(HandledClassItem) - reinterpret_cast<std::ptrdiff_t>(MemoryItemBase);
		AssignToMe(MemoryItemBase);
	}
	else
	{
		ItemDiff = 0;
		AssignToMe(nullptr);
	}
}

template<typename HandledClass>
template<typename T>
inline typename std::enable_if<!std::is_base_of<FCustomMemoryItemBase, T>::value>::type
TCustomMemoryHandle<HandledClass>::AssignInternal(const HandledClass* HandledClassItem)
{
	const TCustomMemoryItem<HandledClass>* InterfaceItem = static_cast<const TCustomMemoryItem<HandledClass>*>(HandledClassItem);
	ItemDiff = 0;
	if (InterfaceItem && InterfaceItem->InnerItemBase.IsInterface())
	{
		AssignToMe(&InterfaceItem->InnerItemBase);
	}
	else
	{
		AssignToMe(nullptr);
	}
}

//template<typename HandledClass>
//HandledClass* TCustomMemoryHandle<HandledClass>::Get()
//{
//	static_assert(std::is_base_of<FCustomMemoryItemBase, HandledClass>::value || std::is_base_of<TCustomMemoryItem<HandledClass>, HandledClass>::value,
//		"HandledClass must inherit form FCustomMemoryItemBase");
//
//	return const_cast<HandledClass*>(Get_Internal());
//}

template<typename HandledClass>
inline HandledClass* TCustomMemoryHandle<HandledClass>::Get() const
{
	//std::lock_guard<std::recursive_mutex> lock(ItemMutex());
	static_assert(std::is_base_of<FCustomMemoryItemBase, HandledClass>::value || std::is_base_of<TCustomMemoryItem<HandledClass>, HandledClass>::value,
		"HandledClass must inherit form FCustomMemoryItemBase");

	return const_cast<HandledClass*>(Get_Internal());
}


template<typename HandledClass>
template<typename T>
inline typename std::enable_if<!std::is_base_of<FCustomMemoryItemBase, T>::value, const HandledClass*>::type
TCustomMemoryHandle<HandledClass>::Get_Internal() const
{
	const FCustomMemoryItemBase* BaseItemPtr = GetPtr();
	if (BaseItemPtr && BaseItemPtr->IsInterface())
	{
		const typename TCustomMemoryItem<HandledClass>::TCustomMemoryItemInterface* InterfaceItem
			= static_cast<const typename TCustomMemoryItem<HandledClass>::TCustomMemoryItemInterface*>(BaseItemPtr);
		const TCustomMemoryItem<HandledClass>* InterfaceBaseItem = InterfaceItem->CustomMemoryItem;
		return static_cast<const HandledClass*>(InterfaceBaseItem);
	}
	return nullptr;
}

template<typename HandledClass>
template<typename T>
inline typename std::enable_if<std::is_base_of<FCustomMemoryItemBase, T>::value, const HandledClass*>::type
TCustomMemoryHandle<HandledClass>::Get_Internal() const
{
	const FCustomMemoryItemBase* BaseItemPtr = GetPtr();
	if (BaseItemPtr && !BaseItemPtr->IsInterface())
	{
		return static_cast<const HandledClass*>(BaseItemPtr);
		//return reinterpret_cast<HandledClass*>(reinterpret_cast<std::ptrdiff_t>(BaseItemPtr) + ItemDiff);
	}
	return nullptr;
}

template<typename HandledClass>
class TConstCustomMemoryHandle final
{
private:
	TCustomMemoryHandle<HandledClass> InnerHandle;

public:
	TConstCustomMemoryHandle() = default;
	TConstCustomMemoryHandle(decltype(nullptr)) : InnerHandle(nullptr) {}
	TConstCustomMemoryHandle(const HandledClass* InOrigin) : InnerHandle(InOrigin) {}
	TConstCustomMemoryHandle(const TCustomMemoryHandle<HandledClass>& InOrigin) : InnerHandle(InOrigin) {}

	TConstCustomMemoryHandle<HandledClass>& operator=(decltype(nullptr))
	{
		InnerHandle = nullptr;
		return *this;
	}

	TConstCustomMemoryHandle<HandledClass>& operator=(const TCustomMemoryHandle<HandledClass>& InOriginHandle)
	{
		InnerHandle = InOriginHandle;
		return *this;
	}

	TConstCustomMemoryHandle<HandledClass>& operator=(const HandledClass* InOriginPtr)
	{
		InnerHandle = InOriginPtr;
		return *this;
	}

	bool operator==(const TConstCustomMemoryHandle<HandledClass>& Other) const
	{
		return InnerHandle == Other.InnerHandle;
	}

	bool operator==(const TCustomMemoryHandle<HandledClass>& Other) const
	{
		return InnerHandle == Other;
	}

	template<typename ComHandledClass>
	bool operator==(const TConstCustomMemoryHandle<ComHandledClass>& Other) const
	{
		static_assert(std::is_base_of<HandledClass, ComHandledClass>::value || std::is_base_of<ComHandledClass, HandledClass>::value, "Error");
		return Get() == Other.Get();// InnerHandle == Other.InnerHandle;
	}

	template<typename ComHandledClass>
	bool operator==(const TCustomMemoryHandle<ComHandledClass>& Other) const
	{
		static_assert(std::is_base_of<HandledClass, ComHandledClass>::value || std::is_base_of<ComHandledClass, HandledClass>::value, "Error");
		return InnerHandle == Other;
	}

	bool operator==(const HandledClass* Other) const
	{
		return Get() == Other;
	}

	operator bool() const { return nullptr != Get(); }



	const HandledClass* Get() const {
		return InnerHandle.Get();
	}

	const HandledClass* operator->() const {
		return Get();
	}

	std::tuple<void*, long long int> GetInnerPair() const
	{
		return InnerHandle.GetInnerPair();
	}
};

namespace std
{
	template<typename HandleClass>
	struct less<TConstCustomMemoryHandle<HandleClass>>
	{
		bool operator()(const TConstCustomMemoryHandle<HandleClass>& A, const TConstCustomMemoryHandle<HandleClass>& B)
		{
			auto&& APair = A.GetInnerPair();
			auto&& BPair = B.GetInnerPair();
			if (std::get<0>(APair) != std::get<0>(BPair))
			{
				return std::get<0>(APair) < std::get<0>(BPair);
			}
			else
			{
				return std::get<1>(APair) < std::get<1>(BPair);
			}
		}
	};
}

#if !defined(RED_HOTFIX_PARSE) && !KF_VALID_SERVER 

#endif

//PRAGMA_ENABLE_OPTIMIZATION