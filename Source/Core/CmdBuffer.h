#pragma once

template<class T>
concept CmdType = requires
{
	// requires static constexpr uint8_t kId
	requires std::same_as<decltype(T::kId), const uint8_t>; 
	requires std::is_trivially_copyable_v<T>;
	// we don't need to test for trivial ctor since I'm invoking it directly
	// and it would be a compile error
	requires std::is_trivially_destructible_v<T>;
	// Note: what I really want is an is_implicit_lifetime_v, since that's what the
	// CmdBuffer does - we don't "recreate" objects when we resize, so it's possible
	// that when reading, we will be reading just bytes, without a C++ object backing
	// it, thus invoking UB. Also, we never explicitly destroy objects (even if we call
	// Clear), which is why we test for trivial dtor
};

// Utility class to store heterogeneous commands. Commands must satisfy
// CmdType concept (trivial type + kId member), and CmdBuffer will be able to
// serialize them via Write. The only caveat is that parsing those out have 
// to be done by hand via GetBuffer. Also, if maximum performance is disabled,
// the branch of growing can be disabled by calling Write<T, false>
class CmdBuffer
{
public:
	CmdBuffer() = default;
	CmdBuffer(uint32_t anInitialSize)
	{
		Resize(anInitialSize);
	}

	template<CmdType T, bool CanGrow = true>
	T& Write()
	{
		if constexpr (CanGrow)
		{
			if (myIndex + 1 + sizeof(T) > myBuffer.size())
			{
				myBuffer.resize(myBuffer.size() * 2);
			}
		}

		myBuffer[myIndex++] = static_cast<std::byte>(T::kId);
		T* cmd = new(&myBuffer[myIndex]) T;
		myIndex += sizeof(T);
		return *cmd;
	}

	// Get a span of binary represenatation of commands, in form of
	// <Cmd Id, Cmd Bytes>, <Cmd Id, Cmd Bytes>
	// TODO: Not user friendly, since they have to know how to parse the span
	// But on the flipside, CmdBuffer can't know which commands exist, so
	// without complicating the impl quite a bit, it can't be done here as well
	std::span<const std::byte> GetBuffer() const { return std::span(myBuffer.begin(), myIndex); }

	void Clear()
	{
		myIndex = 0;
		// Potential UB note: this is only safe as long as we have trivial dtors!
	}

	void Resize(uint32_t aSize)
	{
		myBuffer.resize(aSize);
	}

	bool IsEmpty() const
	{
		return myIndex == 0;
	}

	void CopyFrom(const CmdBuffer& aOther)
	{
		myBuffer.insert(myBuffer.end(), aOther.myBuffer.begin(), aOther.myBuffer.end());
		myIndex += aOther.myIndex;
	}

private:
	std::vector<std::byte> myBuffer;
	uint32_t myIndex = 0;
};