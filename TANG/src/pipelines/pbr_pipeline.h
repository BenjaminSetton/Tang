#ifndef PBR_PIPELINE_H
#define PBR_PIPELINE_H

#include "base_pipeline.h"

// Forward declarations
class HDRRenderPass;

namespace TANG
{
	class PBRPipeline : public BasePipeline
	{
	public:

		PBRPipeline();
		~PBRPipeline();
		PBRPipeline(PBRPipeline&& other) noexcept;

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