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

		void Initialize();
		void Update(float deltaTime) override;
		void Shutdown();

	private:

		float velocity;

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