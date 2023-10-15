
#include <gtc/matrix_transform.hpp>

#include "base_camera.h"
#include "../utils/sanity_check.h"

namespace TANG
{

	BaseCamera::BaseCamera() : matrix(glm::identity<glm::mat4>())
	{
		// Nothing to do here
	}

	BaseCamera::~BaseCamera()
	{
		// Nothing to do here
	}

	BaseCamera::BaseCamera(const BaseCamera& other) : matrix(other.matrix)
	{
		// Nothing to do here
	}

	BaseCamera::BaseCamera(BaseCamera&& other) noexcept : matrix(std::move(other.matrix))
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

		matrix = other.matrix;

		return *this;
	}

	glm::mat4 BaseCamera::GetViewMatrix() const
	{
		return glm::inverse(matrix);
	}

	glm::vec3 BaseCamera::GetPosition() const
	{
		return glm::vec3(matrix[3]);
	}

}