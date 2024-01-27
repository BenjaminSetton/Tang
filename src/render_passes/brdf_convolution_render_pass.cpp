
#include "../utils/logger.h"
#include "brdf_convolution_render_pass.h"

namespace TANG
{
	BRDFConvolutionRenderPass::BRDFConvolutionRenderPass()
	{
		FlushData();
	}

	BRDFConvolutionRenderPass::~BRDFConvolutionRenderPass()
	{
		FlushData();
	}

	BRDFConvolutionRenderPass::BRDFConvolutionRenderPass(BRDFConvolutionRenderPass&& other) noexcept : colorAttachmentFormat(std::move(other.colorAttachmentFormat))
	{
	}

	void BRDFConvolutionRenderPass::SetData(VkFormat _colorAttachmentFormat)
	{
		colorAttachmentFormat = _colorAttachmentFormat;

		wasDataSet = true;
	}

	bool BRDFConvolutionRenderPass::Build(RenderPassBuilder& out_builder)
	{
		if (!wasDataSet)
		{
			LogWarning("BRDF convolution render pass data has not been set!");
			return false;
		}

		out_builder.PreAllocateAttachmentReferences(1);

		VkAttachmentDescription colorAttachmentDesc{};
		colorAttachmentDesc.format = colorAttachmentFormat;
		colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // HACK! We don't want the validation layes to complain when we manually insert a pipeline barrier to transition to SHADER_READ_ONLY

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

	void BRDFConvolutionRenderPass::FlushData()
	{
		colorAttachmentFormat = VK_FORMAT_UNDEFINED;
		wasDataSet = false;
	}
}