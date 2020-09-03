#include "Precomp.h"
#include "RefCounted.h"

RefCounted::RefCounted()
	: myCounter(0)
{
}

void RefCounted::RemoveRef()
{
	if (myCounter.fetch_sub(1) == 1)
	{
		// we just got rid of the last reference, so time to self-destruct
		Cleanup();
	}
}