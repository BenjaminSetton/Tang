
#include "base_pipeline.h"

namespace TANG
{
	class PBRPipeline : public BasePipeline
	{
	public:

		void Create(VkDevice logicalDevice, VkRenderPass renderPass, const SetLayoutCache& setLayoutCache) override;

	};
}