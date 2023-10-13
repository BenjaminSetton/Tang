
#include <functional>

#include "freefly_camera.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "../input_manager.h"

namespace TANG
{

	FreeflyCamera::FreeflyCamera() : BaseCamera(), velocity(1.0f)
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

	void FreeflyCamera::Initialize(const glm::vec3& _position, const glm::vec3& _focus)
	{
		position = _position;
		focus = _focus;

		RegisterKeyCallbacks();
		RegisterMouseCallbacks();
	}

	void FreeflyCamera::Update(float deltaTime)
	{
		// Cache this frame's deltaTime so that we can use it next frame to dampen the camera's velocity
		dtCache = deltaTime;
	}

	void FreeflyCamera::Shutdown()
	{
		DeregisterMouseCallbacks();
		DeregisterKeyCallbacks();
	}

	void FreeflyCamera::SetVelocity(float _velocity)
	{
		velocity = _velocity;
	}

	float FreeflyCamera::GetVelocity() const
	{
		return velocity;
	}

	void FreeflyCamera::RegisterKeyCallbacks()
	{
		REGISTER_KEY_CALLBACK(KeyType::KEY_SPACEBAR , FreeflyCamera::MoveUp);
		REGISTER_KEY_CALLBACK(KeyType::KEY_RSHIFT   , FreeflyCamera::MoveDown);
		REGISTER_KEY_CALLBACK(KeyType::KEY_K        , FreeflyCamera::MoveLeft);
		REGISTER_KEY_CALLBACK(KeyType::KEY_SEMICOLON, FreeflyCamera::MoveRight);
		REGISTER_KEY_CALLBACK(KeyType::KEY_O        , FreeflyCamera::MoveForward);
		REGISTER_KEY_CALLBACK(KeyType::KEY_L        , FreeflyCamera::MoveBackward);
	}

	void FreeflyCamera::DeregisterKeyCallbacks()
	{
		DEREGISTER_KEY_CALLBACK(KeyType::KEY_SPACEBAR , FreeflyCamera::MoveUp);
		DEREGISTER_KEY_CALLBACK(KeyType::KEY_RSHIFT   , FreeflyCamera::MoveDown);
		DEREGISTER_KEY_CALLBACK(KeyType::KEY_K        , FreeflyCamera::MoveLeft);
		DEREGISTER_KEY_CALLBACK(KeyType::KEY_SEMICOLON, FreeflyCamera::MoveRight);
		DEREGISTER_KEY_CALLBACK(KeyType::KEY_O        , FreeflyCamera::MoveForward);
		DEREGISTER_KEY_CALLBACK(KeyType::KEY_L        , FreeflyCamera::MoveBackward);
	}

	void FreeflyCamera::RegisterMouseCallbacks()
	{
		REGISTER_MOUSE_CALLBACK(FreeflyCamera::RotateCamera);
	}

	void FreeflyCamera::DeregisterMouseCallbacks()
	{
		DEREGISTER_MOUSE_CALLBACK(FreeflyCamera::RotateCamera);
	}

	void FreeflyCamera::MoveUp(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.y += velocity * dtCache;
		}
	}

	void FreeflyCamera::MoveDown(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.y -= velocity * dtCache;
		}
	}

	void FreeflyCamera::MoveLeft(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.x -= velocity * dtCache;
		}
	}

	void FreeflyCamera::MoveRight(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.x += velocity * dtCache;
		}
	}

	void FreeflyCamera::MoveForward(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.z -= velocity * dtCache;
		}
	}

	void FreeflyCamera::MoveBackward(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			position.z += velocity * dtCache;
		}
	}

	void FreeflyCamera::RotateCamera(double xPosition, double yPosition)
	{
		UNUSED(xPosition);
		UNUSED(yPosition);
		TNG_TODO();

		//LogInfo("Mouse position: (%f, %f)", xPosition, yPosition);
	}

}