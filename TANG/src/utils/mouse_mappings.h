#ifndef MOUSE_MAPPINGS_H
#define MOUSE_MAPPINGS_H

#include <unordered_map>

#include "glfw/glfw3.h"
#include "mouse_declarations.h"

namespace TANG
{
	std::unordered_map<int, MouseType> MouseTypeMappings =
	{
		{ GLFW_MOUSE_BUTTON_LEFT,   MouseType::MOUSE_LMB     }, // Defined in GLFW header as GLFW_MOUSE_BUTTON_1
		{ GLFW_MOUSE_BUTTON_RIGHT,  MouseType::MOUSE_RMB     }, // Defined in GLFW header as GLFW_MOUSE_BUTTON_2
		{ GLFW_MOUSE_BUTTON_MIDDLE, MouseType::MOUSE_MMB     }, // Defined in GLFW header as GLFW_MOUSE_BUTTON_3
		{ GLFW_MOUSE_BUTTON_4,      MouseType::MOUSE_BUTTON1 },
		{ GLFW_MOUSE_BUTTON_5,      MouseType::MOUSE_BUTTON2 },
		{ GLFW_MOUSE_BUTTON_6,      MouseType::MOUSE_BUTTON2 },
	};
}

#endif