
#include <cstdarg>

#include "tang.h"

#include "renderer.h"
#include "main_window.h"
#include "utils/sanity_check.h"

namespace TANG
{
	// The core state of the TANG API
	struct CoreState
	{
		RenderBackend selectedBackend;
	} g_coreState;

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

	void SetRenderBackend(RenderBackend backend)
	{
		if (g_coreState.selectedBackend != backend)
		{
			// TODO - Delete and re-init stuff to clean up the old backend?
			g_coreState.selectedBackend = backend;
		}
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

		renderer.Update(deltaTime);
	}

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

	void RegisterSwapChainRecreatedCallback(SwapChainRecreatedCallback callback)
	{
		Renderer::GetInstance().RegisterSwapChainRecreatedCallback(callback);
	}

	void RegisterRendererShutdownCallback(RendererShutdownCallback callback)
	{
		Renderer::GetInstance().RegisterRendererShutdownCallback(callback);
	}

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

	///////////////////////////////////////////////////////////
	//
	//		UPDATE
	// 
	///////////////////////////////////////////////////////////

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