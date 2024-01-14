#ifndef TANG_MAIN_WINDOW_H
#define TANG_MAIN_WINDOW_H

struct GLFWwindow;

namespace TANG
{
	// Main window class, we do not support multiple windows
	class MainWindow
	{

	public:

		~MainWindow();
		MainWindow(const MainWindow& other);
		MainWindow(MainWindow&& other) noexcept;
		MainWindow& operator=(const MainWindow& other);

		static MainWindow& Get()
		{
			static MainWindow instance;
			return instance;
		}

		GLFWwindow* GetHandle() const;

		void Create(uint32_t width, uint32_t height);
		void Update(float deltaTime);
		void Destroy();

		bool ShouldClose() const;
		bool IsInFocus() const;

		// Returns true once the window is resized. Whenever true is returned, it MUST be handled appropriately
		// since it will not be returned more than once unless the window is constantly being resized
		bool WasWindowResized();

		// Returns the immediately available framebuffer width and height
		void GetFramebufferSize(uint32_t* outWidth, uint32_t* outHeight);

		// Blocks the calling thread if the window is minimized, and unblocks only until the window is maximized again.
		// The two out parameters are populated with the windows' size after it's maximized
		void BlockIfMinimized(uint32_t* outWidth, uint32_t* outHeight);

	private:

		MainWindow();
		void LMBCallback(InputState state);
		void ESCCallback(InputState state);

	private:

		GLFWwindow* glfwWinHandle;


	};
}

#endif