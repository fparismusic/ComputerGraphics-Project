#pragma once

#include <glm/glm.hpp>
#include <string>

#define M_PI		3.14159265358979323846	/* pi */
#define M_PI_2		1.57079632679489661923	/* pi/2 */

// The uniform buffer objects data structures
// Remember to use the correct alignas(...) value
//        float : alignas(4)
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)
struct UniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct GlobalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;

	alignas(16) glm::vec3 cameraPos;
	alignas(4) float time;

	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec3 lightColor;
	alignas(4) float lightIntensity;
};

struct SkyBoxUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
};

// The vertices data structures
struct Vertex {
	glm::vec3 pos;
	glm::vec2 UV;
	glm::vec3 norm;
};

struct VertexTan {
	glm::vec3 pos;
	glm::vec2 UV;
	glm::vec3 normal;
	glm::vec4 tangent;
};

struct skyBoxVertex {
	glm::vec3 pos;
	glm::vec2 UV;
};