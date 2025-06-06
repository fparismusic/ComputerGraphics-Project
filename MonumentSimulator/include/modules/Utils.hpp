#pragma once

#include <glm/glm.hpp>
#include <string>

#define GAME_STATE_START_SCREEN 0
#define GAME_STATE_PLAY 1
#define GAME_STATE_GAME_WIN 2
#define GAME_STATE_GAME_LOSE 3

#define LIGHTS_NUM 4

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
	alignas(16) glm::vec3 lightDir[LIGHTS_NUM];			// Direction of the lights
	alignas(16) glm::vec3 lightPos[LIGHTS_NUM];			// Position of the lights
	alignas(16) glm::vec4 lightColor[LIGHTS_NUM];		// Color of the lights
	alignas(16) glm::vec3 eyePos;						// Position of the camera/eye
	alignas(16) glm::vec4 lightOn;						// Lights on/off flag (point, direct, spot, ambient component)
	alignas(4) float cosIn;								// Spot light inner cone angle
	alignas(4) float cosOut;							// Spot light outer cone angle
	alignas(4) bool gameOver;							// Game over flag
};

struct SkyBoxUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;		// Field for MVP matrix
	alignas(4) float time;				// Field for time
};

struct AnimatedUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
	alignas(4) float time;				// Time variable for animation
	alignas(4) float speed;
};

struct OverlayUniformBlock {
	alignas(4) float visible;
};

// The vertices data structures
struct Vertex {
	glm::vec3 pos;
	glm::vec2 UV;
	glm::vec3 norm;
};

struct skyBoxVertex {
	glm::vec3 pos;
};

struct VertexOverlay {
	glm::vec2 pos;
	glm::vec2 UV;
};

struct VertexTan {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec4 tangent;
	glm::vec2 UV;
};