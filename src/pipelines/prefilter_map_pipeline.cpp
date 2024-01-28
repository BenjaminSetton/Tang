
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
			LogError("Failed to create cubemap preprocessing pipeline! Create data has not been set correctly");
			return;
		}

		// Read the compiled shaders
		// NOTE - We reuse the cubemap preprocessing vertex and geometry shaders since we need the exact
		//        same functionality
		Shader vertexShader(ShaderType::CUBEMAP_PREPROCESSING, ShaderStage::VERTEX_SHADER);
		Shader geometryShader(ShaderType::CUBEMAP_PREPROCESSING, ShaderStage::GEOMETRY_SHADER);
		Shader fragmentShader(ShaderType::PREFILTER_MAP, ShaderStage::FRAGMENT_SHADER);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShader.GetShaderObject();
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo geoShaderStageInfo{};
		geoShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geoShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geoShaderStageInfo.module = geometryShader.GetShaderObject();
		geoShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragmentShader.GetShaderObject();
		fragShaderStageInfo.pName = "main";

		std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages =
		{
			vertShaderStageInfo,
			geoShaderStageInfo,
			fragShaderStageInfo
		};

		// Vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

		auto bindingDescription = CubemapVertex::GetBindingDescription();
		auto attributeDescriptions = CubemapVertex::GetAttributeDescriptions();
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// Input assembler
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// Viewports and scissors
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(viewportSize.width);
		viewport.height = static_cast<float>(viewportSize.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = viewportSize;

		// We're declaring these as dynamic states, meaning we can change
		// them at any point. Usually the pipeline states in Vulkan are static,
		// meaning a pipeline is created and never changed. This allows
		// the GPU to heavily optimize for the pipelines defined. In this
		// case though, we face a negligible penalty for making these dynamic.
		std::array<VkDynamicState, 2> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		// Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		// Depth stencil
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		// Pipeline layout
		std::vector<VkDescriptorSetLayout> vkDescSetLayouts;

		for (uint32_t i = 0; i < cubemapSetLayoutCache->GetLayoutCount(); i++)
		{
			vkDescSetLayouts.push_back(cubemapSetLayoutCache->GetSetLayout(i).value().GetLayout());
		}

		for (uint32_t i = 0; i < roughnessSetLayoutCache->GetLayoutCount(); i++)
		{
			vkDescSetLayouts.push_back(roughnessSetLayoutCache->GetSetLayout(i).value().GetLayout());
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkDescSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = vkDescSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			TNG_ASSERT_MSG(false, "Failed to create pipeline layout!");
		}

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

		if (!CreatePipelineObject(pipelineInfo))
		{
			TNG_ASSERT_MSG(false, "Failed to create prefilter map pipeline!");
		}
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


