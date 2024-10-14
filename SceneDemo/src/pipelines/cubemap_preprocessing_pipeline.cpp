
#include <array>
#include <vector>

#include "../device_cache.h"
#include "../render_passes/cubemap_preprocessing_render_pass.h"
#include "../shaders/shader.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "../vertex_types.h"
#include "cubemap_preprocessing_pipeline.h"

namespace TANG
{
	CubemapPreprocessingPipeline::CubemapPreprocessingPipeline()
	{
		FlushData();
	}

	CubemapPreprocessingPipeline::~CubemapPreprocessingPipeline()
	{
		FlushData();
	}

	CubemapPreprocessingPipeline::CubemapPreprocessingPipeline(CubemapPreprocessingPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
		other.FlushData();
	}

	// Get references to the data required in Create(), it's not needed
	void CubemapPreprocessingPipeline::SetData(const CubemapPreprocessingRenderPass* _renderPass, const SetLayoutCache* _setLayoutCache, VkExtent2D _viewportSize)
	{
		renderPass = _renderPass;
		setLayoutCache = _setLayoutCache;
		viewportSize = _viewportSize;

		wasDataSet = true;
	}

	void CubemapPreprocessingPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create cubemap preprocessing pipeline! Create data has not been set correctly");
			return;
		}

		std::vector<VkDescriptorSetLayout> setLayoutArray;
		setLayoutCache->FlattenCache(setLayoutArray);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(setLayoutArray.data(), static_cast<uint32_t>(setLayoutArray.size()), nullptr, 0);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create cubemap preprocessing pipeline layout!");
			return;
		}

		// Read the compiled shaders
		Shader vertexShader(ShaderType::CUBEMAP_PREPROCESSING, ShaderStage::VERTEX_SHADER);
		Shader geometryShader(ShaderType::CUBEMAP_PREPROCESSING, ShaderStage::GEOMETRY_SHADER);
		Shader fragmentShader(ShaderType::CUBEMAP_PREPROCESSING, ShaderStage::FRAGMENT_SHADER);

		if (!vertexShader.IsValid() || !geometryShader.IsValid() || !fragmentShader.IsValid())
		{
			LogError("Failed to create cubemap preprocessing pipeline. Shader creation failed!");
			return;
		}

		const std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages =
		{
			PopulateShaderCreateInfo(vertexShader),
			PopulateShaderCreateInfo(geometryShader),
			PopulateShaderCreateInfo(fragmentShader)
		};

		// Fill out the rest of the pipeline info
		VkPipelineVertexInputStateCreateInfo		vertexInputInfo			= PopulateVertexInputCreateInfo<CubemapVertex>();
		VkPipelineInputAssemblyStateCreateInfo		inputAssembly			= PopulateInputAssemblyCreateInfo();
		VkViewport									viewport				= PopulateViewportInfo(viewportSize.width, viewportSize.height);
		VkRect2D									scissor					= PopulateScissorInfo(viewportSize);
		VkPipelineDynamicStateCreateInfo			dynamicState			= PopulateDynamicStateCreateInfo();
		VkPipelineViewportStateCreateInfo			viewportState			= PopulateViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo		rasterizer				= PopulateRasterizerStateCreateInfo(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineMultisampleStateCreateInfo		multisampling			= PopulateMultisamplingStateCreateInfo();
		VkPipelineColorBlendAttachmentState			colorBlendAttachment	= PopulateColorBlendAttachment();
		VkPipelineColorBlendStateCreateInfo			colorBlending			= PopulateColorBlendStateCreateInfo(&colorBlendAttachment, 1);
		VkPipelineDepthStencilStateCreateInfo		depthStencil			= PopulateDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

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
			LogError("Failed to create cubemap preprocessing pipeline!");
			return;
		}
	}

	PipelineType CubemapPreprocessingPipeline::GetType() const
	{
		return PipelineType::GRAPHICS;
	}

	void CubemapPreprocessingPipeline::FlushData()
	{
		renderPass = nullptr;
		setLayoutCache = nullptr;
		viewportSize = VkExtent2D();

		wasDataSet = false;
	}
}


