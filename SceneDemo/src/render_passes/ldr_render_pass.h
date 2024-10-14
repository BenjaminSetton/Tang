#ifndef LDR_RENDER_PASS_H
#define LDR_RENDER_PASS_H

#include "TANG/base_render_pass.h"

class LDRRenderPass : public TANG::BaseRenderPass
{
public:

	LDRRenderPass();
	~LDRRenderPass();

private:

	bool Build(TANG::RenderPassBuilder& out_builder) override;
};

#endif