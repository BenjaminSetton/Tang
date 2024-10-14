#ifndef BRDF_CONVOLUTION_H
#define BRDF_CONVOLUTION_H

#include "base_pipeline.h"

namespace TANG
{
	// Forward declarations
	class BRDFConvolutionRenderPass;

	class BRDFConvolutionPipeline : public BasePipeline
	{
	public:

		BRDFConvolutionPipeline();
		~BRDFConvolutionPipeline();
		BRDFConvolutionPipeline(BRDFConvolutionPipeline&& other) noexcept;

		void SetData(const BRDFConvolutionRenderPass* renderPass, VkExtent2D viewportSize);

		void Create() override;

		PipelineType GetType() const override;

	private:

		void FlushData() override;

		const BRDFConvolutionRenderPass* renderPass;
		VkExtent2D viewportSize;
	
	};
}

#endif