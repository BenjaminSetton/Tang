#ifndef TANG_INPUT_H
#define TANG_INPUT_H

#include <functional>
#include <unordered_map>
#include <vector>

#include "utils/key_declarations.h"
#include "utils/mouse_declarations.h"

struct GLFWwindow;

namespace TANG
{
	enum class InputState
	{
		INVALID = -1,
		RELEASED,
		PRESSED,
		HELD,
	};

	class InputManager
	{
	public:

		////////////////////////////////////////////////////////////
		//
		//	CALLBACKS
		//
		////////////////////////////////////////////////////////////

		using KeyCallback = std::function<void(InputState)>;
#define REGISTER_KEY_CALLBACK(keyType, funcPtr) InputManager::GetInstance().RegisterKeyCallback(keyType, std::bind(&funcPtr, this, std::placeholders::_1));
#define DEREGISTER_KEY_CALLBACK(keyType, funcPtr) InputManager::GetInstance().DeregisterKeyCallback(keyType, std::bind(&funcPtr, this, std::placeholders::_1));

		using MouseMovedCallback = std::function<void(double, double)>;
#define REGISTER_MOUSE_MOVED_CALLBACK(funcPtr) InputManager::GetInstance().RegisterMouseMovedCallback(std::bind(&funcPtr, this, std::placeholders::_1, std::placeholders::_2));
#define DEREGISTER_MOUSE_MOVED_CALLBACK(funcPtr) InputManager::GetInstance().DeregisterMouseMovedCallback(std::bind(&funcPtr, this, std::placeholders::_1, std::placeholders::_2));

		using MouseButtonCallback = std::function<void(InputState)>;
#define REGISTER_MOUSE_BUTTON_CALLBACK(mouseType, funcPtr) InputManager::GetInstance().RegisterMouseButtonCallback(mouseType, std::bind(&funcPtr, this, std::placeholders::_1));
#define DEREGISTER_MOUSE_BUTTON_CALLBACK(mouseType, funcPtr) InputManager::GetInstance().DeregisterMouseButtonCallback(mouseType, std::bind(&funcPtr, this, std::placeholders::_1));


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
		InputState GetKeyState(int key);

		// Sets the 'isFirstMouseMovementAfterFocus' flag to prevent the mouse from snapping
		// after the window regains focus
		void ResetMouseDeltaCache();

		void RegisterKeyCallback(KeyType type, KeyCallback callback);
		void DeregisterKeyCallback(KeyType type, KeyCallback callback);

		void RegisterMouseMovedCallback(MouseMovedCallback callback);
		void DeregisterMouseMovedCallback(MouseMovedCallback callback);

		void RegisterMouseButtonCallback(MouseType type, MouseButtonCallback callback);
		void DeregisterMouseButtonCallback(MouseType type, MouseButtonCallback callback);

		// Private callback implementations - DO NOT CALL
		void KeyCallbackEvent_Impl(KeyType type, InputState state);
		void MouseCallbackEvent_Impl(double xPosition, double yPosition);
		void MouseButtonCallbackEvent_Impl(MouseType type, InputState state);

	private:

		InputManager();

		// Stores a list of callbacks for different KeyTypes. A class may register a callback for TANG::KEY_E, for example, so
		// whenever we receive an event from GLFW we go through the list of callbacks and call all the registered callbacks
		// for that particular key
		std::unordered_map<KeyType, std::vector<KeyCallback>> keyCallbacks;

		// Stores the state of all keys. This is used during the update loop to determine whether we want to send events to
		// the callbacks or not, rather than relying on GLFW events directly for this purpose.
		std::vector<InputState> keyStates;

		// Stores a list of all callbacks for mouse coordinates.
		std::vector<MouseMovedCallback> mouseMovedCallbacks;

		// Stores a list of callbacks for different MouseTypes
		std::unordered_map<MouseType, std::vector<MouseButtonCallback>> mouseButtonCallbacks;
		std::vector<InputState> mouseButtonStates;

		// Stores the previous and current mouse coordinates. The mouse coordinates callbacks are called every frame
		std::pair<double, double> previousMouseCoordinates;
		std::pair<double, double> currentMouseCoordinates;

		// Stores true as long as the mouse has NOT been moved. This is so we can prevent huge deltas when the mouse is initially moved
		bool isFirstMouseMovementAfterFocus;

		GLFWwindow* windowHandle;

	};

}

#endif