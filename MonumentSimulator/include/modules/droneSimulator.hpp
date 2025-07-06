#pragma once // We ensure that the file is only included once during compilation

// This has been adapted from the Vulkan tutorial
#include <sstream>

#include <json.hpp>
#include <random> // For random number generation

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"
#include "modules/Animations.hpp"
#include "modules/Utils.hpp"

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// DroneSimulator: subclass of BaseProject
class DroneSimulator : public BaseProject {
protected:

	// --- Game States ---
	enum class AppState { Menu, Playing, GameOver };

	// --- Difficulty parameter --
	enum class Difficulty { Easy, Medium, Hard };
	Difficulty currentDifficulty = Difficulty::Medium;

	// --- Menu fields ---
	AppState state = AppState::Menu;
	TextMaker menuTxt;
	float gameOver = 0.0f;
	bool showStartText = false;
	bool showCommandsKeyboard = false;
	bool prevEscPressed = false;

	// --- Window parameters ---
	float Ar; // Aspect Ratio

	// --- Camera controls ---
	glm::vec3 CamPos = glm::vec3(0.0f, 0.3f, 2.0f);		// Initial camera position
	float	CamYaw = 0.0f,										// Horizontal rotation (left/right)
			CamPitch = 0.0f,									// Vertical rotation (up/down)
			CamRoll = 0.0f,										// Roll rotation (tilt)
			CamDist = 0.0f;										// Distance of the camera from the drone

	// --- Time parameters ---
	std::chrono::time_point<std::chrono::high_resolution_clock> gameStartTime;
	float totalElapsedTime = 0.0f;
	float initialGameDuration = 300.0f;		// 5 minutes
	float gameTime = 300.0f;				// 5 minutes
	bool gameStarted = false;

	// --- HUD tracking variables ---
	int lastPassedCount = -1;		// Tracking the number of passed rings
	int lastTotalSec    = -1;
	char hudBuffer[32];				// buffer big enough
	std::string lastTimeStr = "";   // Tracking time
	int HUD_ID = 2;

	// --- Drone parameters ---
	bool thirdPerson   = true;		// 3-rd person camera
	bool topAngle   = false;		// 1-st person camera
	bool firstPerson    = false;		// 1-st person camera
	glm::vec3 global_pos_drone = glm::vec3(-1000.0f, 250.0f, 130.0f);  // Drone's world coordinates
	float droneYaw = 0.0f, dronePitch = 0.0f, droneRoll = 0.0f;

	// --- Valid bounds (world space) for the drone ---
	const float minX = -3000.0f;
	const float maxX =  1000.0f;
	const float minY =  250.0f;
	const float maxY =  500.0f;
	const float minZ = -3000.0f;
	const float maxZ =  1000.0f;

	// --- Render Pass ---
	RenderPass RP;

	// --- Descriptor Set Layouts ---
	DescriptorSetLayout
		DSL_map,
		DSL_drone,
		DSL_overlay,
		DSL_skyBox,
		DSL_global;

	// --- Vertex Descriptors ---
	VertexDescriptor VD_phong, VD_pbr, VD_overlay, VD_skyBox;

	// --- Pipelines ---
	Pipeline
		P_phong,
		P_pbr,
		P_overlay,
		P_skyBox;

	// --- Model ---
	Model
		M_drone,
		M_overlay[3],
		M_skyBox;

	// --- Textures ---
	Texture
		tex_drone_baseColor, tex_drone_normal, tex_drone_roughness, tex_drone_emissive,
		tex_overlay[3],
		tex_skyBox;

	// --- Descriptor Sets ---
	DescriptorSet
		DS_drone,
		DS_skyBox,
		DS_overlay[3],
		DS_global;

	// --- Uniform Buffers ---
	UniformBufferObject
		UBO_drone,
		ubosStart{},
		ubos{};

	OverlayUniformBlock UBO_overlay[3];
	SkyBoxUniformBufferObject UBO_skyBox;
	GlobalUniformBufferObject GUBO;

	// --- Scene ---
	Scene SC;
	std::vector<VertexDescriptorRef>  VDRs;		// JSON Vertex Descriptors
	std::vector<TechniqueRef> PRs;				// JSON Techniques
	std::vector<glm::vec3> mountainPoints;

	// --- Rings ---
	std::vector<glm::vec3> ringPositions = {
		{60.753, 410.153, 201.635},
		{329.919, 299.502, -141.467},
		{161.293, 254.75, -501.018},
		{329.234, 250, -1480.34},
		{86.0901, 413.327, -1801.31},
		{-229.292, 263.179, -1885.49},
		{-1456.62, 256.705, -1772.74},
		{-2084.15, 403.717, -2197.66},
		{-2241.4, 250, -1323.08},
		{-2088.51, 416.687, -195.252}
	};

	// Flags to track which rings were passed
	std::vector<bool> ringPassed = std::vector<bool>(10, false);

	std::vector<float> ringScale;
	std::vector<glm::mat4> originalRingWm; // Stores the world matrix of each ring


	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you set the main application parameters (window size, title, vsync)
	void setWindowParameters();

	// What to do when the window changes size
	void onWindowResize(int w, int h);

    // Here you load and setup all your Vulkan Models and Textures.
	// Here you also create your DescriptorSetLayouts and load the Shaders for the pipelines
    void localInit();

    // Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit();

    // Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup();

    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup();

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw, with their buffers and textures
	static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params);
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage);

    // Here is where you update the uniforms. Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage);

	//************************************************************************************************
	// LookAt matrix
	glm::mat4 LookAtMat(glm::vec3 Pos, glm::vec3 aim, float Roll);

	// LookInDri matrix
	glm::mat4 LookInDirMat(glm::vec3 Pos, glm::vec3 Angs);

	// Control camera positions
	void setCameraMode(GLFWwindow* w);

	// Control drone movements
	void getDroneInput(GLFWwindow* w, float deltaT);

	// Control GUBO light color
	void updateGlobalUBO(GlobalUniformBufferObject& gubo, float elapsedTime);

	void reset();

	// Get mountain vertices for bounds
	void loadMountainPoints(std::vector<glm::vec3>& points);

	// Bounds checking for the drone w.r.t. mountains
	bool isTooCloseToMountain(const glm::vec3& pos, const std::vector<glm::vec3>& mountainPoints, float threshold = 5.0f);

	// Logic of passing through rings
	float checkRingPassage(glm::vec3 dronePos, std::vector<glm::vec3>& rings, std::vector<bool>& passed, float radius = 10.0f);
};