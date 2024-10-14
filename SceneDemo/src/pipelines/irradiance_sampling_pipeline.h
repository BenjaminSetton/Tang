#ifndef IRRADIANCE_SAMPLING_PIPELINE_H
#define IRRADIANCE_SAMPLING_PIPELINE_H

#include "base_pipeline.h"

namespace TANG
{
	// Forward declarations
	class CubemapPreprocessingRenderPass;

	class IrradianceSamplingPipeline : public BasePipeline
	{
	public:

		IrradianceSamplingPipeline();
		~IrradianceSamplingPipeline();
		IrradianceSamplingPipeline(IrradianceSamplingPipeline&& other) noexcept;

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