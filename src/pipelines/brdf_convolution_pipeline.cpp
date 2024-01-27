
#include <array>

#include "../render_passes/brdf_convolution_render_pass.h"
#include "../shader.h"
#include "../utils/logger.h"
#include "../vertex_types.h"
#include "brdf_convolution_pipeline.h"

namespace TANG
{
	BRDFConvolutionPipeline::BRDFConvolutionPipeline()
	{
		FlushData();
	}

	BRDFConvolutionPipeline::~BRDFConvolutionPipeline()
	{
		FlushData();
	}

	BRDFConvolutionPipeline::BRDFConvolutionPipeline(BRDFConvolutionPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
	}

	// Get references to the data required in Create(), it's not needed
	void BRDFConvolutionPipeline::SetData(const BRDFConvolutionRenderPass* _renderPass, VkExtent2D _viewportSize)
	{
		renderPass = _renderPass;
		viewportSize = _viewportSize;

		wasDataSet = true;
	}

	void BRDFConvolutionPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create BRDF convolution pipeline! Create data has not been set correctly");
			return;
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(nullptr, 0, nullptr, 0);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create pipeline layout for BRDF convolution pipeline!");
			return;
		}

		// Read the compiled shaders
		Shader vertexShader(ShaderType::FULLSCREEN_QUAD, ShaderStage::VERTEX_SHADER);
		Shader fragmentShader(ShaderType::BRDF_CONVOLUTION, ShaderStage::FRAGMENT_SHADER);

		if (!vertexShader.IsValid() || !fragmentShader.IsValid())
		{
			LogError("Failed to create BRDF convolution pipeline. Shader creation failed!");
			return;
		}

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
		{
			PopulateShaderCreateInfo(vertexShader),
			PopulateShaderCreateInfo(fragmentShader)
		};

		// Fill out the rest of the pipeline info
		VkPipelineVertexInputStateCreateInfo		vertexInputInfo			= PopulateVertexInputCreateInfo<UVVertex>();
		VkPipelineInputAssemblyStateCreateInfo		inputAssembly			= PopulateInputAssemblyCreateInfo();
		VkViewport									viewport				= PopulateViewportInfo(viewportSize.width, viewportSize.height);
		VkRect2D									scissor					= PopulateScissorInfo(viewportSize);
		VkPipelineDynamicStateCreateInfo			dynamicState			= PopulateDynamicStateCreateInfo();
		VkPipelineViewportStateCreateInfo			viewportState			= PopulateViewportStateCreateInfo();
		VkPipelineRasterizationStateCreateInfo		rasterizer				= PopulateRasterizerStateCreateInfo(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineMultisampleStateCreateInfo		multisampling			= PopulateMultisamplingStateCreateInfo();
		VkPipelineColorBlendAttachmentState			colorBlendAttachment	= PopulateColorBlendAttachment();
		VkPipelineColorBlendStateCreateInfo			colorBlending			= PopulateColorBlendStateCreateInfo(&colorBlendAttachment, 1);
		VkPipelineDepthStencilStateCreateInfo		depthStencil			= PopulateDepthStencilStateCreateInfo();

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
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// Optional
		pipelineInfo.basePipelineIndex = -1;				// Optional

		if (!CreatePipelineObject(pipelineInfo))
		{
			LogError("Failed to create BRDF convolution pipeline!");
			return;
		}
	}

	void BRDFConvolutionPipeline::FlushData()
	{
		renderPass = nullptr;
		viewportSize = VkExtent2D();

		wasDataSet = false;
	}
}