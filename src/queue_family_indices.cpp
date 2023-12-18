
#include <limits>

#include "device_cache.h"
#include "queue_family_indices.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

namespace TANG
{
	// If anything changes with the QueueType enum, make sure to change this struct as well!
	TNG_ASSERT_COMPILE(static_cast<uint32_t>(QueueType::COUNT) == 3);

	QueueFamilyIndices::QueueFamilyIndices()
	{
		queueFamilies[QueueType::GRAPHICS] = INVALID_INDEX;
		queueFamilies[QueueType::PRESENT]  = INVALID_INDEX;
		queueFamilies[QueueType::TRANSFER] = INVALID_INDEX;
		queueFamilies[QueueType::COUNT]    = INVALID_INDEX;
	}

	QueueFamilyIndices::~QueueFamilyIndices()
	{
		// Nothing to do here
	}

	QueueFamilyIndices::QueueFamilyIndices(const QueueFamilyIndices& other) : queueFamilies(other.queueFamilies)
	{
	}

	QueueFamilyIndices::QueueFamilyIndices(QueueFamilyIndices&& other) noexcept : queueFamilies(std::move(other.queueFamilies))
	{
	}

	QueueFamilyIndices& QueueFamilyIndices::operator=(const QueueFamilyIndices& other)
	{
		if (this == &other) return *this;

		queueFamilies[QueueType::GRAPHICS] = other.queueFamilies.at(QueueType::GRAPHICS);
		queueFamilies[QueueType::PRESENT]  = other.queueFamilies.at(QueueType::PRESENT);
		queueFamilies[QueueType::TRANSFER] = other.queueFamilies.at(QueueType::TRANSFER);
		queueFamilies[QueueType::COUNT]    = other.queueFamilies.at(QueueType::COUNT);

		return *this;
	}

	void QueueFamilyIndices::SetIndex(QueueType type, QueueFamilyIndexType index)
	{
		if (type == QueueType::COUNT) return;

		queueFamilies[type] = index;
	}

	QueueFamilyIndices::QueueFamilyIndexType QueueFamilyIndices::GetIndex(QueueType type)
	{
		if (type == QueueType::COUNT) return INVALID_INDEX;

		return queueFamilies[type];
	} 

	bool QueueFamilyIndices::IsValid(QueueFamilyIndexType index)
	{
		return index != INVALID_INDEX;
	}

	bool QueueFamilyIndices::IsComplete()
	{
		return	IsValid(queueFamilies[QueueType::GRAPHICS]) &&
				IsValid(queueFamilies[QueueType::PRESENT]) &&
				IsValid(queueFamilies[QueueType::TRANSFER]);
	}

	// Global function
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		uint32_t graphicsTransferQueue = std::numeric_limits<uint32_t>::max();
		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			// Check that the device supports a graphics queue
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.SetIndex(QueueType::GRAPHICS, i);
			}

			// Check that the device supports present queues
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.SetIndex(QueueType::PRESENT, i);
			}

			// Check that the device supports a transfer queue
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				// Choose a different queue from the graphics queue, if possible
				if (indices.GetIndex(QueueType::GRAPHICS) == i)
				{
					graphicsTransferQueue = i;
				}
				else
				{
					indices.SetIndex(QueueType::TRANSFER, i);
				}
			}

			if (indices.IsComplete()) break;

			i++;
		}

		// If we couldn't find a different queue for the TRANSFER and GRAPHICS operations, then simply
		// use the same queue for both (if it supports TRANSFER operations)
		if (!indices.IsValid(static_cast<QueueFamilyIndices::QueueFamilyIndexType>(QueueType::TRANSFER)) && graphicsTransferQueue != QueueFamilyIndices::INVALID_INDEX)
		{
			indices.SetIndex(QueueType::TRANSFER, graphicsTransferQueue);
		}

		// Check that we filled in all of our queue families, otherwise log a warning
		// NOTE - I'm not sure if there's a problem with not finding queue families for
		//        every queue type...
		if (!indices.IsComplete())
		{
			LogWarning("Failed to find all queue families!");
		}

		return indices;
	}
}