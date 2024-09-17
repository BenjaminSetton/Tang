#ifndef HDR_RENDER_PASS_H
#define HDR_RENDER_PASS_H

#include "base_render_pass.h"

class HDRRenderPass : public TANG::BaseRenderPass
{
public:

	HDRRenderPass();
	~HDRRenderPass();

private:

	bool Build(TANG::RenderPassBuilder& out_builder) override;
	void FlushData() override;
};

#endif