
#include "asset_loader.h"

namespace TANG
{
	AssetContainer::AssetContainer() { }

	AssetContainer::~AssetContainer()
	{
		// We clear the pointers upon program exit. It's the responsibility of
		// the LoaderUtils to clean up the memory properly
		container.clear();
	}

	AssetCore* AssetContainer::GetAsset(UUID uuid) const
	{
		for (const auto& iter : container)
		{
			AssetCore* asset = iter.second;
			if (asset->uuid == uuid) return asset;
		}

		return nullptr;
	}

	AssetCore* AssetContainer::GetAsset(const char* name) const
	{
		for (const auto& iter : container)
		{
			AssetCore* asset = iter.second;
			if (asset->name == name) return asset;
		}

		return nullptr;
	}

	void AssetContainer::InsertAsset(AssetCore* asset, bool forceOverride)
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

	AssetCore* AssetContainer::RemoveAsset(UUID uuid)
	{
		auto iter = container.find(uuid);
		if (iter == container.end())
		{
			return nullptr;
		}

		AssetCore* asset = iter->second;
		container.erase(iter);
		return asset;
	}

	bool AssetContainer::AssetExists(UUID uuid) const
	{
		return container.find(uuid) != container.end();
	}

	AssetCore* AssetContainer::GetFirst() const
	{
		// Edge case for empty container
		if (container.begin() == container.end()) return nullptr;

		return container.begin()->second;
	}
}