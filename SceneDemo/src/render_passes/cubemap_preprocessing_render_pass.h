#ifndef CUBEMAP_PREPROCESSING_RENDER_PASS_H
#define CUBEMAP_PREPROCESSING_RENDER_PASS_H

#include "TANG/base_render_pass.h"

namespace TANG
{
	class CubemapPreprocessingRenderPass : public BaseRenderPass
	{
	public:

		CubemapPreprocessingRenderPass();
		~CubemapPreprocessingRenderPass();
		CubemapPreprocessingRenderPass(CubemapPreprocessingRenderPass&& other) noexcept;
		// Copying this object is not allowed

	private: 

		bool Build(RenderPassBuilder& out_builder) override;
		void FlushData() override;
	};
}

#endif