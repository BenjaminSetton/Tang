
#include <array>

#include "../render_passes/hdr_render_pass.h"
#include "../shader.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"
#include "../vertex_types.h"
#include "skybox_pipeline.h"

namespace TANG
{
	SkyboxPipeline::SkyboxPipeline()
	{
		FlushData();
	}

	SkyboxPipeline::~SkyboxPipeline()
	{
		FlushData();
	}

	SkyboxPipeline::SkyboxPipeline(SkyboxPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
		other.FlushData();
	}

	// Get references to the data required in Create(), it's not needed
	void SkyboxPipeline::SetData(const HDRRenderPass* _renderPass, const SetLayoutCache* _setLayoutCache, VkExtent2D _viewportSize)
	{
		renderPass = _renderPass;
		setLayoutCache = _setLayoutCache;
		viewportSize = _viewportSize;

		wasDataSet = true;
	}

	void SkyboxPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create skybox pipeline! Create data has not been set correctly");
			return;
		}

		std::vector<VkDescriptorSetLayout> setLayoutArray;
		setLayoutCache->FlattenCache(setLayoutArray);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(setLayoutArray.data(), static_cast<uint32_t>(setLayoutArray.size()), nullptr, 0);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create skybox pipeline layout!");
		}

		// Read the compiled shaders
		Shader vertexShader(ShaderType::SKYBOX, ShaderStage::VERTEX_SHADER);
		Shader fragmentShader(ShaderType::SKYBOX, ShaderStage::FRAGMENT_SHADER);

		if (!vertexShader.IsValid() || !fragmentShader.IsValid())
		{
			LogError("Failed to create skybox pipeline. Shader creation failed!");
			return;
		}

		const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = 
		{ 
			PopulateShaderCreateInfo(vertexShader),
			PopulateShaderCreateInfo(fragmentShader)
		};

		// We're declaring these as dynamic states, meaning we can change
		// them at any point. Usually the pipeline states in Vulkan are static,
		// meaning a pipeline is created and never changed. This allows
		// the GPU to heavily optimize for the pipelines defined. In this
		// case though, we face a negligible penalty for making these dynamic.
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
		VkPipelineRasterizationStateCreateInfo		rasterizer				= PopulateRasterizerStateCreateInfo(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
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
			LogError("Failed to create skybox pipeline!");
			return;
		}
	}

	PipelineType SkyboxPipeline::GetType() const
	{
		return PipelineType::GRAPHICS;
	}

	void SkyboxPipeline::FlushData()
	{
		renderPass = nullptr;
		setLayoutCache = nullptr;
		viewportSize = VkExtent2D();

		wasDataSet = false;
	}
}


