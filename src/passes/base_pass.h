#ifndef BASE_PASS_H
#define BASE_PASS_H

#include <cstdint> // uint32_t

#include "../descriptors/descriptor_pool.h"
#include "vulkan/vulkan.h"

namespace TANG
{
	// Forward declaration
	class SecondaryCommandBuffer;
	class AssetResources;
	class Framebuffer;
	class BaseRenderPass;

	// Contains any data that is NOT owned by the pass
	struct DrawData
	{
		uint32_t framebufferWidth;
		uint32_t framebufferHeight;
		SecondaryCommandBuffer* cmdBuffer;
		BaseRenderPass* renderPass;
		Framebuffer* framebuffer;
		AssetResources* asset;
	};

	class BasePass
	{
	public:

		virtual void Create(const DescriptorPool& descriptorPool);
		virtual void Destroy() = 0;
		virtual void Draw(uint32_t currentFrame, const DrawData& data) = 0;

		VkFence GetFence();

	protected:

		bool IsDrawDataValid(const DrawData& data) const;

		virtual void CreateFramebuffers();
		virtual void CreatePipelines();
		virtual void CreateRenderPasses();
		virtual void CreateSetLayoutCaches();
		virtual void CreateDescriptorSets(const DescriptorPool& descriptorPool);
		virtual void CreateUniformBuffers();
		virtual void CreateSyncObjects();

	protected:

		VkFence fence;

	};
}


#endif