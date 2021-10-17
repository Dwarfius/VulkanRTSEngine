#pragma once

#ifdef ASSERT_MUTEX
#define HANDLE_HEAVY_DEBUG
#endif

#ifdef HANDLE_HEAVY_DEBUG
#include "Threading/AssertRWMutex.h"
#endif

// A base for objects that need to be managed via Handle.
// Using an intrusive implementation to allow to spawn a Handle from a
// pointer, allowing forced extension of life
// This gives more freedom down the line, and if it turns out as unnecessary
// we can always discard it
class RefCounted
{
public:
	RefCounted();
	virtual ~RefCounted() {}

private:
	// ==========================
	// Handle<T> support
	template<typename T>
	friend class Handle;
	void AddRef() {	myCounter++; }
	void RemoveRef();
	uint32_t GetRefCount() const { return myCounter; }
	// ==========================

private:
	virtual void Cleanup() { delete this; }
	std::atomic<uint32_t> myCounter;
};

// A special subclass of RefCounted that prevents deletion of
// the object, and instead delegates it to Destroy func
// User has to provide static void Destroy(T*)!
template<class T>
class RefCountedWithDestroy : public RefCounted
{
	void Cleanup() override final { T::Destroy(static_cast<T*>(this)); }
};

// =======================================================

// A smart-handle that shares ownership of RefCounted-ables.
// Not thread-safe. Will assert on contention.
template<class T>
class Handle
{
public:
	Handle()
		: myObject(nullptr)
	{
	}

	Handle(RefCounted* anObject)
		: myObject(anObject)
	{
		if (myObject)
		{
			myObject->AddRef();
		}
	}

	Handle(const Handle& aHandle)
	{
#ifdef HANDLE_HEAVY_DEBUG
		AssertReadLock theirLock(aHandle.myDebugMutex);
#endif
		myObject = aHandle.myObject;
		if (myObject)
		{
			myObject->AddRef();
		}
	}

	Handle(Handle&& aHandle) noexcept
	{
		myObject = aHandle.myObject;
		aHandle.myObject = nullptr;
	}

	~Handle()
	{
		if (myObject)
		{
			myObject->RemoveRef();
		}
	}

	Handle& operator=(const Handle& aHandle)
	{
#ifdef HANDLE_HEAVY_DEBUG
		AssertWriteLock ourLock(myDebugMutex);
		AssertReadLock theirLock(aHandle.myDebugMutex);
#endif
		if (myObject)
		{
			myObject->RemoveRef();
		}

		myObject = aHandle.myObject;
		if (myObject)
		{
			myObject->AddRef();
		}
		return *this;
	}

	// Accessor of internal object casted to type of handle
	T* Get() 
	{
#ifdef HANDLE_HEAVY_DEBUG
		AssertReadLock ourLock(myDebugMutex);
#endif
		return static_cast<T*>(myObject); 
	}

	// Const accessor of internal object casted to type of handle
	const T* Get() const 
	{ 
#ifdef HANDLE_HEAVY_DEBUG
		AssertReadLock ourLock(myDebugMutex);
#endif
		return static_cast<const T*>(myObject); 
	}

	// Accessor of internal object casted to other type, if possible
	template<typename TOther>
	TOther* Get() 
	{ 
#ifdef HANDLE_HEAVY_DEBUG
		AssertReadLock ourLock(myDebugMutex);
#endif
		return static_cast<TOther*>(myObject); 
	}

	// Const accessor of internal object casted to other type, if possible
	template<typename TOther>
	const TOther* Get() const 
	{ 
#ifdef HANDLE_HEAVY_DEBUG
		AssertReadLock ourLock(myDebugMutex);
#endif
		return static_cast<const TOther*>(myObject); 
	}

	bool IsValid() const 
	{ 
#ifdef HANDLE_HEAVY_DEBUG
		AssertReadLock ourLock(myDebugMutex);
#endif
		return myObject != nullptr; 
	}

	bool IsLastHandle() const
	{
#ifdef HANDLE_HEAVY_DEBUG
		AssertReadLock ourLock(myDebugMutex);
#endif
		return myObject->GetRefCount() == 1;
	}

	T* operator->()
	{
		return Get();
	}

	const T* operator->() const
	{
		return Get();
	}

	// Allow conversions to super-classes of T
	template<class TOther> requires std::is_base_of_v<TOther, T>
	operator Handle<TOther>()
	{
		return Get<TOther>();
	}

	template<class TOther> requires std::is_base_of_v<TOther, T>
	operator const Handle<TOther>() const
	{
#ifdef HANDLE_HEAVY_DEBUG
		AssertReadLock ourLock(myDebugMutex);
#endif
		// Usually casting away constness is UB,
		// but since we can only create these objects
		// with either nullptr or mutable RefCounteds
		// we can never cast away original constness
		RefCounted* refObj = const_cast<RefCounted*>(myObject);
		return Handle<TOther>(refObj);
	}

private:
	template<class T1, class T2>
	friend bool operator==(const Handle<T1>& aLeft, const Handle<T2>& aRight);

	template<class TOther>
	friend class Handle;

	RefCounted* myObject;
#ifdef HANDLE_HEAVY_DEBUG
	mutable AssertRWMutex myDebugMutex;
#endif
};

template<class T1, class T2>
bool operator==(const Handle<T1>& aLeft, const Handle<T2>& aRight)
{
	static_assert(std::is_base_of_v<T2, T1>, "Invalid comparison - unrelated types!");
	return aLeft.myObject == aRight.myObject;
}