#ifndef FREEFLY_CAMERA_H
#define FREEFLY_CAMERA_H

#include "base_camera.h"

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

		void Update(float deltaTime) override;

		void RegisterKeyCallbacks() override;
		void DeregisterKeyCallbacks() override;

	private:

		// Define the key callback functions below

	};
}

#endif