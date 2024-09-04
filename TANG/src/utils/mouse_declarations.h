#ifndef MOUSE_DECLARATIONS_H
#define MOUSE_DECLARATIONS_H

namespace TANG
{
	enum class MouseType
	{
		MOUSE_LMB, // Left-click
		MOUSE_RMB, // Right-click
		MOUSE_MMB, // Middle-click
		MOUSE_BUTTON1,
		MOUSE_BUTTON2,
		MOUSE_BUTTON3,

		COUNT // Keep as the last entry!
	};
}

#endif