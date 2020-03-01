#pragma once

class Descriptor;

class IPipeline
{
public:
	enum class Type
	{
		Graphics,
		Compute
	};

	virtual size_t GetDescriptorCount() const = 0;
	virtual const Descriptor& GetDescriptor(size_t) const = 0;
};