#ifndef CUBEMAP_PREPROCESSING_PIPELINE_H
#define CUBEMAP_PREPROCESSING_PIPELINE_H

#include "base_pipeline.h"

namespace TANG
{
	// Forward declarations
	class CubemapPreprocessingRenderPass;

	class CubemapPreprocessingPipeline : public BasePipeline
	{
	public:

		CubemapPreprocessingPipeline();
		~CubemapPreprocessingPipeline();
		CubemapPreprocessingPipeline(CubemapPreprocessingPipeline&& other) noexcept;

		void SetData(const CubemapPreprocessingRenderPass* renderPass, const SetLayoutCache* setLayoutCache, VkExtent2D viewportSize);

		void Create() override;

		PipelineType GetType() const override;

	private:

		void FlushData() override;

		const CubemapPreprocessingRenderPass* renderPass;
		const SetLayoutCache* setLayoutCache;
		VkExtent2D viewportSize;

	};
}

#endif