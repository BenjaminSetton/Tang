#ifndef ASSET_TYPES_H
#define ASSET_TYPES_H

#include <glm.hpp>

#include "utils/logger.h"
#include "utils/uuid.h"
#include "vertex_type.h"

#include "data_buffer/vertex_buffer.h"
#include "data_buffer/index_buffer.h"

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

	// The asset pipeline can be represented as follows: 
	// 
	//		Disc       ->       Core       ->       Renderer Resources
	//            [AssetCore]        [AssetResources]
	//
	// [AssetCore] is the representation of the asset after it's been loaded from disc. The layout is the same
	// regardless of whether we're importing the asset (loading from pre-defined format such as .OBJ or .FBX) or
	// reading the binary directly from our own file format (.TASSET).
	// [AssetResources] is the representation of the asset that the renderer can use. The resources are created directly 
	// from an AssetCore instance, at which point we may (TODO) unload the AssetCore instance.
	//
	// Note that both AssetCore and AssetResource instances share a UUID. This is used so that we know where the AssetResources
	// came from, since the UUID from an AssetResource instance is guaranteed to be equivalent to the UUID from the AssetCore instance
	// it was created from.
	struct AssetCore
	{
		std::string name;
		std::vector<Mesh> meshes;
		std::vector<Texture> textures;
		UUID uuid;
	};

	struct AssetResources
	{
		std::vector<VertexBuffer> vertexBuffers;
		std::vector<uint32_t> offsets;				// Describes the offsets into a single combined buffer of vertex buffers, and the length of the offsets vector must match that of the vertex buffer vector!
		IndexBuffer indexBuffer;
		uint64_t indexCount = 0;					// Used when calling vkCmdDrawIndexed
		UUID uuid;
	};
}

#endif