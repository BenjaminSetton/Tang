#ifndef BLOOM_DOWNSCALING_PIPELINE_H
#define BLOOM_DOWNSCALING_PIPELINE_H

#include "base_pipeline.h"

namespace TANG
{
	class BloomDownscalingPipeline : public BasePipeline
	{
	public:

		BloomDownscalingPipeline();
		~BloomDownscalingPipeline();
		BloomDownscalingPipeline(BloomDownscalingPipeline&& other) noexcept;

		BloomDownscalingPipeline(const BloomDownscalingPipeline& other) = delete;
		BloomDownscalingPipeline& operator=(const BloomDownscalingPipeline& other) = delete;

		void SetData(const SetLayoutCache* setLayoutCache);

		void Create() override;

		PipelineType GetType() const override;

	private:

		void FlushData() override;

		const SetLayoutCache* setLayoutCache;
	};
}

#endif