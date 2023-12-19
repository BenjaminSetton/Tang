
// DISABLE WARNINGS FROM GLM
#pragma warning(push)
#pragma warning(disable : 4201 4244)

#include <glm/glm.hpp>

#pragma warning(pop)

namespace TANG
{
	struct VertexType
	{
		VertexType() : pos(0.0f, 0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f), tangent(0.0f, 0.0f, 0.0f), bitangent(0.0f, 0.0f, 0.0f), uv(0.0f, 0.0f)
		{
		}

		~VertexType()
		{
		}

		VertexType(const VertexType& other) : pos(other.pos), normal(other.normal), 
			tangent(other.tangent), bitangent(other.bitangent), uv(other.uv)
		{
		}

		VertexType(VertexType&& other) noexcept : pos(std::move(other.pos)), normal(std::move(other.normal)), 
			tangent(std::move(other.tangent)), bitangent(std::move(other.bitangent)), uv(std::move(other.uv))
		{
		}

		VertexType& operator=(const VertexType& other)
		{
			if (this == &other)
			{
				return *this;
			}

			pos = other.pos;
			normal = other.normal;
			tangent = other.tangent;
			bitangent = other.bitangent;
			uv = other.uv;

			return *this;
		}

		// Utility functions / operator overloads
		bool operator==(const VertexType& other) const
		{
			return pos == other.pos && 
				normal == other.normal && 
				tangent == other.tangent &&
				bitangent == other.bitangent &&
				uv == other.uv;
		}

		// Member variables
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
		glm::vec2 uv;
	};

}