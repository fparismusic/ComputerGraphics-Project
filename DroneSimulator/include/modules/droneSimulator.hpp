#pragma once // we ensure that the file is only included once during compilation

// this has been adapted from the Vulkan tutorial
#include <sstream>

#include <json.hpp>
#include <random> // for random number generation

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"
#include "modules/Utils.hpp"
#include <soloud.h>
#include <soloud_wav.h>

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// DroneSimulator: subclass of BaseProject
class DroneSimulator : public BaseProject {
protected:

	SoLoud::Soloud soloud;      // Engine
	SoLoud::Wav wav1, master, win, lose, ring;

	// --- game states ---
	enum class AppState { Menu, Playing, GameOver };

	// --- difficulty parameter --
	enum class Difficulty { Easy, Medium, Hard };
	Difficulty currentDifficulty = Difficulty::Easy;

	// --- menu fields ---
	AppState state = AppState::Menu;
	TextMaker menuTxt;
	float gameOver = 0.0f;

	// --- window parameters ---
	float Ar; // aspect ratio


	// --- time parameters ---
	std::chrono::time_point<std::chrono::high_resolution_clock> gameStartTime;
	float totalElapsedTime = 0.0f;
	float initialGameDuration = 300.0f;		// 5 minutes
	float gameTime = 300.0f;				// 5 minutes
	bool gameStarted = false;

	// --- HUD tracking variables ---
	int lastPassedCount = -1;		// tracking the number of passed rings
	int lastTotalSec    = -1;
	char hudBuffer[32];				// buffer big enough
	std::string lastTimeStr = "";   // tracking time
	int HUD_ID = 2;

	// --- drone parameters ---
	bool thirdPerson   = true;		
	bool topAngle   = false;		
	bool firstPerson    = false;		
	glm::vec3 global_pos_drone = glm::vec3(-1000.0f, 250.0f, 130.0f);  // drone's world coordinates
	float droneYaw = 0.0f, dronePitch = 0.0f, droneRoll = 0.0f;

	// --- valid bounds (world space) for the drone ---
	const float minX = -2960.0f;
	const float maxX =  960.0f;
	const float minY =  250.0f;
	const float maxY =  500.0f;
	const float minZ = -2960.0f;
	const float maxZ =  960.0f;

	// --- render pass ---
	RenderPass RP;

	// --- descriptor set layouts ---
	DescriptorSetLayout
		DSL_map,
		DSL_drone,
		DSL_overlay,
		DSL_skyBox,
		DSL_global;

	// --- vertex descriptors ---
	VertexDescriptor VD_phong, VD_pbr, VD_overlay, VD_skyBox;

	// --- pipelines ---
	Pipeline
		P_phong,
		P_pbr,
		P_overlay,
		P_skyBox;

	// --- model ---
	Model
		M_drone,
		M_overlay[3],
		M_skyBox;

	// --- textures ---
	Texture
		tex_drone_baseColor, tex_drone_normal, tex_drone_roughness, tex_drone_emissive,
		tex_overlay[3],
		tex_skyBox;

	// --- descriptor sets ---
	DescriptorSet
		DS_drone,
		DS_skyBox,
		DS_overlay[3],
		DS_global;

	// --- uniform buffers ---
	UniformBufferObject
		UBO_drone,
		ubosStart{},
		ubos{};

	OverlayUniformBlock UBO_overlay[3];
	SkyBoxUniformBufferObject UBO_skyBox;
	GlobalUniformBufferObject GUBO;

	// --- scene ---
	Scene SC;
	std::vector<VertexDescriptorRef>  VDRs;		// JSON Vertex Descriptors
	std::vector<TechniqueRef> PRs;				// JSON Techniques
	std::vector<glm::vec3> mountainPoints;

	// --- rings ---
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

	// flags to track which rings were passed
	std::vector<bool> ringPassed = std::vector<bool>(10, false);

	std::vector<float> ringScale;
	std::vector<glm::mat4> originalRingWm; // stores the world matrix of each ring


    // here we set the main application parameters (window size, title, vsync)
	void setWindowParameters();

	// what to do when the window changes size
	void onWindowResize(int w, int h);

    // here we load and setup all our Vulkan Models and Textures.
	// here we also create our DescriptorSetLayouts and load the Shaders for the pipelines
    void localInit();

    // here we create our pipelines and Descriptor Sets
	void pipelinesAndDescriptorSetsInit();

    // here we destroy our pipelines and Descriptor Sets
	// all the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup();

    // here we destroy all the Models, Texture and Desc. Set Layouts we created
	// all the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// we also have to destroy the pipelines: since they need to be rebuilt, they have two methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup();

	// here it is the creation of the command buffer:
	// we send to the GPU all the objects we want to draw, with their buffers and textures
	static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params);
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage);

    // here is where we update the uniforms.
	void updateUniformBuffer(uint32_t currentImage);

	// LookInDir matrix
	glm::mat4 LookInDirMat(glm::vec3 Pos, glm::vec3 Angs);

	// control camera positions
	void setCameraMode(GLFWwindow* w);

	// control drone movements
	void getDroneInput(GLFWwindow* w, float deltaT);

	// control GUBO light color
	void updateGlobalUBO(GlobalUniformBufferObject& gubo, float elapsedTime);

	void reset();

	// get mountain vertices for bounds
	void loadMountainPoints(std::vector<glm::vec3>& points);

	// bounds checking for the drone w.r.t. mountains
	bool isTooCloseToMountain(const glm::vec3& pos, const std::vector<glm::vec3>& mountainPoints, float threshold = 5.0f);

	// logic of passing through rings
	float checkRingPassage(glm::vec3 dronePos, std::vector<glm::vec3>& rings, std::vector<bool>& passed, float radius = 10.0f);
};