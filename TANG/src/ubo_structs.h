#ifndef UBO_STRUCTS_H
#define UBO_STRUCTS_H

// DISABLE WARNINGS FROM GLM
// 4201: warning C4201: nonstandard extension used: nameless struct/union
// 4244: warning C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(push)
#pragma warning(disable : 4201 4244)

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#pragma warning(pop)

namespace TANG
{
	// This UBO is updated every frame for every different asset, to properly reflect their location. Matches
	// up with the Transform struct inside asset_types.h
	struct TransformUBO
	{
		TransformUBO() : transform(glm::identity<glm::mat4>())
		{
		}

		TransformUBO(glm::mat4 trans) : transform(trans)
		{
		}

		glm::mat4 transform;
	};

	struct ViewUBO
	{
		glm::mat4 view;
	};
	TNG_ASSERT_COMPILE(sizeof(ViewUBO) == 64);

	struct ProjUBO
	{
		glm::mat4 proj;
	};
	TNG_ASSERT_COMPILE(sizeof(ProjUBO) == 64);

	struct ViewProjUBO
	{
		glm::mat4 view;
		glm::mat4 proj;
	};
	TNG_ASSERT_COMPILE(sizeof(ViewProjUBO) == 128);

	// The minimum uniform buffer alignment of the chosen physical device is 64 bytes...an entire matrix 4
	// 
	struct CameraDataUBO
	{
		glm::vec4 position;
		float exposure;
		char padding[44];
	};
	TNG_ASSERT_COMPILE(sizeof(CameraDataUBO) == 64);
}

#endif