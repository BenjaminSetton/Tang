
#include "freefly_camera.h"
#include "../utils/sanity_check.h"

namespace TANG
{

	FreeflyCamera::FreeflyCamera() : BaseCamera()
	{
		// Nothing to do here
	}

	FreeflyCamera::~FreeflyCamera()
	{
		// Nothing to do here
	}

	FreeflyCamera::FreeflyCamera(const FreeflyCamera& other) : BaseCamera(other)
	{
		// Nothing to do here
	}

	FreeflyCamera::FreeflyCamera(FreeflyCamera&& other) noexcept : BaseCamera(std::move(other))
	{
		// Nothing to do here
	}

	FreeflyCamera& FreeflyCamera::operator=(const FreeflyCamera& other)
	{
		UNUSED(other);
		TNG_TODO();

		return *this;
	}

	void FreeflyCamera::Update(float deltaTime)
	{
		UNUSED(deltaTime);
		TNG_TODO();
	}

	void FreeflyCamera::RegisterKeyCallbacks()
	{
		TNG_TODO();
	}

	void FreeflyCamera::DeregisterKeyCallbacks()
	{
		TNG_TODO();
	}

}