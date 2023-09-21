#ifndef ASSET_TYPES_H
#define ASSET_TYPES_H

#include <glm.hpp>

#include "utils/logger.h"
#include "utils/uuid.h"
#include "vertex_type.h"

typedef uint32_t IndexType;

namespace TANG
{
	// Maybe this has to be moved somewhere else, specifically more texture-related?
	struct Texture
	{
		Texture() : data(nullptr), size(0.0f, 0.0f), bytesPerPixel(0)
		{
		}
		~Texture() { }
		Texture(const Texture& other)
		{
			// Temporary debug :)
			LogWarning("Deep-copying texture!");

			data = new char[static_cast<uint32_t>(other.size.x) * static_cast<uint32_t>(other.size.y) * bytesPerPixel];
			size = other.size;
			bytesPerPixel = other.bytesPerPixel;
		}

		void* data;
		glm::vec2 size;
		uint32_t bytesPerPixel; // Guaranteed to be 4 by assimp loader
	};

	struct Mesh
	{
		std::vector<VertexType> vertices;
		std::vector<IndexType> indices;
		//std::vector<Material> materials;
	};

	struct Asset
	{
		std::string name;
		std::vector<Mesh> meshes;
		std::vector<Texture> textures;
		UUID uuid;
	};
}

#endif