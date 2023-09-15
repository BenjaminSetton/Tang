
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

	const Asset* AssetContainer::GetAsset(std::string_view name) const
	{
		for (const auto& iter : container)
		{
			Asset* asset = iter.second;
			if (asset->name == name) return asset;
		}

		return nullptr;
	}
}