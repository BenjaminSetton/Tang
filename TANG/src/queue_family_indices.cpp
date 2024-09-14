
#include <limits>

#include "device_cache.h"
#include "queue_family_indices.h"
#include "utils/logger.h"
#include "utils/sanity_check.h"

namespace TANG
{
	// If anything changes with the QueueType enum, make sure to change every reference to queueFamilies below!
	TNG_ASSERT_COMPILE(static_cast<uint32_t>(QUEUE_TYPE::COUNT) == 4);

	QueueFamilyIndices::QueueFamilyIndices()
	{
		queueFamilies[QUEUE_TYPE::GRAPHICS] = INVALID_INDEX;
		queueFamilies[QUEUE_TYPE::PRESENT]  = INVALID_INDEX;
		queueFamilies[QUEUE_TYPE::TRANSFER] = INVALID_INDEX;
		queueFamilies[QUEUE_TYPE::COMPUTE]  = INVALID_INDEX;
		queueFamilies[QUEUE_TYPE::COUNT]    = INVALID_INDEX;
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

		queueFamilies[QUEUE_TYPE::GRAPHICS] = other.queueFamilies.at(QUEUE_TYPE::GRAPHICS);
		queueFamilies[QUEUE_TYPE::PRESENT]  = other.queueFamilies.at(QUEUE_TYPE::PRESENT);
		queueFamilies[QUEUE_TYPE::TRANSFER] = other.queueFamilies.at(QUEUE_TYPE::TRANSFER);
		queueFamilies[QUEUE_TYPE::COMPUTE]  = other.queueFamilies.at(QUEUE_TYPE::COMPUTE);
		queueFamilies[QUEUE_TYPE::COUNT]    = other.queueFamilies.at(QUEUE_TYPE::COUNT);

		return *this;
	}

	void QueueFamilyIndices::SetIndex(QUEUE_TYPE type, QueueFamilyIndexType index)
	{
		if (type == QUEUE_TYPE::COUNT) return;

		queueFamilies[type] = index;
	}

	QueueFamilyIndices::QueueFamilyIndexType QueueFamilyIndices::GetIndex(QUEUE_TYPE type) const
	{
		if (type == QUEUE_TYPE::COUNT) return INVALID_INDEX;

		return queueFamilies.at(type);
	} 

	bool QueueFamilyIndices::IsValid(QueueFamilyIndexType index) const
	{
		return index != INVALID_INDEX;
	}

	bool QueueFamilyIndices::IsComplete()
	{
		return	IsValid(queueFamilies[QUEUE_TYPE::GRAPHICS]) &&
				IsValid(queueFamilies[QUEUE_TYPE::PRESENT]) &&
				IsValid(queueFamilies[QUEUE_TYPE::COMPUTE]) &&
				IsValid(queueFamilies[QUEUE_TYPE::TRANSFER]);
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
			// Check that the device supports a graphics queue and compute queue
			// NOTE - We could potentially select separate queues for graphics and
			//        compute, but let's keep it simple for now
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				indices.SetIndex(QUEUE_TYPE::GRAPHICS, i);
				indices.SetIndex(QUEUE_TYPE::COMPUTE, i);
			}

			// Check that the device supports present queues
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.SetIndex(QUEUE_TYPE::PRESENT, i);
			}

			// Check that the device supports a transfer queue
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				// Choose a different queue from the graphics queue, if possible
				if (indices.GetIndex(QUEUE_TYPE::GRAPHICS) == i)
				{
					graphicsTransferQueue = i;
				}
				else
				{
					indices.SetIndex(QUEUE_TYPE::TRANSFER, i);
				}
			}

			if (indices.IsComplete()) break;

			i++;
		}

		// If we couldn't find a different queue for the TRANSFER and GRAPHICS operations, then simply
		// use the same queue for both (if it supports TRANSFER operations)
		if (!indices.IsValid(static_cast<QueueFamilyIndices::QueueFamilyIndexType>(QUEUE_TYPE::TRANSFER)) && graphicsTransferQueue != QueueFamilyIndices::INVALID_INDEX)
		{
			indices.SetIndex(QUEUE_TYPE::TRANSFER, graphicsTransferQueue);
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