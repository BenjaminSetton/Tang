
#include <array>

#include "../render_passes/hdr_render_pass.h"
#include "../shaders/shader.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "../vertex_types.h"
#include "pbr_pipeline.h"

namespace TANG
{
	PBRPipeline::PBRPipeline()
	{
		FlushData();
	}

	PBRPipeline::~PBRPipeline()
	{
		FlushData();
	}

	PBRPipeline::PBRPipeline(PBRPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
		other.FlushData();
	}

	// Get references to the data required in Create(), it's not needed
	void PBRPipeline::SetData(const HDRRenderPass* _renderPass, const SetLayoutCache* _setLayoutCache, VkExtent2D _viewportSize)
	{
		renderPass = _renderPass;
		setLayoutCache = _setLayoutCache;
		viewportSize = _viewportSize;

		wasDataSet = true;
	}

	void PBRPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create PBR pipeline! Create data has not been set correctly");
			return;
		}

		std::vector<VkDescriptorSetLayout> setLayoutArray;
		setLayoutCache->FlattenCache(setLayoutArray);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(setLayoutArray.data(), static_cast<uint32_t>(setLayoutArray.size()), nullptr, 0);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create PBR pipeline layout!");
			return;
		}

		// Read the compiled shaders
		Shader vertexShader(ShaderType::PBR, ShaderStage::VERTEX_SHADER);
		Shader fragmentShader(ShaderType::PBR, ShaderStage::FRAGMENT_SHADER);

		if (!vertexShader.IsValid() || !fragmentShader.IsValid())
		{
			LogError("Failed to create PBR pipeline. Shader creation failed!");
			return;
		}

		const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = 
		{ 
			PopulateShaderCreateInfo(vertexShader),
			PopulateShaderCreateInfo(fragmentShader)
		};

		// Fill out the rest of the pipeline info
		const std::array<VkDynamicState, 2> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineVertexInputStateCreateInfo		vertexInputInfo			= PopulateVertexInputCreateInfo<PBRVertex>();
		VkPipelineInputAssemblyStateCreateInfo		inputAssembly			= PopulateInputAssemblyCreateInfo();
		VkViewport									viewport				= PopulateViewportInfo(viewportSize.width, viewportSize.height);
		VkRect2D									scissor					= PopulateScissorInfo(viewportSize);
		VkPipelineDynamicStateCreateInfo			dynamicState			= PopulateDynamicStateCreateInfo(dynamicStates.data(), static_cast<uint32_t>(dynamicStates.size()));
		VkPipelineViewportStateCreateInfo			viewportState			= PopulateViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo		rasterizer				= PopulateRasterizerStateCreateInfo(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineMultisampleStateCreateInfo		multisampling			= PopulateMultisamplingStateCreateInfo();
		VkPipelineColorBlendAttachmentState			colorBlendAttachment	= PopulateColorBlendAttachment();
		VkPipelineColorBlendStateCreateInfo			colorBlending			= PopulateColorBlendStateCreateInfo(&colorBlendAttachment, 1);
		VkPipelineDepthStencilStateCreateInfo		depthStencil			= PopulateDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE);

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = GetPipelineLayout();
		pipelineInfo.renderPass = renderPass->GetRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (!CreateGraphicsPipelineObject(pipelineInfo))
		{
			LogError("Failed to create PBR pipeline!");
			return;
		}
	}

	PipelineType PBRPipeline::GetType() const
	{
		return PipelineType::GRAPHICS;
	}

	void PBRPipeline::FlushData()
	{
		renderPass = nullptr;
		setLayoutCache = nullptr;
		viewportSize = VkExtent2D();

		wasDataSet = false;
	}
}


