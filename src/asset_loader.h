
#include <glm.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "log_utils.h"

namespace TANG
{
	// TODO - Find a better place for these definitions!
	struct Vertex
	{
		Vertex() : pos(0.0f, 0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f), uv(0.0f, 0.0f, 0.0f)
		{
		}
		~Vertex() { }
		// Too lazy to implement copy-constructor; should work just fine in this case

		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 uv;
	};

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

			data = new char[other.size.x * other.size.y * bytesPerPixel];
			size = other.size;
			bytesPerPixel = other.bytesPerPixel;
		}

		void* data;
		glm::vec2 size;
		uint32_t bytesPerPixel; // Guaranteed to be 4 by assimp loader
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		//std::vector<Material> materials;
	};

	struct Asset
	{
		std::string name;
		std::vector<Mesh> meshes;
		std::vector<Texture> textures;
	};

	// Defines utilities for loading assets from file.
	// Uses assimp for FBX loading, there is currently no plan for supporting multiple
	// asset-loading libraries which is why the library code is not abstracted away
	class LoaderUtils
	{

	public:

		static bool Load(std::string_view filePath)
		{
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(filePath.data(), aiProcess_Triangulate | aiProcess_FlipUVs);

			if (scene != nullptr && (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 && scene->mRootNode != nullptr)
			{
				LogWarning(importer.GetErrorString());
				return false;
			}

			uint32_t numMeshes = scene->mNumMeshes;
			uint32_t numTextures = scene->mNumTextures;

			// Check that we have at least one mesh
			if (numMeshes < 1)
			{
				LogWarning("Failed to load asset! At least one mesh is required");
				return false;
			}

			// Now we can create the Asset instance
			Asset* asset = new Asset();
			asset->meshes.reserve(numMeshes);
			asset->textures.reserve(numTextures);

			// Load the mesh(es)
			for (uint32_t i = 0; i < numMeshes; i++)
			{
				aiMesh* importedMesh = scene->mMeshes[i];

				Mesh& mesh = asset->meshes[i];
				uint32_t vertexCount = importedMesh->mNumVertices;
				uint32_t faceCount = importedMesh->mNumFaces;
				mesh.vertices.reserve(vertexCount);
				mesh.indices.reserve(faceCount * 3);

				// Fill out the vertices from the imported mesh object
				for (uint32_t i = 0; i < vertexCount; i++)
				{
					const aiVector3D& importedPos = importedMesh->mVertices[i];
					const aiVector3D& importedNormal = importedMesh->mNormals[i];
					const aiVector3D& importedUVs = importedMesh->HasTextureCoords(0) ? importedMesh->mTextureCoords[0][i] : aiVector3D(0, 0, 0);

					Vertex vertex{};
					vertex.pos    = { importedPos.x, importedPos.y, importedPos.z };
					vertex.normal = { importedNormal.x, importedNormal.y, importedNormal.z };
					vertex.uv     = { importedUVs.x, importedUVs.y, importedUVs.z };

					mesh.vertices[i] = vertex;
				}

				// Fill out the indices from the imported mesh object
				for (uint32_t i = 0; i < faceCount; i++)
				{
					uint32_t indexCount = i * 3;
					const aiFace& importedFace = importedMesh->mFaces[i];

					mesh.indices[indexCount]     = importedFace.mIndices[0];
					mesh.indices[indexCount + 1] = importedFace.mIndices[1];
					mesh.indices[indexCount + 2] = importedFace.mIndices[2];
				}
			}

			// Load the texture(s)
			for (uint32_t i = 0; i < numTextures; i++)
			{
				aiTexture* importedTexture = scene->mTextures[i];

				Texture& texture = asset->textures[i];
				texture.size = { importedTexture->mWidth, importedTexture->mHeight };

				// Populate the texture data. Note from the assimp implementation:
				// The format of the data from the imported texture is always ARGB8888, meaning it's 32-bit aligned
				uint32_t texSize = texture.size.x * texture.size.y;
				char* data = new char[texSize];
				memcpy(data, importedTexture->pcData, texSize * 4);
				texture.data = data;
			}

			auto assetContainer = AssetContainer::GetInstance()->container;
			assetContainer[filePath] = asset;
		}

		static bool Unload(std::string_view filePath)
		{
			auto assetContainer = AssetContainer::GetInstance()->container;
			auto assetIter = assetContainer.find(filePath);
			if (assetIter == assetContainer.end())
			{
				LogWarning("Failed to find and unload model! Invalid name");
				return false;
			}

			// Delete the Asset*
			delete assetIter->second;
		}

		static void UnloadAll()
		{
			auto assetContainer = AssetContainer::GetInstance()->container;
			for (const auto& iter : assetContainer)
			{
				// Delete the Asset*
				delete iter.second;
			}
		}

	};

	// Holds references to all the loaded assets. Assets are loaded into the internal container through
	// LoaderUtils::Load() and unloaded through LoaderUtils::Unload()
	class AssetContainer
	{
		friend class LoaderUtils;

	public:

		AssetContainer();
		~AssetContainer();

		// Singletons should not be assignable nor copyable
		AssetContainer(const AssetContainer& other) = delete;
		void operator=(const AssetContainer& other) = delete;

		static AssetContainer* GetInstance()
		{
			if (instance == nullptr)
			{
				instance = new AssetContainer();
			}

			return instance;
		}

		const Asset* GetAsset(std::string_view name) const;

	private:

		static AssetContainer* instance;
		std::unordered_map<std::string_view, Asset*> container;

	};
}