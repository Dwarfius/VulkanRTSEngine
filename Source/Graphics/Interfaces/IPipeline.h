#pragma once

class IPipeline
{
public:
	enum class Type
	{
		Graphics,
		Compute
	};

	virtual size_t GetDescriptorCount() const = 0;
};