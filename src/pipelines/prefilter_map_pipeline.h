#ifndef PREFILTER_SKYBOX_PIPELINE_H
#define PREFILTER_SKYBOX_PIPELINE_H

#include "base_pipeline.h"

namespace TANG
{
	// Forward declarations
	class CubemapPreprocessingRenderPass;

	class PrefilterMapPipeline : public BasePipeline
	{
	public:

		PrefilterMapPipeline();
		~PrefilterMapPipeline();
		PrefilterMapPipeline(PrefilterMapPipeline&& other) noexcept;

		void SetData(const CubemapPreprocessingRenderPass* renderPass, const SetLayoutCache* cubemapSetLayoutCache, const SetLayoutCache* roughnessSetLayoutCache, VkExtent2D viewportSize);

		void Create() override;

	private:

		void FlushData() override;

		const CubemapPreprocessingRenderPass* renderPass;
		const SetLayoutCache* cubemapSetLayoutCache;
		const SetLayoutCache* roughnessSetLayoutCache;
		VkExtent2D viewportSize;

	};
}

#endif