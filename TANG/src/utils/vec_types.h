#ifndef VEC_TYPES_H
#define VEC_TYPES_H

#include "../utils/sanity_check.h"

namespace TANG
{
	// Only valid for integral types and for vectors holding 2-4 elements
	template<typename T, unsigned N, class = typename std::enable_if<std::is_integral<T>::value && (N > 1 && N <= 4)>>
	struct TVec
	{
		TVec()
		{
			std::memset(values, 0, N);
		}

		TVec(T v)
		{
			std::memset(values, static_cast<int>(v), N);
		}

		//TVec(T _x, T _y, T _z)
		//{
		//	// https://stackoverflow.com/questions/48818462/is-there-any-way-for-a-c-template-function-to-take-exactly-n-arguments
		//	TNG_TODO();
		//}

		~TVec() = default;

		TVec(const TVec& other)
		{
			if (this != &other)
			{
				CopyValues(other);
			}
		}

		TVec(const TVec&& other) noexcept 
		{
			CopyValues(other);
		}

		TVec& operator=(const TVec& other)
		{
			if (this == &other)
			{
				return *this;
			}

			CopyValues(other);
			return *this;
		}

		TVec& operator=(const std::initializer_list<T>& other)
		{
			TNG_ASSERT_MSG(other.size() == N, "Invalid initalizer-list copy assignment! Initializer list must contain the same number of elements as the vector type");

			if (other.size() == N)
			{
				std::copy_n(other.begin(), N, values);
			}

			return *this;
		}

		bool operator==(const TVec& other) const
		{
			return (std::memcmp(values, other.values, N * sizeof(T)) == 0);
		}

		template<class = typename std::enable_if<N >= 1>>
		T GetX() const { return values[0]; }

		template<class = typename std::enable_if<N >= 2>>
		T GetY() const { return values[1]; }

		template<class = typename std::enable_if<N >= 3>>
		T GetZ() const { return values[2]; }

		template<class = typename std::enable_if<N == 4>>
		T GetW() const { return values[3]; }

	private:

		T values[N];

		void CopyValues(const TVec& other)
		{
			std::memcpy(values, other.values, N * sizeof(T));
		}
	};

	//// Only valid for integral types
	//template<typename T, class = typename std::enable_if<std::is_integral<T>::value>>
	//struct TVec3
	//{
	//	Vec3() : x(0), y(0), z(0) { }
	//	Vec3(T v) : x(v), y(v), z(v) { }
	//	Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) { }
	//	~Vec3() = default;

	//	Vec3(const Vec3& other) : x(other.x), y(other.y), z(other.z) { }
	//	Vec3(const Vec3&& other) : x(other.x), y(other.y), z(other.z) noexcept { }
	//	Vec3& operator=(const Vec3& other)
	//	{
	//		if (this == &other)
	//		{
	//			return *this;
	//		}

	//		x = other.x;
	//		y = other.y;
	//		z = other.z;

	//		return *this;
	//	}

	//	T x, y, z;
	//};

	//// Only valid for integral types
	//template<typename T, class = typename std::enable_if<std::is_integral<T>::value>>
	//struct TVec4
	//{
	//	Vec4() : x(0), y(0), z(0), w(0) { }
	//	Vec4(T v) : x(v), y(v), z(v), w(v) { }
	//	Vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) { }
	//	~Vec4() = default;

	//	Vec4(const Vec4& other) : x(other.x), y(other.y), z(other.z), w(other.w) { }
	//	Vec4(const Vec4&& other) : x(other.x), y(other.y), z(other.z), w(other.w) noexcept { }
	//	Vec4& operator=(const Vec4& other)
	//	{
	//		if (this == &other)
	//		{
	//			return *this;
	//		}

	//		x = other.x;
	//		y = other.y;
	//		z = other.z;
	//		w = other.w;

	//		return *this;
	//	}

	//	T x, y, z, w;
	//};

	// Template type aliases
	using Vec2 = TVec<float, 2>;
	using IVec2 = TVec<int32_t, 2>;
	using UVec2 = TVec<uint32_t, 2>;

	using Vec3 = TVec<float, 3>;
	using IVec3 = TVec<int32_t, 3>;
	using UVec3 = TVec<uint32_t, 3>;

	using Vec4 = TVec<float, 4>;
	using IVec4 = TVec<int32_t, 4>;
	using UVec4 = TVec<uint32_t, 4>;
}

#endif