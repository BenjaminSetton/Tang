#ifndef VERTEX_TYPE_H
#define VERTEX_TYPE_H

#include <vector>

#include "sanity_check.h"
#include "vec_types.h"

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
	// Only lists the supported vertex formats
	enum class VertexFormat
	{
		RGB32_SFLOAT, // 32-bit 3D vector, signed float (24 bytes total)
		RG32_SFLOAT,  // 32-bit 2D vector, signed float (16 bytes total)
	};

	struct VertexInputBindingDescription
	{
		uint32_t binding;
		uint32_t stride;
	};

	struct VertexInputAttributeDescription
	{
		uint32_t     location;
		uint32_t     binding;
		VertexFormat format;
		uint32_t     offset;
	};

	// Parent vertex type.
	// Derived vertex types MUST implement these three functions
	template<typename T>
	struct VertexType
	{
		VertexInputBindingDescription GetBindingDescription()
		{
			constexpr bool isSameReturn = std::is_same_v<decltype(std::declval<T>().GetBindingDescription()), VertexInputBindingDescription>;
			TNG_STATIC_ASSERT_MSG(isSameReturn, "Derived vertex type must implement GetBindingDescription()!");
			return static_cast<const T*>(this)->GetBindingDescription();
		}

		constexpr uint32_t GetAttributeDescriptionCount()
		{
			constexpr bool isSameReturn = std::is_same_v<decltype(std::declval<T>().GetAttributeDescriptionCount()), uint32_t>;
			TNG_STATIC_ASSERT_MSG(isSameReturn, "Derived vertex type must implement GetAttributeDescriptionCount()!");
			return static_cast<const T*>(this)->GetAttributeDescriptionCount();
		}

		const VertexInputAttributeDescription* GetAttributeDescriptions()
		{
			constexpr bool isSameReturn = std::is_same_v<decltype(std::declval<T>().GetAttributeDescriptions()), VertexInputAttributeDescription*>;
			TNG_STATIC_ASSERT_MSG(isSameReturn, "Derived vertex type must implement GetAttributeDescriptions()!");
			return static_cast<const T*>(this)->GetAttributeDescriptions();
		}
	};

	// A special vertex type used when pre-processing the skybox from an equirectangular 2D texture to a cubemap
	struct CubemapVertex : public VertexType<CubemapVertex>
	{
		static VertexInputBindingDescription GetBindingDescription()
		{
			static VertexInputBindingDescription bindingDesc
			{
				0,						// binding
				sizeof(CubemapVertex)	// stride
			};
			return bindingDesc;
		}

		static constexpr uint32_t GetAttributeDescriptionCount()
		{
			return 1;
		}

		static const VertexInputAttributeDescription* GetAttributeDescriptions()
		{
			TNG_STATIC_ASSERT(sizeof(CubemapVertex) == 12);

			static VertexInputAttributeDescription attributeDescriptions[GetAttributeDescriptionCount()] =
			{
				// POSITION
				{
					0,								// location
					0,								// binding
					VertexFormat::RGB32_SFLOAT,		// format: vec3 (12 bytes)
					offsetof(CubemapVertex, pos)	// offset
				}
			};

			return attributeDescriptions;
		}

		CubemapVertex() : pos(0.0f)
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

		Vec3 pos;
	};

	struct UVVertex : public VertexType<UVVertex>
	{
		static VertexInputBindingDescription GetBindingDescription()
		{
			static VertexInputBindingDescription bindingDesc
			{
				0,					// binding
				sizeof(UVVertex)	// stride
			};
			return bindingDesc;
		}

		static constexpr uint32_t GetAttributeDescriptionCount()
		{
			return 2;
		}

		static const VertexInputAttributeDescription* GetAttributeDescriptions()
		{
			TNG_STATIC_ASSERT(sizeof(UVVertex) == 20);

			static VertexInputAttributeDescription attributeDescriptions[GetAttributeDescriptionCount()] =
			{
				// POSITION
				{
					0,								// location
					0,								// binding
					VertexFormat::RGB32_SFLOAT,		// format
					offsetof(UVVertex, pos)			// offset
				},
				// UV
				{
					1,								// location
					0,								// binding
					VertexFormat::RG32_SFLOAT,		// format
					offsetof(UVVertex, uv)			// offset
				}
			};

			return attributeDescriptions;
		}

		UVVertex() : pos(0.0f), uv(0.0f)
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

		Vec3 pos;
		Vec2 uv;
	};

	struct PBRVertex : public VertexType<PBRVertex>
	{
		static VertexInputBindingDescription GetBindingDescription()
		{
			static VertexInputBindingDescription bindingDesc
			{
				0,							// binding
				sizeof(TANG::PBRVertex)		// stride
			};

			return bindingDesc;
		}

		static constexpr uint32_t GetAttributeDescriptionCount()
		{
			return 5;
		}

		static const VertexInputAttributeDescription* GetAttributeDescriptions()
		{
			TNG_STATIC_ASSERT(sizeof(PBRVertex) == 56);

			static VertexInputAttributeDescription attributeDescriptions[GetAttributeDescriptionCount()] =
			{
				// POSITION
				{
					0,								// location
					0,								// binding
					VertexFormat::RGB32_SFLOAT,		// format
					offsetof(PBRVertex, pos)		// offset
				},
				// NORMAL
				{
					1,								// location
					0,								// binding
					VertexFormat::RGB32_SFLOAT,		// format
					offsetof(PBRVertex, normal)		// offset
				},
				// TANGENT
				{
					2,								// location
					0,								// binding
					VertexFormat::RGB32_SFLOAT,		// format
					offsetof(PBRVertex, tangent)	// offset
				},
				// BITANGENT
				{
					3,								// location
					0,								// binding
					VertexFormat::RGB32_SFLOAT,		// format
					offsetof(PBRVertex, bitangent)	// offset
				},
				// UV
				{
					4,								// location
					0,								// binding
					VertexFormat::RG32_SFLOAT,		// format
					offsetof(PBRVertex, uv)			// offset
				}
			};

			return attributeDescriptions;
		}

		PBRVertex() : pos(0.0f), normal(0.0f), tangent(0.0f), bitangent(0.0f), uv(0.0f)
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

		Vec3 pos;
		Vec3 normal;
		Vec3 tangent;
		Vec3 bitangent;
		Vec2 uv;
	};

}

#endif