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

		void Initialize(const glm::vec3& _position, const glm::vec3& _rotationDegrees);
		void Update(float deltaTime) override;
		void Shutdown();

		void SetSpeed(float speed);
		float GetSpeed() const;

		void SetSensitivity(float sensitivity);
		float GetSensitivity() const;

	private:

		// Persistent data - describes how fast the camera translates (speed) and how sensitive it is to
		//                   mouse input for rotation (sensitivity)
		float speed;
		float sensitivity;

		// Persistent data - describes the position and rotation of the camera. This data is NOT reset every frame
		glm::vec3 position;
		glm::vec3 rotation;

		// Per frame - the displacement is accumulated through the callbacks, and reset every frame after being used
		//             to translate the camera
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