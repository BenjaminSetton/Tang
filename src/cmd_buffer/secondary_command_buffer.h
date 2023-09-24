#ifndef SECONDARY_COMMAND_BUFFER_H
#define SECONDARY_COMMAND_BUFFER_H

#include "command_buffer.h"

namespace TANG
{
	struct AssetResources;

	class SecondaryCommandBuffer : public CommandBuffer
	{
	public:

		SecondaryCommandBuffer();
		~SecondaryCommandBuffer();
		SecondaryCommandBuffer(const SecondaryCommandBuffer& other);
		SecondaryCommandBuffer(SecondaryCommandBuffer&& other) noexcept;
		SecondaryCommandBuffer& operator=(const SecondaryCommandBuffer& other);

		void Create(VkDevice logicalDevice, VkCommandPool commandPool) override;

		void CMD_BindMesh(AssetResources* resources);
		void CMD_BindDescriptorSets(VkPipelineLayout pipelineLayout, uint32_t descriptorSetCount, VkDescriptorSet* descriptorSets);
		void CMD_BindGraphicsPipeline(VkPipeline graphicsPipeline);
		void CMD_SetViewport(float width, float height);
		void CMD_SetScissor(VkOffset2D scissorOffset, VkExtent2D scissorExtent);

		void CMD_Draw(uint32_t vertexCount);
		void CMD_DrawIndexed(uint32_t indexCount);
		void CMD_DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount);

	private:
	};
}

#endif