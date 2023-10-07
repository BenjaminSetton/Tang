#ifndef BASE_CAMERA_H
#define BASE_CAMERA_H

#include "glm.hpp"

namespace TANG
{
	// Defines an abstract base camera class which other cameras will derive from. All derived cameras must implement
	// the RegisterKeyCallbacks() and DeregisterKeyCallbacks() functions. These will register and deregister the camera 
	// controls on the input handler
	class BaseCamera
	{
	public:

		BaseCamera();
		~BaseCamera();
		BaseCamera(const BaseCamera& other);
		BaseCamera(BaseCamera&& other) noexcept;
		BaseCamera& operator=(const BaseCamera& other);

		virtual void Update(float deltaTime) = 0;

		glm::mat4 GetViewMatrix() const;

	protected:

		virtual void RegisterKeyCallbacks() = 0;
		virtual void DeregisterKeyCallbacks() = 0;

		void CalculateViewMatrix();

		glm::vec3 position;
		glm::vec3 focus;
		bool shouldUpdateMatrix;

	private:

		glm::mat4 viewMatrix;

	};
}

#endif