
#include <array>

#include "TANG/device_cache.h"
#include "TANG/logger.h"
#include "cubemap_preprocessing_render_pass.h"

namespace TANG
{
	CubemapPreprocessingRenderPass::CubemapPreprocessingRenderPass()
	{
		FlushData();
	}

	CubemapPreprocessingRenderPass::~CubemapPreprocessingRenderPass()
	{
		FlushData();
	}

	CubemapPreprocessingRenderPass::CubemapPreprocessingRenderPass(CubemapPreprocessingRenderPass&& other) noexcept : BaseRenderPass(std::move(other))
	{
	}

	bool CubemapPreprocessingRenderPass::Build(RenderPassBuilder& out_builder)
	{
		out_builder.PreAllocateAttachmentReferences(1);

		VkAttachmentDescription colorAttachmentDesc{};
		colorAttachmentDesc.format = VK_FORMAT_R32G32B32A32_SFLOAT; // Using 32-bit float components for cubemap faces
		colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference& colorAttachmentRef = out_builder.GetNextAttachmentReference();
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.pResolveAttachments = nullptr;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// Push the objects into the render pass builder
		out_builder.AddAttachment(colorAttachmentDesc)
				   .AddSubpass(subpass, &dependency);

		return out_builder.IsValid();
	}

	void CubemapPreprocessingRenderPass::FlushData()
	{
		wasDataSet = true; // No external data is necessary
	}
}