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

		// Pure virtual Create() function. Derived classes are meant to override this function and call up to Buffer::CreateBase() to specify
		// the type of buffer they are.
		virtual void Create(VkDevice& logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t memoryTypeIndex) = 0;

		// Destroys internal buffer and its associated memory. Please note that this function assumes no custom allocators we used
		// to allocate the buffers' memory to begin with. If that changes, make sure to change it here as well!
		void Destroy(VkDevice& logicalDevice);

		// Pure virtual function to map generic data onto the internal buffer
		virtual void MapData(VkDevice& logicalDevice, VkCommandBuffer& commandBuffer, void* data, VkDeviceSize bufferSize) = 0;

		// Returns the member variable "buffer"
		VkBuffer GetBuffer() const;

		// Returns the member variable "bufferMemory"
		VkDeviceMemory GetBufferMemory() const;

	protected:

		// Usually this function is called to copy data from a staging buffer, when copying data from the SRAM (CPU) to VRAM (GPU)
		void CopyFromBuffer(VkCommandBuffer& commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		// Notice the optional _buffer and _bufferMemory parameters. Derived classes may choose to create temporary buffers other
		// than themselves (for example the vertex buffer creating a staging buffer) by filling out these parameters. If left as
		// nullptr then the member variables will be filled out instead.
		void CreateBase(VkDevice& logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t memoryTypeIndex, VkBuffer _buffer = nullptr, VkDeviceMemory _bufferMemory = nullptr);

	protected:

		VkBuffer buffer;
		VkDeviceMemory bufferMemory;
	};
}

#endif