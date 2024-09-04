
#include "../shaders/shader.h"
#include "../utils/logger.h"
#include "bloom_composition_pipeline.h"

namespace TANG
{

	BloomCompositionPipeline::BloomCompositionPipeline() : BasePipeline()
	{
		FlushData();
	}

	BloomCompositionPipeline::~BloomCompositionPipeline()
	{
		FlushData();
	}

	BloomCompositionPipeline::BloomCompositionPipeline(BloomCompositionPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
		other.FlushData();
	}

	void BloomCompositionPipeline::SetData(const SetLayoutCache* _setLayoutCache)
	{
		setLayoutCache = _setLayoutCache;

		wasDataSet = true;
	}

	void BloomCompositionPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create bloom composition pipeline! Create data has not been set correctly");
			return;
		}

		std::vector<VkDescriptorSetLayout> setLayoutArray;
		setLayoutCache->FlattenCache(setLayoutArray);

		// Bloom intensity
		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(glm::vec2);
		pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(setLayoutArray.data(), static_cast<uint32_t>(setLayoutArray.size()), &pushConstant, 1);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create bloom composition pipeline layout!");
			return;
		}

		Shader compShader(ShaderType::BLOOM_COMPOSITION, ShaderStage::COMPUTE_SHADER);
		if (!compShader.IsValid())
		{
			LogError("Failed to create bloom composition pipeline. Shader creation failed!");
			return;
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = GetPipelineLayout();
		pipelineInfo.stage = PopulateShaderCreateInfo(compShader);

		if (!CreateComputePipelineObject(pipelineInfo))
		{
			LogError("Failed to create bloom composition pipeline!");
		}
	}

	PipelineType BloomCompositionPipeline::GetType() const
	{
		return PipelineType::COMPUTE;
	}

	void BloomCompositionPipeline::FlushData()
	{
		setLayoutCache = nullptr;

		wasDataSet = false;
	}


}