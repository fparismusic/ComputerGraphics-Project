// This has been adapted from the Vulkan tutorial
#include <sstream>

#include <json.hpp>

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"
#include "modules/Animations.hpp"
#include "modules/Utils.hpp"

using namespace std;
using namespace glm;

// --- Game States ---
enum class AppState {
	Menu,
	Playing
};

// MonumentSimulator: subclass of BaseProject
class MonumentSimulator : public BaseProject {
protected:

	// --- Menu fields ---
	AppState state = AppState::Menu;

	// --- Window parameters ---
	float Ar; // Aspect Ratio
	bool showStartText = true;
	bool showCommandsKeyboard = false;
	TextMaker menuTxt;

	// Camera controls
	glm::vec3 CamPos = glm::vec3(0.0f, 0.3f, 2.0f);;
	float CamYaw = 0.0f, CamPitch = 0.0f, CamRoll = 0.0f, CamDist = 0.0f;

	bool seenCenter   = true;
	bool seenFollow   = false;
	bool seenDrone    = false;
	glm::vec3 global_pos_drone = glm::vec3(0.0f);
	float droneYaw = 0.0f, dronePitch = 0.0f, droneRoll = 0.0f;
	const float deltaHeight = 4.0f;

	// Speed controls
	const float ROT_SPEED  = glm::radians(120.0f);
	const float MOVE_SPEED = 4.0f;

	// Time
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
	float totalElapsedTime = 0.0f;

	// --- Render Pass ---
	RenderPass RP;

	// --- Descriptor Set Layouts ---
	DescriptorSetLayout
		DSL_mountain,
		DSL_drone,
		DSL_skyBox,
		DSL_global;

	// --- Vertex Descriptors ---
	VertexDescriptor VD_phong, VD_pbr, VD_skyBox;

	// --- Pipelines ---
	Pipeline
		P_phong,
		P_pbr,
		P_skyBox;

	// --- Model ---
	Model
		M_mountain,
		M_drone,
		M_skyBox;

	// --- Textures ---
	Texture
		tex_mountain_baseColor,
		tex_drone_baseColor, tex_drone_normal, tex_drone_roughness, tex_drone_emissive,
		tex_skyBox;

	// --- Descriptor Sets ---
	DescriptorSet
		DS_mountain,
		DS_drone,
		DS_skyBox,
		DS_global;

	// --- Uniform Buffers ---
	UniformBufferObject
		UBO_mountain,
		UBO_drone;

	SkyBoxUniformBufferObject UBO_skyBox;
	GlobalUniformBufferObject GUBO;

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you set the main application parameters
	void setWindowParameters()
	{
		// Window size, title and initial background
		windowWidth = 1280;
		windowHeight = 720;
		windowTitle = "Drone Simulator";
		windowResizable = GLFW_TRUE;

		Ar = (float)windowWidth / (float)windowHeight;
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
	// What to do when the window changes size
	void onWindowResize(int w, int h)
	{
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
		// Update Render Pass
		RP.width = w;
		RP.height = h;

		// updates the textual output
		menuTxt.resizeScreen(w, h);
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you load and setup all your Vulkan Models and Textures.
	// Here you also create your DescriptorSetLayouts and load the Shaders for the pipelines
    void localInit()
    {
		// Descriptor Layouts [what will be passed to the shaders]
		DSL_global.init(this, {
			// this array contains the bindings:
			// first  element : the binding number
			// second element : the type of element (buffer or texture) using the corresponding Vulkan constant
			// third  element : the pipeline stage where it will be used using the corresponding Vulkan constant
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(GlobalUniformBufferObject), 1}
		});

		DSL_mountain.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(UniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
		});

		DSL_drone.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(UniformBufferObject), 1},
			// binding 1: baseColor (albedo)
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			// binding 2: metallic-roughness map
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			// binding 3: emissive
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			// binding 4: normal
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
		});

		DSL_skyBox.init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(SkyBoxUniformBufferObject), 1},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
		});

		//Initialize vertex descriptor for Vertex { vec3 pos; vec2 UV; vec3 norm; }
		VD_phong.init(this, {
			// this array contains the bindings
			// first  element : the binding number
			// second element : the stride of this binging
			// third  element : whether this parameter change per vertex or per instance using the corresponding Vulkan constant
			{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			// this array contains the location
			// first  element : the binding number
			// second element : the location number
			// third  element : the offset of this element in the memory record
			// fourth element : the data type of the element the corresponding Vulkan constant
			// fifth  elmenet : the size in byte of the element
			// sixth  element : a constant defining the element usage
			//                   POSITION - a vec3 with the position
			//                   NORMAL   - a vec3 with the normal vector
			//                   UV       - a vec2 with a UV coordinate
			//                   COLOR    - a vec4 with a RGBA color
			//                   TANGENT  - a vec4 with the tangent vector
			//                   OTHER    - anything else
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(vec3), POSITION},
			{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),  sizeof(vec2), UV},
			{0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm), sizeof(vec3), NORMAL}
		});

		VD_pbr.init(this, {
			{0, sizeof(VertexTan), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexTan, pos), sizeof(vec3), POSITION},
			{0, 1, VK_FORMAT_R32G32_SFLOAT,   offsetof(VertexTan, UV), sizeof(vec2), UV},
			{0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexTan, normal), sizeof(vec3), NORMAL},
			{0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexTan, tangent), sizeof(vec4), TANGENT}
		});

		VD_skyBox.init(this, {
			{0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos), sizeof(glm::vec3), POSITION},
			{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(skyBoxVertex, UV), sizeof(glm::vec2), UV}
		});

		// Render pass
		RP.init(this);
		// sets the background
		RP.properties[0].clearValue = {0.0f,0.9f,1.0f,1.0f};

		// Pipelines [Shader couples]
		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders
		// The last array, is a vector of pointer to the layouts of the sets that will be used in this pipeline. The first element will be set 0, and so on..
		P_phong.init(this, &VD_phong, "shaders/Phong.vert.spv", "shaders/Phong.frag.spv", { &DSL_global, &DSL_mountain });
		P_phong.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		P_phong.setCullMode(VK_CULL_MODE_BACK_BIT);
		P_phong.setPolygonMode(VK_POLYGON_MODE_FILL);

		P_pbr.init(this, &VD_pbr, "shaders/PBR.vert.spv", "shaders/PBR.frag.spv", { &DSL_global, &DSL_drone });
		P_pbr.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		P_pbr.setCullMode(VK_CULL_MODE_NONE);
		P_pbr.setPolygonMode(VK_POLYGON_MODE_FILL);

		P_skyBox.init(this, &VD_skyBox, "shaders/SkyBox.vert.spv", "shaders/Skybox.frag.spv", { &DSL_global, &DSL_skyBox });
		P_skyBox.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		P_skyBox.setCullMode(VK_CULL_MODE_NONE);
		P_skyBox.setPolygonMode(VK_POLYGON_MODE_FILL);

		// Create models
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ or GLTF
		M_mountain.init(this, &VD_phong, "assets/models/mountain.gltf", GLTF);
		M_drone.init(this, &VD_pbr, "assets/models/drone.gltf", GLTF);
		M_skyBox.init(this, &VD_skyBox, "assets/models/skybox.gltf", GLTF);

		// Create the textures
		// The second parameter is the file name
		tex_mountain_baseColor.init(this, "assets/textures/Mountain/material_0_baseColor_4096.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);

		tex_drone_baseColor.init(this, "assets/textures/Drone/DefaultMaterial_baseColor.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);
		tex_drone_roughness.init(this, "assets/textures/Drone/DefaultMaterial_metallicRoughness.png", VK_FORMAT_R8G8B8A8_SRGB, true);
		tex_drone_emissive.init(this, "assets/textures/Drone/DefaultMaterial_emissive.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);
		tex_drone_normal.init(this,    "assets/textures/Drone/DefaultMaterial_normal.jpeg", VK_FORMAT_R8G8B8A8_UNORM, true);

		tex_skyBox.init(this, "assets/textures/Sky_diffuse.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);

		// Number of UBO and textures that we will use
		DPSZs.uniformBlocksInPool = 5;  // UBOs
		DPSZs.texturesInPool      = 8;  // Textures
		DPSZs.setsInPool          = 5;  // DS

		// INIT TEXT
		cout << "Initializing text\n";
		menuTxt.init(this, windowWidth, windowHeight);
		cout << "Initialization completed!\n";

		submitCommandBuffer("main", 0, populateCommandBufferAccess, this);

		startTime = std::chrono::high_resolution_clock::now();

		menuTxt.print(1.0f, 1.0f, "[ENTER] Start Simulation\n[H] Help & Controls\n[ESC] Exit\n",1,"CO",false,false,true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
    }

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit()
	{
		// creates the render pass
		RP.create();

		// This creates a new pipeline (with the current surface), using its shaders
		P_phong.create(&RP);
		P_pbr.create(&RP);
		P_skyBox.create(&RP);

		// Here you define the data set
		DS_global.init(this, &DSL_global, {});

		std::vector<VkDescriptorImageInfo> tex_mountain = {
			tex_mountain_baseColor.getViewAndSampler()
		};
		DS_mountain.init(this, &DSL_mountain, tex_mountain);

		std::vector<VkDescriptorImageInfo> tex_drone = {
			tex_drone_baseColor.getViewAndSampler(),
			tex_drone_roughness.getViewAndSampler(),
			tex_drone_emissive.getViewAndSampler(),
			tex_drone_normal.getViewAndSampler()
		};
		DS_drone.init(this, &DSL_drone, tex_drone);

		std::vector<VkDescriptorImageInfo> tex_sky = {
			tex_skyBox.getViewAndSampler()
		};
		DS_skyBox.init(this, &DSL_skyBox, tex_sky);

		// INIT TEXT
		menuTxt.pipelinesAndDescriptorSetsInit();
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup()
	{
		// Cleanup pipelines
		P_phong.cleanup();
		P_pbr.cleanup();
		P_skyBox.cleanup();

		// Cleanup datasets
		DS_global.cleanup();
		DS_mountain.cleanup();
		DS_drone.cleanup();
		DS_skyBox.cleanup();

		// Cleanup render pass
		RP.cleanup();

		// INIT TEXT
		menuTxt.pipelinesAndDescriptorSetsCleanup();
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup()
	{
		// Cleanup textures
		tex_mountain_baseColor.cleanup();

		tex_drone_baseColor.cleanup();
		tex_drone_roughness.cleanup();
		tex_drone_emissive.cleanup();
		tex_drone_normal.cleanup();

		tex_skyBox.cleanup();

		// Cleanup models
		M_mountain.cleanup();
		M_drone.cleanup();
		M_skyBox.cleanup();

		// Cleanup descriptor set layouts
		DSL_global.cleanup();
		DSL_mountain.cleanup();
		DSL_drone.cleanup();
		DSL_skyBox.cleanup();

		// Destroy the pipelines
		P_phong.destroy();
		P_pbr.destroy();
		P_skyBox.destroy();

		RP.destroy();

		// INIT TEXT
		menuTxt.localCleanup();
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params)
	{
		// Simple trick to avoid having always 'T->'
		//std::cout << "Populating command buffer for " << currentImage << "\n";
		MonumentSimulator *T = (MonumentSimulator *) Params;
		T->populateCommandBuffer(commandBuffer, currentImage);
	}
    // Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw, with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage)
	{
		// Begin standard pass
		//RP.begin(commandBuffer, currentImage);

		if (state == AppState::Menu) {
			RP.begin(commandBuffer, currentImage);
			menuTxt.print(0.0f, 0.0f, "[ENTER] Start Simulation\n[H] Help & Controls\n[ESC] Exit\n",1,"CO",false,false,true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
			menuTxt.updateCommandBuffer();
			RP.end(commandBuffer);
			return;
		}

		RP.begin(commandBuffer, currentImage);

		// For a pipeline object, this command binds the corresponing pipeline to the command buffer passed in its parameter binds the data set
		P_phong.bind(commandBuffer);
		// For a Dataset object, this command binds the corresponing dataset to the command buffer and pipeline passed in its first and second parameters.
		// The third parameter is the number of the set being bound
		// As described in the Vulkan tutorial, a different dataset is required for each image in the swap chain.
		// This is done automatically in file Starter.hpp, however the command here needs also the index of the current image in the swap chain, passed in its last parameter binds the model
		DS_global.bind(commandBuffer, P_phong, 0, currentImage);
		DS_mountain.bind(commandBuffer, P_phong, 1, currentImage);

		// For a Model object, this command binds the corresponing index and vertex buffer
		// to the command buffer passed in its parameter
		// record the drawing command in the command buffer
		M_mountain.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M_mountain.indices.size()), 1, 0, 0, 0);

		P_pbr.bind(commandBuffer);
		DS_global.bind(commandBuffer, P_pbr, 0, currentImage);
		DS_drone.bind(commandBuffer,  P_pbr, 1, currentImage);
		M_drone.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M_drone.indices.size()), 1, 0, 0, 0);

		// P_skyBox pipeline
		P_skyBox.bind(commandBuffer);
		DS_global.bind(commandBuffer, P_skyBox, 0, currentImage);
		DS_skyBox.bind(commandBuffer, P_skyBox, 1, currentImage);
		M_skyBox.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M_skyBox.indices.size()), 1, 0, 0, 0);

		RP.end(commandBuffer);
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here is where you update the uniforms. Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage)
	{
		bool escPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
		bool hPressed   = glfwGetKey(window, GLFW_KEY_H)      == GLFW_PRESS;
		bool cPressed   = glfwGetKey(window, GLFW_KEY_C)      == GLFW_PRESS;
		bool enterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;

		switch (state)
		{
		case AppState::Menu:
			if (escPressed) {
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			}
			else if (hPressed) {
				menuTxt.print(0.0f, 0.0f, "Move with W-A-S-D | Q-E | R-F\nMove arrows to look around\nChange camera with I-O-P\nPress SPACE to take pictures\nPress C to close this text\nPress ESC to return to the menu", 2, "CO", false, false, true, TAL_LEFT, TRH_LEFT, TRV_TOP, {1.0f, 0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.0f, 1.0f});
				menuTxt.updateCommandBuffer();
			}
			else if (cPressed) {
				menuTxt.removeText(2);
			}
			else if (enterPressed) {
				menuTxt.removeText(1);
				menuTxt.removeText(2);
				state = AppState::Playing;
				cout << "Launch the simualation...!\n";
				RebuildPipeline();
			}
			break;

		case AppState::Playing:
			if (escPressed) {
				menuTxt.removeText(1);
				state = AppState::Menu;
				cout << "Return to Menu...!\n";
				RebuildPipeline();
			}
			if (showStartText)
			{
				menuTxt.removeText(1);
				menuTxt.print(0.0f, 0.0f, "Drone Simulator\n\nFilippo Paris\nFrancesco Moretti\nMoein Zadeh", 1, "CO", false, false, true, TAL_LEFT, TRH_LEFT, TRV_TOP, {1.0f, 0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.0f, 1.0f});
				menuTxt.updateCommandBuffer();
			}
			else if (showCommandsKeyboard)
			{
				menuTxt.removeText(1);
				menuTxt.print(0.0f, 0.0f, "Move with W-A-S-D | Q-E | R-F\nMove arrows to look around\nChange camera with I-O-P\nPress SPACE to take pictures\nPress C to close this text\nPress ESC to return to the menu", 1, "CO", false, false, true, TAL_LEFT, TRH_LEFT, TRV_TOP, {1.0f, 0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.0f, 1.0f});
				menuTxt.updateCommandBuffer();
			}
			else
			{
				menuTxt.removeText(1);
				menuTxt.print(0.0f, 0.0f, "Press K (Keyboard)\nto see the command list", 1, "CO", false, false, true, TAL_LEFT, TRH_LEFT, TRV_TOP, {1.0f, 0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.0f, 1.0f});
				menuTxt.updateCommandBuffer();
			}
			break;
		}
		//-----------------------------------------------------------------------------------------------------
		//-----------------------------------------------------------------------------------------------------
		// State == Playing: update the game logic
		float time = chrono::duration<float>(chrono::high_resolution_clock::now() - startTime).count();
		static int index = 0;
		static bool debounce = false;
		static int curDebounce = 0;

		// First, we manage the text display
		if (showStartText) {
			if (time > 5.0f) {
				showStartText = false;
			}
		}
		// With [K] we can read game controls
		if (glfwGetKey(window, GLFW_KEY_K)) {
			showCommandsKeyboard = true;
			RebuildPipeline();
		}
		// With [C] we can close the game controls
		if (glfwGetKey(window, GLFW_KEY_C)) {
			showCommandsKeyboard = false;
			RebuildPipeline();
		}
		// With [SPACE] we can take the pictures
		if(glfwGetKey(window, GLFW_KEY_SPACE)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_SPACE;

				showCommandsKeyboard = false;
				std::string filename = "screen-" + std::to_string(index++) + ".png";
				saveScreenshot(filename.c_str(), currentImage);
				//RebuildPipeline();
			}
		} else {
			if((curDebounce == GLFW_KEY_SPACE) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}
		//-----------------------------------------------------------------------------------------------------
		//-----------------------------------------------------------------------------------------------------
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire); // deltaT: time since last frame
		totalElapsedTime += deltaT;
		getDroneInput(window, deltaT); // update drone position and orientation
		setCameraMode(window); // set camera mode based on key presses

		// proj
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 100.0f);
		proj[1][1] *= -1;  // Vulkan

		glm::vec3 dronePos = global_pos_drone;

		// view
		glm::mat4 view;
		const float DRONE_SCALE = 0.05f;  // or whatever your drone scale is

		if (seenCenter) {
			glm::vec3 camP = dronePos + glm::vec3(0, 0, 4.0f * DRONE_SCALE);  // adjust based on how far you want to be
			view = LookAtMat(camP, dronePos, 0.0f);
		}
		else if (seenFollow) {
			glm::vec3 camP = dronePos + glm::vec3(0, 2.0f * DRONE_SCALE, 2.0f * DRONE_SCALE);  // follow from above/behind
			view = LookInDirMat(camP, glm::vec3(droneYaw, dronePitch, droneRoll));
		}
		else if (seenDrone) {
			glm::vec3 camP = dronePos + glm::vec3(glm::rotate(
				glm::mat4(1.0f),
				droneYaw,
				glm::vec3(0, 1, 0)) * glm::vec4(0.0f, 0.5f * DRONE_SCALE, 0.0f, 1.0f));
			view = LookInDirMat(camP, glm::vec3(droneYaw, dronePitch, droneRoll));
		}


		CamPos = glm::vec3(glm::inverse(view)[3]);

/*		CamYaw   = CamYaw   - ROT_SPEED * deltaT * r.y; // camera rotation around Y axis
		CamPitch = CamPitch - ROT_SPEED * deltaT * r.x; // camera rotation around X axis
		CamPitch = CamPitch < glm::radians(-90.0f) ? glm::radians(-90.0f) :
				  (CamPitch > glm::radians( 90.0f) ? glm::radians( 90.0f) : CamPitch); //clamp
		CamRoll = CamRoll - ROT_SPEED * deltaT * r.z; // camera rotation around Z axis

		// Complete Rotation: roll (Z) → pitch (X) → yaw (Y)
		glm::mat4 R_yaw   = glm::rotate(glm::mat4(1.0f), CamYaw,   glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 R_pitch = glm::rotate(glm::mat4(1.0f), CamPitch, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 R_roll  = glm::rotate(glm::mat4(1.0f), CamRoll,  glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 R = R_yaw * R_pitch * R_roll;
		glm::vec3 forward = glm::normalize(glm::vec3(R * glm::vec4(0, 0, -1, 0)));
		glm::vec3 right   = glm::normalize(glm::vec3(R * glm::vec4(1, 0,  0, 0)));
		glm::vec3 up      = glm::normalize(glm::vec3(R * glm::vec4(0, 1,  0, 0)));

		// Position of the camera in the world
		CamPos += MOVE_SPEED * m.y * forward * deltaT;  // W/S
		CamPos += MOVE_SPEED * m.x * right   * deltaT;  // A/D
		CamPos += MOVE_SPEED * m.z * up      * deltaT;  // R/F
*/
		GUBO.proj = proj;
		GUBO.view = view;
		GUBO.cameraPos = CamPos;
		GUBO.time = totalElapsedTime;
		updateGlobalUBO(GUBO, totalElapsedTime);
		DS_global.map(currentImage, &GUBO, 0);

		// UBO mountain
		glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f));  // scale terrain up

		UBO_mountain.mvpMat = proj * view * model;
		UBO_mountain.mMat   = model;
		UBO_mountain.nMat   = glm::transpose(glm::inverse(model));
		DS_mountain.map(currentImage, &UBO_mountain, 0);

		// UBO drone
		// model
		glm::mat4 modelDrone = glm::translate(glm::mat4(1.0f), global_pos_drone)
							 * glm::rotate(glm::mat4(1.0f), droneYaw,   glm::vec3(0,1,0))
							 * glm::rotate(glm::mat4(1.0f), dronePitch, glm::vec3(1,0,0))
							 * glm::rotate(glm::mat4(1.0f), droneRoll,  glm::vec3(0,0,1))
							 * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,1,0))
							 * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));  // Shrink drone;;




		UBO_drone.mvpMat = proj * view * modelDrone;
		UBO_drone.mMat = modelDrone;
		UBO_drone.nMat = glm::transpose(glm::inverse(modelDrone));
		DS_drone.map(currentImage, &UBO_drone, 0);

		// Sky Box UBO update
		glm::mat4 skyboxModel =
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0)) *  // fix orientation
			glm::scale(glm::mat4(1.0f), glm::vec3(50.0f));                             // enlarge

		UBO_skyBox.mvpMat = proj * glm::mat4(glm::mat3(view)) * skyboxModel;

		DS_skyBox.map(currentImage, &UBO_skyBox, 0);
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
	// Here are some util functions
	glm::mat4   LookAtMat(glm::vec3 Pos, glm::vec3 aim, float Roll) {
	    glm::mat4 I(1.0f);
	    glm::mat4 R = glm::rotate(I, glm::radians(Roll), glm::vec3(0,1,0));
	    return R * glm::lookAt(Pos, aim, glm::vec3(0,1,0));
	}

	glm::mat4   LookInDirMat(glm::vec3 Pos, glm::vec3 Angs) {
	    glm::mat4 I(1.0f);
	    glm::mat4 T   = glm::translate(I, -Pos);
	    glm::mat4 Ry  = glm::rotate(I, -Angs.x, glm::vec3(0,1,0));
	    glm::mat4 Rx  = glm::rotate(I, -Angs.y, glm::vec3(1,0,0));
	    glm::mat4 Rz  = glm::rotate(I, -Angs.z, glm::vec3(0,0,1));
	    return Rz * Rx * Ry * T;
	}

	void setCameraMode(GLFWwindow* w) {
	    if (glfwGetKey(w, GLFW_KEY_I)) { seenCenter=true; seenFollow=false; seenDrone=false;  } // 3-rd
	    if (glfwGetKey(w, GLFW_KEY_O)) { seenCenter=false; seenFollow=true; seenDrone=false;  } // 1-st
	    if (glfwGetKey(w, GLFW_KEY_P)) { seenCenter=false; seenFollow=false; seenDrone=true;  } // 1-st
	}

	void getDroneInput(GLFWwindow* w, float deltaT) {
	    const float ROT_SPEED  = glm::radians(45.0f);
	    const float MOVE_SPEED = 4.0f;

	    // rotations
	    if(glfwGetKey(w, GLFW_KEY_LEFT))  droneYaw   += deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_RIGHT)) droneYaw   -= deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_UP))    dronePitch += deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_DOWN))  dronePitch -= deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_Q))     droneRoll  -= deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_E))     droneRoll  += deltaT * ROT_SPEED;

		// traslations
	    glm::mat4 R_yaw = glm::rotate(glm::mat4(1.0f), droneYaw, glm::vec3(0,1,0));
	    glm::vec3 forward = glm::vec3(R_yaw * glm::vec4(0,0,-1,0));
	    glm::vec3 right   = glm::vec3(R_yaw * glm::vec4(1,0, 0,0));
	    if(glfwGetKey(w, GLFW_KEY_W))    global_pos_drone += MOVE_SPEED * forward * deltaT;
	    if(glfwGetKey(w, GLFW_KEY_S))    global_pos_drone -= MOVE_SPEED * forward * deltaT;
	    if(glfwGetKey(w, GLFW_KEY_D))    global_pos_drone += MOVE_SPEED * right   * deltaT;
	    if(glfwGetKey(w, GLFW_KEY_A))    global_pos_drone -= MOVE_SPEED * right   * deltaT;
	    if(glfwGetKey(w, GLFW_KEY_R))    global_pos_drone += MOVE_SPEED * glm::vec3(0,1,0) * deltaT;
	    if(glfwGetKey(w, GLFW_KEY_F))    global_pos_drone -= MOVE_SPEED * glm::vec3(0,1,0) * deltaT;
	}

	const glm::vec3 dawnColor    = glm::vec3(0.8f, 0.4f, 0.2f);
	const glm::vec3 noonColor    = glm::vec3(1.0f, 1.0f, 0.9f);
	const glm::vec3 sunsetColor  = glm::vec3(0.9f, 0.3f, 0.1f);

	void updateGlobalUBO(GlobalUniformBufferObject& gubo, float elapsedTime)
	{
		gubo.time = elapsedTime;

		// 3 min  cycle
		float t = fmod(elapsedTime, 180.0f);

		// Sun color and direction
		glm::vec3 lightColor;
		if (t < 60.0f) {
			float f = glm::smoothstep(0.0f, 60.0f, t);
			lightColor = glm::mix(dawnColor, noonColor, f);
		}
		else if (t < 120.0f) {
			float f = glm::smoothstep(60.0f, 120.0f, t);
			lightColor = glm::mix(noonColor, sunsetColor, f);
		}
		else {
			float f = glm::smoothstep(120.0f, 180.0f, t);
			lightColor = glm::mix(sunsetColor, dawnColor, f);
		}
		gubo.lightColor = lightColor;

		// Sun angle and direction
		float angle = (t / 180.0f) * glm::two_pi<float>();  // 0→2π
		glm::vec3 sunDir = glm::normalize(glm::vec3(cos(angle), sin(angle), 0.3f));
		gubo.lightDir = sunDir;

		gubo.lightIntensity = glm::clamp(glm::dot(sunDir, glm::vec3(0.0f, 1.0f, 0.0f)), 0.3f, 1.0f);
	};
};
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
/*
 * MAIN FUNCTION: do not touch this
 */
int main() {
    MonumentSimulator app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}