#ifndef CUBEMAP_PREPROCESSING_PIPELINE_H
#define CUBEMAP_PREPROCESSING_PIPELINE_H

#include "../render_passes/cubemap_preprocessing_render_pass.h"
#include "base_pipeline.h"

namespace TANG
{
	class CubemapPreprocessingPipeline : public BasePipeline
	{
	public:

		CubemapPreprocessingPipeline();
		~CubemapPreprocessingPipeline();
		CubemapPreprocessingPipeline(CubemapPreprocessingPipeline&& other) noexcept;

		// Get references to the data required in Create(), it's not needed
		void SetData(const CubemapPreprocessingRenderPass& renderPass, const SetLayoutCache& setLayoutCache, VkExtent2D viewportSize);

		void Create() override;

	private:

		void FlushData() override;

		const CubemapPreprocessingRenderPass* renderPass;
		const SetLayoutCache* setLayoutCache;
		VkExtent2D viewportSize;

	};
}

#endif