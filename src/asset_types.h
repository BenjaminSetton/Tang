#ifndef ASSET_TYPES_H
#define ASSET_TYPES_H

#include <glm.hpp>

#include "logger.h"

namespace TANG
{
	struct Vertex
	{
		Vertex() : pos(0.0f, 0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f), uv(0.0f, 0.0f, 0.0f)
		{
		}
		~Vertex() { }
		// Too lazy to implement copy-constructor; default should work just fine in this case

		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 uv;
	};

	// Maybe this has to be moved somewhere else, specifically more texture-related?
	struct Texture
	{
		Texture() : data(nullptr), size(0.0f, 0.0f), bytesPerPixel(0)
		{
		}
		~Texture() { }
		Texture(const Texture& other)
		{
			// Temporary debug :)
			LogWarning("Deep-copying texture!");

			data = new char[other.size.x * other.size.y * bytesPerPixel];
			size = other.size;
			bytesPerPixel = other.bytesPerPixel;
		}

		void* data;
		glm::vec2 size;
		uint32_t bytesPerPixel; // Guaranteed to be 4 by assimp loader
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		//std::vector<Material> materials;
	};

	struct Asset
	{
		std::string name;
		std::vector<Mesh> meshes;
		std::vector<Texture> textures;
	};
}

#endif