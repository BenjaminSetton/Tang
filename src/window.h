#ifndef WINDOW_H
#define WINDOW_H

#define VK_USE_PLATFORM_WIN32_KHR
#include <glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

namespace TANG
{
	// Singleton Window class, we do not support multiple windows
	class Window
	{
	public:

		using RecreateSwapChainCallback = void(*)(uint32_t, uint32_t);

		Window();
		~Window();
		Window(const Window& other);
		Window(Window&& other) noexcept;
		Window& operator=(const Window& other);

		GLFWwindow* GetHandle() const;

		void Create(uint32_t width, uint32_t height);
		void Update(float deltaTime);
		void Destroy();

		bool ShouldClose() const;

		// Returns true once the window is resized. Whenever true is returned, it MUST be handled appropriately
		// since it will not be returned more than once unless the window is constantly being resized
		bool WasWindowResized();

		// Returns the immediately available framebuffer width and height
		void GetFramebufferSize(int* outWidth, int* outHeight) const;

		// Blocks the calling thread if the window is minimized, and unblocks only until the window is maximized again.
		// The two out parameters are populated with the windows' size after it's maximized
		void BlockIfMinimized(uint32_t* outWidth, uint32_t* outHeight) const;

		// Sets the callback to recreate the renderer's swap chain upon receiving a window-resized event from GLFW
		void SetRecreateSwapChainCallback(RecreateSwapChainCallback callback);

		// Set to true only by the GLFW callback, set to false and read only by WasWindowResized
		bool windowResized;
		RecreateSwapChainCallback swapChainCallback;

	private:

		GLFWwindow* glfwWinHandle;


	};
}


#endif