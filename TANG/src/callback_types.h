#ifndef CALLBACK_TYPES_H
#define CALLBACK_TYPES_H

namespace TANG
{
	using SwapChainRecreatedCallback = void(*)(uint32_t, uint32_t); // Parameters are new width and height of the swap chain framebuffer
	using RendererShutdownCallback = void(*)(); // Called once the TANG renderer is shutting down. All graphics resources must be cleaned up in this function
}

#endif