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
	struct Transform
	{
		Transform() : position(0), rotation(0), scale(1)
		{
		}

		Transform(glm::vec3 _position, glm::vec3 _rotation, glm::vec3 _scale) :
			position(_position), rotation(_rotation), scale(_scale)
		{
		}

		Transform(const Transform& other) : 
			position(other.position), rotation(other.rotation), scale(other.scale)
		{
		}

		Transform(Transform&& other) :
			position(other.position), rotation(other.rotation), scale(other.scale)
		{
			other.position = glm::vec3(0);
			other.rotation = glm::vec3(0);
			other.scale = glm::vec3(1);
		}

		Transform& operator=(const Transform& other)
		{
			if (this == &other)
			{
				return *this;
			}

			position = other.position;
			rotation = other.rotation;
			scale = other.scale;

			return *this;
		}

		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
	};

	// Maybe this has to be moved somewhere else, specifically more texture-related?
	struct Texture
	{
		Texture() : data(nullptr), size(0, 0), bytesPerPixel(0)
		{
		}

		~Texture() 
		{
			delete[] data;
			size = { 0, 0 };
			bytesPerPixel = 0;
		}

		Texture(const Texture& other)
		{
			// Temporary debug :)
			LogWarning("Deep-copying texture!");

			data = new char[static_cast<uint32_t>(other.size.x) * static_cast<uint32_t>(other.size.y) * bytesPerPixel];
			size = other.size;
			bytesPerPixel = other.bytesPerPixel;
		}

		Texture(Texture&& other)
		{
			// Transfer ownership of the texture data
			data = other.data;
			size = other.size;
			bytesPerPixel = other.bytesPerPixel;

			other.data = nullptr;
			other.size = { 0, 0 };
			other.bytesPerPixel = 0;
		}

		Texture& operator=(const Texture& other)
		{
			// Protect against self-assignment
			if (this == &other)
			{
				return *this;
			}

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
	//		Disk       ->       Core       ->       Renderer Resources
	//            [AssetDisk]        [AssetResources]
	//
	// [AssetDisk] is the representation of the asset after it's been loaded from disc. The layout is the same
	// regardless of whether we're importing the asset (loading from pre-defined format such as .OBJ or .FBX) or
	// reading the binary directly from our own file format (.TASSET).
	// [AssetResources] is the representation of the asset that the renderer can use. The resources are created directly 
	// from an AssetDisk instance, at which point we may (TODO) unload the AssetDisk instance.
	//
	// Note that both AssetDisk and AssetResource instances share a UUID. This is used so that we know where the AssetResources
	// came from, since the UUID from an AssetResource instance is guaranteed to be equivalent to the UUID from the AssetDisk instance
	// it was created from.
	struct AssetDisk
	{
		UUID uuid;
		std::string name;
		std::vector<Mesh> meshes;
		std::vector<Texture> textures;
	};

	// TODO - Convert AssetResources into a structure of arrays, rather than an array of structs.
	//        The two members below are unordered_maps, accessed by the Asset's UUID.
	struct AssetResources
	{
		UUID uuid;
		std::vector<VertexBuffer> vertexBuffers;
		std::vector<uint32_t> offsets;				// Describes the offsets into a single combined buffer of vertex buffers, and the length of the offsets vector must match that of the vertex buffer vector!
		IndexBuffer indexBuffer;
		uint64_t indexCount = 0;					// Used when calling vkCmdDrawIndexed

		// NOTE - The API user must update and keep track of the transform data for the assets,
		//        and pass it to the renderer every frame for drawing. The design decision behind
		//        this is so we can own copy of the data, rather than holding a ton of pointers 
		//        to data somewhere else which will probably be very slow
		Transform transform;

		bool shouldDraw;							// Determines whether the asset should be drawn on the current frame. This value is reset every frame
	};
}

#endif