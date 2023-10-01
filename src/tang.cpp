
#include "asset_loader.h"
#include "renderer.h"
#include "utils/sanity_check.h"
#include "tang.h"

namespace TANG
{
	// Let's make extra sure our conversions from float* to glm::vec3 for asset transforms below
	// works exactly as expected
	TNG_ASSERT_COMPILE(sizeof(glm::vec3) == 3 * sizeof(float));

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