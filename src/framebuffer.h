#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <vector>

#include "vulkan/vulkan.h"

namespace TANG
{
	// Forward declarations
	class TextureResource;
	class BaseRenderPass;

	struct FramebufferCreateInfo
	{
		BaseRenderPass* renderPass;
		TextureResource** attachments;
		uint32_t attachmentCount;
		uint32_t width;
		uint32_t height;
		uint32_t layers;
	};

	class Framebuffer
	{
	public:

		Framebuffer();
		~Framebuffer();
		Framebuffer(Framebuffer&& other);
		Framebuffer(const Framebuffer& other) = delete;
		Framebuffer& operator=(const Framebuffer& other) = delete;

		void Create(const FramebufferCreateInfo& createInfo);
		void Destroy();

		VkFramebuffer GetFramebuffer();
		const VkFramebuffer GetFramebuffer() const;

		TextureResource** GetAttachmentImages();
		uint32_t GetAttachmentCount();

	private:
		VkFramebuffer framebuffer;

		// Hold a pointer to all the attachments so that we know which images make up the framebuffer
		// In theory this should be fine, since if the images become invalid at any point the
		// framebuffer should be re-created.
		std::vector<TextureResource*> attachments;
	};
}

#endif