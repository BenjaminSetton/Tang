#ifndef ASSET_TYPES_H
#define ASSET_TYPES_H

// SILENCE WARNINGS FROM GLM
// 4201: warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)

#include <glm/glm.hpp>

#pragma warning(pop)

#include "utils/logger.h"
#include "utils/uuid.h"
#include "vertex_types.h"

#include "data_buffer/vertex_buffer.h"
#include "data_buffer/index_buffer.h"
#include "texture_resource.h"

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

	// TODO - Create a texture registry which holds all the texture data. The interface will only expose the texture UUIDs
	//        that refer to the internal registry. This implies that anything outside the registry MUST NOT hold any type of
	//        pointer to a Texture instance, but will rather store a UUID. It is much safer to do it this way because we cannot
	//        have dangling pointers, but the drawback is that we must query the texture registry for the texture data potentially
	//        very often, so best-case scenario is that it has a look-up complexity closest to O(1)
	struct Texture
	{
		Texture() : data(nullptr), size(0, 0), bytesPerPixel(0), fileName("")
		{
		}

		~Texture() 
		{
			delete[] data;
			size = { 0, 0 };
			bytesPerPixel = 0;
			fileName = "";
		}

		Texture(const Texture& other) : 
			size(other.size), bytesPerPixel(other.bytesPerPixel), fileName(other.fileName)
		{
			// Temporary debug :)
			LogWarning("Deep-copying texture!");

			data = new char[static_cast<uint32_t>(other.size.x) * static_cast<uint32_t>(other.size.y) * bytesPerPixel];
		}

		Texture(Texture&& other) : 
			data(std::move(other.data)), size(std::move(other.size)), bytesPerPixel(std::move(other.bytesPerPixel)),
			fileName(std::move(other.fileName))
		{
			other.data = nullptr;
			other.size = { 0, 0 };
			other.bytesPerPixel = 0;
			other.fileName = "";
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
			fileName = other.fileName;
		}

		void* data;
		glm::vec2 size;
		uint32_t bytesPerPixel; // Guaranteed to be 4 by assimp loader
		std::string fileName;
	};

	class Material
	{
	public:

		// NOTE - We depend on the numbering of this enum being consecutive
		enum class TEXTURE_TYPE
		{
			DIFFUSE = 0,
			SPECULAR,
			NORMAL,
			AMBIENT_OCCLUSION,
			METALLIC,
			ROUGHNESS,
			LIGHTMAP,
			_COUNT      // DO NOT USE. THIS MUST COME LAST
		};


		Material() : name("None"), textureCount(0)
		{
			// Guarantee that the internal textures vector will have exactly _COUNT entries
			textures.resize(static_cast<size_t>(TEXTURE_TYPE::_COUNT));
			std::fill(textures.begin(), textures.end(), nullptr);
		}

		~Material()
		{
			textures.clear();
			name.clear();
			textureCount = 0;
		}

		Material(const Material& other) : name(other.name), textures(other.textures), textureCount(other.textureCount)
		{
			// Nothing to do here
		}

		Material(Material&& other) : name(std::move(other.name)), textures(std::move(other.textures)), textureCount(std::move(other.textureCount))
		{
			// Do I have to clear the other things?
		}

		Material& operator=(const Material& other)
		{
			// Protect against self-assignment
			if (this == &other)
			{
				return *this;
			}

			name = other.name;
			textures = other.textures;
			textureCount = other.textureCount;

			return *this;
		}

		void AddTextureOfType(const TEXTURE_TYPE type, Texture* texture)
		{
			uint32_t typeIndex = static_cast<uint32_t>(type);

			if (type == TEXTURE_TYPE::_COUNT)
			{
				LogWarning("Cannot add texture of type _COUNT!");
				return;
			}

			if (textures[typeIndex] != nullptr)
			{
				LogError("Failed to add texture of type '%u' to material: '%s'. This slot already contained a texture!", typeIndex, name.c_str());
				return;
			}

			textures[typeIndex] = texture;
			textureCount++;
		}

		bool HasTextureOfType(const TEXTURE_TYPE type)
		{
			if (type == TEXTURE_TYPE::_COUNT) return false;

			return textures[static_cast<uint32_t>(type)] != nullptr;
		}

		Texture* GetTextureOfType(const TEXTURE_TYPE type)
		{
			return textures[static_cast<size_t>(type)];
		}

		std::string_view GetName() const 
		{
			return name;
		}

		void SetName(const std::string_view& _name)
		{
			name = _name;
		}

		uint32_t GetTextureCount() const
		{
			return textureCount;
		}

	private:

		std::string name;
		std::vector<Texture*> textures;
		uint32_t textureCount;
	};

	struct Mesh
	{
		std::vector<PBRVertex> vertices;
		std::vector<IndexType> indices;
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
		std::vector<Material> materials;
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
		std::vector<TextureResource> material;		// Every entry in this vector corresponds to a type of texture, specifically from Material::TEXTURE_TYPE

		// NOTE - The API user must update and keep track of the transform data for the assets,
		//        and pass it to the renderer every frame for drawing. The design decision behind
		//        this is so we can own copy of the data, rather than holding a ton of pointers 
		//        to data somewhere else which will probably be very slow
		Transform transform;

		bool shouldDraw;							// Determines whether the asset should be drawn on the current frame. This value is reset every frame
	};
}

#endif