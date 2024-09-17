
#include <cstdarg>

#include "tang.h"

#include "asset_loader.h"
#include "asset_manager.h"
#include "renderer.h"
#include "main_window.h"
#include "utils/sanity_check.h"

static TANG::CorePipeline GetCorePipelineFromFilePath(const std::string& filePath)
{
	if (filePath == TANG::CONFIG::SkyboxCubeMeshFilePath)
	{
		return TANG::CorePipeline::CUBEMAP_PREPROCESSING;
	}
	else if (filePath == TANG::CONFIG::FullscreenQuadMeshFilePath)
	{
		return TANG::CorePipeline::FULLSCREEN_QUAD;
	}
	else
	{
		return TANG::CorePipeline::PBR;
	}
}

namespace TANG
{
	// Let's make extra sure our conversions from float* to glm::vec3 for asset transforms below
	// works exactly as expected
	TNG_ASSERT_COMPILE(sizeof(glm::vec3) == 3 * sizeof(float));

	///////////////////////////////////////////////////////////
	//
	//		CORE
	// 
	///////////////////////////////////////////////////////////
	void Initialize(const char* windowTitle)
	{
		MainWindow& window = MainWindow::Get();
		Renderer& renderer = Renderer::GetInstance();

		const char* title = windowTitle == nullptr ? "TANG" : windowTitle;
		window.Create(CONFIG::WindowWidth, CONFIG::WindowHeight, title);

		InputManager::GetInstance().Initialize(window.GetHandle());
		renderer.Initialize(window.GetHandle(), CONFIG::WindowWidth, CONFIG::WindowHeight);
	}

	void Update(float deltaTime)
	{
		MainWindow& window = MainWindow::Get();
		Renderer& renderer = Renderer::GetInstance();
		InputManager& inputManager = InputManager::GetInstance();

		window.Update(deltaTime);

		inputManager.Update();

		// Only move the camera if the window is focused, otherwise the 
		// mouse cursor can freely move around
		if (!window.IsInFocus())
		{
			// Reset the internal mouse delta of the input manager to prevent snapping
			inputManager.ResetMouseDeltaCache();
		}

		// Poll the main window for resizes, rather than doing it through events
		if (window.WasWindowResized())
		{
			// Notify the renderer that the window was resized
			uint32_t width, height;
			window.GetFramebufferSize(&width, &height);

			renderer.SetNextFramebufferSize(width, height);
		}

		// Update the camera data that the renderer is holding with the most up-to-date info
		//renderer.UpdateCameraData(camera.GetPosition(), camera.GetViewMatrix(), camera.GetProjMatrix());

		renderer.Update(deltaTime);
	}

	// TEMP TEMP TEMP
	DescriptorSet AllocateDescriptorSet(const DescriptorSetLayout& setLayout)
	{
		return Renderer::GetInstance().AllocateDescriptorSet(setLayout);
	}

	PrimaryCommandBuffer AllocatePrimaryCommandBuffer(QUEUE_TYPE type)
	{
		return Renderer::GetInstance().AllocatePrimaryCommandBuffer(type);
	}

	SecondaryCommandBuffer AllocateSecondaryCommandBuffer(QUEUE_TYPE type)
	{
		return Renderer::GetInstance().AllocateSecondaryCommandBuffer(type);
	}

	bool CreateSemaphore(VkSemaphore* semaphore, const VkSemaphoreCreateInfo& info)
	{
		return Renderer::GetInstance().CreateSemaphore(semaphore, info);
	}

	void DestroySemaphore(VkSemaphore* semaphore)
	{
		Renderer::GetInstance().DestroySemaphore(semaphore);
	}

	bool CreateFence(VkFence* fence, const VkFenceCreateInfo& info)
	{
		return Renderer::GetInstance().CreateFence(fence, info);
	}

	void DestroyFence(VkFence* fence)
	{
		Renderer::GetInstance().DestroyFence(fence);
	}

	VkFormat FindDepthFormat()
	{
		return Renderer::GetInstance().FindDepthFormat();
	}

	bool HasStencilComponent(VkFormat format)
	{
		return Renderer::GetInstance().HasStencilComponent(format);
	}

	void QueueCommandBuffer(const PrimaryCommandBuffer& cmdBuffer, const QueueSubmitInfo& info)
	{
		Renderer::GetInstance().QueueCommandBuffer(cmdBuffer, info);
	}

	void Submit()
	{
		//Renderer::GetInstance().Submit();
	}

	void WaitForFence(VkFence fence, uint64_t timeout)
	{
		Renderer::GetInstance().WaitForFence(fence, timeout);
	}

	uint32_t GetCurrentFrameIndex()
	{
		return Renderer::GetInstance().GetCurrentFrameIndex();
	}

	VkSemaphore GetCurrentImageAvailableSemaphore()
	{
		return Renderer::GetInstance().GetCurrentImageAvailableSemaphore();
	}

	VkSemaphore GetCurrentRenderFinishedSemaphore()
	{
		return Renderer::GetInstance().GetCurrentRenderFinishedSemaphore();
	}

	VkFence GetCurrentFrameFence()
	{
		return Renderer::GetInstance().GetCurrentFrameFence();
	}

	Framebuffer* GetCurrentSwapChainFramebuffer()
	{
		return Renderer::GetInstance().GetCurrentSwapChainFramebuffer();
	}
	// TEMP TEMP TEMP

	void BeginFrame()
	{
		Renderer::GetInstance().BeginFrame();
	}

	void Draw()
	{
		Renderer::GetInstance().Draw();
	}

	void EndFrame()
	{
		Renderer::GetInstance().EndFrame();
	}

	void Shutdown()
	{
		LoaderUtils::UnloadAll();
		Renderer::GetInstance().Shutdown();
		MainWindow::Get().Destroy();
		InputManager::GetInstance().Shutdown();
	}

	///////////////////////////////////////////////////////////
	//
	//		STATE
	// 
	///////////////////////////////////////////////////////////
	bool WindowShouldClose()
	{
		return MainWindow::Get().ShouldClose();
	}

	bool WindowInFocus()
	{
		return MainWindow::Get().IsInFocus();
	}

	void SetWindowTitle(const char* format, ...)
	{
		char buffer[100];
		va_list va;
		va_start(va, format);
		vsprintf_s(buffer, format, va);
		va_end(va);

		MainWindow::Get().SetWindowTitle(buffer);
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

		// TODO - Find a better way to determine which pipeline type to use
		CorePipeline corePipeline = GetCorePipelineFromFilePath(std::string(filepath));

		bool succeeded = AssetManager::Get().CreateAssetResources(asset, corePipeline);
		if (!succeeded)
		{
			LogError("Failed to create asset resources for asset '%s'", filepath);
			return INVALID_UUID;
		}

		return asset->uuid;
	}

	AssetResources* GetAssetResources(UUID uuid)
	{
		return AssetManager::Get().GetAssetResourcesFromUUID(uuid);
	}

	///////////////////////////////////////////////////////////
	//
	//		UPDATE
	// 
	///////////////////////////////////////////////////////////
	//void ShowAsset(UUID uuid)
	//{
	//	Renderer::GetInstance().SetAssetDrawState(uuid);
	//}

	//void UpdateAssetTransform(UUID uuid, float* position, float* rotation, float* scale)
	//{
	//	TNG_ASSERT_MSG(position != nullptr, "Position cannot be null!");
	//	TNG_ASSERT_MSG(rotation != nullptr, "Rotation cannot be null!");
	//	TNG_ASSERT_MSG(scale != nullptr, "Scale cannot be null!");

	//	Transform transform(
	//		*(reinterpret_cast<glm::vec3*>(position)),
	//		*(reinterpret_cast<glm::vec3*>(rotation)),
	//		*(reinterpret_cast<glm::vec3*>(scale)));
	//	Renderer::GetInstance().SetAssetTransform(uuid, transform);
	//}

	//void UpdateAssetPosition(UUID uuid, float* position)
	//{
	//	TNG_ASSERT_MSG(position != nullptr, "Position cannot be null!");
	//	Renderer::GetInstance().SetAssetPosition(uuid, *(reinterpret_cast<glm::vec3*>(position)));
	//}

	//void UpdateAssetRotation(UUID uuid, float* rotation, bool isDegrees)
	//{
	//	TNG_ASSERT_MSG(rotation != nullptr, "Rotation cannot be null!");
	//	glm::vec3 rotVector = *(reinterpret_cast<glm::vec3*>(rotation));

	//	// If the rotation was given in degrees, we must convert it to radians
	//	// since that's what glm uses
	//	if (isDegrees)
	//	{
	//		rotVector = glm::radians(rotVector);
	//	}

	//	Renderer::GetInstance().SetAssetRotation(uuid, rotVector);
	//}

	//void UpdateAssetScale(UUID uuid, float* scale)
	//{
	//	TNG_ASSERT_MSG(scale != nullptr, "Scale cannot be null!");
	//	Renderer::GetInstance().SetAssetScale(uuid, *(reinterpret_cast<glm::vec3*>(scale)));
	//}

	bool IsKeyPressed(int key)
	{
		return InputManager::GetInstance().IsKeyPressed(key);
	}

	bool IsKeyReleased(int key)
	{
		return InputManager::GetInstance().IsKeyReleased(key);
	}

	InputState GetKeyState(int key)
	{
		return InputManager::GetInstance().GetKeyState(key);
	}
}