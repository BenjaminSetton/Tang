#ifndef SKYBOX_PIPELINE_H
#define SKYBOX_PIPELINE_H

#include "base_pipeline.h"

// Forward declarations
class HDRRenderPass;

namespace TANG
{
	class SkyboxPipeline : public BasePipeline
	{
	public:

		SkyboxPipeline();
		~SkyboxPipeline();
		SkyboxPipeline(SkyboxPipeline&& other) noexcept;

		// Get references to the data required in Create(), it's not needed
		void SetData(const HDRRenderPass* renderPass, const SetLayoutCache* setLayoutCache, VkExtent2D viewportSize);

		void Create() override;

		PipelineType GetType() const override;

	private:

		void FlushData() override;

		const HDRRenderPass* renderPass;
		const SetLayoutCache* setLayoutCache;
		VkExtent2D viewportSize;

	};
}


#endif