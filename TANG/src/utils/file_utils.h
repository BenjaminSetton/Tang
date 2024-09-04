
#include <string_view>

namespace TANG
{
	// Reads the provided file and returns it's contents. Upon success returns the number of characters read and a pointer to the
	// allocated file contents. If this call fails at any point, it will return 0 and the outBuffer parameter
	// will remain as nullptr
	// NOTE - This function dynamically allocates a buffer and returns the size, it is up to the caller to
	//        clean up the memory! Alternatively, the caller might choose the other ReadFile() override that
	//        takes in a buffer that's already been allocated and it's size
	[[nodiscard]] uint32_t ReadFile(const std::string_view& fileName, char** outBuffer);

	// Reads the provided file and returns it's contents. Upon success returns the number of character read and sets
	// the content of the file inside the outBuffer array. This function will read up to maxBufferSize. In the case where
	// the file has more characters than were initially allocated in outBuffer, the 'allowIncompleteRead' flag can be set
	// to fill the buffer with the files' incomplete contents. Otherwise, this function will return early and read zero
	// characters
	uint32_t ReadFile(const std::string_view& fileName, char* outBuffer, uint32_t maxBufferSize, bool allowIncompleteRead);

	void WriteToFile(const std::string_view& fileName, const std::string_view& msg);

	void AppendToFile(const std::string_view& fileName, const std::string_view& msg);

	uint32_t FileChecksum(const std::string_view& fileName);
}