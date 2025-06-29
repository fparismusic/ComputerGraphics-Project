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

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
void loadMountainPoints(std::vector<glm::vec3>& points) {
	std::ifstream objFile("assets/models/snowyMountain.obj");
	if (!objFile.is_open()) {
		std::cerr << "Error loading snowyMountain.obj" << std::endl;
		return;
	}

	std::vector<glm::vec3> rawVertices;
	std::string line;
	while (std::getline(objFile, line)) {
		if (line.substr(0, 2) == "v ") {
			std::istringstream iss(line.substr(2));
			glm::vec3 v;
			iss >> v.x >> v.y >> v.z;
			rawVertices.push_back(v);
		}
	}

	objFile.close();

	// Define transforms for the 4 mountains
	std::vector<std::pair<glm::vec3, float>> transforms = {
		{{0, 150, 0}, 0.0f},
		{{-2000, 150, 0}, 180.0f},
		{{-2000, 150, -2000}, 180.0f},
		{{0, 150, -2000}, 0.0f}
	};

	float scale = 2000.0f;

	for (const auto& [pos, yawDeg] : transforms) {
		float yawRad = glm::radians(yawDeg);
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), yawRad, glm::vec3(0, 1, 0));
		for (const auto& v : rawVertices) {
			glm::vec3 scaled = v * scale;
			glm::vec3 rotated = glm::vec3(rotation * glm::vec4(scaled, 1.0f));
			glm::vec3 transformed = rotated + pos;
			points.push_back(transformed);
		}
	}
	std::cout << "Loaded " << points.size() << " mountain vertices.\n";
}


bool isTooCloseToMountain(const glm::vec3& pos, const std::vector<glm::vec3>& mountainPoints, float threshold = 5.0f) {
	for (const auto& pt : mountainPoints) {
		if (glm::distance(pos, pt) < threshold)
			return true;
	}
	return false;
}

float checkRingPassage(glm::vec3 dronePos, std::vector<glm::vec3>& rings, std::vector<bool>& passed, float radius = 10.0f) {
	float res = 0.0f;
	for (size_t i = 0; i < rings.size(); ++i) {
		if (!passed[i] && glm::distance(dronePos, rings[i]) < radius) {
			passed[i] = true;
			std::cout << "Great, Ring " << (i + 1) << " passed!" << std::endl;
		}
	}

	// Check for game completion
	if (std::all_of(passed.begin(), passed.end(), [](bool b) { return b; })) {
		std::cout << "🎉 All rings passed! Game complete!" << std::endl;
		res =  1.0f;
	}
	return res;
}
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// --- Game States ---
enum class AppState {
	Menu,
	Playing,
	GameOver
};

// MonumentSimulator: subclass of BaseProject
class MonumentSimulator : public BaseProject {
protected:
	
	// --- Menu fields ---
	AppState state = AppState::Menu;
	TextMaker menuTxt;
	float gameOver = 0.0f;
	bool showStartText = false;
	bool showCommandsKeyboard = false;
	bool prevEscPressed = false;

	// --- Window parameters ---
	float Ar; // Aspect Ratio

	// Camera controls
	glm::vec3 CamPos = glm::vec3(0.0f, 0.3f, 2.0f);
	float CamYaw = 0.0f, CamPitch = 0.0f, CamRoll = 0.0f, CamDist = 0.0f;

	//Time parameters
	float gameTime = 300.0f; // 5 minutes
	bool gameStarted = false;
	std::chrono::time_point<std::chrono::high_resolution_clock> gameStartTime;

	// --- HUD tracking variables ---
	int lastPassedCount = -1;  // Per tracciare quando cambia il conteggio degli anelli
	std::string lastTimeStr = "";  // Per tracciare quando cambia il tempo

	// --- Drone parameters ---
	bool seenCenter   = true;
	bool seenFollow   = false;
	bool seenDrone    = false;
	glm::vec3 global_pos_drone = glm::vec3(-1000.0f, 250.0f, 130.0f);
	float droneYaw = 0.0f, dronePitch = 0.0f, droneRoll = 0.0f;

	const float minX = -3000.0f;
	const float maxX =  1000.0f;
	const float minY =  250.0f;
	const float maxY =  500.0f;
	const float minZ = -3000.0f;
	const float maxZ =  1000.0f;

	// Time
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
	float totalElapsedTime = 0.0f;

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

	Scene SC;
	std::vector<VertexDescriptorRef>  VDRs;
	std::vector<TechniqueRef> PRs;
	std::vector<glm::vec3> mountainPoints;


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

		DSL_map.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(UniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
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

		DSL_overlay.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(OverlayUniformBlock), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
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

		VD_overlay.init(this, {
			{0, sizeof(VertexOverlay), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, pos), sizeof(glm::vec2), OTHER},
			{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, UV), sizeof(glm::vec2), UV}
		});

		VD_skyBox.init(this, {
			{0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos), sizeof(glm::vec3), POSITION},
			{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(skyBoxVertex, UV), sizeof(glm::vec2), UV}
		});

		VDRs.resize(2);
		VDRs[0].init("VD_phong",   &VD_phong);
		VDRs[1].init("VD_pbr",   &VD_pbr);

		// Render pass
		RP.init(this);
		loadMountainPoints(mountainPoints);

		// sets the background
		RP.properties[0].clearValue = {0.53f, 0.81f, 0.92f, 0.8f};

		// Pipelines [Shader couples]
		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders
		// The last array, is a vector of pointer to the layouts of the sets that will be used in this pipeline. The first element will be set 0, and so on..
		P_phong.init(this, &VD_phong, "shaders/Phong.vert.spv", "shaders/Phong.frag.spv", { &DSL_global, &DSL_map });
		P_phong.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		P_phong.setCullMode(VK_CULL_MODE_NONE);
		P_phong.setPolygonMode(VK_POLYGON_MODE_FILL);

		P_pbr.init(this, &VD_pbr, "shaders/PBR.vert.spv", "shaders/PBR.frag.spv", { &DSL_global, &DSL_drone });
		P_pbr.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		P_pbr.setCullMode(VK_CULL_MODE_NONE);
		P_pbr.setPolygonMode(VK_POLYGON_MODE_FILL);

		P_overlay.init(this, &VD_overlay, "shaders/OverlayVert.vert.spv", "shaders/OverlayFrag.frag.spv", { &DSL_overlay });
		P_overlay.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		P_overlay.setCullMode(VK_CULL_MODE_NONE);
		P_overlay.setPolygonMode(VK_POLYGON_MODE_FILL);

		P_skyBox.init(this, &VD_skyBox, "shaders/SkyBox.vert.spv", "shaders/Skybox.frag.spv", { &DSL_global, &DSL_skyBox });
		P_skyBox.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		P_skyBox.setCullMode(VK_CULL_MODE_NONE);
		P_skyBox.setPolygonMode(VK_POLYGON_MODE_FILL);

		PRs.resize(2);
		PRs[0].init("Phong", {
							 {&P_phong, {//Pipeline and DSL for the first pass
								 /*DSL_global*/{},
								 /*DSL_map*/{
										/*t0*/{true,  0, {}},// index 0 of the "texture" field in the json file
										/*t1*/{true,  1, {}} // index 1 of the "texture" field in the json file
									 }
									}}
							 }, /*TotalNtextures*/2, &VD_phong),
		PRs[1].init("PBR", {
								{&P_pbr, {//Pipeline and DSL for the first pass
							 		/*DSLglobal*/{},
									/*DSLlocalPBR*/{
										/*t0*/{true,  0, {}},// index 0 of the "texture" field in the json file
										/*t1*/{true,  1, {}},// index 1 of the "texture" field in the json file
										/*t2*/{true,  2, {}},// index 2 of the "texture" field in the json file
										/*t3*/{true,  3, {}}// index 3 of the "texture" field in the json file
										}
										}}
								}, /*TotalNtextures*/4, &VD_pbr);
		// Create models
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ or GLTF
		M_drone.init(this, &VD_pbr, "assets/models/drone.gltf", GLTF);
		M_skyBox.init(this, &VD_skyBox, "assets/models/skybox.gltf", GLTF);

		// Create HUD screens
		float ndc_width  = 1472.0f / windowWidth * 2.0f;
		float ndc_height = 832.0f / windowHeight * 2.0f;
		ndc_width  *= 0.85f;
		ndc_height *= 0.85f;
		// Center the overlay in NDC coordinates
		glm::vec2 anchor = glm::vec2(-ndc_width / 2.0f, -ndc_height / 2.0f);
		for (int i = 0; i < 3; i++) {
			M_overlay[i].vertices.clear();
			std::vector<VertexOverlay> tempVerts = {
				{ glm::vec2(anchor.x, anchor.y), glm::vec2(0.0f, 0.0f) },
				{ glm::vec2(anchor.x, anchor.y + ndc_height), glm::vec2(0.0f, 1.0f) },
				{ glm::vec2(anchor.x + ndc_width, anchor.y), glm::vec2(1.0f, 0.0f) },
				{ glm::vec2(anchor.x + ndc_width, anchor.y + ndc_height), glm::vec2(1.0f, 1.0f) },
			};

			// Copia binaria nei vertices
			M_overlay[i].vertices.resize(sizeof(VertexOverlay) * tempVerts.size());
			std::memcpy(M_overlay[i].vertices.data(), tempVerts.data(), M_overlay[i].vertices.size());

			M_overlay[i].indices = { 0, 1, 2,    1, 2, 3 };
			M_overlay[i].initMesh(this, &VD_overlay);
		}

		// Create the textures
		// The second parameter is the file name
		tex_drone_baseColor.init(this, "assets/textures/Drone/DefaultMaterial_baseColor.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);
		tex_drone_roughness.init(this, "assets/textures/Drone/DefaultMaterial_metallicRoughness.png", VK_FORMAT_R8G8B8A8_SRGB, true);
		tex_drone_emissive.init(this, "assets/textures/Drone/DefaultMaterial_emissive.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);
		tex_drone_normal.init(this,    "assets/textures/Drone/DefaultMaterial_normal.jpeg", VK_FORMAT_R8G8B8A8_UNORM, true);

		tex_overlay[0].init(this, "assets/textures/Menu/menu.png");
		tex_overlay[1].init(this, "assets/textures/Menu/win.jpg");
		tex_overlay[2].init(this, "assets/textures/Menu/lose.jpg");

		tex_skyBox.init(this, "assets/textures/Sky_diffuse.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);

		// Number of UBO and textures that we will use
		DPSZs.uniformBlocksInPool = 1*10 +  // Rings
									1*4  +  // Mountain
									1*1	 +  // Station
									1*1	 +  // Drone
									1*3	 +  // Menu
									1*1	 +  // SkyBox
									1*1;	// GUBO
		DPSZs.texturesInPool      = 2*10 +  // Rings
									2*4	 +  // Mountain
									1*4  +  // Station
									4*1	 +  // Drone
									1*3  +  // Menu
									1*1  +  // SkyBox
									1*1;	// Fonts
		DPSZs.setsInPool          = 6;		// DS

		std::cout << "\nLoading the scene\n\n";
		if(SC.init(this, /*Npasses*/1, VDRs, PRs, "assets/models/scene.json") != 0) {
			std::cout << "ERROR LOADING THE SCENE\n";
			exit(0);
		}

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
		P_overlay.create(&RP);
		P_skyBox.create(&RP);

		// Here you define the data set
		DS_global.init(this, &DSL_global, {});

		std::vector<VkDescriptorImageInfo> tex_drone = {
			tex_drone_baseColor.getViewAndSampler(),
			tex_drone_roughness.getViewAndSampler(),
			tex_drone_emissive.getViewAndSampler(),
			tex_drone_normal.getViewAndSampler()
		};
		DS_drone.init(this, &DSL_drone, tex_drone);

		std::vector<VkDescriptorImageInfo> tex_screen0 = {
			tex_overlay[0].getViewAndSampler()
		};
		DS_overlay[0].init(this, &DSL_overlay, tex_screen0);
		std::vector<VkDescriptorImageInfo> tex_screen1 = {
			tex_overlay[1].getViewAndSampler()
		};
		DS_overlay[1].init(this, &DSL_overlay, tex_screen1);
		std::vector<VkDescriptorImageInfo> tex_screen2 = {
			tex_overlay[2].getViewAndSampler()
		};
		DS_overlay[2].init(this, &DSL_overlay, tex_screen2);

		std::vector<VkDescriptorImageInfo> tex_sky = {
			tex_skyBox.getViewAndSampler()
		};
		DS_skyBox.init(this, &DSL_skyBox, tex_sky);

		SC.pipelinesAndDescriptorSetsInit();

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
		P_overlay.cleanup();
		P_skyBox.cleanup();

		// Cleanup datasets
		DS_global.cleanup();
		DS_drone.cleanup();
		for (int i = 0; i < 3; i++) {
			DS_overlay[i].cleanup();
		}
		DS_skyBox.cleanup();

		// Cleanup render pass
		RP.cleanup();

		SC.pipelinesAndDescriptorSetsCleanup();

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
		tex_drone_baseColor.cleanup();
		tex_drone_roughness.cleanup();
		tex_drone_emissive.cleanup();
		tex_drone_normal.cleanup();
		for (int i = 0; i < 3; i++) {
			tex_overlay[i].cleanup();
		}
		tex_skyBox.cleanup();

		// Cleanup models
		M_drone.cleanup();
		for (int i = 0; i < 3; i++) {
			M_overlay[i].cleanup();
		}
		M_skyBox.cleanup();

		// Cleanup descriptor set layouts
		DSL_global.cleanup();
		DSL_map.cleanup();
		DSL_drone.cleanup();
		DSL_overlay.cleanup();
		DSL_skyBox.cleanup();

		// Destroy the pipelines
		P_phong.destroy();
		P_pbr.destroy();
		P_overlay.destroy();
		P_skyBox.destroy();

		RP.destroy();

		SC.localCleanup();

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
    // —— MENU ——
    if (state == AppState::Menu) {
        RP.begin(commandBuffer, currentImage);

        UBO_overlay[0].visible = true;
        UBO_overlay[1].visible = false;
        UBO_overlay[2].visible = false;

        P_overlay.bind(commandBuffer);
        M_overlay[0].bind(commandBuffer);
        DS_overlay[0].bind(commandBuffer, P_overlay, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_overlay[0].indices.size()),
            1, 0, 0, 0
        );

        // Ristampo il testo del menu
        menuTxt.print(
            -0.85f, 0.7f,
            "[ENTER] Start Simulation   [H] Help & Controls   [ESC] Exit",
            1, "CO",
            true, true, false,
            TAL_LEFT, TRH_LEFT, TRV_BOTTOM,
            {0,0,0,1}, {0,0,0,0}
        );
        menuTxt.updateCommandBuffer();

        RP.end(commandBuffer);
        return;
    }

	
    // —— GAME OVER ——
    if (state == AppState::GameOver) {
        RP.begin(commandBuffer, currentImage);

        // Mostro solo la texture win/lose  
        UBO_overlay[0].visible = false;
        UBO_overlay[1].visible = (gameOver > 0.0f);
        UBO_overlay[2].visible = (gameOver < 0.0f);

        // Disabilita sempre depth-test/write e forza il passaggio
        P_overlay.bind(commandBuffer);
        for (int i = 1; i <= 2; ++i) {
            M_overlay[i].bind(commandBuffer);
            DS_overlay[i].bind(commandBuffer, P_overlay, 0, currentImage);
            vkCmdDrawIndexed(
                commandBuffer,
                static_cast<uint32_t>(M_overlay[i].indices.size()),
                1, 0, 0, 0
            );
        }

        // “Back to Menu”
        menuTxt.print(
            0.0f, -0.8f,
            "[B] Back to Menu", 3, "SS",
            true, true, false,
            TAL_CENTER, TRH_CENTER, TRV_BOTTOM,
            {1,1,1,1},{0,0,0,0.7f}
        );
        menuTxt.updateCommandBuffer();

        RP.end(commandBuffer);
        return;
    }

    // 2) Altrimenti procedo con il rendering “normale” di scena, drone, HUD, skybox...
    RP.begin(commandBuffer, currentImage);

    // --- scena e drone ---
    P_phong.bind(commandBuffer);
    DS_global.bind(commandBuffer, P_phong, 0, currentImage);

    P_pbr.bind(commandBuffer);
    DS_global.bind(commandBuffer, P_pbr, 0, currentImage);
    DS_drone.bind(commandBuffer,  P_pbr, 1, currentImage);
    M_drone.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer,
        static_cast<uint32_t>(M_drone.indices.size()), 1, 0, 0, 0
    );

    SC.populateCommandBuffer(commandBuffer, 0, currentImage);

    // --- HUD (se vuoi tenerlo in PLAYING) ---
    P_overlay.bind(commandBuffer);
    for (int i = 0; i < 3; i++) {
        M_overlay[i].bind(commandBuffer);
        DS_overlay[i].bind(commandBuffer, P_overlay, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_overlay[i].indices.size()), 1, 0, 0, 0
        );
    }

    // --- skybox ---
    P_skyBox.bind(commandBuffer);
    DS_global.bind(commandBuffer, P_skyBox, 0, currentImage);
    DS_skyBox.bind(commandBuffer, P_skyBox, 1, currentImage);
    M_skyBox.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer,
        static_cast<uint32_t>(M_skyBox.indices.size()), 1, 0, 0, 0
    );
menuTxt.print(
  -0.95f, -0.8f,   
  "Filippo Paris\nFrancesco Moretti\nMoein Zadeh",
  3, "CO",
  false, true, false,
  TAL_LEFT, TRH_LEFT, TRV_BOTTOM,   
  {1, 1, 1, 1},
  {0, 0, 0, 1.0f}  
);
menuTxt.print(-0.95f, -0.52f,
        "Move with W-A-S-D | Q-E | R-F\n"
        "Move arrows to look around\n"
        "Change camera with I-O-P\n"
        "Press ESC to return to the menu",
        4, "SS",
        false, true, true,
        TAL_LEFT, TRH_LEFT, TRV_BOTTOM,
        {1,0.99,0.99f,1}, 
		{0,0,0,1.0f}
        );

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
		bool bPressed   = glfwGetKey(window, GLFW_KEY_B)      == GLFW_PRESS;
		bool enterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;

		bool escJustPressed = escPressed && !prevEscPressed;
		const float DRONE_SCALE = 0.065f;

		// ─── 1) LOGICA DI GIOCO (solo PLAYING) ─────────────────────────
if (state == AppState::Playing) {
	if (escJustPressed) {
        // una sola volta al momento in cui premi ESC
		menuTxt.removeText(2);
		menuTxt.removeText(3);
		menuTxt.removeText(4);
        reset();
        state = AppState::Menu;
        std::cout << "Return to Menu...!\n";
        RebuildPipeline();
        return;
    }
    // 1.a) Avvia timer
    if (!gameStarted) {
        gameStartTime = std::chrono::high_resolution_clock::now();
        gameStarted   = true;
    }
    // 1.b) Calcola gameTime residuo
    float elapsed = std::chrono::duration<float>(
        std::chrono::high_resolution_clock::now() - gameStartTime).count();
    gameTime = std::max(0.0f, 300.0f - elapsed);

	if (gameTime < 0.1f) {
    seenCenter = false;
    seenFollow = false;
    seenDrone  = true;  
}

    // 1.c) Input drone + collisioni (checkRingPassage inside)
    float deltaT; glm::vec3 m, r; bool fire=false;
    getSixAxis(deltaT, m, r, fire);
    totalElapsedTime += deltaT;
    getDroneInput(window, deltaT);
    setCameraMode(window);

    // 1.d) Conteggio anelli + formattazione timer
    int passedCount = std::count(ringPassed.begin(), ringPassed.end(), true);
    int totalSec    = static_cast<int>(std::ceil(gameTime));
    char timeStr[6];
    std::snprintf(timeStr, sizeof(timeStr), "%02d:%02d",
                  totalSec/60, totalSec%60);

    // 1.e) Aggiorna HUD se cambia
    if (passedCount != lastPassedCount || std::string(timeStr) != lastTimeStr) {
        lastPassedCount = passedCount;
        lastTimeStr     = timeStr;
        std::string hud = "Rings: " + std::to_string(passedCount)
                        + "/10   Time: " + timeStr;
        constexpr int HUD_ID = 2;
        menuTxt.removeText(HUD_ID);
        menuTxt.print(0.0f, 0.90f, hud, HUD_ID, "SS",
                      false, true, false,
                      TAL_CENTER, TRH_CENTER, TRV_TOP,
                      {1,1,1,1}, {0,0,0,1});
        menuTxt.updateCommandBuffer();
		
    }

    // 1.f) Verifica GameOver
    if (gameTime <= 0.0f) {
        gameOver = -1.0f;
        UBO_overlay[2].visible = true;
		menuTxt.removeText(2);
		menuTxt.removeText(3);
		menuTxt.removeText(4);
        state = AppState::GameOver;
    } else if (passedCount == static_cast<int>(ringPassed.size())) {
        gameOver = 1.0f;
        UBO_overlay[1].visible = true;
		menuTxt.removeText(2);
		menuTxt.removeText(3);
		menuTxt.removeText(4);
        state = AppState::GameOver;
    }

    // 1.g) ESC torna a Menu
    if (escPressed) {
        reset();
        state = AppState::Menu;
        RebuildPipeline();
    }
}

// ─── 2) SWITCH sui restanti stati ───────────────────────────────
switch (state) {
  case AppState::Menu:
    // bind overlay menu
    for (int i = 0; i < 3; ++i)
        DS_overlay[i].map(currentImage, &UBO_overlay[i], 0);

    if (hPressed) {
      // stampa help…
      menuTxt.updateCommandBuffer();
    } else if (cPressed) {
      menuTxt.removeText(2);
    } else if (enterPressed) {
      menuTxt.removeText(1);
    menuTxt.removeText(2);
    reset();

    // Nascondi il menu overlay proprio adesso:
    UBO_overlay[0].visible = false;
    UBO_overlay[1].visible = false;
    UBO_overlay[2].visible = false;
    // (assicurati poi di mappare questi UBO, se necessario)

    state = AppState::Playing;
    showStartText = true;
    RebuildPipeline();
    } else if (escPressed) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    break;

  case AppState::GameOver:
  	seenCenter = false;
    seenFollow = false;
    seenDrone  = true;
    // bind overlay win/lose
    for (int i = 0; i < 3; ++i)
        DS_overlay[i].map(currentImage, &UBO_overlay[i], 0);

    if (bPressed) {
      reset();
      state = AppState::Menu;
      RebuildPipeline();
    }
    break;

  default: break;
}

    // ─── 3) Calcolo delle matrici di vista e proiezione ───────────────
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 5000.0f);
    proj[1][1] *= -1;

    glm::mat4 view;
    // (ricalcola view esattamente come prima, in base a seenCenter/seenFollow/seenDrone)
    if (seenCenter) {
        glm::vec3 offset = glm::vec3(0, 0, 4.0f * DRONE_SCALE);
        glm::mat4 R = glm::rotate(glm::mat4(1.0f), droneYaw,   glm::vec3(0,1,0))
                    * glm::rotate(glm::mat4(1.0f), dronePitch, glm::vec3(1,0,0))
                    * glm::rotate(glm::mat4(1.0f), droneRoll,  glm::vec3(0,0,1));
        glm::vec3 camP = global_pos_drone + glm::vec3(R * glm::vec4(offset,0.0f));
        glm::vec3 up  = glm::normalize(glm::vec3(R * glm::vec4(0,1,0,0)));
        view = glm::lookAt(camP, global_pos_drone, up);
    }
    else if (seenFollow) {
        glm::vec3 camP = global_pos_drone + glm::vec3(0, 4.0f*DRONE_SCALE, 1.5f*DRONE_SCALE);
        view = LookInDirMat(camP, {droneYaw, dronePitch, droneRoll});
    }
    else {
        // seenDrone
        glm::vec3 camP = global_pos_drone + glm::vec3(
            glm::rotate(glm::mat4(1.0f), droneYaw, glm::vec3(0,1,0))
            * glm::vec4(0, 1.5f*DRONE_SCALE, 0, 1));
        view = LookInDirMat(camP, {droneYaw, dronePitch, droneRoll});
    }

    // ─── 4) AGGIORNO UBO GLOBALI ───────────────────────────────────────
    GUBO.proj       = proj;
    GUBO.view       = view;
    GUBO.cameraPos  = CamPos = glm::vec3(glm::inverse(view)[3]);
    GUBO.lightDir   = glm::normalize(glm::vec3(0,1,0));
    GUBO.time       = totalElapsedTime;
    updateGlobalUBO(GUBO, totalElapsedTime);
    DS_global.map(currentImage, &GUBO, 0);

// ─── 5) UBOs Montagna ─────────────────────────────────────────────
for (int i = 0; i < SC.TI[0].InstanceCount; ++i) {
    ubos.mMat   = SC.TI[0].I[i].Wm;
    ubos.mvpMat = proj * view * ubos.mMat;
    ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));
    // Mappa prima il GUBO (set 0), poi ubos (set 1):
    SC.TI[0].I[i].DS[0][0]->map(currentImage, &GUBO, 0);
    SC.TI[0].I[i].DS[0][1]->map(currentImage, &ubos, 0);
}

// ─── 6) UBOs Stazione ────────────────────────────────────────────
for (int i = 0; i < SC.TI[1].InstanceCount; ++i) {
    ubosStart.mMat   = SC.TI[1].I[i].Wm;
    ubosStart.mvpMat = proj * view * ubosStart.mMat;
    ubosStart.nMat   = glm::inverse(glm::transpose(ubosStart.mMat));
    // Anche qui, prima GUBO poi ubosStart:
    SC.TI[1].I[i].DS[0][0]->map(currentImage, &GUBO, 0);
    SC.TI[1].I[i].DS[0][1]->map(currentImage, &ubosStart, 0);
}

    // ─── 7) UBO Drone ────────────────────────────────────────────────
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), global_pos_drone)
                        * glm::rotate(glm::mat4(1.0f), droneYaw,   glm::vec3(0,1,0))
                        * glm::rotate(glm::mat4(1.0f), dronePitch, glm::vec3(1,0,0))
                        * glm::rotate(glm::mat4(1.0f), droneRoll,  glm::vec3(0,0,1))
                        * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,1,0))
                        * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
        UBO_drone.mvpMat = proj * view * model;
        UBO_drone.mMat   = model;
        UBO_drone.nMat   = glm::inverse(glm::transpose(model));
        DS_drone.map(currentImage, &UBO_drone, 0);
    }

    // ─── 8) UBO SkyBox ───────────────────────────────────────────────
    {
        glm::mat4 skyModel = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1,0,0))
                           * glm::scale(glm::mat4(1.0f), glm::vec3(50.0f));
        UBO_skyBox.mvpMat = proj * glm::mat4(glm::mat3(view)) * skyModel;
        DS_skyBox.map(currentImage, &UBO_skyBox, 0);
    }
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
	    const float MOVE_SPEED = 50.0f;

	    // rotations
	    if(glfwGetKey(w, GLFW_KEY_LEFT))  droneYaw   += deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_RIGHT)) droneYaw   -= deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_UP))    dronePitch += deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_DOWN))  dronePitch -= deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_Q))     droneRoll  -= deltaT * ROT_SPEED;
	    if(glfwGetKey(w, GLFW_KEY_E))     droneRoll  += deltaT * ROT_SPEED;

		// traslations
		// Movement
		glm::mat4 R_yaw = glm::rotate(glm::mat4(1.0f), droneYaw, glm::vec3(0,1,0));
		glm::vec3 forward = glm::vec3(R_yaw * glm::vec4(0,0,-1,0));
		glm::vec3 right   = glm::vec3(R_yaw * glm::vec4(1,0, 0,0));

		if (glfwGetKey(w, GLFW_KEY_W)) {
			glm::vec3 attempt = global_pos_drone + MOVE_SPEED * forward * deltaT;
			if (!isTooCloseToMountain(attempt, mountainPoints)) global_pos_drone = attempt;
		}
		if (glfwGetKey(w, GLFW_KEY_S)) {
			glm::vec3 attempt = global_pos_drone - MOVE_SPEED * forward * deltaT;
			if (!isTooCloseToMountain(attempt, mountainPoints)) global_pos_drone = attempt;
		}
		if (glfwGetKey(w, GLFW_KEY_D)) {
			glm::vec3 attempt = global_pos_drone + MOVE_SPEED * right * deltaT;
			if (!isTooCloseToMountain(attempt, mountainPoints)) global_pos_drone = attempt;
		}
		if (glfwGetKey(w, GLFW_KEY_A)) {
			glm::vec3 attempt = global_pos_drone - MOVE_SPEED * right * deltaT;
			if (!isTooCloseToMountain(attempt, mountainPoints)) global_pos_drone = attempt;
		}
		if (glfwGetKey(w, GLFW_KEY_R)) {
			glm::vec3 attempt = global_pos_drone + MOVE_SPEED * glm::vec3(0,1,0) * deltaT;
			if (!isTooCloseToMountain(attempt, mountainPoints)) global_pos_drone = attempt;
		}
		if (glfwGetKey(w, GLFW_KEY_F)) {
			glm::vec3 attempt = global_pos_drone - MOVE_SPEED * glm::vec3(0,1,0) * deltaT;
			if (!isTooCloseToMountain(attempt, mountainPoints)) global_pos_drone = attempt;
		}

		// Clamp the drone position to a defined range
		global_pos_drone.x = glm::clamp(global_pos_drone.x, minX, maxX);
		global_pos_drone.y = glm::clamp(global_pos_drone.y, minY, maxY);
		global_pos_drone.z = glm::clamp(global_pos_drone.z, minZ, maxZ);

		gameOver = checkRingPassage(global_pos_drone, ringPositions, ringPassed);

		if (glfwGetKey(w, GLFW_KEY_P) == GLFW_PRESS) {
			std::cout << "Drone Position: "
					  << global_pos_drone.x << ", "
					  << global_pos_drone.y << ", "
					  << global_pos_drone.z << std::endl;
		}
	}

	const glm::vec3 dawnColor = glm::vec3(1.0f, 0.45f, 0.2f);
	const glm::vec3 noonColor = glm::vec3(1.0f, 1.0f, 0.95f);
	const glm::vec3 sunsetColor = glm::vec3(1.0f, 0.2f, 0.1f);

	void updateGlobalUBO(GlobalUniformBufferObject& gubo, float elapsedTime)
	{
		gubo.time = elapsedTime;

		// 3 min  cycle
		float t = fmod(elapsedTime, 300.0f);

		// Sun color and direction
		glm::vec3 lightColor;
		if (t < 60.0f) {
			float f = glm::smoothstep(0.0f, 60.0f, t);
			gubo.lightColor = glm::mix(dawnColor, noonColor, f);
			gubo.lightIntensity = 0.8f; // light intensity
		}
		else if (t < 120.0f) {
			float f = glm::smoothstep(60.0f, 120.0f, t);
			gubo.lightColor = glm::mix(noonColor, sunsetColor, f);
			gubo.lightIntensity = 1.0f; // light intensity
		}
		else {
			float f = glm::smoothstep(120.0f, 180.0f, t);
			gubo.lightColor = glm::mix(sunsetColor, dawnColor, f);
			gubo.lightIntensity = 0.8f; // light intensity
		}
	};

	void reset() {
		gameOver = 0.0f;
		showStartText = false;
		showCommandsKeyboard = false;

		CamPos = glm::vec3(0.0f, 0.3f, 2.0f);
		CamYaw = 0.0f, CamPitch = 0.0f, CamRoll = 0.0f, CamDist = 0.0f;

		seenCenter   = true;
		seenFollow   = false;
		seenDrone    = false;
		global_pos_drone = glm::vec3(-1000.0f, 250.0f, 130.0f);
		droneYaw = 0.0f, dronePitch = 0.0f, droneRoll = 0.0f;

		ringPassed = std::vector<bool>(10, false);
		 // Resetta il timer
		gameTime = 300.0f;
		gameStarted = false;
		lastPassedCount = -1;
		lastTimeStr = "";
	}
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
