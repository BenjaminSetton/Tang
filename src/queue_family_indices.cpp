
#include <limits>

#include "queue_family_indices.h"
#include "queue_types.h"
#include "utils/sanity_check.h"

namespace TANG
{
	// If anything changes with the QueueType enum, make sure to change this struct as well!
	static_assert(static_cast<uint32_t>(QueueType::COUNT) == 3);

	QueueFamilyIndices::QueueFamilyIndices()
	{
		queueFamilies[QueueType::GRAPHICS] = std::numeric_limits<QueueFamilyIndexType>::max();
		queueFamilies[QueueType::PRESENT] = std::numeric_limits<QueueFamilyIndexType>::max();
		queueFamilies[QueueType::TRANSFER] = std::numeric_limits<QueueFamilyIndexType>::max();
	}

	QueueFamilyIndices::~QueueFamilyIndices()
	{
		// Nothing to do here
	}

	QueueFamilyIndices::QueueFamilyIndices(const QueueFamilyIndices& other)
	{
		queueFamilies[QueueType::GRAPHICS] = other.queueFamilies.at(QueueType::GRAPHICS);
		queueFamilies[QueueType::PRESENT] = other.queueFamilies.at(QueueType::PRESENT);
		queueFamilies[QueueType::TRANSFER] = other.queueFamilies.at(QueueType::TRANSFER);
	}

	QueueFamilyIndices::QueueFamilyIndices(QueueFamilyIndices&& other) noexcept
	{
		TNG_TODO();
	}

	QueueFamilyIndices& QueueFamilyIndices::operator=(const QueueFamilyIndices& other)
	{
		TNG_TODO();
	}


	bool QueueFamilyIndices::IsValid(QueueFamilyIndexType index)
	{
		return index != std::numeric_limits<QueueFamilyIndexType>::max();
	}

	bool QueueFamilyIndices::IsComplete()
	{
		return	IsValid(queueFamilies[QueueType::GRAPHICS]) &&
				IsValid(queueFamilies[QueueType::PRESENT]) &&
				IsValid(queueFamilies[QueueType::TRANSFER]);
	}

}