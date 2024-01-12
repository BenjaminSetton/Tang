#ifndef BASE_PASS_H
#define BASE_PASS_H

#include <cstdint> // uint32_t

#include "../descriptors/descriptor_pool.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	// Forward declaration
	class SecondaryCommandBuffer;
	struct AssetResources;
	class Framebuffer;
	class BaseRenderPass;

	// Contains any data that is borrowed by the pass for a draw operation. The only
	// exception is the command buffer, which we use to record commands
	struct DrawData
	{
		uint32_t framebufferWidth;
		uint32_t framebufferHeight;
		SecondaryCommandBuffer* cmdBuffer;
		const BaseRenderPass* renderPass;
		const Framebuffer* framebuffer;
		const AssetResources* asset;
	};

	class BasePass
	{
	public:

		virtual void Create();
		virtual void Destroy() = 0;
		virtual void Draw(uint32_t currentFrame, const DrawData& data) = 0;

		VkFence GetFence();

	protected:

		bool IsDrawDataValid(const DrawData& data) const;

		virtual void CreateFramebuffers();
		virtual void CreatePipelines();
		virtual void CreateRenderPasses();
		virtual void CreateSetLayoutCaches();
		virtual void CreateDescriptorSets();
		virtual void CreateUniformBuffers();
		virtual void CreateSyncObjects();

		void ResetBaseMembers();
		virtual void ResetBorrowedData() = 0;

	protected:

		VkFence fence;
		bool wasCreated;

	};
}


#endif