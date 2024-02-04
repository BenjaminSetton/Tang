#ifndef BLOOM_PREFILTER_PIPELINE_H
#define BLOOM_PREFILTER_PIPELINE_H

#include "base_pipeline.h"

namespace TANG
{
	class BloomPrefilterPipeline : public BasePipeline
	{
	public:

		BloomPrefilterPipeline();
		~BloomPrefilterPipeline();
		BloomPrefilterPipeline(BloomPrefilterPipeline&& other) noexcept;

		BloomPrefilterPipeline(const BloomPrefilterPipeline& other) = delete;
		BloomPrefilterPipeline& operator=(const BloomPrefilterPipeline& other) = delete;

		void SetData(const SetLayoutCache* setLayoutCache);

		void Create() override;

		PipelineType GetType() const override;

	private:

		void FlushData() override;

		const SetLayoutCache* setLayoutCache;
	};
}

#endif