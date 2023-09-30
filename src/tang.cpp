
#include "asset_loader.h"
#include "renderer.h"
#include "utils/sanity_check.h"
#include "tang.h"

namespace TANG
{
	static Renderer rendererHandle;

	///////////////////////////////////////////////////////////
	//
	//		CORE
	// 
	///////////////////////////////////////////////////////////
	void Initialize()
	{
		rendererHandle.Initialize();
	}

	void Update(float* deltaTime)
	{
		rendererHandle.Update(deltaTime);
	}

	void Draw()
	{
		rendererHandle.Draw();
	}

	void Shutdown()
	{
		LoaderUtils::UnloadAll();
		rendererHandle.Shutdown();
	}

	///////////////////////////////////////////////////////////
	//
	//		STATE
	// 
	///////////////////////////////////////////////////////////
	bool WindowShouldClose()
	{
		// NOTE - This is definitely going to move to a separate Window class at some point...
		//        This function call should not be part of the renderer, it's just that GLFW
		//        and the renderer class are too tightly coupled atm
		return rendererHandle.WindowShouldClose();
	}

	UUID LoadAsset(const char* filepath)
	{
		AssetDisk* asset = LoaderUtils::Load(filepath);
		// If Load() returns nullptr, we know it didn't allocate memory on the heap, so no need to de-allocate anything here
		if (asset == nullptr)
		{
			LogError("Failed to load asset '%s'", filepath);
			return INVALID_UUID;
		}

		AssetResources* resources = rendererHandle.CreateAssetResources(asset);
		if (resources == nullptr)
		{
			LogError("Failed to create asset resources for asset '%s'", filepath);
			return INVALID_UUID;
		}

		rendererHandle.CreateAssetCommandBuffer(resources);

		return asset->uuid;
	}

	///////////////////////////////////////////////////////////
	//
	//		UPDATE
	// 
	///////////////////////////////////////////////////////////
	void RenderAsset(UUID uuid)
	{
		rendererHandle.SetAssetDrawState(uuid);
	}

	void UpdateAssetTransform(UUID uuid, float* position, float* rotation, float* scale)
	{
		TNG_ASSERT_TODO()
	}

	void UpdateAssetPosition(UUID uuid, float* position)
	{
		TNG_ASSERT_TODO()
	}

	void UpdateAssetRotation(UUID uuid, float* rotation)
	{
		TNG_ASSERT_TODO()
	}

	void UpdateAssetScale(UUID uuid, float* scale)
	{
		TNG_ASSERT_TODO()
	}
}