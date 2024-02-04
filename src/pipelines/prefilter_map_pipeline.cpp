
#include <array>

#include "../device_cache.h"
#include "../render_passes/cubemap_preprocessing_render_pass.h"
#include "../shader.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "../vertex_types.h"
#include "prefilter_map_pipeline.h"

namespace TANG
{
	PrefilterMapPipeline::PrefilterMapPipeline()
	{
		FlushData();
	}

	PrefilterMapPipeline::~PrefilterMapPipeline()
	{
		FlushData();
	}

	PrefilterMapPipeline::PrefilterMapPipeline(PrefilterMapPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
		other.FlushData();
	}

	// Get references to the data required in Create(), it's not needed
	void PrefilterMapPipeline::SetData(const CubemapPreprocessingRenderPass* _renderPass, const SetLayoutCache* _cubemapSetLayoutCache, const SetLayoutCache* _roughnessSetLayoutCache, VkExtent2D _viewportSize)
	{
		renderPass = _renderPass;
		cubemapSetLayoutCache = _cubemapSetLayoutCache;
		roughnessSetLayoutCache = _roughnessSetLayoutCache;
		viewportSize = _viewportSize;

		wasDataSet = true;
	}

	void PrefilterMapPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create prefilter map pipeline! Create data has not been set correctly");
			return;
		}

		std::vector<VkDescriptorSetLayout> setLayoutArray;
		cubemapSetLayoutCache->FlattenCache(setLayoutArray);
		roughnessSetLayoutCache->FlattenCache(setLayoutArray);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(setLayoutArray.data(), static_cast<uint32_t>(setLayoutArray.size()), nullptr, 0);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create prefilter map pipeline layout!");
			return;
		}

		// Read the compiled shaders
		// NOTE - We reuse the cubemap preprocessing vertex and geometry shaders since we need the exact
		//        same functionality
		Shader vertexShader(ShaderType::CUBEMAP_PREPROCESSING, ShaderStage::VERTEX_SHADER);
		Shader geometryShader(ShaderType::CUBEMAP_PREPROCESSING, ShaderStage::GEOMETRY_SHADER);
		Shader fragmentShader(ShaderType::PREFILTER_MAP, ShaderStage::FRAGMENT_SHADER);

		if (!vertexShader.IsValid() || !geometryShader.IsValid() || !fragmentShader.IsValid())
		{
			LogError("Failed to create prefilter map pipeline. Shader creation failed!");
			return;
		}

		const std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages =
		{
			PopulateShaderCreateInfo(vertexShader),
			PopulateShaderCreateInfo(geometryShader),
			PopulateShaderCreateInfo(fragmentShader)
		};

		// Fill out the rest of the pipeline info
		const std::array<VkDynamicState, 2> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineVertexInputStateCreateInfo		vertexInputInfo			= PopulateVertexInputCreateInfo<CubemapVertex>();
		VkPipelineInputAssemblyStateCreateInfo		inputAssembly			= PopulateInputAssemblyCreateInfo();
		VkViewport									viewport				= PopulateViewportInfo(viewportSize.width, viewportSize.height);
		VkRect2D									scissor					= PopulateScissorInfo(viewportSize);
		VkPipelineDynamicStateCreateInfo			dynamicState			= PopulateDynamicStateCreateInfo(dynamicStates.data(), static_cast<uint32_t>(dynamicStates.size()));
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
			LogError("Failed to create prefilter map pipeline!");
			return;
		}
	}

	PipelineType PrefilterMapPipeline::GetType() const
	{
		return PipelineType::GRAPHICS;
	}

	void PrefilterMapPipeline::FlushData()
	{
		renderPass = nullptr;
		cubemapSetLayoutCache = nullptr;
		roughnessSetLayoutCache = nullptr;
		viewportSize = VkExtent2D();

		wasDataSet = false;
	}
}


