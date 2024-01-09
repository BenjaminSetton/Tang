
#include "../device_cache.h"
#include "../utils/logger.h"
#include "ldr_render_pass.h"

namespace TANG
{
	LDRRenderPass::LDRRenderPass()
	{
		FlushData();
	}

	LDRRenderPass::~LDRRenderPass()
	{
		FlushData();
	}

	LDRRenderPass::LDRRenderPass(LDRRenderPass&& other) noexcept : colorAttachmentFormat(std::move(other.colorAttachmentFormat)), depthAttachmentFormat(std::move(other.depthAttachmentFormat))
	{
	}

	void LDRRenderPass::SetData(VkFormat _colorAttachmentFormat, VkFormat _depthAttachmentFormat)
	{
		colorAttachmentFormat = _colorAttachmentFormat;
		depthAttachmentFormat = _depthAttachmentFormat;

		wasDataSet = true;
	}

	bool LDRRenderPass::Build(RenderPassBuilder& out_builder)
	{
		if (!wasDataSet)
		{
			LogWarning("LDR render pass data has not been set!");
			return false;
		}

		// We're going to use 2 attachment references (no depth buffer necessary)
		out_builder.PreAllocateAttachmentReferences(2);

		VkAttachmentDescription colorAttachmentDesc{};
		colorAttachmentDesc.format = colorAttachmentFormat;
		colorAttachmentDesc.samples = DeviceCache::Get().GetMaxMSAA();
		colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference& colorAttachmentRef = out_builder.GetNextAttachmentReference();
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = colorAttachmentFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT; // NOTE - Resolve attachment doesn't require multi-sampling, that comes from the color attachment instead
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference& colorAttachmentResolveRef = out_builder.GetNextAttachmentReference();
		colorAttachmentResolveRef.attachment = 1;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// Push the objects into the render pass builder
		out_builder.AddAttachment(colorAttachmentDesc)
			.AddAttachment(colorAttachmentResolve)
			.AddSubpass(subpass, &dependency);

		return out_builder.IsValid();
	}

	void LDRRenderPass::FlushData()
	{
		colorAttachmentFormat = VK_FORMAT_UNDEFINED;
		depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		wasDataSet = false;
	}
}