
#include <gtc/matrix_transform.hpp>

#include "base_camera.h"
#include "../utils/sanity_check.h"

namespace TANG
{

	BaseCamera::BaseCamera() : position(glm::vec3(0.0)), focus(glm::vec3(0.0)), shouldUpdateMatrix(false)
	{
		// Nothing to do here
	}

	BaseCamera::~BaseCamera()
	{
		// Nothing to do here
	}

	BaseCamera::BaseCamera(const BaseCamera& other) : position(other.position), focus(other.focus), shouldUpdateMatrix(other.shouldUpdateMatrix)
	{
		// Nothing to do here
	}

	BaseCamera::BaseCamera(BaseCamera&& other) noexcept : position(std::move(other.position)), focus(std::move(other.focus)), shouldUpdateMatrix(std::move(other.shouldUpdateMatrix))
	{
		// Nothing to do here
	}

	BaseCamera& BaseCamera::operator=(const BaseCamera& other)
	{
		// Protect against self-assignment
		if (this == &other)
		{
			return *this;
		}

		position = other.position;
		focus = other.focus;
		shouldUpdateMatrix = other.shouldUpdateMatrix;

		return *this;
	}

	glm::mat4 BaseCamera::GetViewMatrix() const
	{
		return viewMatrix;
	}

	void BaseCamera::CalculateViewMatrix()
	{
		viewMatrix = glm::lookAt(position, focus, glm::vec3(0.0f, 1.0f, 0.0f));
	}

}