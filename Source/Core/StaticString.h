#pragma once
#include <type_traits>

// TODO: add static-runtime asserts for safety
// TODO: add raw c-string initializer
// A String with on-stack allocated buffer. Constexpr context usable.
template<int N>
class StaticString
{
	// utility constructors to enable efficient concatenation
	template<int M, int K>
	constexpr StaticString(const char(&aBuffer)[M], const StaticString<K>& aString)
		: myBuffer{0}
		, myLength(M+K-1)
	{
		for (int i = 0; i < M - 1; i++) // skip \0
		{
			myBuffer[i] = aBuffer[i];
		}

		for (int i = 0; i < K; i++)
		{
			myBuffer[M - 1 + i] = aString[i];
		}
	}

	template<int M, int K>
	constexpr StaticString(const StaticString<M>& aString, const char(&aBuffer)[K])
		: myBuffer{ 0 }
		, myLength(M + K - 1)
	{
		for (int i = 0; i < M - 1; i++) // skip \0
		{
			myBuffer[i] = aString[i];
		}

		for (int i = 0; i < K; i++) 
		{
			myBuffer[M - 1 + i] = aBuffer[i];
		}
	}

	template<int M, int K>
	constexpr StaticString(const StaticString<M>& aString1, const StaticString<K>& aString2)
		: myBuffer{ 0 }
		, myLength(M + K - 1)
	{
		for (int i = 0; i < M - 1; i++) // skip \0
		{
			myBuffer[i] = aString1[i];
		}

		for (int i = 0; i < K; i++)
		{
			myBuffer[M - 1 + i] = aString2[i];
		}
	}

	// friend declarations
	template<int N, int M>
	friend constexpr StaticString<N + M - 1> operator+(const char(&aBuffer)[N],
		const StaticString<M>& aString);

	template<int N, int M>
	friend constexpr StaticString<N + M - 1> operator+(const StaticString<N>& aString,
		const char(&aBuffer)[M]);

	template<int N, int M>
	friend constexpr StaticString<N + M - 1> operator+(const StaticString<N>& aString1,
		const StaticString<M>& aString2);

public:
	constexpr StaticString(const char(&aBuffer)[N])
		: myBuffer{0} // constexpr requires members to be initialized
		, myLength(N)
	{
		for (int i = 0; i < N; i++)
		{
			myBuffer[i] = aBuffer[i];
		}
	}

	template<int M, class = std::enable_if_t<M <= N>>
	constexpr StaticString(const char(&aBuffer)[M])
		: myBuffer{0} // constexpr requires members to be initialized
		, myLength(M)
	{
		for (int i = 0; i < M; i++)
		{
			myBuffer[i] = aBuffer[i];
		}
	}

	template<int M, class = std::enable_if_t<M <= N>>
	constexpr StaticString(const StaticString<M>& aOther)
		: myBuffer{0}
		, myLength(M)
	{
		for (int i = 0; i < M; i++)
		{
			myBuffer[i] = aOther[i];
		}
	}

	constexpr char& operator[](int index)
	{
		// TODO: add check to enforce last element to be \0
		return myBuffer[index];
	}

	constexpr const char& operator[](int index) const
	{
		return myBuffer[index];
	}

	constexpr int GetLength() const
	{
		return myLength;
	}

	constexpr int GetMaxLength() const
	{
		return N;
	}

	template<int M>
	constexpr bool operator==(const StaticString<M>& aOther)
	{
		if (myLength != aOther.GetLength())
		{
			return false;
		}

		for (int i = 0; i < myLength; i++)
		{
			if (myBuffer[i] != aOther[i])
			{
				return false;
			}
		}
		return true;
	}

	constexpr const char* CStr() const
	{
		return myBuffer;
	}

private:
	int myLength;
	char myBuffer[N];
};

template<int N, int M>
constexpr StaticString<N + M - 1> operator+(const char(&aBuffer)[N], 
											const StaticString<M>& aString)
{
	return StaticString<N + M - 1>(aBuffer, aString);
}

template<int N, int M>
constexpr StaticString<N + M - 1> operator+(const StaticString<N>& aString, 
											const char(&aBuffer)[M])
{
	return StaticString<N + M - 1>(aString, aBuffer);
}

template<int N, int M>
constexpr StaticString<N + M - 1> operator+(const StaticString<N>& aString1, 
											const StaticString<M>& aString2)
{
	return StaticString<N + M - 1>(aString1, aString2);
}