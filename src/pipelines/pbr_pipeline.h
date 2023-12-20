
#include "../render_passes/pbr_render_pass.h"
#include "base_pipeline.h"

namespace TANG
{
	class PBRPipeline : public BasePipeline
	{
	public:

		PBRPipeline();
		~PBRPipeline();
		PBRPipeline(PBRPipeline&& other) noexcept;

		// Get references to the data required in Create(), it's not needed
		void SetData(const PBRRenderPass& renderPass, const SetLayoutCache& setLayoutCache, VkExtent2D viewportSize);

		void Create() override;

	private:

		void FlushData() override;

		const PBRRenderPass* renderPass;
		const SetLayoutCache* setLayoutCache;
		VkExtent2D viewportSize;

	};
}