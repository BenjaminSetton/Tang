
#include "../device_cache.h"
#include "base_pipeline.h"

namespace TANG
{
	BasePipeline::BasePipeline() : pipelineObject(VK_NULL_HANDLE)
	{ }

	BasePipeline::~BasePipeline()
	{ }

	BasePipeline::BasePipeline(BasePipeline&& other) noexcept : pipelineObject(std::move(other.pipelineObject))
	{ }

	void BasePipeline::Destroy()
	{
		VkDevice logicalDevice = GetLogicalDevice();

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

	bool BasePipeline::CreatePipelineObject(const VkGraphicsPipelineCreateInfo& pipelineCreateInfo)
	{
		return (vkCreateGraphicsPipelines(GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineObject) == VK_SUCCESS);
	}

	bool BasePipeline::CreatePipelineLayout(const VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo)
	{
		return (vkCreatePipelineLayout(GetLogicalDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) == VK_SUCCESS);
	}
}