
#include "../shader.h"
#include "../utils/logger.h"
#include "bloom_prefilter_pipeline.h"

namespace TANG
{

	BloomPrefilterPipeline::BloomPrefilterPipeline() : BasePipeline()
	{
		FlushData();
	}

	BloomPrefilterPipeline::~BloomPrefilterPipeline()
	{
		FlushData();
	}

	BloomPrefilterPipeline::BloomPrefilterPipeline(BloomPrefilterPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
		other.FlushData();
	}

	void BloomPrefilterPipeline::SetData(const SetLayoutCache* _setLayoutCache)
	{
		setLayoutCache = _setLayoutCache;

		wasDataSet = true;
	}

	void BloomPrefilterPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create bloom prefilter pipeline! Create data has not been set correctly");
			return;
		}

		std::vector<VkDescriptorSetLayout> setLayoutArray;
		setLayoutCache->FlattenCache(setLayoutArray);

		// Bloom brightness threshold
		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(float);
		pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(setLayoutArray.data(), static_cast<uint32_t>(setLayoutArray.size()), &pushConstant, 1);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create bloom prefilter pipeline layout!");
			return;
		}

		Shader compShader(ShaderType::BLOOM_PREFILTER, ShaderStage::COMPUTE_SHADER);
		if (!compShader.IsValid())
		{
			LogError("Failed to create bloom prefilter pipeline. Shader creation failed!");
			return;
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = GetPipelineLayout();
		pipelineInfo.stage = PopulateShaderCreateInfo(compShader);

		if (!CreateComputePipelineObject(pipelineInfo))
		{
			LogError("Failed to create bloom prefilter pipeline!");
		}
	}

	PipelineType BloomPrefilterPipeline::GetType() const
	{
		return PipelineType::COMPUTE;
	}

	void BloomPrefilterPipeline::FlushData()
	{
		setLayoutCache = nullptr;

		wasDataSet = false;
	}


}