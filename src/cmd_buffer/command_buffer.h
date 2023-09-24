#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <vulkan/vulkan.h>

namespace TANG
{
	// This holds all the possible states that a command buffer may be in throughout it's lifetime. The state flow can
	// be represented as follows:
	//		DEFAULT -> ALLOCATED -> RECORDING -> SEALED -> DESTROYED
	// The command buffer may cycle between the RECORDING and SEALED states only, and not between any other states. If you're cycling between
	// ALLOCATED and DESTROYED you're doing something wrong...
	enum class COMMAND_BUFFER_STATE
	{
		DEFAULT,		// This is the default value of the command buffer state. It will never return to the DEFAULT state after the first state-altering function called
		ALLOCATED,		// Command buffer has been allocated
		RESET,			// Command buffer has been reset, and therefore contains no Vulkan commands
		RECORDING,		// Command buffer is being recorded
		SEALED,			// Command buffer is NOT being recorded
		DESTROYED		// Command buffer has already been destroyed
	};

	// Pure virtual base command buffer class. This encapsulates all basic functionality shared between
	// a primary and secondary command buffer, both of which derive from this class
	class CommandBuffer
	{
	public:

		CommandBuffer();
		~CommandBuffer();
		CommandBuffer(const CommandBuffer& other);
		CommandBuffer(CommandBuffer&& other) noexcept;
		CommandBuffer& operator=(const CommandBuffer& other);

		virtual void Create(VkDevice logicalDevice, VkCommandPool commandPool) = 0;
		void Destroy(VkDevice logicalDevice, VkCommandPool commandPool);

		void BeginRecording(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo* inheritanceInfo);
		void EndRecording();

		void Reset(bool releaseMemory = false);

		VkCommandBuffer GetBuffer() const;

		// Returns true if the command buffer is currently recording. All derived class functions record commands
		// into the command buffer follow a convention where the function name is preprended with a "CMD_". Also,
		// all of the aforementioned functions must call IsRecording() at the very beginning to ensure that the
		// command buffer is currently recording, and prematurely exit otherwise.
		bool IsRecording() const;

		bool IsReset() const;

		// Performs a few checks that tell us whether we can write commands to this command buffer
		bool IsCommandBufferValid() const;

	protected:

		VkCommandBuffer commandBuffer;
		COMMAND_BUFFER_STATE cmdBufferState;
		bool isOneTimeSubmit;
	};
}

#endif