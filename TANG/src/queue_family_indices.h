#ifndef QUEUE_FAMILY_INDICES_H
#define QUEUE_FAMILY_INDICES_H

#include <unordered_map>

#include "queue_types.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	class QueueFamilyIndices
	{
	public:

		typedef uint32_t QueueFamilyIndexType;
		static constexpr uint32_t INVALID_INDEX = std::numeric_limits<QueueFamilyIndexType>::max();

		QueueFamilyIndices();
		~QueueFamilyIndices();
		QueueFamilyIndices(const QueueFamilyIndices& other);
		QueueFamilyIndices(QueueFamilyIndices&& other) noexcept;
		QueueFamilyIndices& operator=(const QueueFamilyIndices& other);

		void SetIndex(QUEUE_TYPE type, QueueFamilyIndexType index);
		QueueFamilyIndexType GetIndex(QUEUE_TYPE type) const;

		bool IsValid(QueueFamilyIndexType index) const;
		bool IsComplete();

	private:

		std::unordered_map<QUEUE_TYPE, QueueFamilyIndexType> queueFamilies;
	};

	// Global function
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
}

#endif