//#ifndef PBR_RENDER_PASS_H
//#define PBR_RENDER_PASS_H
//
//#include "base_render_pass.h"
//
//namespace TANG
//{
//	class PBRRkjbnkbss : public BaseRenderPass
//	{
//	public:
//
//		PBRRenderPass();
//		~PBRRenderPass();
//		PBRRenderPass(PBRRenderPass&& other) noexcept;
//		// Copying this object is not allowed
//
//		void SetData(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);
//
//	private:
//
//		bool Build(RenderPassBuilder& out_builder) override;
//		void FlushData() override;
//
//		// This data is copied from the renderer
//		VkFormat colorAttachmentFormat;
//		VkFormat depthAttachmentFormat;
//	};
//}
//
//#endif