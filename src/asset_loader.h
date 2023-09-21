#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "asset_types.h"
#include "utils/logger.h"

namespace TANG
{
	class LoaderUtils;

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

		Asset* GetAsset(std::string_view name) const;

	private:

		static AssetContainer* instance;
		std::unordered_map<std::string_view, Asset*> container;

	};

	// Defines utilities for loading assets from file.
	// Uses assimp for FBX loading, there is currently no plan for supporting multiple
	// asset-loading libraries which is why the library code is not abstracted away
	class LoaderUtils
	{
	public:

		// Takes in the filePath to an FBX file, and upon success returns a pointer
		// to the loaded Asset object. Note that this object is also stored in the
		// AssetContainer, so it may also be retrieved again later through it's filePath
		static Asset* Load(std::string_view filePath)
		{
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(filePath.data(), aiProcess_Triangulate | aiProcess_FlipUVs);

			if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 1 || scene->mRootNode == nullptr)
			{
				LogWarning(importer.GetErrorString());
				return nullptr;
			}

			uint32_t numMeshes = scene->mNumMeshes;
			uint32_t numTextures = scene->mNumTextures;

			// Check that we have at least one mesh
			if (numMeshes < 1)
			{
				LogWarning("Failed to load asset! At least one mesh is required");
				return nullptr;
			}

			// Now we can create the Asset instance
			Asset* asset = new Asset();
			asset->meshes.resize(numMeshes);
			asset->textures.resize(numTextures);

			// Load the mesh(es)
			for (uint32_t i = 0; i < numMeshes; i++)
			{
				aiMesh* importedMesh = scene->mMeshes[i];

				Mesh& mesh = asset->meshes[i];
				uint32_t vertexCount = importedMesh->mNumVertices;
				uint32_t faceCount = importedMesh->mNumFaces;
				mesh.vertices.resize(vertexCount);
				mesh.indices.resize(faceCount * 3);

				// Fill out the vertices from the imported mesh object
				for (uint32_t i = 0; i < vertexCount; i++)
				{
					const aiVector3D& importedPos = importedMesh->mVertices[i];
					const aiVector3D& importedNormal = importedMesh->mNormals[i];
					const aiVector3D& importedUVs = importedMesh->HasTextureCoords(0) ? importedMesh->mTextureCoords[0][i] : aiVector3D(0, 0, 0);

					VertexType vertex{};
					vertex.pos    = { importedPos.x, importedPos.y, importedPos.z };
					vertex.normal = { importedNormal.x, importedNormal.y, importedNormal.z };
					vertex.uv     = { importedUVs.x, importedUVs.y };

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
				uint32_t texelSize = static_cast<uint32_t>(texture.size.x) * static_cast<uint32_t>(texture.size.y);
				uint64_t numBytes = texelSize * 4;
				char* data = new char[numBytes];
				memcpy(data, importedTexture->pcData, numBytes);
				texture.data = data;
			}

			auto assetContainer = AssetContainer::GetInstance()->container;

			std::string baseName = std::filesystem::path(filePath).stem().u8string();
			assetContainer[baseName] = asset;

			// Calculate UUID
			asset->uuid = GetUUID();

			// We're good to go!
			return asset;
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
}

#endif