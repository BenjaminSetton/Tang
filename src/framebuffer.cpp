
#include <utility> // move

#include "device_cache.h"
#include "framebuffer.h"
#include "render_passes/base_render_pass.h"
#include "texture_resource.h"
#include "utils/logger.h"

namespace TANG
{

	Framebuffer::Framebuffer() : framebuffer(VK_NULL_HANDLE)
	{
	}

	Framebuffer::~Framebuffer()
	{
		if (framebuffer != VK_NULL_HANDLE)
		{
			LogWarning("Framebuffer object was destroyed but handle is still valid!");
		}

		framebuffer = VK_NULL_HANDLE;
		attachmentsCache.clear();
	}

	Framebuffer::Framebuffer(Framebuffer&& other) : framebuffer(std::move(other.framebuffer))
	{
	}

	void Framebuffer::Create(const FramebufferCreateInfo& createInfo)
	{
		if (framebuffer != VK_NULL_HANDLE)
		{
			LogError("Attempting to create a framebuffer twice!");
			return;
		}

		if (createInfo.attachments.size() == 0)
		{
			LogError("Attempting to create a framebuffer with no attachments!");
			return;
		}

		if (createInfo.width == 0 || createInfo.height == 0 || createInfo.layers == 0)
		{
			LogError("Attempting to create a framebuffer with no width, height or layer count!");
			return;
		}

		if (createInfo.renderPass == nullptr)
		{
			LogError("Attempting to create framebuffer with an invalid render pass. Pointer is null");
			return;
		}

		if (createInfo.imageViewIndices.size() == 0)
		{
			LogError("Attempting to create framebuffer with no image view indices! Every attachment must have a corresponding image view index");
			return;
		}

		if (createInfo.attachments.size() != createInfo.imageViewIndices.size())
		{
			LogError("Attempting to create framebuffer with mismatched attachment and image view index counts! Every attachment must have a corresponding image view index");
			return;
		}

		uint32_t attachmentCount = static_cast<uint32_t>(createInfo.attachments.size());

		std::vector<VkImageView> imageViews;
		imageViews.resize(attachmentCount);

		for (uint32_t i = 0; i < attachmentCount; i++)
		{
			uint32_t imageViewIndex = createInfo.imageViewIndices[i];
			imageViews[i] = (createInfo.attachments[i]->GetImageView(imageViewIndex));
		}

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType			=	VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass		= createInfo.renderPass->GetRenderPass();
		framebufferInfo.attachmentCount = attachmentCount;
		framebufferInfo.pAttachments	= imageViews.data();
		framebufferInfo.width			= createInfo.width;
		framebufferInfo.height			= createInfo.height;
		framebufferInfo.layers			= createInfo.layers;

		if (vkCreateFramebuffer(GetLogicalDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
		{
			LogError("Failed to create framebuffer!");
			return;
		}

		// Cache the attachments
		attachmentsCache = createInfo.attachments;
	}

	void Framebuffer::Destroy()
	{
		vkDestroyFramebuffer(GetLogicalDevice(), framebuffer, nullptr);
		framebuffer = VK_NULL_HANDLE;

		attachmentsCache.clear();
	}

	VkFramebuffer Framebuffer::GetFramebuffer()
	{
		return framebuffer;
	}

	const VkFramebuffer Framebuffer::GetFramebuffer() const
	{
		return framebuffer;
	}

	std::vector<TextureResource*> Framebuffer::GetAttachmentImages()
	{
		return attachmentsCache;
	}
}