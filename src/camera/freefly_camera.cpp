
#define GLM_FORCE_RADIANS
#include <gtc/matrix_transform.hpp>

// DISABLE WARNING FROM "type_quat.hpp": nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)

#include <gtc/quaternion.hpp>

#pragma warning(pop)

#include <functional>

#include "freefly_camera.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "../input_manager.h"

namespace TANG
{

	FreeflyCamera::FreeflyCamera() : BaseCamera(), speed(5.0f), sensitivity(5.0f), 
		position(glm::vec3(0.0f)), rotation(glm::vec3(0.0f)), displacement(glm::vec3(0.0f))
	{
		// Nothing to do here
	}

	FreeflyCamera::~FreeflyCamera()
	{
		// Nothing to do here
	}

	FreeflyCamera::FreeflyCamera(const FreeflyCamera& other) : BaseCamera(other), 
		speed(other.speed), sensitivity(other.sensitivity), position(other.position), 
		rotation(other.rotation)
	{
		// NOTE - Don't copy the displacement, since that's wiped every frame anyway
	}

	FreeflyCamera::FreeflyCamera(FreeflyCamera&& other) noexcept : BaseCamera(std::move(other)), 
		speed(std::move(other.speed)), sensitivity(std::move(other.sensitivity)), position(std::move(other.position)),
		rotation(std::move(other.rotation))
	{
		// NOTE - Don't copy the displacement, since that's wiped every frame anyway

		// Deregister the callbacks from the other camera object, since it will become invalid
		other.DeregisterKeyCallbacks();
		other.DeregisterMouseCallbacks();
	}

	FreeflyCamera& FreeflyCamera::operator=(const FreeflyCamera& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		BaseCamera::operator=(other);
		speed = other.speed;
		sensitivity = other.sensitivity;
		position = other.position;
		rotation = other.rotation;

		return *this;
	}

	void FreeflyCamera::Initialize(const glm::vec3& _position, const glm::vec3& _rotationDegrees)
	{
		position = _position;
		rotation = _rotationDegrees;

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

		// Start off with a fresh view matrix to avoid accumulating rotational errors
		glm::mat4 newMatrix = glm::identity<glm::mat4>();

		// Clamp the pitch
		constexpr float minPitch = -90.0f;
		constexpr float maxPitch = 90.0f;
		rotation.y = glm::clamp(rotation.y, minPitch, maxPitch);

		// Mod the yaw to wrap around to 0 every 360 degrees
		float yawFractional = rotation.x - static_cast<int>(rotation.x);
		rotation.x = static_cast<float>((static_cast<int>(rotation.x) % 360)) + yawFractional;

		// Rotate the camera
		glm::quat cameraOrientation = glm::quat(glm::vec3(glm::radians(-rotation.y), glm::radians(-rotation.x), 0.0f));
		glm::mat4 rotationMatrix = glm::mat4_cast(cameraOrientation);
		newMatrix = rotationMatrix * newMatrix;

		// Translate it in local coordinates, except for up/down which translate in global coordinates
		glm::vec3 adjustedDisplacement = displacement * deltaTime * speed;
		float verticalDisplacement = adjustedDisplacement.y;
		adjustedDisplacement.y = 0;

		newMatrix[3] = glm::vec4(position, 1.0f);
		glm::mat4 newTranslation = glm::translate(glm::identity<glm::mat4>(), adjustedDisplacement);

		newMatrix = newMatrix * newTranslation;
		newMatrix[3].y += verticalDisplacement;

		// Store the new camera position and wipe the displacement
		position = glm::vec3(newMatrix[3]);
		displacement = { 0.0f, 0.0f, 0.0f };

		// Set the new matrix
		matrix = newMatrix;
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