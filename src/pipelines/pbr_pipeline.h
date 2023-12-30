#ifndef PBR_PIPELINE_H
#define PBR_PIPELINE_H

#include "base_pipeline.h"

namespace TANG
{
	// Forward declarations
	class PBRRenderPass;

	class PBRPipeline : public BasePipeline
	{
	public:

		PBRPipeline();
		~PBRPipeline();
		PBRPipeline(PBRPipeline&& other) noexcept;

		void SetData(const PBRRenderPass* renderPass, const SetLayoutCache* setLayoutCache, VkExtent2D viewportSize);

		void Create() override;

	private:

		void FlushData() override;

		const PBRRenderPass* renderPass;
		const SetLayoutCache* setLayoutCache;
		VkExtent2D viewportSize;

	};
}

#endif