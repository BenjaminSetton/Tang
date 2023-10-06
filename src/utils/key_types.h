
#include <unordered_map>

// Needed to include GLFW key types for mapping
#include "glfw3.h"

namespace TANG
{
	enum class KeyType
	{
		// Numbers
		KEY_0,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,

		// Letters
		KEY_A,
		KEY_B,
		KEY_C,
		KEY_D,
		KEY_E,
		KEY_F,
		KEY_G,
		KEY_H,
		KEY_I,
		KEY_J,
		KEY_K,
		KEY_L,
		KEY_M,
		KEY_N,
		KEY_O,
		KEY_P,
		KEY_Q,
		KEY_R,
		KEY_S,
		KEY_T,
		KEY_U,
		KEY_V,
		KEY_W,
		KEY_X,
		KEY_Y,
		KEY_Z,

		// Utility keys
		KEY_LSHIFT,
		KEY_RSHIFT,
		KEY_LCTRL,
		KEY_RCTRL,
		KEY_SPACE,
		KEY_LALT,
		KEY_RALT,
		KEY_TAB,
		KEY_CAPS,
		KEY_ENTER,
		KEY_BACKSPACE,
		KEY_OSKEY,
		KEY_ESC,
		KEY_HOME,
		KEY_END,
		KEY_INSERT,
		KEY_DELETE,
		KEY_PGUP,
		KEY_PGDOWN,

		// Special characters
		KEY_TILDA,
		KEY_HYPHEN,
		KEY_EQUALS,
		KEY_LSQUAREBRACKET,
		KEY_RSQUAREBRACKET,
		KEY_BACKSLASH,
		KEY_FORWARDSLASH,
		KEY_SEMICOLON,
		KEY_APOSTROPHE,
		KEY_COMMA,
		KEY_PERIOD,

		// Function keys
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,
		KEY_F9,
		KEY_F10,
		KEY_F11,
		KEY_F12
	};

	static const std::unordered_map<int, KeyType> KeyMappings =
	{
		{ GLFW_KEY_0, KeyType::KEY_0 },
		{ GLFW_KEY_1, KeyType::KEY_1 },
		{ GLFW_KEY_2, KeyType::KEY_2 },
		{ GLFW_KEY_3, KeyType::KEY_3 },
		{ GLFW_KEY_4, KeyType::KEY_4 },
		{ GLFW_KEY_5, KeyType::KEY_5 },
		{ GLFW_KEY_6, KeyType::KEY_6 },
		{ GLFW_KEY_7, KeyType::KEY_7 },
		{ GLFW_KEY_8, KeyType::KEY_8 },
		{ GLFW_KEY_9, KeyType::KEY_9 },
		// TODO
	};

}