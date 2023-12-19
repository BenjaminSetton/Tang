
#include "input_manager.h"
#include "glfw/glfw3.h"
#include "utils/key_mappings.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

namespace TANG
{

	// Defines a default number of callbacks per key type, which we pre-allocate. If we exceed this number it's not
	// the end of the world, we'll just have to resize the vector to fit the extra callback. In that case it might be
	// worth it to expand the default value
	static constexpr uint32_t CALLBACKS_PER_KEY = 15;

	static void GLFW_KEY_CALLBACK(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		UNUSED(window);
		UNUSED(scancode);
		UNUSED(mods);

		// Attempt to find the key in the key mappings, to go from GLFW key to TANG::KeyType
		auto keyTypeMappingsIter = KeyTypeMappings.find(key);
		if (keyTypeMappingsIter == KeyTypeMappings.end())
		{
			LogError("Failed to map key type %i in key callback!", key);
			return;
		}
		
		auto keyStateMappingsIter = KeyStateMappings.find(action);
		if (keyStateMappingsIter == KeyStateMappings.end())
		{
			LogError("Failed to map key state %i in key callback!", action);
			return;
		}

		// Call the implementation
		InputManager::GetInstance().KeyCallbackEvent_Impl(keyTypeMappingsIter->second, keyStateMappingsIter->second);	
	}

	static void GLFW_MOUSE_CALLBACK(GLFWwindow* window, double xPos, double yPos)
	{
		UNUSED(window);

		// Call the implementation
		InputManager::GetInstance().MouseCallbackEvent_Impl(xPos, yPos);
	}

	InputManager::InputManager() : windowHandle(nullptr), keyCallbacks(), isFirstMouseMovement(true)
	{
		// Initialize the key states vector to have as many entries as there are key types
		keyStates.resize(static_cast<size_t>(KeyType::COUNT));
	}

	InputManager::~InputManager()
	{
		windowHandle = nullptr;
		keyCallbacks.clear();
		keyStates.clear();
	}

	InputManager::InputManager(InputManager&& other) noexcept : keyCallbacks(std::move(other.keyCallbacks)), keyStates(std::move(other.keyStates))
	{
		windowHandle = other.windowHandle;
		isFirstMouseMovement = other.isFirstMouseMovement;

		other.windowHandle = nullptr;
		other.keyCallbacks.clear();
		other.keyStates.clear();
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
		glfwSetCursorPosCallback(window, GLFW_MOUSE_CALLBACK);
	}

	void InputManager::Update()
	{
		glfwPollEvents();

		// Call the key callbacks for all keys
		for (uint32_t i = 0; i < keyStates.size(); i++)
		{
			KeyState state = keyStates[i];
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
		for (auto& callback : mouseCallbacks)
		{
			double deltaX = currentMouseCoordinates.first - previousMouseCoordinates.first;
			double deltaY = currentMouseCoordinates.second - previousMouseCoordinates.second;

			callback(deltaX, deltaY);
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

	KeyState InputManager::GetKeyState(int key)
	{
		int state = glfwGetKey(windowHandle, key);
		switch (state)
		{
		case GLFW_PRESS:
			return KeyState::PRESSED;
		case GLFW_RELEASE:
			return KeyState::RELEASED;
		default:
			return KeyState::INVALID;
		}

		// TODO - Implement a way to get the KeyState::HELD state
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
			newCallbackVec.reserve(CALLBACKS_PER_KEY);
			keyCallbacks.insert({ type, std::move(newCallbackVec) });
			callbackVec = &keyCallbacks.at(type);
		}
		else // push back the callback into the existing vector
		{
			callbackVec = &keyIter->second;
		}

		// Check that we have sensible defaults
		if (callbackVec->size() + 1 >= CALLBACKS_PER_KEY)
		{
			LogWarning("Exceeding maximum callbacks per key, currently set to %u. Consider increasing it", CALLBACKS_PER_KEY);
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
			if((*callbackIter).target<void(*)(KeyState)>() == callback.target<void(*)(KeyState)>())
			{
				callbackVec.erase(callbackIter);
				return;
			}
		}

		LogWarning("Failed to deregister key callback, the provided callback was not found!");
	}

	void InputManager::KeyCallbackEvent_Impl(KeyType type, KeyState state)
	{
		// Store the key events
		// NOTE - We follow the way the OS handles key events, where when you first hold down a key it'll send a PRESSED
		//        event once, and after a second it will repeatedly send HELD events for that key
		keyStates[static_cast<uint32_t>(type)] = state;
	}

	void InputManager::RegisterMouseCallback(MouseCallback callback)
	{
		mouseCallbacks.push_back(callback);
	}

	void InputManager::DeregisterMouseCallback(MouseCallback callback)
	{
		for (auto callbackIter = mouseCallbacks.begin(); callbackIter < mouseCallbacks.end(); callbackIter++)
		{
			// We're using the target for the equality check. This might not work in all cases
			if ((*callbackIter).target<void(*)(KeyState)>() == callback.target<void(*)(KeyState)>())
			{
				mouseCallbacks.erase(callbackIter);
				return;
			}
		}

		LogWarning("Failed to deregister key callback, the provided callback was not found!");
	}

	void InputManager::MouseCallbackEvent_Impl(double xPosition, double yPosition)
	{
		currentMouseCoordinates.first = xPosition;
		currentMouseCoordinates.second = yPosition;

		// Prevent huge deltas when we receive the first mouse callback
		if (isFirstMouseMovement)
		{
			previousMouseCoordinates = currentMouseCoordinates;
			isFirstMouseMovement = false;
		}
	}

}