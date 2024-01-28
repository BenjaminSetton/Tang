#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

#include <vector>

// DISABLE WARNINGS FROM GLM
#pragma warning(push)
#pragma warning(disable : 4201 4244)

#include <glm/glm.hpp>

#pragma warning(pop)

#include "utils/sanity_check.h"
#include "vulkan/vulkan.h"

// Stores a collection of vertex types used in different rendering pipelines.
// Every single vertex type must implement the two following STATIC methods:
// 
//		static const VkVertexInputBindingDescription& GetBindingDescription()
//		static const std::vector<VkVertexInputAttributeDescription>& GetAttributeDescriptions()
//
// The return type is a const ref because we interpret those as arrays and send the address of
// these structs to Vulkan to create a pipeline. This is also why internally we create a static variable
// and return it. It makes sense to make it const since it's never going to change at runtime

namespace TANG
{
	// Parent vertex type.
	// NOTE - Derived vertex types must inherit from this class just so we can tell that the derived type
	//        IS a VertexType. This is important because a templated utility method in BasePipeline depends
	//        on this fact to only allow template instantiations of derived vertex types
	class VertexType
	{ };

	// A special vertex type used when pre-processing the skybox from an equirectangular 2D texture to a cubemap
	struct CubemapVertex : public VertexType
	{
		static const VkVertexInputBindingDescription& GetBindingDescription()
		{
			static VkVertexInputBindingDescription bindingDesc{};
			bindingDesc.binding = 0;
			bindingDesc.stride = sizeof(CubemapVertex);
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDesc;
		}

		static const std::vector<VkVertexInputAttributeDescription>& GetAttributeDescriptions()
		{
			TNG_ASSERT_COMPILE(sizeof(CubemapVertex) == 12);

			static std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			if (attributeDescriptions.size() != 0)
			{
				return attributeDescriptions;
			}

			attributeDescriptions.reserve(1);

			// POSITION
			VkVertexInputAttributeDescription position;
			position.binding = 0;
			position.location = 0;
			position.format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			position.offset = offsetof(CubemapVertex, pos);
			attributeDescriptions.push_back(position);

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

	struct UVVertex : public VertexType
	{
		static const VkVertexInputBindingDescription& GetBindingDescription()
		{
			static VkVertexInputBindingDescription bindingDesc{};
			bindingDesc.binding = 0;
			bindingDesc.stride = sizeof(UVVertex);
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDesc;
		}

		static const std::vector<VkVertexInputAttributeDescription>& GetAttributeDescriptions()
		{
			TNG_ASSERT_COMPILE(sizeof(UVVertex) == 20);

			static std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			if (attributeDescriptions.size() != 0)
			{
				return attributeDescriptions;
			}

			attributeDescriptions.reserve(2);

			// POSITION
			VkVertexInputAttributeDescription position;
			position.binding = 0;
			position.location = 0;
			position.format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			position.offset = offsetof(UVVertex, pos);
			attributeDescriptions.push_back(position);

			// UV
			VkVertexInputAttributeDescription uv;
			uv.binding = 0;
			uv.location = 1;
			uv.format = VK_FORMAT_R32G32_SFLOAT; // vec2 (8 bytes)
			uv.offset = offsetof(UVVertex, uv);
			attributeDescriptions.push_back(uv);

			return attributeDescriptions;
		}

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

	struct PBRVertex : public VertexType
	{
		static const VkVertexInputBindingDescription& GetBindingDescription()
		{
			static VkVertexInputBindingDescription bindingDesc{};
			bindingDesc.binding = 0;
			bindingDesc.stride = sizeof(TANG::PBRVertex);
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDesc;
		}

		static const std::vector<VkVertexInputAttributeDescription>& GetAttributeDescriptions()
		{
			TNG_ASSERT_COMPILE(sizeof(PBRVertex) == 56);

			static std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			if (attributeDescriptions.size() != 0)
			{
				return attributeDescriptions;
			}

			attributeDescriptions.reserve(5);

			// POSITION
			VkVertexInputAttributeDescription position;
			position.binding = 0;
			position.location = 0;
			position.format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			position.offset = offsetof(PBRVertex, pos);
			attributeDescriptions.push_back(position);

			// NORMAL
			VkVertexInputAttributeDescription normal;
			normal.binding = 0;
			normal.location = 1;
			normal.format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			normal.offset = offsetof(PBRVertex, normal);
			attributeDescriptions.push_back(normal);

			// TANGENT
			VkVertexInputAttributeDescription tangent;
			tangent.binding = 0;
			tangent.location = 2;
			tangent.format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			tangent.offset = offsetof(PBRVertex, tangent);
			attributeDescriptions.push_back(tangent);

			// BITANGENT
			VkVertexInputAttributeDescription bitangent;
			bitangent.binding = 0;
			bitangent.location = 3;
			bitangent.format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (12 bytes)
			bitangent.offset = offsetof(PBRVertex, bitangent);
			attributeDescriptions.push_back(bitangent);

			// UV
			VkVertexInputAttributeDescription uv;
			uv.binding = 0;
			uv.location = 4;
			uv.format = VK_FORMAT_R32G32_SFLOAT; // vec2 (8 bytes)
			uv.offset = offsetof(PBRVertex, uv);
			attributeDescriptions.push_back(uv);

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