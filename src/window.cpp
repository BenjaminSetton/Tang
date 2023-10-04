
#include <utility>

#include "window.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

namespace TANG
{

	static void FramebufferResizeCallback(GLFWwindow* windowHandle, int windowWidth, int windowHeight)
	{
		UNUSED(windowWidth);
		UNUSED(windowHeight);

		auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(windowHandle));
		app->windowResized = true;

		// Block if the window is minimized
		uint32_t width, height;
		app->BlockIfMinimized(&width, &height);

		// Call the renderer's ResizeSwapChain callback
		app->swapChainCallback(width, height);
	}

	Window::Window() : glfwWinHandle(nullptr)
	{
		// Nothing to do here
	}

	Window::~Window()
	{
		if (glfwWinHandle != nullptr)
		{
			LogError("Window destrutor called but window has not been destroyed");
		}

		glfwWinHandle = nullptr;
	}

	Window::Window(const Window& other) : glfwWinHandle(other.glfwWinHandle)
	{
		// Nothing to do here
	}

	Window::Window(Window&& other) noexcept : glfwWinHandle(std::move(other.glfwWinHandle))
	{
		other.glfwWinHandle = nullptr;
	}

	Window& Window::operator=(const Window& other)
	{
		glfwWinHandle = other.glfwWinHandle;
	}

	GLFWwindow* Window::GetHandle() const
	{
		return glfwWinHandle;
	}

	void Window::Create(uint32_t width, uint32_t height)
	{
		if (glfwWinHandle != nullptr)
		{
			LogError("Attempted to initialize a window multiple times!");
			return;
		}

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		glfwWinHandle = glfwCreateWindow(width, height, "TANG", nullptr, nullptr);
		glfwSetWindowUserPointer(glfwWinHandle, this);
		glfwSetFramebufferSizeCallback(glfwWinHandle, FramebufferResizeCallback);
	}

	void Window::Update(float deltaTime)
	{
		UNUSED(deltaTime);

		glfwPollEvents();
	}

	void Window::Destroy()
	{
		if (glfwWinHandle == nullptr)
		{
			LogError("Attempted to destroy window when handle is invalid!");
			return;
		}

		glfwDestroyWindow(glfwWinHandle);
		glfwTerminate();

		glfwWinHandle = nullptr;
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(glfwWinHandle);
	}

	bool Window::WasWindowResized()
	{
		bool res = windowResized;
		windowResized = false;

		return res;
	}

	void Window::GetFramebufferSize(int* outWidth, int* outHeight) const
	{
		TNG_ASSERT_MSG(outWidth != nullptr, "Width parameter must not be nullptr!");
		TNG_ASSERT_MSG(outHeight != nullptr, "Height parameter must not be nullptr!");

		glfwGetFramebufferSize(glfwWinHandle, outWidth, outHeight);
	}

	void Window::BlockIfMinimized(uint32_t* outWidth, uint32_t* outHeight) const
	{
		TNG_ASSERT_MSG(outWidth != nullptr, "Width parameter must not be nullptr!");
		TNG_ASSERT_MSG(outHeight != nullptr, "Height parameter must not be nullptr!");

		int width, height;
		glfwGetFramebufferSize(glfwWinHandle, &width, &height);
		while(width == 0 || height == 0)
		{
			glfwGetFramebufferSize(glfwWinHandle, &width, &height);
			glfwWaitEvents();
		}

		*outWidth = width;
		*outHeight = height;
	}

	void Window::SetRecreateSwapChainCallback(RecreateSwapChainCallback callback)
	{
		swapChainCallback = callback;
	}
}

