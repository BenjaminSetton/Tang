
#include <functional>

#include "freefly_camera.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "../input_manager.h"

namespace TANG
{

	FreeflyCamera::FreeflyCamera() : BaseCamera(), velocity(5.0f)
	{
		// Nothing to do here
	}

	FreeflyCamera::~FreeflyCamera()
	{
		// Nothing to do here
	}

	FreeflyCamera::FreeflyCamera(const FreeflyCamera& other) : BaseCamera(other), velocity(other.velocity)
	{
		// Nothing to do here
	}

	FreeflyCamera::FreeflyCamera(FreeflyCamera&& other) noexcept : BaseCamera(std::move(other)), velocity(std::move(other.velocity))
	{
		// Nothing to do here
	}

	FreeflyCamera& FreeflyCamera::operator=(const FreeflyCamera& other)
	{
		UNUSED(other);
		TNG_TODO();

		return *this;
	}

	void FreeflyCamera::Initialize()
	{
		RegisterKeyCallbacks();
	}

	void FreeflyCamera::Update(float deltaTime)
	{
		UNUSED(deltaTime);

		if (shouldUpdateMatrix)
		{
			CalculateViewMatrix();
		}

		shouldUpdateMatrix = false;
	}

	void FreeflyCamera::Shutdown()
	{
		DeregisterKeyCallbacks();
	}

	void FreeflyCamera::RegisterKeyCallbacks()
	{
		REGISTER_CALLBACK(KeyType::KEY_SPACEBAR , FreeflyCamera::MoveUp);
		REGISTER_CALLBACK(KeyType::KEY_RSHIFT   , FreeflyCamera::MoveDown);
		REGISTER_CALLBACK(KeyType::KEY_K        , FreeflyCamera::MoveLeft);
		REGISTER_CALLBACK(KeyType::KEY_SEMICOLON, FreeflyCamera::MoveRight);
		REGISTER_CALLBACK(KeyType::KEY_O        , FreeflyCamera::MoveForward);
		REGISTER_CALLBACK(KeyType::KEY_L        , FreeflyCamera::MoveBackward);
	}

	void FreeflyCamera::DeregisterKeyCallbacks()
	{
		TNG_TODO();

		DEREGISTER_CALLBACK(KeyType::KEY_SPACEBAR, FreeflyCamera::MoveUp);
		DEREGISTER_CALLBACK(KeyType::KEY_RSHIFT, FreeflyCamera::MoveDown);
		DEREGISTER_CALLBACK(KeyType::KEY_K, FreeflyCamera::MoveLeft);
		DEREGISTER_CALLBACK(KeyType::KEY_SEMICOLON, FreeflyCamera::MoveRight);
		DEREGISTER_CALLBACK(KeyType::KEY_O, FreeflyCamera::MoveForward);
		DEREGISTER_CALLBACK(KeyType::KEY_L, FreeflyCamera::MoveBackward);
	}

	void FreeflyCamera::MoveUp(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.y += velocity;
		}
		LogInfo("Moved up");
		shouldUpdateMatrix = true;
	}

	void FreeflyCamera::MoveDown(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.y -= velocity;
		}
		LogInfo("Moved down");
		shouldUpdateMatrix = true;
	}

	void FreeflyCamera::MoveLeft(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.x -= velocity;
		}
		LogInfo("Moved left");
		shouldUpdateMatrix = true;
	}

	void FreeflyCamera::MoveRight(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.x += velocity;
		}
		LogInfo("Moved right");
		shouldUpdateMatrix = true;
	}

	void FreeflyCamera::MoveForward(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.z += velocity;
		}
		LogInfo("Moved forward");
		shouldUpdateMatrix = true;
	}

	void FreeflyCamera::MoveBackward(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.z -= velocity;
		}
		LogInfo("Moved backward");
		shouldUpdateMatrix = true;
	}

}