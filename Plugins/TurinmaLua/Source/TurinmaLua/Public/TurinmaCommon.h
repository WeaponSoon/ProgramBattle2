#pragma once

#include "CoreMinimal.h"

class FAR
{
public:
	void F()
	{
		typedef typename TChooseClass<
			FDefaultAllocator::NeedsElementType,
			typename FDefaultAllocator::template ForElementType<FString>,
			typename FDefaultAllocator::ForAnyElementType
		>::Result ElementAllocatorType;
		ElementAllocatorType AllocatorInstance;

		
	}
};

template <typename InElementType, typename SizeType = int32>
class TTurinmaCircularQueue
{
public:
	static constexpr SizeType DEFAULT_CAPACITY = 4;
	static constexpr int32 RESIZE_FACTOR = 2;
private:
	
	SizeType             Head;
	SizeType             Tail;
	SizeType			 ArrayMax;//including an empty tail
private:
	using USizeType = typename TMakeUnsigned<SizeType>::Type;
	InElementType* InnerBuffer;

	template<typename U = InElementType>
	void DestroyCapacity(typename std::enable_if<std::is_trivially_destructible<U>::value, void*>::type Dst, SizeType Count)
	{
		//do nothing
	}
	template<typename U = InElementType>
	void DestroyCapacity(typename std::enable_if<!std::is_trivially_destructible<U>::value, void*>::type Dst, SizeType Count)
	{
		for (int i = 0; i < Count; ++i)
		{
			(static_cast<InElementType*>(Dst) + i)->~InElementType();
		}
	}


	template<typename U = InElementType>
	void CopyOrMoveToNewCapacity(typename std::enable_if<std::is_trivially_copyable<U>::value, void*>::type Dst, void* Src, SizeType Count)
	{
		FMemory::Memcpy(Dst, Src, Count * sizeof(InElementType));
	}
	template<typename U = InElementType>
	void CopyOrMove(typename std::enable_if<std::is_nothrow_move_constructible<U>::value, InElementType*>::type Dst, InElementType* Src)
	{
		new (Dst) InElementType(MoveTemp(*Src));
	}
	template<typename U = InElementType>
	void CopyOrMove(typename std::enable_if<!std::is_nothrow_move_constructible<U>::value, InElementType*>::type Dst, InElementType* Src)
	{
		new (Dst) InElementType(*Src);
	}
	template<typename U = InElementType>
	void CopyOrMoveToNewCapacity(typename std::enable_if<!std::is_trivially_copyable<U>::value, void*>::type Dst, void* Src, SizeType Count)
	{
		for(int i = 0; i < Count; ++i)
		{
			CopyOrMove(static_cast<InElementType*>(Dst) + i, static_cast<InElementType*>(Src) + i);
		}
	}
	template<typename U = InElementType>
	void CopyToNewCapacity(typename std::enable_if<std::is_trivially_copyable<U>::value, void*>::type Dst, void* Src, SizeType Count)
	{
		FMemory::Memcpy(Dst, Src, Count * sizeof(InElementType));
	}
	template<typename U = InElementType>
	void CopyToNewCapacity(typename std::enable_if<!std::is_trivially_copyable<U>::value, void*>::type Dst, void* Src, SizeType Count)
	{
		for (int i = 0; i < Count; ++i)
		{
			new (static_cast<InElementType*>(Dst) + i) InElementType(*(static_cast<InElementType*>(Src) + i));
		}
	}


	//void CopyOrMoveToNewPosition(typename std::enable_if<std::is_trivially_copyable<InElementType>::value, void*>::type Dst, void* Src, SizeType Count)
	//{
	//	FMemory::Memmove(Dst, Src, Count * sizeof(InElementType));
	//}



	//void CopyOrMoveOrRector(InElementType* Dst, InElementType* Src)
	//{
	//	if constexpr (std::is_nothrow_move_assignable<InElementType>::value)
	//	{
	//		*Dst = std::move(*Src);
	//	}
	//	else if constexpr (std::is_copy_assignable<InElementType>::value)
	//	{
	//		*Dst = *Src;
	//	}
	//	else
	//	{
	//		DestroyElement(Dst);
	//		CopyOrMove(Dst, Src);
	//	}
	//}

	//void CopyOrMoveToNewPosition(typename std::enable_if<!std::is_trivially_copyable<InElementType>::value, void*>::type Dst, void* Src, SizeType Count)
	//{
	//	if(Dst == Src)
	//	{
	//		return;
	//	}
	//	InElementType* TypeDst = static_cast<InElementType*>(Dst);
	//	InElementType* TypeSrc = static_cast<InElementType*>(Src);

	//	InElementType* First = TypeSrc;
	//	InElementType* Last = TypeSrc + Count;
	//	InElementType* Dest = TypeDst;

	//	if(TypeDst - TypeSrc > 0)
	//	{
	//		while (First != Last) {

	//			CopyOrMoveOrRector(--Dest, --Last);
	//		}
	//	}
	//	else
	//	{
	//		/*std::vector<int> a;
	//		a.erase();*/
	//		for (; First != Last; ++Dest, (void) ++First) {
	//			CopyOrMoveOrRector(Dest, Last);
	//		}
	//	}

	//}
public:
	TTurinmaCircularQueue() :  Head(0), Tail(0), ArrayMax(DEFAULT_CAPACITY), InnerBuffer(static_cast<InElementType*>(FMemory::Malloc(ArrayMax * sizeof(InElementType))))
	{
		
	}

	~TTurinmaCircularQueue()
	{
		if (Tail >= Head)
		{
			DestroyCapacity(InnerBuffer + Head, GetCount());
		}
		else
		{
			DestroyCapacity(InnerBuffer + Head, ArrayMax - Head);
			DestroyCapacity(InnerBuffer, Tail);
		}
		FMemory::Free(InnerBuffer);
		
	}
	TTurinmaCircularQueue(const TTurinmaCircularQueue& InOther) : Head(0), Tail(0), ArrayMax(InOther.ArrayMax), InnerBuffer(static_cast<InElementType*>(FMemory::Malloc(ArrayMax * sizeof(InElementType))))
	{
		if (InOther.Tail > InOther.Head)
		{
			CopyToNewCapacity(InnerBuffer, InOther.InnerBuffer + InOther.Head, InOther.GetCount());
		}
		else
		{
			CopyToNewCapacity(InnerBuffer, InOther.InnerBuffer + InOther.Head, InOther.ArrayMax - InOther.Head);
			CopyToNewCapacity(InnerBuffer + InOther.ArrayMax - InOther.Head, InOther.InnerBuffer, InOther.Tail);
		
		}
		Head = 0;
		Tail = InOther.GetCount();
	}
	
	TTurinmaCircularQueue(TTurinmaCircularQueue&& Other) noexcept
		: Head(Other.Head), Tail(Other.Tail), ArrayMax(Other.ArrayMax), InnerBuffer(Other.InnerBuffer)
	{
		Other.Head = 0;
		Other.Tail = 0;
		Other.ArrayMax = DEFAULT_CAPACITY;
		Other.InnerBuffer = static_cast<InElementType*>(FMemory::Malloc(Other.ArrayMax * sizeof(InElementType)));
	}

	TTurinmaCircularQueue& operator=(const TTurinmaCircularQueue& InOther)
	{
		if(Tail >= Head)
		{
			return *this;
		}

		if (Tail >= Head)
		{
			DestroyCapacity(InnerBuffer + Head, GetCount());
		}
		else
		{
			DestroyCapacity(InnerBuffer + Head, ArrayMax - Head);
			DestroyCapacity(InnerBuffer, Tail);
		}
		FMemory::Free(InnerBuffer);

		InnerBuffer = static_cast<InElementType*>(FMemory::Malloc(InOther.ArrayMax * sizeof(InElementType)));

		if (InOther.Tail > InOther.Head)
		{
			CopyToNewCapacity(InnerBuffer, InOther.InnerBuffer + InOther.Head, InOther.GetCount());
		}
		else
		{
			CopyToNewCapacity(InnerBuffer, InOther.InnerBuffer + InOther.Head, InOther.ArrayMax - InOther.Head);
			CopyToNewCapacity(InnerBuffer + InOther.ArrayMax - InOther.Head, InOther.InnerBuffer, InOther.Tail);

		}
		Head = 0;
		Tail = InOther.GetCount();

		return *this;
	}

	TTurinmaCircularQueue& operator=(TTurinmaCircularQueue&& Other) noexcept
	{
		if(this == &Other)
		{
			return *this;
		}

		SizeType TempHead = Head;
		SizeType TempTail = Tail;
		SizeType TempArrayMax = ArrayMax;
		InElementType* TempInnerBuffer = InnerBuffer;

		Head = Other.Head;
		Tail = Other.Tail;
		ArrayMax = Other.ArrayMax;
		InnerBuffer = Other.InnerBuffer;

		Other.Head = TempHead;
		Other.Tail = TempTail;
		Other.ArrayMax = TempArrayMax;
		Other.InnerBuffer = TempInnerBuffer;

		return *this;
	}

	template<typename... CtorElem>
	InElementType* Enqueue(CtorElem&&...Params)
	{
		if(GetCount() == GetCapacity())
		{
			SizeType OldArrayMax = ArrayMax;
			SizeType NewArrayMax = OldArrayMax * RESIZE_FACTOR;
			SizeType TempCount = GetCount();
			InElementType* NewStorage = static_cast<InElementType*>(FMemory::Malloc(NewArrayMax * sizeof(InElementType)));
			if (Tail > Head)
			{
				CopyOrMoveToNewCapacity(NewStorage, InnerBuffer + Head, TempCount);
				DestroyCapacity(InnerBuffer + Head, TempCount);
			}
			else
			{
				CopyOrMoveToNewCapacity(NewStorage, InnerBuffer + Head, OldArrayMax - Head);
				CopyOrMoveToNewCapacity(NewStorage + OldArrayMax - Head, InnerBuffer, Tail);
				DestroyCapacity(InnerBuffer + Head, OldArrayMax - Head);
				DestroyCapacity(InnerBuffer, Tail);
			}
			Head = 0;
			Tail = TempCount;
			
			FMemory::Free(InnerBuffer);
			InnerBuffer = NewStorage;
			ArrayMax = NewArrayMax;
		}

		InElementType* Ret = new (InnerBuffer + Tail) InElementType(std::forward<CtorElem>(Params)...);
		Tail = (Tail + 1) % ArrayMax;

		return Ret;
	}

	InElementType* PeekFirst() const
	{
		if(GetCount())
		{
			return InnerBuffer + Head;
		}
		return nullptr;
	}

	InElementType* PeekLast() const
	{
		if (GetCount())
		{
			return InnerBuffer + (Tail + ArrayMax - 1) % ArrayMax;
		}
		return nullptr;
	}

	bool PopQueueNoRet()
	{
		if(GetCount())
		{
			DestroyCapacity(InnerBuffer + Head, 1);
			Head = (Head + 1) % ArrayMax;
			return true;
		}
		return false;
	}

	bool PopStackNoRet()
	{
		if(GetCount())
		{
			DestroyCapacity(InnerBuffer + (Tail + ArrayMax - 1) % ArrayMax, 1);
			Tail = (Tail + ArrayMax - 1) % ArrayMax;
			return true;
		}
		return false;
	}
	template<typename U = InElementType>
	typename std::enable_if<std::is_move_constructible<U>::value,InElementType>::type PopQueue()
	{
		InElementType Ret(std::move(*PeekFirst()));
		PopQueueNoRet();
		return Ret;
	}
	template<typename U = InElementType>
	typename std::enable_if<!std::is_move_constructible<U>::value, InElementType>::type PopQueue()
	{
		InElementType Ret(*PeekFirst());
		PopQueueNoRet();
		return Ret;
	}

	template<typename U = InElementType>
	typename std::enable_if<std::is_move_constructible<U>::value, InElementType>::type PopStack()
	{
		InElementType Ret(std::move(*PeekLast()));
		PopStackNoRet();
		return Ret;
	}
	template<typename U = InElementType>
	typename std::enable_if<!std::is_move_constructible<U>::value, InElementType>::type PopStack()
	{
		InElementType Ret(*PeekLast());
		PopStackNoRet();
		return Ret;
	}


	SizeType GetCount() const { return Tail - Head >= 0 ? Tail - Head : Tail - Head + ArrayMax; }
	SizeType GetCapacity() const { return ArrayMax - 1; }


};

class TURINMALUA_API FTestCicular
{
public:
	static FTestCicular LL;

public:
	TTurinmaCircularQueue<FString> S;
	UE_DISABLE_OPTIMIZATION
	FTestCicular()
	{
		
		S.Enqueue(TEXT("Test1"));
		UE_LOG(LogTemp, Log, TEXT("%s"), **S.PeekFirst());
		S.Enqueue(TEXT("Test2"));
		UE_LOG(LogTemp, Log, TEXT("%s"), **S.PeekFirst());
		S.Enqueue(TEXT("Test3"));
		UE_LOG(LogTemp, Log, TEXT("%s"), **S.PeekFirst());
		S.PopQueueNoRet();
		UE_LOG(LogTemp, Log, TEXT("%s"), **S.PeekFirst());
		S.Enqueue(TEXT("Test4"));
		UE_LOG(LogTemp, Log, TEXT("%s"), **S.PeekFirst());
		S.Enqueue(TEXT("Test5"));
		UE_LOG(LogTemp, Log, TEXT("%s"), **S.PeekFirst());
		S.Enqueue(TEXT("Test6"));
		UE_LOG(LogTemp, Log, TEXT("%s"), **S.PeekFirst());
		S.Enqueue(TEXT("Test7"));
		UE_LOG(LogTemp, Log, TEXT("%s"), **S.PeekFirst());

	}
	UE_ENABLE_OPTIMIZATION
};