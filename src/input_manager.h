#ifndef TANG_INPUT_H
#define TANG_INPUT_H

#include <functional>
#include <unordered_map>
#include <vector>

#include "utils/key_declarations.h"

struct GLFWwindow;

namespace TANG
{
	class InputManager
	{
	public:

		using KeyCallback = std::function<void(KeyState)>;

		~InputManager();
		InputManager(const InputManager& other) = delete;
		InputManager(InputManager&& other) noexcept;
		InputManager& operator=(const InputManager& other) = delete;

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

		// Private callback implementations - DO NOT CALL
		void KeyCallbackEvent_Impl(KeyType type, KeyState state);

	private:

		InputManager();

		// Stores a list of callbacks for different KeyTypes. A class may register a callback for TANG::KEY_E, for example, so
		// whenever we receive an event from GLFW we go through the list of callbacks and call all the registered callbacks
		// for that particular key
		std::unordered_map<KeyType, std::vector<KeyCallback>> keyCallbacks;

		GLFWwindow* windowHandle;

	};

#define REGISTER_CALLBACK(keyType, funcPtr) InputManager::GetInstance().RegisterKeyCallback(keyType, std::bind(&funcPtr, this, std::placeholders::_1));
#define DEREGISTER_CALLBACK(keyType, funcPtr) InputManager::GetInstance().DeregisterKeyCallback(keyType, std::bind(&funcPtr, this, std::placeholders::_1));

}

#endif