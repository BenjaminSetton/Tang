#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

#include <array>

// DISABLE WARNINGS FROM GLM
#pragma warning(push)
#pragma warning(disable : 4201 4244)

#include <glm/glm.hpp>

#pragma warning(pop)

#include "utils/sanity_check.h"
#include "vulkan/vulkan.h"

namespace TANG
{

	// A special vertex type used when pre-processing the skybox from an equirectangular 2D texture to a cubemap
	struct CubemapVertex
	{
		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDesc{};
			bindingDesc.binding = 0;
			bindingDesc.stride = sizeof(CubemapVertex);
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDesc;
		}

		static std::array<VkVertexInputAttributeDescription, 1> GetAttributeDescriptions()
		{
			TNG_ASSERT_COMPILE(sizeof(TANG::CubemapVertex) == 12);

			std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

			// POSITION
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			attributeDescriptions[0].offset = offsetof(CubemapVertex, pos);

			return attributeDescriptions;
		}

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
		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDesc{};
			bindingDesc.binding = 0;
			bindingDesc.stride = sizeof(TANG::PBRVertex);
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDesc;
		}


		static std::array<VkVertexInputAttributeDescription, 5> GetAttributeDescriptions()
		{
			TNG_ASSERT_COMPILE(sizeof(PBRVertex) == 56);

			std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

			// POSITION
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			attributeDescriptions[0].offset = offsetof(PBRVertex, pos);

			// NORMAL
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			attributeDescriptions[1].offset = offsetof(PBRVertex, normal);

			// TANGENT
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			attributeDescriptions[2].offset = offsetof(PBRVertex, tangent);

			// BITANGENT
			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			attributeDescriptions[3].offset = offsetof(PBRVertex, tangent);

			// UV
			attributeDescriptions[4].binding = 0;
			attributeDescriptions[4].location = 4;
			attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT; // vec2 (8 bytes)
			attributeDescriptions[4].offset = offsetof(PBRVertex, uv);

			return attributeDescriptions;
		}

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

#endif