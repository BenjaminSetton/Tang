#ifndef BASE_PASS_H
#define BASE_PASS_H

#include <cstdint> // uint32_t

#include "TANG/descriptor_pool.h"
//#include "vulkan/vulkan.h"

namespace TANG
{
	// Forward declaration
	class CommandBuffer;
	struct AssetResources;
	class Framebuffer;
	class BaseRenderPass;

	// Contains any data that is borrowed by the pass for a draw operation. The only
	// exception is the command buffer, which we use to record commands
	struct DrawData
	{
		uint32_t framebufferWidth;
		uint32_t framebufferHeight;
		CommandBuffer* cmdBuffer;
		const BaseRenderPass* renderPass;
		const Framebuffer* framebuffer;
		const AssetResources* asset;

		bool IsValid() const;
	};


	/////////////////////////////////////////////////////////
	// DEPRECATED
	/////////////////////////////////////////////////////////
	class BasePass
	{
	public:

		virtual void Create();
		virtual void Destroy() = 0;

		VkFence GetFence();

	protected:

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
	/////////////////////////////////////////////////////////
	// DEPRECATED
	/////////////////////////////////////////////////////////
}


#endif