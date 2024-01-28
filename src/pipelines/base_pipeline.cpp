
#include "../device_cache.h"
#include "../shader.h"
#include "../utils/sanity_check.h"
#include "base_pipeline.h"

namespace TANG
{
	BasePipeline::BasePipeline() : pipelineObject(VK_NULL_HANDLE), wasDataSet(false)
	{ }

	BasePipeline::~BasePipeline()
	{ }

	BasePipeline::BasePipeline(BasePipeline&& other) noexcept : pipelineObject(std::move(other.pipelineObject)), wasDataSet(std::move(other.wasDataSet))
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

	VkPipelineInputAssemblyStateCreateInfo BasePipeline::PopulateInputAssemblyCreateInfo(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable) const
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = topology;
		inputAssembly.primitiveRestartEnable = primitiveRestartEnable;

		return inputAssembly;
	}

	VkPipelineDynamicStateCreateInfo BasePipeline::PopulateDynamicStateCreateInfo(const VkDynamicState* dynamicStates, uint32_t dynamicStateCount) const
	{
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = dynamicStateCount;
		dynamicState.pDynamicStates = dynamicStates;

		return dynamicState;
	}

	VkPipelineViewportStateCreateInfo BasePipeline::PopulateViewportStateCreateInfo(const VkViewport* viewports, uint32_t viewportCount, const VkRect2D* scissors, uint32_t scissorCount) const
	{
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pViewports = viewports;
		viewportState.viewportCount = viewportCount;
		viewportState.pScissors = scissors;
		viewportState.scissorCount = scissorCount;

		return viewportState;
	}

	VkPipelineRasterizationStateCreateInfo BasePipeline::PopulateRasterizerStateCreateInfo(VkCullModeFlags cullMode, VkFrontFace windingOrder)
	{
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = cullMode;
		rasterizer.frontFace = windingOrder;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;	// Optional
		rasterizer.depthBiasClamp = 0.0f;			// Optional
		rasterizer.depthBiasSlopeFactor = 0.0f;		// Optional

		return rasterizer;
	}

	VkPipelineMultisampleStateCreateInfo BasePipeline::PopulateMultisamplingStateCreateInfo(VkSampleCountFlagBits sampleCount)
	{
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = sampleCount;
		multisampling.minSampleShading = 1.0f;				// Optional
		multisampling.pSampleMask = nullptr;				// Optional
		multisampling.alphaToCoverageEnable = VK_FALSE;		// Optional
		multisampling.alphaToOneEnable = VK_FALSE;			// Optional

		return multisampling;
	}

	VkPipelineColorBlendAttachmentState BasePipeline::PopulateColorBlendAttachment()
	{
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	// Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;				// Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	// Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;				// Optional

		return colorBlendAttachment;
	}

	VkPipelineColorBlendStateCreateInfo BasePipeline::PopulateColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState* attachments, uint32_t blendAttachmentCount)
	{
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;				// Optional
		colorBlending.attachmentCount = blendAttachmentCount;
		colorBlending.pAttachments = attachments;
		colorBlending.blendConstants[0] = 0.0f;					// Optional
		colorBlending.blendConstants[1] = 0.0f;					// Optional
		colorBlending.blendConstants[2] = 0.0f;					// Optional
		colorBlending.blendConstants[3] = 0.0f;					// Optional

		return colorBlending;
	}

	VkPipelineDepthStencilStateCreateInfo BasePipeline::PopulateDepthStencilStateCreateInfo(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp compareOp, VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable)
	{
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = depthTestEnable;
		depthStencil.depthWriteEnable = depthWriteEnable;
		depthStencil.depthCompareOp = compareOp;
		depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
		depthStencil.minDepthBounds = 0.0f;				// Optional
		depthStencil.maxDepthBounds = 1.0f;				// Optional
		depthStencil.stencilTestEnable = stencilTestEnable;
		depthStencil.front = {};						// Optional
		depthStencil.back = {};							// Optional

		return depthStencil;
	}

	VkPipelineLayoutCreateInfo BasePipeline::PopulatePipelineLayoutCreateInfo(const VkDescriptorSetLayout* setLayouts, uint32_t setLayoutCount, const VkPushConstantRange* pushConstantRanges, uint32_t pushConstantCount)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = setLayoutCount;
		pipelineLayoutInfo.pSetLayouts = setLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantCount;
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;

		return pipelineLayoutInfo;
	}

	VkPipelineShaderStageCreateInfo BasePipeline::PopulateShaderCreateInfo(const Shader& shader) const
	{
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_ALL;
		switch (shader.GetShaderStage())
		{
		case ShaderStage::VERTEX_SHADER:
		{
			shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		}
		case ShaderStage::GEOMETRY_SHADER:
		{
			shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		}
		case ShaderStage::FRAGMENT_SHADER:
		{
			shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		}
		default:
		{
			TNG_ASSERT_MSG(false, "Failed to populate shader create info. Invalid shader stage!");
			return {};
		}
		}

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = shaderStage;
		shaderStageInfo.module = shader.GetShaderObject();
		shaderStageInfo.pName = "main";

		return shaderStageInfo;
	}

	VkViewport BasePipeline::PopulateViewportInfo(uint32_t width, uint32_t height) const
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(width);
		viewport.height = static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		return viewport;
	}

	VkRect2D BasePipeline::PopulateScissorInfo(VkExtent2D viewportSize) const
	{
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = viewportSize;

		return scissor;
	}
}