#ifndef QUEUE_FAMILY_INDICES_H
#define QUEUE_FAMILY_INDICES_H

#include <unordered_map>

namespace TANG
{
	class QueueFamilyIndices
	{
	public:

		typedef uint32_t QueueFamilyIndexType;

		QueueFamilyIndices();
		~QueueFamilyIndices();
		QueueFamilyIndices(const QueueFamilyIndices& other);
		QueueFamilyIndices(QueueFamilyIndices&& other) noexcept;
		QueueFamilyIndices& operator=(const QueueFamilyIndices& other);

		bool IsValid(QueueFamilyIndexType index);
		bool IsComplete();

	private:

		std::unordered_map<QueueType, QueueFamilyIndexType> queueFamilies;
	};
}

#endif