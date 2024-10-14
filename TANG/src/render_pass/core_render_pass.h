#ifndef CORE_RENDER_PASS_H
#define CORE_RENDER_PASS_H

#include "base_render_pass.h"

namespace TANG
{
	struct CorePassStructure
	{

	};

	class CoreRenderPass final : public BaseRenderPass
	{
	public:

		explicit CoreRenderPass();
		~CoreRenderPass();

	private:

		bool Build(TANG::RenderPassBuilder& out_builder) override;
	};
}


#endif