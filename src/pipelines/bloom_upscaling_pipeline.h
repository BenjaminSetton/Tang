#ifndef BLOOM_UPSCALING_PIPELINE_H
#define BLOOM_UPSCALING_PIPELINE_H

#include "base_pipeline.h"

namespace TANG
{
	class BloomUpscalingPipeline : public BasePipeline
	{
	public:

		BloomUpscalingPipeline();
		~BloomUpscalingPipeline();
		BloomUpscalingPipeline(BloomUpscalingPipeline&& other) noexcept;

		BloomUpscalingPipeline(const BloomUpscalingPipeline& other) = delete;
		BloomUpscalingPipeline& operator=(const BloomUpscalingPipeline& other) = delete;

		void SetData(const SetLayoutCache* setLayoutCache);

		void Create() override;

		PipelineType GetType() const override;

	private:

		void FlushData() override;

		const SetLayoutCache* setLayoutCache;
	};
}


#endif