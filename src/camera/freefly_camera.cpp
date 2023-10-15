
#define GLM_FORCE_RADIANS
#include <gtc/matrix_transform.hpp>

#include <functional>

#include "freefly_camera.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "../input_manager.h"

namespace TANG
{

	FreeflyCamera::FreeflyCamera() : BaseCamera(), speed(5.0f), sensitivity(5.0f)
	{
		// Nothing to do here
	}

	FreeflyCamera::~FreeflyCamera()
	{
		// Nothing to do here
	}

	FreeflyCamera::FreeflyCamera(const FreeflyCamera& other) : BaseCamera(other), 
		speed(other.speed), sensitivity(other.sensitivity)
	{
		// Nothing to do here
	}

	FreeflyCamera::FreeflyCamera(FreeflyCamera&& other) noexcept : BaseCamera(std::move(other)), 
		speed(std::move(other.speed)), sensitivity(std::move(other.sensitivity))
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
		rotation = _focus;

		RegisterKeyCallbacks();
		RegisterMouseCallbacks();
	}

	void FreeflyCamera::Update(float deltaTime)
	{
		// Normalize the displacement to prevent moving faster when moving diagonally. We should
		// only normalize it if the magnitude is not zero though...
		if (glm::length(displacement) != 0.0f)
		{
			displacement = glm::normalize(displacement);
		}

		// Reset the view matrix and re-calculate to avoid accumulating rotational errors
		matrix = glm::identity<glm::mat4>();

		// Rotate the camera
		glm::mat4 xRotationMatrix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(-rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 yRotationMatrix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(-rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
		matrix = yRotationMatrix * matrix * xRotationMatrix;

		// TODO - Prevent camera roll on rotation

		// Translate it in local coordinates
		matrix[3] = glm::vec4(position, 1.0f);
		glm::mat4 newTranslation = glm::translate(glm::identity<glm::mat4>(), displacement * deltaTime * speed);
		matrix = matrix * newTranslation;

		// Store the new camera position and wipe the displacement
		position = glm::vec3(matrix[3]);
		displacement = { 0.0f, 0.0f, 0.0f };
	}

	void FreeflyCamera::Shutdown()
	{
		DeregisterMouseCallbacks();
		DeregisterKeyCallbacks();
	}

	void FreeflyCamera::SetSpeed(float _speed)
	{
		speed = _speed;
	}

	float FreeflyCamera::GetSpeed() const
	{
		return speed;
	}

	void FreeflyCamera::SetSensitivity(float _sensitivity)
	{
		if (_sensitivity <= 0.0)
		{
			return;
		}

		sensitivity = _sensitivity;
	}
	float FreeflyCamera::GetSensitivity() const
	{
		return sensitivity;
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
			displacement.y++;
		}
	}

	void FreeflyCamera::MoveDown(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			displacement.y--;
		}
	}

	void FreeflyCamera::MoveLeft(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			displacement.x--;
		}
	}

	void FreeflyCamera::MoveRight(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			displacement.x++;
		}
	}

	void FreeflyCamera::MoveForward(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			displacement.z--;
		}
	}

	void FreeflyCamera::MoveBackward(KeyState state)
	{
		if (state == KeyState::PRESSED || state == KeyState::HELD)
		{
			displacement.z++;
		}
	}

	void FreeflyCamera::RotateCamera(double xDelta, double yDelta)
	{
		// Get the number of degrees we're going to rotate by this frame (in degrees)
		// We multiply the delta mouse coordinates with the camera's sensitivity, as well
		// as divide by a magic number to accomodate the sensitivity range between 1 and 10. For example,
		// a sensitivity of 5 should feel average, sensitivity of 1 should feel really slow and
		// 10 should feel really fast
		rotation.x += static_cast<float>(xDelta) * sensitivity / 50.0f;
		rotation.y += static_cast<float>(yDelta) * sensitivity / 50.0f;
	}

}