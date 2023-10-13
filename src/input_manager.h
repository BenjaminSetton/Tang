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
#define REGISTER_KEY_CALLBACK(keyType, funcPtr) InputManager::GetInstance().RegisterKeyCallback(keyType, std::bind(&funcPtr, this, std::placeholders::_1));
#define DEREGISTER_KEY_CALLBACK(keyType, funcPtr) InputManager::GetInstance().DeregisterKeyCallback(keyType, std::bind(&funcPtr, this, std::placeholders::_1));

		using MouseCallback = std::function<void(double, double)>;
#define REGISTER_MOUSE_CALLBACK(funcPtr) InputManager::GetInstance().RegisterMouseCallback(std::bind(&funcPtr, this, std::placeholders::_1, std::placeholders::_2));
#define DEREGISTER_MOUSE_CALLBACK(funcPtr) InputManager::GetInstance().DeregisterMouseCallback(std::bind(&funcPtr, this, std::placeholders::_1, std::placeholders::_2));


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

		void RegisterMouseCallback(MouseCallback callback);
		void DeregisterMouseCallback(MouseCallback callback);

		// Private callback implementations - DO NOT CALL
		void KeyCallbackEvent_Impl(KeyType type, KeyState state);
		void MouseCallbackEvent_Impl(double xPosition, double yPosition);

	private:

		InputManager();

		// Stores a list of callbacks for different KeyTypes. A class may register a callback for TANG::KEY_E, for example, so
		// whenever we receive an event from GLFW we go through the list of callbacks and call all the registered callbacks
		// for that particular key
		std::unordered_map<KeyType, std::vector<KeyCallback>> keyCallbacks;

		// Stores the state of all keys. This is used during the update loop to determine wheter we want to send events to
		// the callbacks or not, rather than relying on GLFW events directly for this purpose.
		std::vector<KeyState> keyStates;

		// Stores a list of all callbacks for mouse coordinates.
		std::vector<MouseCallback> mouseCallbacks;

		// Stores the latest mouse coordinates. The mouse coordinates callbacks are called every frame
		std::pair<double, double> currentMouseCoordinates;

		GLFWwindow* windowHandle;

	};

}

#endif