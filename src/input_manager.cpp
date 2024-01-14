
#include "input_manager.h"
#include "glfw/glfw3.h"
#include "utils/key_mappings.h"
#include "utils/mouse_mappings.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

static const std::unordered_map<int, TANG::InputState> InputStateMappings =
{
	{ -1          , TANG::InputState::INVALID  },
	{ GLFW_RELEASE, TANG::InputState::RELEASED },
	{ GLFW_PRESS  , TANG::InputState::PRESSED  },
	{ GLFW_REPEAT , TANG::InputState::HELD     },
};

namespace TANG
{
	// Defines a default number of callbacks per key/mouse type, which we pre-allocate. If we exceed this number it's not
	// the end of the world, we'll just have to resize the vector to fit the extra callback. In that case it might be
	// worth it to expand the default value
	static constexpr uint32_t MAX_CALLBACK_COUNT = 15;

	static void GLFW_KEY_CALLBACK(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		UNUSED(window);
		UNUSED(scancode);
		UNUSED(mods);

		// Attempt to find the key in the key mappings, to go from GLFW key to TANG::KeyType
		auto keyTypeMappingsIter = KeyTypeMappings.find(key);
		if (keyTypeMappingsIter == KeyTypeMappings.end())
		{
			LogError("Failed to map key type %i for key callback!", key);
			return;
		}
		
		auto keyStateMappingsIter = InputStateMappings.find(action);
		if (keyStateMappingsIter == InputStateMappings.end())
		{
			LogError("Failed to map input state %i for key callback!", action);
			return;
		}

		// Call the implementation
		InputManager::GetInstance().KeyCallbackEvent_Impl(keyTypeMappingsIter->second, keyStateMappingsIter->second);	
	}

	static void GLFW_MOUSE_MOVED_CALLBACK(GLFWwindow* window, double xPos, double yPos)
	{
		UNUSED(window);

		// Call the implementation
		InputManager::GetInstance().MouseCallbackEvent_Impl(xPos, yPos);
	}

	static void GLFW_MOUSE_BUTTON_CALLBACK(GLFWwindow* window, int button, int action, int mods)
	{
		UNUSED(window);
		UNUSED(mods);

		// Attempt to find the key in the key mappings, to go from GLFW mouse type to TANG::MouseType
		auto mouseTypeMappingsIter = MouseTypeMappings.find(button);
		if (mouseTypeMappingsIter == MouseTypeMappings.end())
		{
			LogError("Failed to map mouse type %i for mouse button callback!", button);
			return;
		}

		auto mouseStateMappingsIter = InputStateMappings.find(action);
		if (mouseStateMappingsIter == InputStateMappings.end())
		{
			LogError("Failed to map input state %i for mouse button callback!", action);
			return;
		}

		// Call the implementation
		InputManager::GetInstance().MouseButtonCallbackEvent_Impl(mouseTypeMappingsIter->second, mouseStateMappingsIter->second);
	}

	InputManager::InputManager() : windowHandle(nullptr), keyCallbacks(), isFirstMouseMovementAfterFocus(true)
	{
		// Initialize the states vector to have as many entries as there are key types
		keyStates.resize(static_cast<size_t>(KeyType::COUNT));
		mouseButtonStates.resize(static_cast<size_t>(MouseType::COUNT));
	}

	InputManager::~InputManager()
	{
		windowHandle = nullptr;
		keyCallbacks.clear();
		keyStates.clear();

		mouseButtonCallbacks.clear();
		mouseButtonStates.clear();
	}

	InputManager::InputManager(InputManager&& other) noexcept : keyCallbacks(std::move(other.keyCallbacks)), keyStates(std::move(other.keyStates)),
		mouseButtonCallbacks(std::move(other.mouseButtonCallbacks)), mouseButtonStates(std::move(other.mouseButtonStates))
	{
		windowHandle = other.windowHandle;
		isFirstMouseMovementAfterFocus = other.isFirstMouseMovementAfterFocus;

		other.windowHandle = nullptr;
		other.keyCallbacks.clear();
		other.keyStates.clear();

		other.mouseButtonCallbacks.clear();
		other.mouseButtonStates.clear();
	}

	void InputManager::Initialize(GLFWwindow* window)
	{
		if (windowHandle != nullptr)
		{
			LogError("Attempting to initialize InputManager multiple times!");
			return;
		}

		windowHandle = window;
		glfwSetKeyCallback(window, GLFW_KEY_CALLBACK);
		glfwSetCursorPosCallback(window, GLFW_MOUSE_MOVED_CALLBACK);
		glfwSetMouseButtonCallback(window, GLFW_MOUSE_BUTTON_CALLBACK);
	}

	void InputManager::Update()
	{
		glfwPollEvents();

		// Call the key callbacks for all keys
		for (uint32_t i = 0; i < keyStates.size(); i++)
		{
			InputState state = keyStates[i];
			KeyType type = (KeyType)(i);

			auto keyCallbackIter = keyCallbacks.find(type);
			if (keyCallbackIter == keyCallbacks.end())
			{
				continue;
			}

			for (auto callback : keyCallbackIter->second)
			{
				callback(state);
			}
		}

		// Call the mouse callbacks
		for (auto& callback : mouseMovedCallbacks)
		{
			double deltaX = currentMouseCoordinates.first - previousMouseCoordinates.first;
			double deltaY = currentMouseCoordinates.second - previousMouseCoordinates.second;

			callback(deltaX, deltaY);
		}

		// Call all the mouse button callbacks
		for (uint32_t i = 0; i < mouseButtonCallbacks.size(); i++)
		{
			InputState state = mouseButtonStates[i];
			MouseType type = (MouseType)(i);

			auto mouseButtonCallbackIter = mouseButtonCallbacks.find(type);
			if (mouseButtonCallbackIter == mouseButtonCallbacks.end())
			{
				continue;
			}

			for (auto callback : mouseButtonCallbackIter->second)
			{
				callback(state);
			}
		}

		// Update the previous mouse coordinates
		previousMouseCoordinates = currentMouseCoordinates;
	}

	void InputManager::Shutdown()
	{
		if (windowHandle == nullptr)
		{
			LogError("Attempted to destroy InputManager multiple times?");
		}

		windowHandle = nullptr;
		keyCallbacks.clear();
	}

	bool InputManager::IsKeyPressed(int key)
	{
		int state = glfwGetKey(windowHandle, key);
		return state == GLFW_PRESS;
	}

	bool InputManager::IsKeyReleased(int key)
	{
		int state = glfwGetKey(windowHandle, key);
		return state == GLFW_RELEASE;
	}

	InputState InputManager::GetKeyState(int key)
	{
		int state = glfwGetKey(windowHandle, key);
		switch (state)
		{
		case GLFW_PRESS:
			return InputState::PRESSED;
		case GLFW_RELEASE:
			return InputState::RELEASED;
		default:
			return InputState::INVALID;
		}

		// TODO - Implement a way to get the KeyState::HELD state
	}

	void InputManager::ResetMouseDeltaCache()
	{
		isFirstMouseMovementAfterFocus = true;
	}

	void InputManager::RegisterKeyCallback(KeyType type, KeyCallback callback)
	{
		// Try to get a reference to the callback vector for the given key
		auto keyIter = keyCallbacks.find(type);

		std::vector<KeyCallback>* callbackVec = nullptr;

		// If we can't find an existing vector, we must make one and insert the callback
		if (keyIter == keyCallbacks.end())
		{
			std::vector<KeyCallback> newCallbackVec;
			newCallbackVec.reserve(MAX_CALLBACK_COUNT);
			keyCallbacks.insert({ type, std::move(newCallbackVec) });
			callbackVec = &keyCallbacks.at(type);
		}
		else // push back the callback into the existing vector
		{
			callbackVec = &keyIter->second;
		}

		// Check that we have sensible defaults
		if (callbackVec->size() + 1 >= MAX_CALLBACK_COUNT)
		{
			LogWarning("Exceeding maximum callbacks per key, currently set to %u. Consider increasing it", MAX_CALLBACK_COUNT);
		}

		// Finally, push back the new callback
		callbackVec->push_back(callback);
	}

	void InputManager::DeregisterKeyCallback(KeyType type, KeyCallback callback)
	{
		auto keyIter = keyCallbacks.find(type);
		if (keyIter == keyCallbacks.end())
		{
			LogError("Failed to deregister key callback, the provided key type was not found!");
			return;
		}

		auto& callbackVec = keyIter->second;
		for (auto callbackIter = callbackVec.begin(); callbackIter < callbackVec.end(); callbackIter++)
		{
			// We're using the target for the equality check. This might not work in all cases
			if((*callbackIter).target<void(*)(InputState)>() == callback.target<void(*)(InputState)>())
			{
				callbackVec.erase(callbackIter);
				return;
			}
		}

		LogWarning("Failed to deregister key callback, the provided callback was not found!");
	}

	void InputManager::KeyCallbackEvent_Impl(KeyType type, InputState state)
	{
		// Store the key events
		// NOTE - We follow the way the OS handles key events, where when you first hold down a key it'll send a PRESSED
		//        event once, and after a second it will repeatedly send HELD events for that key
		keyStates[static_cast<uint32_t>(type)] = state;
	}

	void InputManager::RegisterMouseMovedCallback(MouseMovedCallback callback)
	{
		mouseMovedCallbacks.push_back(callback);
	}

	void InputManager::DeregisterMouseMovedCallback(MouseMovedCallback callback)
	{
		for (auto callbackIter = mouseMovedCallbacks.begin(); callbackIter < mouseMovedCallbacks.end(); callbackIter++)
		{
			// We're using the target for the equality check. This might not work in all cases
			if ((*callbackIter).target<void(*)(InputState)>() == callback.target<void(*)(InputState)>())
			{
				mouseMovedCallbacks.erase(callbackIter);
				return;
			}
		}

		LogWarning("Failed to deregister mouse moved callback, the provided callback was not found!");
	}

	void InputManager::RegisterMouseButtonCallback(MouseType type, MouseButtonCallback callback)
	{
		// Try to get a reference to the callback vector for the given mouse button
		auto mouseButtonIter = mouseButtonCallbacks.find(type);

		std::vector<KeyCallback>* callbackVec = nullptr;

		// If we can't find an existing vector, we must make one and insert the callback
		if (mouseButtonIter == mouseButtonCallbacks.end())
		{
			std::vector<MouseButtonCallback> newCallbackVec;
			newCallbackVec.reserve(MAX_CALLBACK_COUNT);
			mouseButtonCallbacks.insert({ type, std::move(newCallbackVec) });
			callbackVec = &mouseButtonCallbacks.at(type);
		}
		else // push back the callback into the existing vector
		{
			callbackVec = &mouseButtonIter->second;
		}

		// Check that we have sensible defaults
		if (callbackVec->size() + 1 >= MAX_CALLBACK_COUNT)
		{
			LogWarning("Exceeding maximum callbacks per mouse button, currently set to %u. Consider increasing it", MAX_CALLBACK_COUNT);
		}

		// Finally, push back the new callback
		callbackVec->push_back(callback);
	}

	void InputManager::DeregisterMouseButtonCallback(MouseType type, MouseButtonCallback callback)
	{
		auto mouseButtonIter = mouseButtonCallbacks.find(type);
		if (mouseButtonIter == mouseButtonCallbacks.end())
		{
			LogError("Failed to deregister mouse button callback, the provided mouse type was not found!");
			return;
		}

		auto& callbackVec = mouseButtonIter->second;
		for (auto callbackIter = callbackVec.begin(); callbackIter < callbackVec.end(); callbackIter++)
		{
			// We're using the target for the equality check. This might not work in all cases
			if ((*callbackIter).target<void(*)(InputState)>() == callback.target<void(*)(InputState)>())
			{
				callbackVec.erase(callbackIter);
				return;
			}
		}

		LogWarning("Failed to deregister mouse button callback, the provided callback was not found!");
	}

	void InputManager::MouseCallbackEvent_Impl(double xPosition, double yPosition)
	{
		currentMouseCoordinates.first = xPosition;
		currentMouseCoordinates.second = yPosition;

		// Prevent huge deltas when we receive the first mouse callback
		if (isFirstMouseMovementAfterFocus)
		{
			previousMouseCoordinates = currentMouseCoordinates;
			isFirstMouseMovementAfterFocus = false;
		}
	}

	void InputManager::MouseButtonCallbackEvent_Impl(MouseType type, InputState state)
	{
		mouseButtonStates[static_cast<size_t>(type)] = state;
	}
}