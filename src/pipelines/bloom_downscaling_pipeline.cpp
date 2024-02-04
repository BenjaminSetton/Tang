
#include "../shader.h"
#include "../utils/logger.h"
#include "bloom_downscaling_pipeline.h"

namespace TANG
{

	BloomDownscalingPipeline::BloomDownscalingPipeline() : BasePipeline()
	{
		FlushData();
	}

	BloomDownscalingPipeline::~BloomDownscalingPipeline()
	{
		FlushData();
	}

	BloomDownscalingPipeline::BloomDownscalingPipeline(BloomDownscalingPipeline&& other) noexcept : BasePipeline(std::move(other))
	{
		other.FlushData();
	}

	void BloomDownscalingPipeline::SetData(const SetLayoutCache* _setLayoutCache)
	{
		setLayoutCache = _setLayoutCache;

		wasDataSet = true;
	}

	void BloomDownscalingPipeline::Create()
	{
		if (!wasDataSet)
		{
			LogError("Failed to create bloom downscaling pipeline! Create data has not been set correctly");
			return;
		}

		std::vector<VkDescriptorSetLayout> setLayoutArray;
		setLayoutCache->FlattenCache(setLayoutArray);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = PopulatePipelineLayoutCreateInfo(setLayoutArray.data(), static_cast<uint32_t>(setLayoutArray.size()), nullptr, 0);
		if (!CreatePipelineLayout(pipelineLayoutInfo))
		{
			LogError("Failed to create bloom downscaling pipeline layout!");
			return;
		}

		Shader compShader(ShaderType::BLOOM_DOWNSCALING, ShaderStage::COMPUTE_SHADER);
		if (!compShader.IsValid())
		{
			LogError("Failed to create bloom downscaling pipeline. Shader creation failed!");
			return;
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = GetPipelineLayout();
		pipelineInfo.stage = PopulateShaderCreateInfo(compShader);

		if (!CreateComputePipelineObject(pipelineInfo))
		{
			LogError("Failed to create bloom downscaling pipeline!");
		}
	}

	PipelineType BloomDownscalingPipeline::GetType() const
	{
		return PipelineType::COMPUTE;
	}

	void BloomDownscalingPipeline::FlushData()
	{
		setLayoutCache = nullptr;

		wasDataSet = false;
	}


}