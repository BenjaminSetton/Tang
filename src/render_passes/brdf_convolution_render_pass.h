#ifndef BRDF_CONVOLUTION_RENDER_PASS_H
#define BRDF_CONVOLUTION_RENDER_PASS_H

#include "base_render_pass.h"

namespace TANG
{
	class BRDFConvolutionRenderPass : public BaseRenderPass
	{
	public:

		BRDFConvolutionRenderPass();
		~BRDFConvolutionRenderPass();
		BRDFConvolutionRenderPass(BRDFConvolutionRenderPass&& other) noexcept;
		// Copying this object is not allowed

		void SetData(VkFormat colorAttachmentFormat);

	private:

		bool Build(RenderPassBuilder& out_builder) override;
		void FlushData() override;

		// This data is copied from the renderer
		VkFormat colorAttachmentFormat;
	};
}

#endif