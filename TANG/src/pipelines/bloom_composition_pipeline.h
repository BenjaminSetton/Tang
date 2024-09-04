#ifndef BLOOM_COMPOSITION_PIPELINE_H
#define BLOOM_COMPOSITION_PIPELINE_H

#include "base_pipeline.h"

namespace TANG
{
	class BloomCompositionPipeline : public BasePipeline
	{
	public:

		BloomCompositionPipeline();
		~BloomCompositionPipeline();
		BloomCompositionPipeline(BloomCompositionPipeline&& other) noexcept;

		BloomCompositionPipeline(const BloomCompositionPipeline& other) = delete;
		BloomCompositionPipeline& operator=(const BloomCompositionPipeline& other) = delete;

		void SetData(const SetLayoutCache* setLayoutCache);

		void Create() override;

		PipelineType GetType() const override;

	private:

		void FlushData() override;

		const SetLayoutCache* setLayoutCache;
	};
}

#endif