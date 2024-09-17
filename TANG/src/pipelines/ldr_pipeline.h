#ifndef LDR_PIPELINE_H
#define LDR_PIPELINE_H

#include "base_pipeline.h"

// Forward declarations
class LDRRenderPass;

namespace TANG
{

	class LDRPipeline : public BasePipeline
	{
	public:

		LDRPipeline();
		~LDRPipeline();
		LDRPipeline(LDRPipeline&& other) noexcept;

		void SetData(const LDRRenderPass* renderPass, const SetLayoutCache* setLayoutCache, VkExtent2D viewportSize);

		void Create() override;

		PipelineType GetType() const override;

	private:

		void FlushData() override;

		const LDRRenderPass* renderPass;
		const SetLayoutCache* setLayoutCache;
		VkExtent2D viewportSize;

	};
}

#endif