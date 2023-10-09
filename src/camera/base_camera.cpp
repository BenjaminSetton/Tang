
#include <gtc/matrix_transform.hpp>

#include "base_camera.h"
#include "../utils/sanity_check.h"

namespace TANG
{

	BaseCamera::BaseCamera() : position(glm::vec3(0.0)), focus(glm::vec3(0.0))
	{
		// Nothing to do here
	}

	BaseCamera::~BaseCamera()
	{
		// Nothing to do here
	}

	BaseCamera::BaseCamera(const BaseCamera& other) : position(other.position), focus(other.focus)
	{
		// Nothing to do here
	}

	BaseCamera::BaseCamera(BaseCamera&& other) noexcept : position(std::move(other.position)), focus(std::move(other.focus))
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

		return *this;
	}

	glm::vec3 BaseCamera::GetPosition() const
	{
		return position;
	}

	glm::vec3 BaseCamera::GetFocus() const
	{
		return focus;
	}

}