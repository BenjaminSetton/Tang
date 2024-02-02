#ifndef COMPUTE_BUFFER_H
#define COMPUTE_BUFFER_H

#include "buffer.h"

namespace TANG
{
	class ComputeBuffer : public Buffer
	{
	public:

		ComputeBuffer();
		~ComputeBuffer();
		ComputeBuffer(const ComputeBuffer& other);
		ComputeBuffer(ComputeBuffer&& other) noexcept;
		ComputeBuffer& operator=(const ComputeBuffer& other);

		void Create(VkDeviceSize size) override;
		void Destroy() override;

	private:
	};
}


#endif