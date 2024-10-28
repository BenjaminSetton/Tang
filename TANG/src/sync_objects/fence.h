#ifndef FENCE_H
#define FENCE_H

namespace TANG
{
	class Fence
	{
	public:

		Fence();
		~Fence();
		Fence(const Fence& other);
		Fence(Fence&& other) noexcept;
		Fence& operator=(const Fence& other);

	private:
	};
}

#endif