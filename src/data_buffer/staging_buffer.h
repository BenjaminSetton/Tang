#ifndef STAGING_BUFFER_H
#define STAGING_BUFFER_H

#include "buffer.h"

namespace TANG
{

	// Small wrapper around Buffer that represents a temporary staging buffer. These objects usually have
	// a super short lifespan, and therefore it doesn't make any sense to copy them
	class StagingBuffer : public Buffer
	{
	public:

		StagingBuffer();
		~StagingBuffer();
		StagingBuffer(const StagingBuffer& other) = delete;
		StagingBuffer(StagingBuffer&& other) noexcept;
		StagingBuffer& operator=(const StagingBuffer& other) = delete;

		void Create(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize size) override;
		void Destroy(VkDevice logicalDevice) override;

		void CopyIntoBuffer(VkDevice logicalDevice, void* sourceData, VkDeviceSize size);

	private:
		// Nothing to see here...
	};
}


#endif