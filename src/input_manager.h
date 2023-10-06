#ifndef TANG_INPUT_H
#define TANG_INPUT_H

#include <unordered_map>
#include <vector>

#include "utils/key_types.h"

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

		typedef void (*KeyCallback)();

		InputManager();
		~InputManager();
		InputManager(const InputManager& other);
		InputManager(InputManager&& other) noexcept;
		InputManager& operator=(const InputManager& other);

		static InputManager& GetInstance()
		{
			static InputManager instance;
			return instance;
		}

		void Initialize(GLFWwindow* window);
		void Update();
		void Shutdown();

		bool IsKeyPressed(int key);
		bool IsKeyReleased(int key);
		KeyState GetKeyState(int key);

		void RegisterKeyCallback(KeyType type, KeyCallback callback);
		void DeregisterKeyCallback(KeyType type, KeyCallback callback);

	private:

		// Stores a list of callbacks for different KeyTypes. A class may register a callback for TANG::KEY_E, for example, so
		// whenever we receive an event from GLFW we go through the list of callbacks and call all the registered callbacks
		// for that particular key
		std::unordered_map<KeyType, std::vector<KeyCallback>> keyCallbacks;

		GLFWwindow* windowHandle;

	};
}

#endif