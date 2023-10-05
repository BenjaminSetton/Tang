
#include "input.h"
#include "glfw3.h"
#include "utils/sanity_check.h"

namespace TANG
{

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
		windowHandle = window;
	}

	void InputManager::Update()
	{
		glfwPollEvents();
	}

	void InputManager::Shutdown()
	{
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

}