#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include "../queue_types.h" // QUEUE_TYPE
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

	enum class COMMAND_BUFFER_TYPE
	{
		PRIMARY,
		SECONDARY
	};

	// Forward declarations
	struct AssetResources;
	class BasePipeline;
	class DescriptorSet;

	// Pure virtual base command buffer class. This encapsulates all basic functionality shared between
	// a primary and secondary command buffer, both of which derive from this class
	class CommandBuffer
	{
	public:

		CommandBuffer();
		~CommandBuffer();
		CommandBuffer(CommandBuffer&& other) noexcept;

		CommandBuffer(const CommandBuffer& other) = delete;
		CommandBuffer& operator=(const CommandBuffer& other) = delete;

		virtual void Allocate(QUEUE_TYPE type) = 0;
		void Destroy();

		void BeginRecording(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo* inheritanceInfo);
		void EndRecording();

		void CMD_BindMesh(const AssetResources* resources);
		void CMD_BindDescriptorSets(const BasePipeline* pipeline, uint32_t descriptorSetCount, DescriptorSet* descriptorSets);
		void CMD_PushConstants(const BasePipeline* pipeline, void* constantData, uint32_t size, VkShaderStageFlags stageFlags);
		void CMD_BindPipeline(const BasePipeline* pipeline);
		void CMD_SetViewport(float width, float height);
		void CMD_SetScissor(VkOffset2D scissorOffset, VkExtent2D scissorExtent);

		void CMD_Draw(uint32_t vertexCount);
		void CMD_DrawIndexed(uint64_t indexCount);
		void CMD_DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount);

		// Dispatch a command buffer to a compute shader
		void CMD_Dispatch(uint32_t x, uint32_t y, uint32_t z);

		void Reset(bool releaseMemory = false);

		VkCommandBuffer GetBuffer() const;

		// Returns true if the command buffer is currently recording. All derived class functions record commands
		// into the command buffer follow a convention where the function name is preprended with a "CMD_". Also,
		// all of the aforementioned functions must call IsRecording() at the very beginning to ensure that the
		// command buffer is currently recording, and prematurely exit otherwise.
		bool IsRecording() const;

		bool IsReset() const;
		bool IsAllocated() const;
		bool IsWritable() const;

		// Performs a few checks that tell us whether we can write commands to this command buffer
		bool IsCommandBufferValid() const;

		// Returns the type of the command buffer (primary or secondary)
		virtual COMMAND_BUFFER_TYPE GetType() = 0;

		QUEUE_TYPE GetAllocatedQueueType() const;

	protected:

		VkCommandBuffer commandBuffer;
		COMMAND_BUFFER_STATE cmdBufferState;
		COMMAND_BUFFER_TYPE cmdBufferType;
		bool isOneTimeSubmit;
		QUEUE_TYPE allocatedQueueType;
	};
}

#endif