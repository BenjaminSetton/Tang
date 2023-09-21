
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

	void Update(float deltaTime)
	{
		UNUSED(deltaTime);
		rendererHandle.Update();
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

	void LoadAsset(const char* name)
	{
		Asset* asset = LoaderUtils::Load(name);
		// If Load() returns nullptr, we know it didn't allocate memory on the heap, so no need to de-allocate anything here
		if (asset == nullptr)
		{
			LogError("Failed to load asset '%s'", name);
			return;
		}

		rendererHandle.CreateAssetResources(asset);
	}

	// UPDATE CALLS
	void DrawAsset(const char* name)
	{
		Asset* asset = AssetContainer::GetInstance()->GetAsset(name);
		rendererHandle.SetAssetDrawState(asset->uuid);
	}


}