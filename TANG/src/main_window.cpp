
#define VK_USE_PLATFORM_WIN32_KHR
#include <glfw/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>

#include <utility>

#include "input_manager.h"
#include "main_window.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

static struct
{
	bool resized = false;
	bool inFocus = true;
} WindowData;

namespace TANG
{
	static void FramebufferResizeCallback(GLFWwindow* windowHandle, int windowWidth, int windowHeight)
	{
		UNUSED(windowWidth);
		UNUSED(windowHeight);

		WindowData.resized = true;

		auto app = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(windowHandle));
		// Block if the window is minimized
		uint32_t width, height;
		app->BlockIfMinimized(&width, &height);
	}

	static void WindowFocusedCallback(GLFWwindow* windowHandle, int focused)
	{
		UNUSED(windowHandle);

		WindowData.inFocus = focused;
	}

	void MainWindow::LMBCallback(InputState state)
	{
		if (state != InputState::PRESSED)
		{
			return;
		}

		// Set cursor to DISABLED when we click on the window
		GLFWwindow* windowHandle = MainWindow::Get().GetHandle();
		if (glfwGetInputMode(windowHandle, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
		{
			glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}

		// Focus the window
		WindowData.inFocus = true;
	}

	void MainWindow::ESCCallback(InputState state)
	{
		if (state != InputState::PRESSED)
		{
			return;
		}

		// Set cursor to NORMAL once we hit the ESC key
		GLFWwindow* windowHandle = MainWindow::Get().GetHandle();
		if (glfwGetInputMode(windowHandle, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
		{
			glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		// Un-focus the window
		WindowData.inFocus = false;
	}

	MainWindow::MainWindow() : glfwWinHandle(nullptr)
	{
		// Nothing to do here
	}

	MainWindow::~MainWindow()
	{
		if (glfwWinHandle != nullptr)
		{
			LogError("Window destructor called but window has not been destroyed");
		}

		glfwWinHandle = nullptr;
	}

	MainWindow::MainWindow(const MainWindow& other) : glfwWinHandle(other.glfwWinHandle)
	{
		// Nothing to do here
	}

	MainWindow::MainWindow(MainWindow&& other) noexcept : glfwWinHandle(std::move(other.glfwWinHandle))
	{
		other.glfwWinHandle = nullptr;
	}

	MainWindow& MainWindow::operator=(const MainWindow& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		glfwWinHandle = other.glfwWinHandle;

		return *this;
	}

	GLFWwindow* MainWindow::GetHandle() const
	{
		return glfwWinHandle;
	}

	void MainWindow::Create(uint32_t width, uint32_t height, const char* windowTitle)
	{
		if (glfwWinHandle != nullptr)
		{
			LogError("Attempted to initialize a window multiple times!");
			return;
		}

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		glfwWinHandle = glfwCreateWindow(width, height, windowTitle, nullptr, nullptr);
		glfwSetWindowUserPointer(glfwWinHandle, this);
		glfwSetInputMode(glfwWinHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetFramebufferSizeCallback(glfwWinHandle, FramebufferResizeCallback);
		glfwSetWindowFocusCallback(glfwWinHandle, WindowFocusedCallback);

		// Use raw mouse motion if available, meaning we won't consider any acceleration or other features that
		// are applied to a desktop mouse to make it "feel" better. Raw mouse motion is better for controlling 3D cameras
		if (glfwRawMouseMotionSupported())
		{
			glfwSetInputMode(glfwWinHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
		else
		{
			// Let's print a message saying raw mouse motion is not supported
			LogInfo("Raw mouse motion is not supported!");
		}

		REGISTER_MOUSE_BUTTON_CALLBACK(MouseType::MOUSE_LMB, MainWindow::LMBCallback);
		REGISTER_KEY_CALLBACK(KeyType::KEY_ESC, MainWindow::ESCCallback);
	}

	void MainWindow::Update(float deltaTime)
	{
		UNUSED(deltaTime);

		glfwPollEvents();
	}

	void MainWindow::Destroy()
	{
		if (glfwWinHandle == nullptr)
		{
			LogError("Attempted to destroy window when handle is invalid!");
			return;
		}

		glfwDestroyWindow(glfwWinHandle);
		glfwTerminate();

		glfwWinHandle = nullptr;

		DEREGISTER_MOUSE_BUTTON_CALLBACK(MouseType::MOUSE_LMB, MainWindow::LMBCallback);
		DEREGISTER_KEY_CALLBACK(KeyType::KEY_ESC, MainWindow::ESCCallback);
	}

	bool MainWindow::ShouldClose() const
	{
		return glfwWindowShouldClose(glfwWinHandle);
	}

	bool MainWindow::IsInFocus() const
	{
		return WindowData.inFocus;
	}

	void MainWindow::SetWindowTitle(const char* windowTitle)
	{
		glfwSetWindowTitle(glfwWinHandle, windowTitle);
	}

	void GetWindowSize(uint32_t& outWidth, uint32_t& outHeight)
	{
		MainWindow::Get().GetFramebufferSize(&outWidth, &outHeight);
	}

	bool MainWindow::WasWindowResized()
	{
		bool res = WindowData.resized;
		WindowData.resized = false; // Reset the cached value upon reading from it

		return res;
	}

	void MainWindow::GetFramebufferSize(uint32_t* outWidth, uint32_t* outHeight)
	{
		TNG_ASSERT_MSG(outWidth != nullptr, "Width parameter must not be nullptr!");
		TNG_ASSERT_MSG(outHeight != nullptr, "Height parameter must not be nullptr!");

		int width, height;
		glfwGetFramebufferSize(glfwWinHandle, &width, &height);
		
		*outWidth = static_cast<uint32_t>(width);
		*outHeight = static_cast<uint32_t>(height);
	}

	void MainWindow::BlockIfMinimized(uint32_t* outWidth, uint32_t* outHeight)
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

		*outWidth = static_cast<uint32_t>(width);
		*outHeight = static_cast<uint32_t>(height);
	}
}

