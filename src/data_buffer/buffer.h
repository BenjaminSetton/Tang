#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan.h>

namespace TANG
{
	enum class BUFFER_STATE
	{
		DEFAULT,	// The default state of the buffer. This state changes after the buffer is created, and it can never go back to DEFAULT
		CREATED,	// The buffer has been created
		MAPPED,		// Memory has been mapped into the internal buffer
		DESTROYED	// The internal buffer has been destroyed (staging buffer doesn't count here)
	};

	class Buffer
	{
	public:
		Buffer();
		~Buffer();
		Buffer(const Buffer& other);
		Buffer(Buffer&& other) noexcept;
		Buffer& operator=(const Buffer& other);

		// Pure virtual function. Derived classes are meant to override this function and call up to Buffer::CreateBase() to specify
		// the type of buffer they are.
		virtual void Create(VkDeviceSize size) = 0;

		// Pure virtual function to destroy internal buffers plus any other buffers that the derived class may have allocated. For example
		// the VertexBuffer derived class must delete it's internal staging buffer as well.
		virtual void Destroy() = 0;

		// Returns the member variable "buffer"
		VkBuffer GetBuffer();
		const VkBuffer GetBuffer() const;

		// Returns the member variable "bufferMemory"
		VkDeviceMemory GetBufferMemory();

		// Returns the size of the created buffer
		VkDeviceSize GetBufferSize() const;

		// Returns whether the buffer is either in the DEFAULT state or the DESTROYED state. Useful for checking early exit conditions
		bool IsInvalid() const;

	protected:

		// Usually this function is called to copy data from a staging buffer, when copying data from the SRAM (CPU) to VRAM (GPU)
		void CopyFromBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		// Notice the optional _buffer and _bufferMemory parameters. Derived classes may choose to create temporary buffers other
		// than themselves (for example the vertex buffer creating a staging buffer) by filling out these parameters. If left as
		// nullptr then the member variables will be filled out instead.
		void CreateBase(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		// Finds a suitable index considering the physical device's memory properties
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memFlags);

	protected:

		VkBuffer buffer;
		VkDeviceMemory bufferMemory;
		VkDeviceSize bufferSize;
		BUFFER_STATE bufferState;
	};
}

#endif