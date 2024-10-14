#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

#include <vector>

#include "TANG/utils/sanity_check.h"
#include "TANG/utils/vec_types.h"
#include "TANG/utils/vertex_type.h"

// A special vertex type used when pre-processing the skybox from an equirectangular 2D texture to a cubemap
struct CubemapVertex : public TANG::VertexType<CubemapVertex>
{
	static TANG::VertexInputBindingDescription GetBindingDescription()
	{
		static TANG::VertexInputBindingDescription bindingDesc
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

	static const TANG::VertexInputAttributeDescription* GetAttributeDescriptions()
	{
		TNG_STATIC_ASSERT(sizeof(CubemapVertex) == 12);

		static TANG::VertexInputAttributeDescription attributeDescriptions[GetAttributeDescriptionCount()] =
		{
			// POSITION
			{
				0,									// location
				0,									// binding
				TANG::VertexFormat::RGB32_SFLOAT,	// format: vec3 (12 bytes)
				offsetof(CubemapVertex, pos)		// offset
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

	TANG::Vec3 pos;
};

struct UVVertex : public TANG::VertexType<UVVertex>
{
	static TANG::VertexInputBindingDescription GetBindingDescription()
	{
		static TANG::VertexInputBindingDescription bindingDesc
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

	static const TANG::VertexInputAttributeDescription* GetAttributeDescriptions()
	{
		TNG_STATIC_ASSERT(sizeof(UVVertex) == 20);

		static TANG::VertexInputAttributeDescription attributeDescriptions[GetAttributeDescriptionCount()] =
		{
			// POSITION
			{
				0,									// location
				0,									// binding
				TANG::VertexFormat::RGB32_SFLOAT,	// format
				offsetof(UVVertex, pos)				// offset
			},
			// UV
			{
				1,									// location
				0,									// binding
				TANG::VertexFormat::RG32_SFLOAT,	// format
				offsetof(UVVertex, uv)				// offset
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

	TANG::Vec3 pos;
	TANG::Vec2 uv;
};

struct PBRVertex : public TANG::VertexType<PBRVertex>
{
	static TANG::VertexInputBindingDescription GetBindingDescription()
	{
		static TANG::VertexInputBindingDescription bindingDesc
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

	static const TANG::VertexInputAttributeDescription* GetAttributeDescriptions()
	{
		TNG_STATIC_ASSERT(sizeof(PBRVertex) == 56);

		static TANG::VertexInputAttributeDescription attributeDescriptions[GetAttributeDescriptionCount()] =
		{
			// POSITION
			{
				0,									// location
				0,									// binding
				TANG::VertexFormat::RGB32_SFLOAT,	// format
				offsetof(PBRVertex, pos)			// offset
			},
			// NORMAL
			{
				1,									// location
				0,									// binding
				TANG::VertexFormat::RGB32_SFLOAT,	// format
				offsetof(PBRVertex, normal)			// offset
			},
			// TANGENT
			{
				2,									// location
				0,									// binding
				TANG::VertexFormat::RGB32_SFLOAT,	// format
				offsetof(PBRVertex, tangent)		// offset
			},
			// BITANGENT
			{
				3,									// location
				0,									// binding
				TANG::VertexFormat::RGB32_SFLOAT,	// format
				offsetof(PBRVertex, bitangent)		// offset
			},
			// UV
			{
				4,									// location
				0,									// binding
				TANG::VertexFormat::RG32_SFLOAT,	// format
				offsetof(PBRVertex, uv)				// offset
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

	TANG::Vec3 pos;
	TANG::Vec3 normal;
	TANG::Vec3 tangent;
	TANG::Vec3 bitangent;
	TANG::Vec2 uv;
};

#endif