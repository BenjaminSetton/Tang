#ifndef KEY_MAPPINGS_H
#define KEY_MAPPINGS_H

#include <unordered_map>

#include "glfw/glfw3.h"
#include "key_declarations.h"

namespace TANG
{
	static const std::unordered_map<int, KeyType> KeyTypeMappings =
	{
		// Numbers
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

		// Letters
		{ GLFW_KEY_A, KeyType::KEY_A },
		{ GLFW_KEY_B, KeyType::KEY_B },
		{ GLFW_KEY_C, KeyType::KEY_C },
		{ GLFW_KEY_D, KeyType::KEY_D },
		{ GLFW_KEY_E, KeyType::KEY_E },
		{ GLFW_KEY_F, KeyType::KEY_F },
		{ GLFW_KEY_G, KeyType::KEY_G },
		{ GLFW_KEY_H, KeyType::KEY_H },
		{ GLFW_KEY_I, KeyType::KEY_I },
		{ GLFW_KEY_J, KeyType::KEY_J },
		{ GLFW_KEY_K, KeyType::KEY_K },
		{ GLFW_KEY_L, KeyType::KEY_L },
		{ GLFW_KEY_M, KeyType::KEY_M },
		{ GLFW_KEY_N, KeyType::KEY_N },
		{ GLFW_KEY_O, KeyType::KEY_O },
		{ GLFW_KEY_P, KeyType::KEY_P },
		{ GLFW_KEY_Q, KeyType::KEY_Q },
		{ GLFW_KEY_R, KeyType::KEY_R },
		{ GLFW_KEY_S, KeyType::KEY_S },
		{ GLFW_KEY_T, KeyType::KEY_T },
		{ GLFW_KEY_U, KeyType::KEY_U },
		{ GLFW_KEY_V, KeyType::KEY_V },
		{ GLFW_KEY_W, KeyType::KEY_W },
		{ GLFW_KEY_X, KeyType::KEY_X },
		{ GLFW_KEY_Y, KeyType::KEY_Y },
		{ GLFW_KEY_Z, KeyType::KEY_Z },
		
		// Utility keys
//	KEY_OSKEY,
		{ GLFW_KEY_LEFT_SHIFT, KeyType::KEY_LSHIFT },
		{ GLFW_KEY_RIGHT_SHIFT, KeyType::KEY_RSHIFT },
		{ GLFW_KEY_LEFT_CONTROL, KeyType::KEY_LCTRL },
		{ GLFW_KEY_RIGHT_CONTROL, KeyType::KEY_RCTRL },
		{ GLFW_KEY_SPACE, KeyType::KEY_SPACEBAR },
		{ GLFW_KEY_LEFT_ALT, KeyType::KEY_LALT },
		{ GLFW_KEY_RIGHT_ALT, KeyType::KEY_RALT },
		{ GLFW_KEY_TAB, KeyType::KEY_TAB },
		{ GLFW_KEY_CAPS_LOCK, KeyType::KEY_CAPS },
		{ GLFW_KEY_ENTER, KeyType::KEY_ENTER },
		{ GLFW_KEY_BACKSPACE, KeyType::KEY_BACKSPACE },
		{ GLFW_KEY_ESCAPE, KeyType::KEY_ESC },
		{ GLFW_KEY_HOME, KeyType::KEY_HOME },
		{ GLFW_KEY_END, KeyType::KEY_END },
		{ GLFW_KEY_INSERT, KeyType::KEY_INSERT },
		{ GLFW_KEY_DELETE, KeyType::KEY_DELETE },
		{ GLFW_KEY_PAGE_UP, KeyType::KEY_PGUP },
		{ GLFW_KEY_PAGE_DOWN, KeyType::KEY_PGDOWN },

		// Special characters
//	KEY_TILDA,
//	KEY_HYPHEN,
		{ GLFW_KEY_EQUAL, KeyType::KEY_EQUALS },
		{ GLFW_KEY_LEFT_BRACKET, KeyType::KEY_LSQUAREBRACKET },
		{ GLFW_KEY_RIGHT_BRACKET, KeyType::KEY_RSQUAREBRACKET },
		{ GLFW_KEY_BACKSLASH, KeyType::KEY_BACKSLASH },
		{ GLFW_KEY_SLASH, KeyType::KEY_FORWARDSLASH },
		{ GLFW_KEY_SEMICOLON, KeyType::KEY_SEMICOLON },
		{ GLFW_KEY_APOSTROPHE, KeyType::KEY_APOSTROPHE },
		{ GLFW_KEY_COMMA, KeyType::KEY_COMMA },
		{ GLFW_KEY_PERIOD, KeyType::KEY_PERIOD },

		// Function keys
		{ GLFW_KEY_F1 , KeyType::KEY_F1  },
		{ GLFW_KEY_F2 , KeyType::KEY_F2  },
		{ GLFW_KEY_F3 , KeyType::KEY_F3  },
		{ GLFW_KEY_F4 , KeyType::KEY_F4  },
		{ GLFW_KEY_F5 , KeyType::KEY_F5  },
		{ GLFW_KEY_F6 , KeyType::KEY_F6  },
		{ GLFW_KEY_F7 , KeyType::KEY_F7  },
		{ GLFW_KEY_F8 , KeyType::KEY_F8  },
		{ GLFW_KEY_F9 , KeyType::KEY_F9  },
		{ GLFW_KEY_F10, KeyType::KEY_F10 },
		{ GLFW_KEY_F11, KeyType::KEY_F11 },
		{ GLFW_KEY_F12, KeyType::KEY_F12 },

	};

}

#endif