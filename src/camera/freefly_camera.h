#ifndef FREEFLY_CAMERA_H
#define FREEFLY_CAMERA_H

#include "base_camera.h"

#include "../utils/key_declarations.h"

namespace TANG
{
	class FreeflyCamera : public BaseCamera
	{
	public:

		FreeflyCamera();
		~FreeflyCamera();
		FreeflyCamera(const FreeflyCamera& other);
		FreeflyCamera(FreeflyCamera&& other) noexcept;
		FreeflyCamera& operator=(const FreeflyCamera& other);

		void Initialize(const glm::vec3& _position, const glm::vec3& _focus);
		void Update(float deltaTime) override;
		void Shutdown();

		void SetSpeed(float speed);
		float GetSpeed() const;

		void SetSensitivity(float sensitivity);
		float GetSensitivity() const;

	private:

		float speed;
		float sensitivity;

		glm::vec3 position;
		glm::vec3 rotation;

		glm::vec3 displacement;

		void RegisterKeyCallbacks() override;
		void DeregisterKeyCallbacks() override;

		void RegisterMouseCallbacks() override;
		void DeregisterMouseCallbacks() override;

		// Key callback functions
		void MoveUp(KeyState state);
		void MoveDown(KeyState state);
		void MoveLeft(KeyState state);
		void MoveRight(KeyState state);
		void MoveForward(KeyState state);
		void MoveBackward(KeyState state);

		// Mouse callback functions
		void RotateCamera(double xDelta, double yDelta);

	};
}

#endif