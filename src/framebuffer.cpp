
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
		attachments.clear();
	}

	Framebuffer::Framebuffer(Framebuffer&& other) : framebuffer(std::move(other.framebuffer))
	{
	}

	void Framebuffer::Create(const FramebufferCreateInfo& createInfo)
	{
		if (framebuffer != VK_NULL_HANDLE)
		{
			LogWarning("Attempting to create a framebuffer twice!");
			return;
		}

		if (createInfo.attachments == nullptr)
		{
			LogWarning("Attempting to create a framebuffer with no attachments!");
			return;
		}

		if (createInfo.width == 0 || createInfo.height == 0 || createInfo.layers == 0)
		{
			LogWarning("Attempting to create a framebuffer with no width, height or layer count!");
			return;
		}

		if (createInfo.renderPass == nullptr)
		{
			LogWarning("Attempting to create framebuffer with an invalid render pass. Pointer is null");
			return;
		}

		std::vector<VkImageView> imageViews;
		imageViews.resize(createInfo.attachmentCount);

		for (uint32_t i = 0; i < createInfo.attachmentCount; i++)
		{
			imageViews[i] = (createInfo.attachments[i]->GetImageView());
		}

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType			=	VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass		= createInfo.renderPass->GetRenderPass();
		framebufferInfo.attachmentCount = createInfo.attachmentCount;
		framebufferInfo.pAttachments	= imageViews.data();
		framebufferInfo.width			= createInfo.width;
		framebufferInfo.height			= createInfo.height;
		framebufferInfo.layers			= createInfo.layers;

		if (vkCreateFramebuffer(GetLogicalDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
		{
			LogError("Failed to create framebuffer!");
		}

		// Cache the images
		uint32_t attachmentCount = createInfo.attachmentCount;

		attachments.resize(attachmentCount);
		for (uint32_t i = 0; i < attachmentCount; i++)
		{
			attachments[i] = createInfo.attachments[i];
		}
	}

	void Framebuffer::Destroy()
	{
		vkDestroyFramebuffer(GetLogicalDevice(), framebuffer, nullptr);
		framebuffer = VK_NULL_HANDLE;
	}

	VkFramebuffer Framebuffer::GetFramebuffer()
	{
		return framebuffer;
	}

	const VkFramebuffer Framebuffer::GetFramebuffer() const
	{
		return framebuffer;
	}

	TextureResource** Framebuffer::GetAttachmentImages()
	{
		return attachments.data();
	}

	uint32_t Framebuffer::GetAttachmentCount()
	{
		return static_cast<uint32_t>(attachments.size());
	}
}