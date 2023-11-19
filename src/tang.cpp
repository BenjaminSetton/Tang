
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
	static constexpr uint32_t WINDOW_WIDTH = 1920;
	static constexpr uint32_t WINDOW_HEIGHT = 1080;
	static FreeflyCamera camera;

	///////////////////////////////////////////////////////////
	//
	//		CORE
	// 
	///////////////////////////////////////////////////////////
	void Initialize()
	{
		MainWindow& window = MainWindow::GetInstance();

		window.Create(WINDOW_WIDTH, WINDOW_HEIGHT);
		InputManager::GetInstance().Initialize(window.GetHandle());
		Renderer::GetInstance().Initialize(window.GetHandle(), WINDOW_WIDTH, WINDOW_HEIGHT);
		camera.Initialize({ 0.0f, 5.0f, 15.0f }, { 0.0f, 0.0f, 0.0f }); // Start the camera facing towards negative Z
	}

	void Update(float deltaTime)
	{
		MainWindow& window = MainWindow::GetInstance();
		Renderer& renderer = Renderer::GetInstance();
		InputManager& inputManager = InputManager::GetInstance();

		window.Update(deltaTime);

		inputManager.Update();

		camera.Update(deltaTime);

		// Poll the main window for resizes, rather than doing it through events
		if (window.WasWindowResized())
		{
			// Notify the renderer that the window was resized
			uint32_t width, height;
			window.GetFramebufferSize(&width, &height);

			renderer.SetNextFramebufferSize(width, height);
		}

		// Update the camera data that the renderer is holding with the most up-to-date info
		renderer.UpdateCameraData(camera.GetPosition(), camera.GetViewMatrix());

		renderer.Update(deltaTime);
	}

	void Draw()
	{
		Renderer::GetInstance().Draw();
	}

	void Shutdown()
	{
		camera.Shutdown();
		LoaderUtils::UnloadAll();
		Renderer::GetInstance().Shutdown();
		InputManager::GetInstance().Shutdown();
		MainWindow::GetInstance().Destroy();
	}

	///////////////////////////////////////////////////////////
	//
	//		STATE
	// 
	///////////////////////////////////////////////////////////
	bool WindowShouldClose()
	{
		return MainWindow::GetInstance().ShouldClose();
	}

	UUID LoadAsset(const char* filepath)
	{
		Renderer& renderer = Renderer::GetInstance();

		AssetDisk* asset = LoaderUtils::Load(filepath);
		// If Load() returns nullptr, we know it didn't allocate memory on the heap, so no need to de-allocate anything here
		if (asset == nullptr)
		{
			LogError("Failed to load asset '%s'", filepath);
			return INVALID_UUID;
		}

		AssetResources* resources = renderer.CreateAssetResources(asset);
		if (resources == nullptr)
		{
			LogError("Failed to create asset resources for asset '%s'", filepath);
			return INVALID_UUID;
		}

		renderer.CreateAssetCommandBuffer(resources);

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
		Renderer::GetInstance().SetAssetDrawState(uuid);
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
		Renderer::GetInstance().SetAssetTransform(uuid, transform);
	}

	void UpdateAssetPosition(UUID uuid, float* position)
	{
		TNG_ASSERT_MSG(position != nullptr, "Position cannot be null!");
		Renderer::GetInstance().SetAssetPosition(uuid, *(reinterpret_cast<glm::vec3*>(position)));
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

		Renderer::GetInstance().SetAssetRotation(uuid, rotVector);
	}

	void UpdateAssetScale(UUID uuid, float* scale)
	{
		TNG_ASSERT_MSG(scale != nullptr, "Scale cannot be null!");
		Renderer::GetInstance().SetAssetScale(uuid, *(reinterpret_cast<glm::vec3*>(scale)));
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