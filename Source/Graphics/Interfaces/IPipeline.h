#pragma once

#include <Core/DataEnum.h>

template<class T> class Handle;
class UniformAdapter;

class IPipeline
{
public:
	DATA_ENUM(Type, char,
		Graphics,
		Compute
	);

	virtual size_t GetAdapterCount() const = 0;
	virtual const UniformAdapter& GetAdapter(size_t) const = 0;
};