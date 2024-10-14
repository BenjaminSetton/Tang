#ifndef BASE_RENDER_PASS_H
#define BASE_RENDER_PASS_H

#include <utility>
#include <vector>

#include "vulkan/vulkan.h"

namespace TANG
{
	// Forward declarations
	class RenderPassBuilder;

	class BaseRenderPass
	{
	public:

		BaseRenderPass();
		~BaseRenderPass();
		BaseRenderPass(BaseRenderPass&& other) noexcept;

		// Disable copying
		BaseRenderPass(const BaseRenderPass& other) = delete;
		BaseRenderPass& operator=(const BaseRenderPass& other) = delete;

		void Create();
		void Destroy();

		VkRenderPass GetRenderPass() const;

		const VkImageLayout* GetFinalImageLayouts() const;
		uint32_t GetAttachmentCount();

	protected:

		virtual bool Build(RenderPassBuilder& out_builder) = 0;

	private:

		// Creates the render pass object through the render pass builder. Derived classes
		// are in charge of populating this object and finally passing it
		void Create_Internal(const RenderPassBuilder& builder);

		VkRenderPass renderPass;

		// Stores the final layout of all the attachments. This is used when CMD_EndRenderPass() is issued so we can
		// reflect the implicit layout transition that Vulkan does for us in the TextureResource object
		std::vector<VkImageLayout> finalImageLayouts;
	};

	// Utility class to make it easy to build render passes by calling AddAttachment
	// and passing the builder to the BaseRenderPass::Create() call
	class RenderPassBuilder
	{
	public:

		friend class BaseRenderPass;

		// Helpers function to add generic attachments and subpasses. These copy the parameters and add to the internal containers
		RenderPassBuilder& AddAttachment(const VkAttachmentDescription& attachmentDesc);
		RenderPassBuilder& AddSubpass(const VkSubpassDescription& subpassDesc, const VkSubpassDependency* subpassDep);

		// Performs sanity checks to ensure that the final state of the builder is valid before being used to construct a render pass
		bool IsValid() const;

		void PreAllocateAttachmentReferences(uint32_t numberOfAttachmentRefs);

		[[nodiscard]] VkAttachmentReference& GetNextAttachmentReference();

	private:

		std::vector<VkAttachmentDescription> attachmentDescriptions;
		// We must track the attachment references so the pointers contained within the subpass don't become invalid when creating the render pass
		std::vector<VkAttachmentReference> attachmentReferences;

		std::vector<VkSubpassDescription> subpassDescriptions;
		std::vector<VkSubpassDependency> subpassDependencies;

	};
}

#endif