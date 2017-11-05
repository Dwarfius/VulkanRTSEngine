#pragma once

struct Vertex
{
	vec3 pos;
	vec2 uv;
	vec3 normal;

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && uv == other.uv && normal == other.normal;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}