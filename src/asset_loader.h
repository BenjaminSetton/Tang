#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "assimp/material.h"

#include "asset_types.h"
#include "utils/uuid.h"

namespace TANG
{
	// Global maps
	static const std::vector<aiTextureType> SupportedTextureTypes =
	{
		aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR,
		aiTextureType_NORMALS,
		aiTextureType_AMBIENT_OCCLUSION,
		aiTextureType_METALNESS,
		aiTextureType_DIFFUSE_ROUGHNESS,
		aiTextureType_LIGHTMAP
	};

	static const std::unordered_map<aiTextureType, Material::TEXTURE_TYPE> AITextureToInternal =
	{
		{ aiTextureType_DIFFUSE				, Material::TEXTURE_TYPE::DIFFUSE				},
		{ aiTextureType_SPECULAR			, Material::TEXTURE_TYPE::SPECULAR				},
		{ aiTextureType_NORMALS				, Material::TEXTURE_TYPE::NORMAL				},
		{ aiTextureType_AMBIENT_OCCLUSION	, Material::TEXTURE_TYPE::AMBIENT_OCCLUSION		},
		{ aiTextureType_METALNESS			, Material::TEXTURE_TYPE::METALLIC				},
		{ aiTextureType_DIFFUSE_ROUGHNESS	, Material::TEXTURE_TYPE::ROUGHNESS				},
		{ aiTextureType_LIGHTMAP			, Material::TEXTURE_TYPE::LIGHTMAP				}
	};

	static const std::unordered_map<Material::TEXTURE_TYPE, std::string> TextureTypeToString =
	{
		{ Material::TEXTURE_TYPE::DIFFUSE				, "diffuse"				},
		{ Material::TEXTURE_TYPE::SPECULAR				, "specular"			},
		{ Material::TEXTURE_TYPE::NORMAL				, "normal"				},
		{ Material::TEXTURE_TYPE::AMBIENT_OCCLUSION		, "ambient occlusion"	},
		{ Material::TEXTURE_TYPE::METALLIC				, "metallic"			},
		{ Material::TEXTURE_TYPE::ROUGHNESS				, "roughness"			},
		{ Material::TEXTURE_TYPE::LIGHTMAP				, "lightmap"			},
		{ Material::TEXTURE_TYPE::_COUNT				, "invalid"				},
	};

	// Holds references to all the loaded assets. Assets are loaded into the internal container through
	// LoaderUtils::Load() and unloaded through LoaderUtils::Unload()
	class AssetContainer
	{
	private:

		AssetContainer();
		~AssetContainer();

	public:

		// Singletons should not be assignable nor copyable
		AssetContainer(const AssetContainer& other) = delete;
		void operator=(const AssetContainer& other) = delete;

		static AssetContainer& GetInstance()
		{
			static AssetContainer instance;
			return instance;
		}

		// Retrieves a pointer to the asset inside the internal container by UUID or by name
		AssetDisk* GetAsset(UUID uuid) const;
		AssetDisk* GetAsset(const char* name) const;

		// Inserts an asset into the internal container. Optionally, the forceOverride flag
		// may be set to overwrite any existing instance of the asset
		void InsertAsset(AssetDisk* asset, bool forceOverride = false);

		// Removes an asset by UUID from the internal container. Returns the pointer to the removed asset,
		// so basically the ownership is transferred to the caller. Make sure to delete the asset after calling this
		// or you will most likely leak memory!
		AssetDisk* RemoveAsset(UUID uuid);

		// Returns whether an asset exists or not through UUID
		bool AssetExists(UUID uuid) const;

		// Returns a pointer to the first asset in the container. This is used when cleaning up
		AssetDisk* GetFirst() const;

	private:

		std::unordered_map<UUID, AssetDisk*> container;

	};

	// Defines utilities for loading assets from file.
	// Uses assimp for FBX loading, there is currently no plan for supporting multiple
	// asset-loading libraries which is why the library code is not abstracted away
	namespace LoaderUtils
	{
		// Takes in the filePath to an FBX file, and upon success returns a pointer
		// to the loaded Asset object. Note that this object is also stored in the
		// AssetContainer, so it may also be retrieved again later through it's filePath
		AssetDisk* Load(std::string_view filePath);

		bool Unload(UUID uuid);

		void UnloadAll();
	};
}

#endif