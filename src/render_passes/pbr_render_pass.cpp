//
//#include "../device_cache.h"
//#include "../utils/logger.h"
//#include "pbr_render_pass.h"
//
//namespace TANG
//{
//	PBRRenderPass::PBRRenderPass()
//	{
//		FlushData();
//	}
//
//	PBRRenderPass::~PBRRenderPass()
//	{
//		FlushData();
//	}
//
//	PBRRenderPass::PBRRenderPass(PBRRenderPass&& other) noexcept : colorAttachmentFormat(std::move(other.colorAttachmentFormat)), depthAttachmentFormat(std::move(other.depthAttachmentFormat))
//	{
//	}
//
//	void PBRRenderPass::SetData(VkFormat _colorAttachmentFormat, VkFormat _depthAttachmentFormat)
//	{
//		colorAttachmentFormat = _colorAttachmentFormat;
//		depthAttachmentFormat = _depthAttachmentFormat;
//
//		wasDataSet = true;
//	}
//
//	bool PBRRenderPass::Build(RenderPassBuilder& out_builder)
//	{
//		if (!wasDataSet)
//		{
//			LogWarning("PBR render pass data has not been set!");
//			return false;
//		}
//
//		// We're going to use 3 attachment references
//		out_builder.PreAllocateAttachmentReferences(3);
//
//		VkAttachmentDescription colorAttachmentDesc{};
//		colorAttachmentDesc.format = colorAttachmentFormat;
//		colorAttachmentDesc.samples = DeviceCache::Get().GetMaxMSAA();
//		colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//		colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//		colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//		colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//		colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//		VkAttachmentReference& colorAttachmentRef = out_builder.GetNextAttachmentReference();
//		colorAttachmentRef.attachment = 0;
//		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//		VkAttachmentDescription depthAttachmentDesc{};
//		depthAttachmentDesc.format = depthAttachmentFormat;
//		depthAttachmentDesc.samples = DeviceCache::Get().GetMaxMSAA();
//		depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//		depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//		depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//		depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//		depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//		depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//		VkAttachmentReference& depthAttachmentRef = out_builder.GetNextAttachmentReference();
//		depthAttachmentRef.attachment = 1;
//		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//		VkAttachmentDescription colorAttachmentResolve{};
//		colorAttachmentResolve.format = colorAttachmentFormat;
//		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
//		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//		VkAttachmentReference& colorAttachmentResolveRef = out_builder.GetNextAttachmentReference();
//		colorAttachmentResolveRef.attachment = 2;
//		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//		VkSubpassDescription subpass{};
//		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//		subpass.colorAttachmentCount = 1;
//		subpass.pColorAttachments = &colorAttachmentRef;
//		subpass.pDepthStencilAttachment = &depthAttachmentRef;
//		subpass.pResolveAttachments = &colorAttachmentResolveRef;
//
//		VkSubpassDependency dependency{};
//		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//		dependency.dstSubpass = 0;
//		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//
//		// Push the objects into the render pass builder
//		out_builder.AddAttachment(colorAttachmentDesc)
//				   .AddAttachment(depthAttachmentDesc)
//				   .AddAttachment(colorAttachmentResolve)
//				   .AddSubpass(subpass, dependency);
//
//		return out_builder.IsValid();
//	}
//
//	void PBRRenderPass::FlushData()
//	{
//		colorAttachmentFormat = VK_FORMAT_UNDEFINED;
//		depthAttachmentFormat = VK_FORMAT_UNDEFINED;
//		wasDataSet = false;
//	}
//}