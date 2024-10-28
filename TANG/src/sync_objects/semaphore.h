#ifndef SEMAPHORE_H
#define SEMAPHORE_H

namespace TANG
{
	class Semaphore
	{
	public:

		Semaphore();
		~Semaphore();
		Semaphore(const Semaphore& other);
		Semaphore(Semaphore&& other) noexcept;
		Semaphore& operator=(const Semaphore& other);

	private:
	};
}

#endif