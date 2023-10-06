
#include "input_manager.h"
#include "glfw3.h"
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
		UNUSED(key);
		UNUSED(scancode);
		UNUSED(action);
		UNUSED(mods);

		// Attempt to find the key in the key mappings, to go from GLFW key to TANG::KeyType
		auto keyMappingsIter = KeyMappings.find(key);
		if (keyMappingsIter == KeyMappings.end())
		{
			LogError("Received callback for key %i, but it's missing from the key mappings!", key);
			return;
		}
		
		// Kinda sketchy, but ok...
		//InputManager* inputManagerHandle = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

		TNG_TODO();
	}

	InputManager::InputManager() : windowHandle(nullptr)
	{
		// Nothing to do here yet
	}

	InputManager::~InputManager()
	{
		windowHandle = nullptr;
	}

	InputManager::InputManager(const InputManager& other) : windowHandle(other.windowHandle)
	{
		// Nothing to do here yet
	}

	InputManager::InputManager(InputManager&& other) noexcept
	{
		windowHandle = other.windowHandle;

		other.windowHandle = nullptr;
	}

	InputManager& InputManager::operator=(const InputManager& other)
	{
		if (this == &other)
		{
			return *this;
		}

		windowHandle = other.windowHandle;

		return *this;
	}

	void InputManager::Initialize(GLFWwindow* window)
	{
		if (windowHandle != nullptr)
		{
			LogError("Attempting to initialize InputManager multiple times!");
			return;
		}

		windowHandle = window;
	}

	void InputManager::Update()
	{
		glfwPollEvents();
	}

	void InputManager::Shutdown()
	{
		if (windowHandle == nullptr)
		{
			LogError("Attempted to destroy InputManager multiple times?");
		}

		windowHandle = nullptr;
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
			keyCallbacks.insert({type, std::vector<KeyCallback>(CALLBACKS_PER_KEY)});
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
			if(*callbackIter == callback)
			{
				callbackVec.erase(callbackIter);
				return;
			}
		}

		LogError("Failed to deregister key callback, the provided callback was not found!");
	}

}