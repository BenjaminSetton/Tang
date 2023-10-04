
#include "asset_loader.h"
#include "renderer.h"
#include "utils/sanity_check.h"
#include "window.h"
#include "tang.h"

namespace TANG
{
	// Let's make extra sure our conversions from float* to glm::vec3 for asset transforms below
	// works exactly as expected
	TNG_ASSERT_COMPILE(sizeof(glm::vec3) == 3 * sizeof(float));

	// TODO - Maybe move this to a config?
	static constexpr uint32_t WINDOW_WIDTH = 1920;
	static constexpr uint32_t WINDOW_HEIGHT = 1080;

	// Static global handles
	static Renderer rendererHandle;
	static Window windowHandle;

	///////////////////////////////////////////////////////////
	//
	//		CORE
	// 
	///////////////////////////////////////////////////////////
	void Initialize()
	{
		windowHandle.Create(WINDOW_WIDTH, WINDOW_HEIGHT);
		rendererHandle.Initialize(windowHandle.GetHandle(), WINDOW_WIDTH, WINDOW_HEIGHT);

		// Register the callbacks
		windowHandle.SetRecreateSwapChainCallback(Renderer::RecreateSwapChain);
	}

	void Update(float deltaTime)
	{
		windowHandle.Update(deltaTime);
		if (windowHandle.WasWindowResized())
		{
			// Notify the renderer that the window was resized
			//rendererHandle.
		}

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
		windowHandle.Destroy();
	}

	///////////////////////////////////////////////////////////
	//
	//		STATE
	// 
	///////////////////////////////////////////////////////////
	bool WindowShouldClose()
	{
		return windowHandle.ShouldClose();
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
	void ShowAsset(UUID uuid)
	{
		rendererHandle.SetAssetDrawState(uuid);
	}

	void UpdateAssetTransform(UUID uuid, float* position, float* rotation, float* scale)
	{
		TNG_ASSERT_MSG(position != nullptr, "Position cannot be null!");
		TNG_ASSERT_MSG(rotation != nullptr, "Rotation cannot be null!");
		TNG_ASSERT_MSG(scale != nullptr, "Scale cannot be null!");

		Transform transform(
			*(reinterpret_cast<glm::vec3*>(position)),
			*(reinterpret_cast<glm::vec3*>(rotation)),
			*(reinterpret_cast<glm::vec3*>(scale)));
		rendererHandle.SetAssetTransform(uuid, transform);
	}

	void UpdateAssetPosition(UUID uuid, float* position)
	{
		TNG_ASSERT_MSG(position != nullptr, "Position cannot be null!");
		rendererHandle.SetAssetPosition(uuid, *(reinterpret_cast<glm::vec3*>(position)));
	}

	void UpdateAssetRotation(UUID uuid, float* rotation, bool isDegrees)
	{
		TNG_ASSERT_MSG(rotation != nullptr, "Rotation cannot be null!");
		glm::vec3 rotVector = *(reinterpret_cast<glm::vec3*>(rotation));

		// If the rotation was given in degrees, we must convert it to radians
		// since that's what glm uses
		if (isDegrees)
		{
			rotVector = glm::radians(rotVector);
		}

		rendererHandle.SetAssetRotation(uuid, rotVector);
	}

	void UpdateAssetScale(UUID uuid, float* scale)
	{
		TNG_ASSERT_MSG(scale != nullptr, "Scale cannot be null!");
		rendererHandle.SetAssetScale(uuid, *(reinterpret_cast<glm::vec3*>(scale)));
	}
}