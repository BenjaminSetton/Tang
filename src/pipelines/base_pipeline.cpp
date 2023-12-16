
#include "base_pipeline.h"

namespace TANG
{
	BasePipeline::BasePipeline() : pipelineObject(VK_NULL_HANDLE)
	{ }

	BasePipeline::~BasePipeline()
	{ }

	BasePipeline::BasePipeline(BasePipeline&& other) noexcept : pipelineObject(std::move(other.pipelineObject))
	{ }

	void BasePipeline::Destroy(VkDevice logicalDevice)
	{
		if(pipelineObject) vkDestroyPipeline(logicalDevice, pipelineObject, nullptr);
		if(pipelineLayout) vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	}

	VkPipeline BasePipeline::GetPipeline() const
	{
		return pipelineObject;
	}

	VkPipelineLayout BasePipeline::GetPipelineLayout() const
	{
		return pipelineLayout;
	}

	bool BasePipeline::CreatePipelineObject(VkDevice logicalDevice, const VkGraphicsPipelineCreateInfo& pipelineCreateInfo)
	{
		return (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineObject) == VK_SUCCESS);
	}

	bool BasePipeline::CreatePipelineLayout(VkDevice logicalDevice, const VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo)
	{
		return (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) == VK_SUCCESS);
	}
}