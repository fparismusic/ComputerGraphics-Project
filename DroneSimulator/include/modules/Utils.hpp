#pragma once

#include <glm/glm.hpp>
#include <string>

#define M_PI		3.14159265358979323846	/* pi */
#define M_PI_2		1.57079632679489661923	/* pi/2 */

// the uniform buffer objects data structures
//        float : alignas(4)
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)
// alignas(n) tells the compiler to align each variable in memory according to Vulkan's strict rules
struct UniformBufferObject {
	// model-View-Projection matrix: used in vertex shader to transform object coordinates to screen space.
	alignas(16) glm::mat4 mvpMat;
	// model matrix: it positions and orients the object in the world.
	alignas(16) glm::mat4 mMat;
	// normal matrix: used to transform normals correctly, especially when the model is scaled.
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

struct OverlayUniformBlock {
	alignas(4) float visible;
};

// the vertices data structures
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

struct VertexOverlay {
	glm::vec2 pos;
	glm::vec2 UV;
};

struct skyBoxVertex {
	glm::vec3 pos;
	glm::vec2 UV;
};