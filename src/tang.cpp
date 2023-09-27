
#include "asset_loader.h"
#include "renderer.h"
#include "utils/sanity_check.h"
#include "tang.h"

namespace TANG
{
	static Renderer rendererHandle;

	void Initialize()
	{
		rendererHandle.Initialize();
	}

	void Update(float* deltaTime)
	{
		rendererHandle.Update(deltaTime);
	}

	void Shutdown()
	{
		LoaderUtils::UnloadAll();
		rendererHandle.Shutdown();
	}

	// STATE CALLS
	bool WindowShouldClose()
	{
		// NOTE - This is definitely going to move to a separate Window class at some point...
		//        This function call should not be part of the renderer, it's just that GLFW
		//        and the renderer class are too tightly coupled atm
		return rendererHandle.WindowShouldClose();
	}

	bool LoadAsset(const char* name)
	{
		AssetDisk* asset = LoaderUtils::Load(name);
		// If Load() returns nullptr, we know it didn't allocate memory on the heap, so no need to de-allocate anything here
		if (asset == nullptr)
		{
			LogError("Failed to load asset '%s'", name);
			return false;
		}

		AssetResources* resources = rendererHandle.CreateAssetResources(asset);
		if (resources == nullptr)
		{
			LogError("Failed to create asset resources for asset '%s'", name);
			return false;
		}

		rendererHandle.CreateAssetCommandBuffer(resources);

		return true;
	}

	// UPDATE CALLS
	void DrawAsset(const char* name)
	{
		AssetContainer& container = AssetContainer::GetInstance();
		AssetDisk* asset = container.GetAsset(name);
		if (asset == nullptr) return;

		rendererHandle.SetAssetDrawState(asset->uuid);
	}


}