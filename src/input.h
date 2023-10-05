#ifndef TANG_INPUT_H
#define TANG_INPUT_H

struct GLFWwindow;

namespace TANG
{
	enum class KeyState
	{
		INVALID = -1,
		PRESSED,
		HELD,
		RELEASED
	};

	class InputManager
	{
	public:

		InputManager();
		~InputManager();
		InputManager(const InputManager& other);
		InputManager(InputManager&& other) noexcept;
		InputManager& operator=(const InputManager& other);

		void Initialize(GLFWwindow* window);
		void Update();
		void Shutdown();

		bool IsKeyPressed(int key);

		// The IsKeyHeld function does not work like the other two here because GLFW_RELEASE
		// is only reported to the callback, so it can't be polled like GLFW_RELEASE or GLFW_PRESS can be
		//
		/*bool IsKeyHeld(int key);*/

		bool IsKeyReleased(int key);

		KeyState GetKeyState(int key);

	private:

		/*static void KeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
		{
		}*/

		GLFWwindow* windowHandle;

	};
}

#endif