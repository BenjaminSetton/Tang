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

		void SetVelocity(float velocity);
		float GetVelocity() const;

	private:

		float velocity;
		float dtCache;

		void RegisterKeyCallbacks() override;
		void DeregisterKeyCallbacks() override;

		// Define the key callback functions below
		void MoveUp(KeyState state);
		void MoveDown(KeyState state);
		void MoveLeft(KeyState state);
		void MoveRight(KeyState state);
		void MoveForward(KeyState state);
		void MoveBackward(KeyState state);

	};
}

#endif