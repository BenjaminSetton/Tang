
#include "asset_loader.h"
#include "renderer.h"
#include "main_window.h"
#include "camera/freefly_camera.h"
#include "tang.h"
#include "utils/sanity_check.h"

namespace TANG
{
	// Let's make extra sure our conversions from float* to glm::vec3 for asset transforms below
	// works exactly as expected
	TNG_ASSERT_COMPILE(sizeof(glm::vec3) == 3 * sizeof(float));

	// TODO - Maybe move this to a config?
	static constexpr uint32_t WINDOW_WIDTH = 1280;
	static constexpr uint32_t WINDOW_HEIGHT = 720;

	// Static global handles
	static Renderer rendererHandle;
	static MainWindow windowHandle;
	static FreeflyCamera camera;

	///////////////////////////////////////////////////////////
	//
	//		CORE
	// 
	///////////////////////////////////////////////////////////
	void Initialize()
	{
		windowHandle.Create(WINDOW_WIDTH, WINDOW_HEIGHT);
		InputManager::GetInstance().Initialize(windowHandle.GetHandle());
		rendererHandle.Initialize(windowHandle.GetHandle(), WINDOW_WIDTH, WINDOW_HEIGHT);
		camera.Initialize({ 0.0f, 5.0f, 15.0f }, { 0.0f, 0.0f, 0.0f });
	}

	void Update(float deltaTime)
	{
		windowHandle.Update(deltaTime);

		InputManager::GetInstance().Update();

		camera.Update(deltaTime);

		// Poll the main window for resizes, rather than doing it through events
		if (windowHandle.WasWindowResized())
		{
			// Notify the renderer that the window was resized
			uint32_t width, height;
			windowHandle.GetFramebufferSize(&width, &height);

			rendererHandle.SetNextFramebufferSize(width, height);
		}

		// Update the camera data that the renderer is holding with the most up-to-date info
		rendererHandle.UpdateCameraData(camera.GetPosition(), camera.GetViewMatrix());

		rendererHandle.Update(deltaTime);
	}

	void Draw()
	{
		rendererHandle.Draw();
	}

	void Shutdown()
	{
		LoaderUtils lu;

		camera.Shutdown();
		lu.UnloadAll();
		rendererHandle.Shutdown();
		InputManager::GetInstance().Shutdown();
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
		LoaderUtils lu;

		AssetDisk* asset = lu.Load(filepath);
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

	void SetCameraSpeed(float speed)
	{
		camera.SetSpeed(speed);
	}

	void SetCameraSensitivity(float sensitivity)
	{
		camera.SetSensitivity(sensitivity);
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

	bool IsKeyPressed(int key)
	{
		return InputManager::GetInstance().IsKeyPressed(key);
	}

	bool IsKeyReleased(int key)
	{
		return InputManager::GetInstance().IsKeyReleased(key);
	}

	KeyState GetKeyState(int key)
	{
		return InputManager::GetInstance().GetKeyState(key);
	}
}