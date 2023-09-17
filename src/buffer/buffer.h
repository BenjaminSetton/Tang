#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <vulkan/vulkan.h>

namespace TANG
{
	class Buffer
	{
	public:
		Buffer();
		~Buffer();
		Buffer(const Buffer& other);

		void operator=(const Buffer& other);

		// Pure virtual function. Derived classes are meant to override this function and call up to Buffer::CreateBase() to specify
		// the type of buffer they are.
		virtual void Create(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size) = 0;

		// Pure virtual function to destroy internal buffers plus any other buffers that the derived class may have allocated. For example
		// the VertexBuffer derived class must delete it's internal staging buffer as well.
		virtual void Destroy(VkDevice& logicalDevice) = 0;

		// Pure virtual function to map generic data onto the internal buffer
		virtual void MapData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize) = 0;

		// Returns the member variable "buffer"
		VkBuffer GetBuffer();

		// Returns the member variable "bufferMemory"
		VkDeviceMemory GetBufferMemory();

	protected:

		// Usually this function is called to copy data from a staging buffer, when copying data from the SRAM (CPU) to VRAM (GPU)
		void CopyFromBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		// Notice the optional _buffer and _bufferMemory parameters. Derived classes may choose to create temporary buffers other
		// than themselves (for example the vertex buffer creating a staging buffer) by filling out these parameters. If left as
		// nullptr then the member variables will be filled out instead.
		void CreateBase(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* outBuffer = nullptr, VkDeviceMemory* outBufferMemory = nullptr);

		// Finds a suitable index considering the physical device's memory properties
		// TODO: Replace the function in renderer.cpp in favor of this one after porting all buffers to their own files
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memFlags, VkPhysicalDevice& physicalDevice);

	protected:

		VkBuffer buffer;
		VkDeviceMemory bufferMemory;
	};
}

#endif