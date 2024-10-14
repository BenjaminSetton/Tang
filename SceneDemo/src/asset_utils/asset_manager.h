#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <vector>
#include <unordered_map>

#include "TANG/asset_types.h"
#include "TANG/utils/uuid.h"

enum class CorePipeline
{
	PBR,
	CUBEMAP_PREPROCESSING,
	SKYBOX,
	FULLSCREEN_QUAD
};

class AssetManager
{
public:

	// No copying!
	AssetManager(const AssetManager& other) = delete;
	AssetManager& operator=(const AssetManager& other) = delete;

	// Singleton
	static AssetManager& Get()
	{
		static AssetManager instance;
		return instance;
	}

	// Loads an asset which implies grabbing the vertices and indices from the asset container
	// and creating vertex/index buffers to contain them. It also includes creating all other
	// API objects necessary for rendering. This resources created depend entirely on the pipeline
	// 
	// Before calling this function, make sure you've called LoaderUtils::LoadAsset() and have
	// successfully loaded an asset from file! This functions assumes this, and if it can't retrieve
	// the loaded asset data it will return prematurely
	bool CreateAssetResources(TANG::AssetDisk* asset, CorePipeline corePipeline);

	void DestroyAssetResources(TANG::UUID uuid);
	void DestroyAllAssetResources();

	TANG::AssetResources* GetAssetResourcesFromUUID(TANG::UUID uuid);

private:

	AssetManager() = default;
	~AssetManager();

private:

	bool CreatePBRAssetResources(TANG::AssetDisk* asset, TANG::AssetResources& out_resources);
	bool CreateSkyboxAssetResources(TANG::AssetDisk* asset, TANG::AssetResources& out_resources);
	bool CreateFullscreenQuadAssetResources(TANG::AssetDisk* asset, TANG::AssetResources& out_resources);
	bool CreateTextureResources(TANG::AssetDisk* asset, TANG::AssetResources& out_resources);

	void DestroyAssetBuffersHelper(TANG::AssetResources* resources);

	// The assetResources vector contains all the important information that we need for every asset in order to render it
	// The resourcesMap maps an asset's UUID to a location within the assetResources vector
	std::unordered_map<TANG::UUID, uint32_t> resourcesMap;
	std::vector<TANG::AssetResources> assetResources;

};


#endif