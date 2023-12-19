
// SILENCE WARNINGS FROM GLM
// 4201: warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)

#include <glm/gtc/matrix_transform.hpp>

#pragma warning(pop)

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