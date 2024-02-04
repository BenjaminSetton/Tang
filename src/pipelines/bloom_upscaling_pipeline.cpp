
#include "../shader.h"
#include "../utils/logger.h"
#include "bloom_upscaling_pipeline.h"

namespace TANG
{

	BloomUpscalingPipeline::BloomUpscalingPipeline() : BasePipeline()
	{
		FlushData();
	}

	BloomUpscalingPipeline::~BloomUpscalingPipeline()
	{ 
		FlushData();
	}

	BloomUpscalingPipeline::BloomUpscalingPipeline(BloomUpscalingPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
		other.FlushData();
	}

	void BloomUpscalingPipeline::SetData(const SetLayoutCache* _setLayoutCache)
	{
		setLayoutCache = _setLayoutCache;

		wasDataSet = true;
	}

	void BloomUpscalingPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create bloom upscaling pipeline! Create data has not been set correctly");
			return;
		}

		std::vector<VkDescriptorSetLayout> setLayoutArray;
		setLayoutCache->FlattenCache(setLayoutArray);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(setLayoutArray.data(), static_cast<uint32_t>(setLayoutArray.size()), nullptr, 0);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create bloom upscaling pipeline layout!");
			return;
		}

		Shader compShader(ShaderType::BLOOM_UPSCALING, ShaderStage::COMPUTE_SHADER);
		if (!compShader.IsValid())
		{
			LogError("Failed to create bloom upscaling pipeline. Shader creation failed!");
			return;
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = GetPipelineLayout();
		pipelineInfo.stage = PopulateShaderCreateInfo(compShader);

		if (!CreateComputePipelineObject(pipelineInfo))
		{
			LogError("Failed to create bloom upscaling pipeline!");
		}
	}

	PipelineType BloomUpscalingPipeline::GetType() const
	{
		return PipelineType::COMPUTE;
	}

	void BloomUpscalingPipeline::FlushData()
	{
		setLayoutCache = nullptr;

		wasDataSet = false;
	}


}