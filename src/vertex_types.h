
// DISABLE WARNINGS FROM GLM
#pragma warning(push)
#pragma warning(disable : 4201 4244)

#include <glm/glm.hpp>

#pragma warning(pop)

namespace TANG
{
	// A special vertex type used when pre-processing the skybox from an equirectangular 2D texture to a cubemap
	struct CubemapVertex
	{
		CubemapVertex() : pos(0.0f, 0.0f, 0.0f)
		{ }

		~CubemapVertex()
		{ }

		CubemapVertex(const CubemapVertex& other) : pos(other.pos)
		{ }

		CubemapVertex(CubemapVertex&& other) noexcept : pos(std::move(other.pos))
		{ }

		CubemapVertex& operator=(const CubemapVertex& other)
		{
			if (this == &other)
			{
				return *this;
			}

			pos = other.pos;

			return *this;
		}

		// Utility functions / operator overloads
		bool operator==(const CubemapVertex& other) const
		{
			return pos == other.pos;
		}

		glm::vec3 pos;
	};

	struct UVVertex
	{
		UVVertex() : pos(0.0f, 0.0f, 0.0f), uv(0.0f, 0.0f)
		{ }

		~UVVertex()
		{ }

		UVVertex(const UVVertex& other) : pos(other.pos), uv(other.uv)
		{ }

		UVVertex(UVVertex&& other) noexcept : pos(std::move(other.pos)), uv(std::move(other.uv))
		{ }

		UVVertex& operator=(const UVVertex& other)
		{
			if (this == &other)
			{
				return *this;
			}

			pos = other.pos;
			uv = other.uv;

			return *this;
		}

		// Utility functions / operator overloads
		bool operator==(const UVVertex& other) const
		{
			return pos == other.pos && uv == other.uv;
		}

		glm::vec3 pos;
		glm::vec2 uv;
	};

	struct PBRVertex
	{
		PBRVertex() : pos(0.0f, 0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f), tangent(0.0f, 0.0f, 0.0f), bitangent(0.0f, 0.0f, 0.0f), uv(0.0f, 0.0f)
		{ }

		~PBRVertex()
		{ }

		PBRVertex(const PBRVertex& other) : pos(other.pos), normal(other.normal), 
			tangent(other.tangent), bitangent(other.bitangent), uv(other.uv)
		{ }

		PBRVertex(PBRVertex&& other) noexcept : pos(std::move(other.pos)), normal(std::move(other.normal)), 
			tangent(std::move(other.tangent)), bitangent(std::move(other.bitangent)), uv(std::move(other.uv))
		{ }

		PBRVertex& operator=(const PBRVertex& other)
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
		bool operator==(const PBRVertex& other) const
		{
			return pos == other.pos && 
				normal == other.normal && 
				tangent == other.tangent &&
				bitangent == other.bitangent &&
				uv == other.uv;
		}


		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
		glm::vec2 uv;
	};

}