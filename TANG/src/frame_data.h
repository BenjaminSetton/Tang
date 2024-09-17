#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include <vector>
#include <vulkan/vulkan.h>

#include "cmd_buffer/primary_command_buffer.h"
#include "cmd_buffer/secondary_command_buffer.h"
#include "framebuffer.h"
#include "texture_resource.h"

namespace TANG
{
	struct FrameData
	{
		// Core sync objects. These are necessary to have a basic back buffer + swap chain setup
		VkSemaphore imageAvailableSemaphore;	// Signalled when swap-chain image becomes available
		VkSemaphore renderFinishedSemaphore;	// Signalled when the render loop has finished
		VkFence inFlightFence;					// Signalled when the GPU has finished processing the frame

		//TextureResource colorAttachment;
		//TextureResource depthAttachment;
		//Framebuffer swapChainFramebuffer;

		uint32_t frameIndex;
		uint32_t swapChainImageIndex;

		// Controlled through TANG API
		// This is used to track all the resources that were allocated through the API 
		// for a particular frame
		std::vector<PrimaryCommandBuffer> primaryCmdBuffers;
		std::vector<SecondaryCommandBuffer> secondaryCmdBuffers;
		std::vector<DescriptorSet> descriptorSets;

	};
}

#endif