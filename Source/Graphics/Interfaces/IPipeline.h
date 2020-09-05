#pragma once

class Descriptor;
template<class T> class Handle;
class UniformAdapter;

class IPipeline
{
public:
	enum class Type
	{
		Graphics,
		Compute
	};

	virtual size_t GetDescriptorCount() const = 0;
	virtual Handle<Descriptor> GetDescriptor(size_t) const = 0;
	virtual const UniformAdapter& GetAdapter(size_t) const = 0;
};