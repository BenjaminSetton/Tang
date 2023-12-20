
#include "../device_cache.h"
#include "../utils/logger.h"
#include "../utils/sanity_check.h"
#include "base_render_pass.h"

namespace TANG
{

	BaseRenderPass::BaseRenderPass() : renderPass(VK_NULL_HANDLE), wasDataSet(false)
	{
	}

	BaseRenderPass::~BaseRenderPass()
	{
		if (renderPass != VK_NULL_HANDLE)
		{
			LogWarning("Render pass destructor has been called, but render pass object has not been destroyed!");
		}
	}

	BaseRenderPass::BaseRenderPass(BaseRenderPass&& other) noexcept
	{
		renderPass = other.renderPass;
		wasDataSet = other.wasDataSet;

		other.renderPass = VK_NULL_HANDLE;
		other.wasDataSet = false;
	}

	void BaseRenderPass::Create()
	{
		RenderPassBuilder builder;
		if (Build(builder))
		{
			Create_Internal(builder);
		}
		else
		{
			LogError("Invalid render pass builder! Render pass object will not be created");
		}
	}

	void BaseRenderPass::Destroy()
	{
		vkDestroyRenderPass(GetLogicalDevice(), renderPass, nullptr);
		renderPass = VK_NULL_HANDLE;
	}

	VkRenderPass BaseRenderPass::GetRenderPass() const
	{
		return renderPass;
	}

	void BaseRenderPass::Create_Internal(const RenderPassBuilder& builder)
	{
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(builder.attachmentDescriptions.size());
		renderPassInfo.pAttachments = builder.attachmentDescriptions.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(builder.subpassDescriptions.size());
		renderPassInfo.pSubpasses = builder.subpassDescriptions.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(builder.subpassDependencies.size());
		renderPassInfo.pDependencies = builder.subpassDependencies.data();

		if (vkCreateRenderPass(GetLogicalDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create render pass!");
		}
	}

	RenderPassBuilder& RenderPassBuilder::AddAttachment(const VkAttachmentDescription& attachmentDesc)
	{
		attachmentDescriptions.push_back(attachmentDesc);
		return *this;
	}

	RenderPassBuilder& RenderPassBuilder::AddSubpass(const VkSubpassDescription& subpassDesc, const VkSubpassDependency& subpassDep)
	{
		subpassDescriptions.push_back(subpassDesc);
		subpassDependencies.push_back(subpassDep);
		return *this;
	}

	bool RenderPassBuilder::IsValid() const
	{
		bool subpassesEqual = subpassDescriptions.size() == subpassDependencies.size();
		bool attachmentsEqual = attachmentDescriptions.size() == attachmentReferences.size();

		// TODO - Add any necessary checks
		return subpassesEqual && attachmentsEqual;
	}

	void RenderPassBuilder::PreAllocateAttachmentReferences(uint32_t numberOfAttachmentRefs)
	{
		attachmentReferences.reserve(numberOfAttachmentRefs);
	}

	VkAttachmentReference& RenderPassBuilder::GetNextAttachmentReference()
	{
		if (attachmentReferences.size() + 1 > attachmentReferences.capacity())
		{
			LogError("Exceeded number of allocated attachment references in render pass builder. This will most likely cause a crash!");
		}

		attachmentReferences.push_back({});
		return attachmentReferences.at(attachmentReferences.size() - 1);
	}
}