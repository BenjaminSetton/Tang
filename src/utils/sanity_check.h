#ifndef SANITY_CHECK_H
#define SANITY_CHECK_H

#include <assert.h>

namespace TANG
{
	// Regular run-time assertions.
	#define TNG_ASSERT(x) assert(x)
	#define TNG_ASSERT_MSG(x, msg) assert(x && msg)														// Msg parameter _must_ be a const char*


	// Compile-time assert
	#define TNG_ASSERT_COMPILE(x) static_assert(x)
	#define TNG_ASSERT_SAME_SIZE(x, y) TNG_ASSERT_COMPILE(x == y)

	// Utility
	#define UNUSED(x) (void)x
	#define TNG_TODO() TNG_ASSERT_MSG(false, "TODO - Implement");										// Utility assert for when incomplete code is run
	#define TNG_SAFE_DEL(x) do { if(x != nullptr) { delete x; x = nullptr; } } while(false)				// Utility delete macro for safely deleting a non-array pointer (e.g. checking if null and also setting it to null)
	#define TNG_SAFE_DEL_ARR(x) do { if(x != nullptr) { delete[] x; x = nullptr; } } while(false)		// Utility delete macro for safely deleting an array pointer (e.g. checking if null and also setting it to null)

}


#endif