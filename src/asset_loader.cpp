
#include "asset_loader.h"

#include <filesystem>
#include <iostream>

// Silence stb_image warnings:
// warning C4244: 'argument': conversion from 'int' to 'short', possible loss of data
#pragma warning(push)
#pragma warning(disable : 4244)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning(pop)

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "config.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

// Enables assimp fast imports, which don't perform nearly as much error checking on the asset data.
// If this is disabled, the loader defaults to "quality" importing which is slower but
// fixes common errors on data and optimizes the asset data slightly
//#define FAST_IMPORT

namespace TANG
{
	template<typename T>
	void LoadMeshVertices(const aiMesh* importedMesh, TANG::Mesh<T>* mesh)
	{
		TNG_ASSERT_MSG(false, "Vertex type specialization not found. Please add a template specialization for the new vertex type");
	}

	// PBR VERTEX
	template<>
	void LoadMeshVertices<TANG::PBRVertex>(const aiMesh* importedMesh, TANG::Mesh<TANG::PBRVertex>* mesh)
	{
		uint32_t vertexCount = importedMesh->mNumVertices;

		for (uint32_t j = 0; j < vertexCount; j++)
		{
			const aiVector3D& importedPos = importedMesh->mVertices[j];
			const aiVector3D& importedNormal = importedMesh->mNormals[j];
			const aiVector3D& importedTangent = importedMesh->mTangents[j];
			const aiVector3D& importedBitangent = importedMesh->mBitangents[j];
			const aiVector3D& importedUVs = importedMesh->HasTextureCoords(0) ? importedMesh->mTextureCoords[0][j] : aiVector3D(0, 0, 0);

			TANG::PBRVertex vertex{};
			vertex.pos = { importedPos.x, importedPos.y, importedPos.z };
			vertex.normal = { importedNormal.x, importedNormal.y, importedNormal.z };
			vertex.tangent = { importedTangent.x, importedTangent.y, importedTangent.z };
			vertex.bitangent = { importedBitangent.x, importedBitangent.y, importedBitangent.z };
			vertex.uv = { importedUVs.x, importedUVs.y };

			mesh->vertices[j] = vertex;
		}
	}

	// CUBEMAP VERTEX
	template<> 
	void LoadMeshVertices<TANG::CubemapVertex>(const aiMesh* importedMesh, TANG::Mesh<TANG::CubemapVertex>* mesh)
	{
		uint32_t vertexCount = importedMesh->mNumVertices;

		for (uint32_t j = 0; j < vertexCount; j++)
		{
			const aiVector3D& importedPos = importedMesh->mVertices[j];

			TANG::CubemapVertex vertex{};
			vertex.pos = { importedPos.x, importedPos.y, importedPos.z };

			mesh->vertices[j] = vertex;
		}
	}

	template<>
	void LoadMeshVertices<TANG::UVVertex>(const aiMesh* importedMesh, TANG::Mesh<TANG::UVVertex>* mesh)
	{
		uint32_t vertexCount = importedMesh->mNumVertices;

		for (uint32_t j = 0; j < vertexCount; j++)
		{
			const aiVector3D& importedPos = importedMesh->mVertices[j];
			const aiVector3D& importedUVs = importedMesh->HasTextureCoords(0) ? importedMesh->mTextureCoords[0][j] : aiVector3D(0, 0, 0);

			TANG::UVVertex vertex{};
			vertex.pos = { importedPos.x, importedPos.y, importedPos.z };
			vertex.uv = { importedUVs.x, 1.0f - importedUVs.y }; // Flip in here because I can't figure out how to flip UVs in Blender

			mesh->vertices[j] = vertex;
		}
	}

	template<typename T>
	void LoadMesh(const aiMesh* importedMesh, TANG::AssetDisk* asset)
	{
		uint32_t faceCount = importedMesh->mNumFaces;

		TANG::Mesh<T>* mesh = new TANG::Mesh<T>();
		mesh->vertices.resize(importedMesh->mNumVertices);
		mesh->indices.resize(faceCount * 3);

		// VERTICES
		LoadMeshVertices<T>(importedMesh, mesh);

		// INDICES
		for (uint32_t j = 0; j < faceCount; j++)
		{
			uint32_t indexCount = j * 3;
			const aiFace& importedFace = importedMesh->mFaces[j];

			mesh->indices[indexCount    ] = importedFace.mIndices[0];
			mesh->indices[indexCount + 1] = importedFace.mIndices[1];
			mesh->indices[indexCount + 2] = importedFace.mIndices[2];
		}

		// Store the mesh pointer in the asset
		asset->mesh = mesh;
	}

	AssetContainer::AssetContainer() { }

	AssetContainer::~AssetContainer()
	{
		// We clear the pointers upon program exit. It's the responsibility of
		// the LoaderUtils to clean up the memory properly
		container.clear();
	}

	AssetDisk* AssetContainer::GetAsset(UUID uuid) const
	{
		for (const auto& iter : container)
		{
			AssetDisk* asset = iter.second;
			if (asset->uuid == uuid) return asset;
		}

		return nullptr;
	}

	AssetDisk* AssetContainer::GetAsset(const char* name) const
	{
		for (const auto& iter : container)
		{
			AssetDisk* asset = iter.second;
			if (asset->name == name) return asset;
		}

		return nullptr;
	}

	void AssetContainer::InsertAsset(AssetDisk* asset, bool forceOverride)
	{
		auto iter = container.find(asset->uuid);
		if (iter != container.end())
		{
			if (!forceOverride) return;

			LogWarning("Overwrote asset in asset container!");
			container[asset->uuid] = asset;
			return;
		}

		container.insert({ asset->uuid, asset });
	}

	AssetDisk* AssetContainer::RemoveAsset(UUID uuid)
	{
		auto iter = container.find(uuid);
		if (iter == container.end())
		{
			return nullptr;
		}

		AssetDisk* asset = iter->second;
		container.erase(iter);
		return asset;
	}

	bool AssetContainer::AssetExists(UUID uuid) const
	{
		return container.find(uuid) != container.end();
	}

	AssetDisk* AssetContainer::GetFirst() const
	{
		// Edge case for empty container
		if (container.begin() == container.end()) return nullptr;

		return container.begin()->second;
	}

	//////////////////////////////////////////////////////////////////
	//
	//	LOADER UTILS
	//
	//////////////////////////////////////////////////////////////////

	namespace LoaderUtils
	{
		AssetDisk* Load(std::string_view filePath)
		{
			LogInfo("Starting asset load for '%s'", filePath.data());

			Assimp::Importer importer;
#if defined(FAST_IMPORT)
			uint32_t importFlags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace;
#else
			uint32_t importFlags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_FixInfacingNormals | aiProcess_FindInvalidData;
#endif
			const aiScene* scene = importer.ReadFile(filePath.data(), importFlags);

			if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 1 || scene->mRootNode == nullptr)
			{
				LogWarning(importer.GetErrorString());
				return nullptr;
			}

			uint32_t numMeshes = scene->mNumMeshes;
			uint32_t numTextures = scene->mNumTextures;
			uint32_t numMaterials = scene->mNumMaterials;

			// Check that we have at least one mesh, and warn if we have have more than one mesh
			if (numMeshes < 1)
			{
				LogError("Failed to load asset from file '%s'! At least one mesh is required", filePath.data());
				return nullptr;
			}
			else if (numMeshes > 1)
			{
				LogWarning("Multiple meshes detected for asset '%s', but multiple meshes are not supported!", filePath.data());
			}

			// Now we can create the Asset instance
			AssetDisk* asset = new AssetDisk();
			asset->textures.resize(numTextures);
			asset->materials.resize(numMaterials);

			// Load the mesh
			// NOTE - Only one mesh per asset is supported currently
			aiMesh* importedMesh = scene->mMeshes[0];

			// Determine the mesh type
			// TODO - Find a better way to do this
			if (filePath == CONFIG::SkyboxCubeMeshFilePath)
			{
				LoadMesh<CubemapVertex>(importedMesh, asset);
				LogInfo("Loaded mesh using CubemapVertex for asset '%s'", filePath.data());
			}
			else if (filePath == CONFIG::FullscreenQuadMeshFilePath)
			{
				LoadMesh<UVVertex>(importedMesh, asset);
				LogInfo("Loaded mesh using UVVertex for asset '%s'", filePath.data());
			}
			else
			{
				LoadMesh<PBRVertex>(importedMesh, asset);
				LogInfo("Loaded mesh using PBRVertex for asset '%s'", filePath.data());

				// Load the standalone texture(s)
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

				// Load the materials
				for (uint32_t i = 0; i < numMaterials; i++)
				{
					Material& currentMaterial = asset->materials[i];

					aiMaterial* currentAIMaterial = scene->mMaterials[i];
					aiString matName = currentAIMaterial->GetName();

					currentMaterial.SetName(std::string(matName.C_Str()));

					// Get all the supported textures
					for (const auto& aiType : SupportedTextureTypes)
					{
						uint32_t textureCount = currentAIMaterial->GetTextureCount(aiType);
						if (textureCount > 0)
						{
							// Warn if we have more than one diffuse texture, we don't currently support multiple texture of a given type
							if (textureCount > 1)
							{
								LogWarning("More than one texture type (%u) detected for material %s! This is not currently supported", static_cast<uint32_t>(aiType), matName.C_Str());
							}

							aiString texturePath;
							if (currentAIMaterial->GetTexture(aiType, 0, &texturePath) == AI_SUCCESS)
							{
								// We're only interested in the filenames, since we store the textures in a very specific directory
								std::filesystem::path textureFilePath = std::filesystem::path(texturePath.data);
								std::filesystem::path textureName = textureFilePath.filename();
								std::filesystem::path assetDirectoryName = std::filesystem::path(filePath).parent_path().filename();
								std::filesystem::path textureSourceFilePath = std::filesystem::path(CONFIG::MaterialTexturesFilePath);

								textureSourceFilePath += assetDirectoryName;
								textureSourceFilePath /= textureName;

								// Load the image using stb_image
								std::string textureSourceFilePathStr = textureSourceFilePath.string();
								int width, height, channels;
								stbi_uc* pixels = stbi_load(textureSourceFilePathStr.c_str(), &width, &height, &channels, STBI_rgb_alpha);
								if (pixels == nullptr)
								{
									LogError("Failed to load texture! '%s'", textureSourceFilePathStr.c_str());
									continue;
								}

								// Create a new texture and add it to the material
								Texture* tex = new Texture();
								tex->data = pixels;
								tex->size = { width, height }; // NOTE - We don't support 3D textures!
								tex->bytesPerPixel = 32;
								tex->fileName = textureSourceFilePathStr;

								auto texTypeIter = AITextureToInternal.find(aiType);
								if (texTypeIter == AITextureToInternal.end())
								{
									LogError("Failed to convert from aiTexture to the internal texture format! AiTexture type '%s'", static_cast<uint32_t>(aiType));
									continue;
								}

								currentMaterial.AddTextureOfType(texTypeIter->second, tex);

								LogInfo("\tMaterial %u: Loaded %s texture '%s' from disk", 
									i, 
									TextureTypeToString.at(AITextureToInternal.at(aiType)).c_str(), 
									textureName.string().c_str()
								);
							}
						}
					}

					// Remove any materials which have no textures, either because we don't support them only textures it has or
					// it was exported incorrectly
					if (currentMaterial.GetTextureCount() == 0)
					{
						LogWarning("Material '%s' in asset '%s' has no supported textures! Deleting empty material...", currentMaterial.GetName().data(), filePath.data());
						asset->materials.erase(asset->materials.begin() + i);
					}
				}
			}

			LogInfo("Finished loading asset with %u materials!", asset->materials.size());

			AssetContainer& container = AssetContainer::GetInstance();

			// Calculate UUID, and keep generating UUIDs in case of collision
			UUID uuid = GetUUID();
			while (container.AssetExists(uuid))
			{
				uuid = GetUUID();
			}

			asset->uuid = uuid;
			asset->name = filePath;

			container.InsertAsset(asset);

			// We're good to go!
			return asset;
		}

		bool Unload(UUID uuid)
		{
			AssetContainer& container = AssetContainer::GetInstance();
			if (!container.AssetExists(uuid))
			{
				LogWarning("Failed to find and unload model with UUID %ull!", uuid);
				return false;
			}

			// Delete the Asset*
			AssetDisk* asset = container.RemoveAsset(uuid);
			if (asset != nullptr)
			{
				delete asset->mesh;
				delete asset;
			}

			return true;
		}

		void UnloadAll()
		{
			AssetContainer& container = AssetContainer::GetInstance();
			AssetDisk* asset = container.GetFirst();
			while (asset != nullptr)
			{
				Unload(asset->uuid);
				asset = container.GetFirst();
			}
		}
	}

}