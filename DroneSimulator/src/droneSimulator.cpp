#include "modules/droneSimulator.hpp"

using namespace std;
using namespace glm;


// here we set the main application parameters
void DroneSimulator::setWindowParameters()
{
	// window size, title and initial background
	windowWidth = 1280;
	windowHeight = 720;
	windowTitle = "Drone Simulator";
	windowResizable = GLFW_TRUE;

	Ar = (float)windowWidth / (float)windowHeight;
}

void DroneSimulator::onWindowResize(int w, int h)
{
	std::cout << "Window resized to: " << w << " x " << h << "\n";
	Ar = (float)w / (float)h;
	// update Render Pass size window
	RP.width = w;
	RP.height = h;

	// update the textual output
	menuTxt.resizeScreen(w, h);
}
// here we load and setup all our Vulkan Models and Textures.
// we also create our DescriptorSetLayouts and load the Shaders for the pipelines
void DroneSimulator::localInit()
{
	// descriptor Layouts [what will be passed to the shaders]
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

	//initialize vertex descriptor for Vertex { vec3 pos; vec2 UV; vec3 norm; }
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
		//                   POSITION - a vec3 with the position of vertexes in 3D space
		//                   NORMAL   - a vec3 with the normal vector: it is used to calculate the light reflection
		//                   UV       - a vec2 with a UV coordinate: it is used to map textures on the surface or decide which part of the texture to use
		//                   COLOR    - a vec4 with a RGBA color
		//                   TANGENT  - a vec4 with the tangent vector: it is used to calculate the light reflection in advanced shading techniques
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

	// VD inside scene.JSON
	VDRs.resize(2);
	VDRs[0].init("VD_phong", &VD_phong);
	VDRs[1].init("VD_pbr", &VD_pbr);

	// render pass
	RP.init(this);
	loadMountainPoints(mountainPoints);

	// sets RP background
	RP.properties[0].clearValue = {0.53f, 0.81f, 0.92f, 0.8f};

	// pipelines [shader couples]
	// the second parameter is the pointer to the vertex definition
	// third and fourth parameters are respectively the vertex and fragment shaders
	// the last array, is a vector of pointer to the layouts of the sets that will be used in this pipeline. The first element will be set 0, and so on..
	P_phong.init(this, &VD_phong, "shaders/Phong.vert.spv", "shaders/Phong.frag.spv", { &DSL_global, &DSL_map });
	P_phong.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);		// helps avoid artifacts in coplanar geometry or transparent surfaces
	P_phong.setCullMode(VK_CULL_MODE_NONE);				// useful if we want to render both sides of a surface
	P_phong.setPolygonMode(VK_POLYGON_MODE_FILL);	// fill mode is the default, but we can also use VK_POLYGON_MODE_LINE for wireframe rendering

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

	// JSON Techniques mapping
	PRs.resize(2);
	PRs[0].init("Phong", {
						 {&P_phong, {//pipeline and DSL for the first pass
							 /*DSL_global*/{},
							 /*DSL_map*/{
									/*t0*/{true,  0, {}},// index 0 of the "texture" field in the json file
									/*t1*/{true,  1, {}} // index 1 of the "texture" field in the json file
								 }
								}}
						 }, /*TotalNtextures*/2, &VD_phong),
	PRs[1].init("PBR", {
							{&P_pbr, {//pipeline and DSL for the first pass
							 	/*DSLglobal*/{},
								/*DSLlocalPBR*/{
									/*t0*/{true,  0, {}},// index 0 of the "texture" field in the json file
									/*t1*/{true,  1, {}},// index 1 of the "texture" field in the json file
									/*t2*/{true,  2, {}},// index 2 of the "texture" field in the json file
									/*t3*/{true,  3, {}}// index 3 of the "texture" field in the json file
									}
									}}
							}, /*TotalNtextures*/4, &VD_pbr);
	// create models
	// the second parameter is the pointer to the vertex definition for this model
	// the third parameter is the file name
	// the last is a constant specifying the file type: currently only OBJ or GLTF
	M_drone.init(this, &VD_pbr, "assets/models/drone.gltf", GLTF);
	M_skyBox.init(this, &VD_skyBox, "assets/models/skybox.gltf", GLTF);

	// create HUD screens
	// calculate the width and height in NDC (Normalized Device Coordinates)
	// original HUD texture size is 1472x832, so we scale it based on window size and convert to NDC ([-1, 1])
	float ndc_width  = 1472.0f / windowWidth * 2.0f;
	float ndc_height = 832.0f / windowHeight * 2.0f;
	ndc_width  *= 0.85f; // uniform scaling factor 85%
	ndc_height *= 0.85f; // uniform scaling factor 85%
	// we compute the anchor point so that the overlay is centered in NDC space
	glm::vec2 anchor = glm::vec2(-ndc_width / 2.0f, -ndc_height / 2.0f);

	// creating 3 different overlays (for menu, win screen, and lose screen)
	for (int i = 0; i < 3; i++) {
		M_overlay[i].vertices.clear();
		// building a 2D quad (rectangle of 2 triangles) [NDC coordinates + UV]
		std::vector<VertexOverlay> tempVerts = {
			{ glm::vec2(anchor.x, anchor.y), glm::vec2(0.0f, 0.0f) },					// bottom-left (0)
			{ glm::vec2(anchor.x, anchor.y + ndc_height), glm::vec2(0.0f, 1.0f) },	// top-left	(1)
			{ glm::vec2(anchor.x + ndc_width, anchor.y), glm::vec2(1.0f, 0.0f) },		// bottom-right (2)
			{ glm::vec2(anchor.x + ndc_width, anchor.y + ndc_height), glm::vec2(1.0f, 1.0f) }, // top-right (3)
		};

		// copy vertex data into the model’s vertex buffer
		M_overlay[i].vertices.resize(sizeof(VertexOverlay) * tempVerts.size());
		std::memcpy(M_overlay[i].vertices.data(), tempVerts.data(), M_overlay[i].vertices.size());

		// define the two triangles that make up the quad
		M_overlay[i].indices = { 0, 1, 2,    1, 2, 3 };
		M_overlay[i].initMesh(this, &VD_overlay);
	}

	// create the textures
	tex_drone_baseColor.init(this, "assets/textures/Drone/DefaultMaterial_baseColor.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);
	tex_drone_roughness.init(this, "assets/textures/Drone/DefaultMaterial_metallicRoughness.png", VK_FORMAT_R8G8B8A8_SRGB, true);
	tex_drone_emissive.init(this, "assets/textures/Drone/DefaultMaterial_emissive.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);
	tex_drone_normal.init(this,    "assets/textures/Drone/DefaultMaterial_normal.jpeg", VK_FORMAT_R8G8B8A8_UNORM, true);

	tex_overlay[0].init(this, "assets/textures/Menu/menu.png");
	tex_overlay[1].init(this, "assets/textures/Menu/win.jpg");
	tex_overlay[2].init(this, "assets/textures/Menu/lose.jpg");

	tex_skyBox.init(this, "assets/textures/Sky_diffuse.jpeg", VK_FORMAT_R8G8B8A8_SRGB, true);

	// number of UBO and textures that we will use
	DPSZs.uniformBlocksInPool = 1*10 +  // rings
								1*4  +  // mountain
								1*1	 +  // station
								1*1	 +  // drone
								1*3	 +  // menu
								1*1	 +  // skyBox
								1*1;	// GUBO
	DPSZs.texturesInPool      = 2*10 +  // rings
								2*4	 +  // mountain
								1*4  +  // station
								4*1	 +  // drone
								1*3  +  // menu
								1*1  +  // skyBox
								1*1;	// fonts
	DPSZs.setsInPool          = 6;		// DS we have created

	std::cout << "\nLoading the scene\n\n";
	if(SC.init(this, /*Npasses*/1, VDRs, PRs, "assets/models/scene.json") != 0) {
		std::cout << "ERROR LOADING THE SCENE\n";
		exit(0);
	}

	// BAKING (precomputing and storing) ALGORITHM FOR RINGS -> we make them disappear in this way
	size_t totalInstances = SC.TI[0].InstanceCount; // total number of instances in the first type of 'objects' in JSON
	size_t firstRingIndex = 4;                      //  first 4 elements are mountains
	size_t numRings = totalInstances - firstRingIndex;

	ringScale      = std::vector<float>(numRings, 1.0f); // vector to hold the scale of each ring (1.0 no scaling)
	originalRingWm = std::vector<glm::mat4>(numRings); // vector to store the original world model matrices (Wm) of each ring

	// initialize a random number generator
	// this will be used to generate random yaw rotations around the Y-axis
	std::mt19937                         rng{ std::random_device{}() };
	// uniform distribution of yaw angles in degrees between -180 and 180
	std::uniform_real_distribution<float> yawDistr(-180.0f, 180.0f);

	// looping over all the rings to apply a random yaw rotation
	for (size_t r = 0; r < numRings; ++r) {
	    int j = int(firstRingIndex + r);

		// getting the original world model matrix (Wm) for this instance
		// this matrix is constructed as: Translation * Rotation(-90 degrees on X-axis) * Scale(2,2,2)
	    glm::mat4 base = SC.TI[0].I[j].Wm;

	    // random Yaw
	    float randomYaw = glm::radians(yawDistr(rng));

		// building a transformation matrix that:
		// 1 - translates the coordinate system to the pivot,
		// 2 - rotates around the Y-axis by the random yaw,
		// 3 - translates back to the original coordinate system,
		// 4 - applies the original base transformation (which already contains rotation and scale)
	    glm::vec3 pivot = glm::vec3(base[3]);   // Column 3 is the position
	    glm::mat4 R = glm::translate(glm::mat4(1.0f), pivot)
	                * glm::rotate   (glm::mat4(1.0f), randomYaw, glm::vec3(0,1,0))
	                * glm::translate(glm::mat4(1.0f), -pivot)
	                * base;

	    // save the bake and apply as the current world model matrix (Wm) for this ring
	    originalRingWm[r] = R;
	    SC.TI[0].I[j].Wm = R;
	}

	// INIT TEXT
	cout << "Initializing text\n";
	menuTxt.init(this, windowWidth, windowHeight);
	cout << "Initialization completed!\n";

	submitCommandBuffer("main", 0, populateCommandBufferAccess, this);

	menuTxt.print(0.0f, 0.95f, "[ENTER] Start Simulation\n[S] Show Help & Controls\n[ESC] Exit\n",
					1,"CO",false,false,true,
					TAL_RIGHT,	// text alignment
					TRH_RIGHT,	// horizontal anchor point
					TRV_BOTTOM, // vertical anchor point
					{1.0f,1.0f,1.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
	const char* diffLabel =
		currentDifficulty==Difficulty::Easy   ? "Easy (6 minutes)" :
		currentDifficulty==Difficulty::Medium?  "Medium (5 minutes)" :
		                                        "Hard (4 minutes)";
	menuTxt.print(
		1.0f, 0.9f,
		std::string("Difficulty: ") + diffLabel,
		2, "SS", false,false,true,
		TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,
		{1,1,1,1},{0,0,0,0.5f}
	);
}

// here we create our pipelines and Descriptor Sets
void DroneSimulator::pipelinesAndDescriptorSetsInit()
{
	// creates the render pass
	RP.create();

	// this creates a new pipeline (with the current surface), using its shaders
	P_phong.create(&RP);
	P_pbr.create(&RP);
	P_overlay.create(&RP);
	P_skyBox.create(&RP);

	// here we define the data set
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

// here we destroy our pipelines and Descriptor Sets
// all the object classes defined in Starter.hpp have a method .cleanup() for this purpose
void DroneSimulator::pipelinesAndDescriptorSetsCleanup()
{
	// cleanup pipelines
	P_phong.cleanup();
	P_pbr.cleanup();
	P_overlay.cleanup();
	P_skyBox.cleanup();

	// cleanup datasets
	DS_global.cleanup();
	DS_drone.cleanup();
	for (int i = 0; i < 3; i++) {
		DS_overlay[i].cleanup();
	}
	DS_skyBox.cleanup();

	// cleanup render pass
	RP.cleanup();

	SC.pipelinesAndDescriptorSetsCleanup();

	// INIT TEXT
	menuTxt.pipelinesAndDescriptorSetsCleanup();
}

// here we destroy all the Models, Texture and Desc. Set Layouts we created.
// all the object classes defined in Starter.hpp have a method .cleanup() for this purpose
// we also have to destroy the pipelines: since they need to be rebuilt, they have two methods: .cleanup() recreates them, while .destroy() delete them completely
void DroneSimulator::localCleanup()
{
	// cleanup textures
	tex_drone_baseColor.cleanup();
	tex_drone_roughness.cleanup();
	tex_drone_emissive.cleanup();
	tex_drone_normal.cleanup();
	for (int i = 0; i < 3; i++) {
		tex_overlay[i].cleanup();
	}
	tex_skyBox.cleanup();

	// cleanup models
	M_drone.cleanup();
	for (int i = 0; i < 3; i++) {
		M_overlay[i].cleanup();
	}
	M_skyBox.cleanup();

	// cleanup descriptor set layouts
	DSL_global.cleanup();
	DSL_map.cleanup();
	DSL_drone.cleanup();
	DSL_overlay.cleanup();
	DSL_skyBox.cleanup();

	// destroy the pipelines
	P_phong.destroy();
	P_pbr.destroy();
	P_overlay.destroy();
	P_skyBox.destroy();

	RP.destroy();

	SC.localCleanup();

	// INIT TEXT
	menuTxt.localCleanup();
}

// for the JSON scene, we need to populate the command buffer once
void DroneSimulator::populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params)
{
	// simple trick to avoid having always 'T->'
	//std::cout << "Populating command buffer for " << currentImage << "\n";
	DroneSimulator *T = (DroneSimulator *) Params;
	T->populateCommandBuffer(commandBuffer, currentImage);
}
// here it is the creation of the command buffer:
// we send to the GPU all the objects we want to draw, with their buffers and textures
void DroneSimulator::populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage)
{
    // —— MENU ────────────────────────────────────────────────────────────────────────────────────────────────────
    if (state == AppState::Menu) {
        RP.begin(commandBuffer, currentImage);

        UBO_overlay[0].visible = true; // menu overlay is always visible in the menu state
        UBO_overlay[1].visible = false;
        UBO_overlay[2].visible = false;

        P_overlay.bind(commandBuffer);
        M_overlay[0].bind(commandBuffer);
        DS_overlay[0].bind(commandBuffer, P_overlay, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_overlay[0].indices.size()),
            1, 0, 0, 0
        );

        // re-print the menu text
        menuTxt.print(
            -0.85f, 0.7f,
            "[ENTER] Start Simulation   [S] Show Help & Controls   [ESC] Exit",
            1, "SS",
            true, true, false,
            TAL_LEFT, TRH_LEFT, TRV_BOTTOM,
            {0,0,0,1}, {0,0,0,0}
        );

		// re-print the difficulty text
    	const char* diffText = "";
	    switch (currentDifficulty) {
			case Difficulty::Easy:   diffText = "Difficulty: Easy (6 minutes)";   break;
			case Difficulty::Medium: diffText = "Difficulty: Medium (5 minutes)"; break;
			case Difficulty::Hard:   diffText = "Difficulty: Hard (4 minutes)";   break;
	    }

	    menuTxt.print(
	        0.0f, 0.90f,
	        diffText,
	        2, "SS",
	        true, false, false,
	        TAL_CENTER, TRH_CENTER, TRV_BOTTOM,
	        {0,0,0,1}, {0,0,0,0}
	    );

        menuTxt.updateCommandBuffer();

        RP.end(commandBuffer);
        return;
    }

    // —— GAME OVER ──────────────────────────────────────────────────────────────────────────────────────────────
    if (state == AppState::GameOver) {
        RP.begin(commandBuffer, currentImage);

        // showing only the appropriate overlay based on gameOver value
        UBO_overlay[0].visible = false;
        UBO_overlay[1].visible = (gameOver > 0.0f);
        UBO_overlay[2].visible = (gameOver < 0.0f);

        P_overlay.bind(commandBuffer);
        for (int i = 1; i <= 2; ++i) {
        	M_overlay[i].bind(commandBuffer);
            DS_overlay[i].bind(commandBuffer, P_overlay, 0, currentImage);
            vkCmdDrawIndexed(commandBuffer,
            	static_cast<uint32_t>(M_overlay[i].indices.size()),
                1, 0, 0, 0
            );
        }

        RP.end(commandBuffer);
        return;
    }

    // —— NORMAL RENDERING ───────────────────────────────────────────────────────────────────────────────────────
    RP.begin(commandBuffer, currentImage);

    // --- scene and drone ---
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

    // --- HUD ---
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

	// --- in-game text ---
	menuTxt.print(
	  -0.95f, -0.8f,
	  "Filippo Paris\nFrancesco Moretti\nMoein Zadeh",
	  3, "CO",
	  true, true, true,
	  TAL_LEFT, TRH_LEFT, TRV_BOTTOM,
	  {1.0f, 0.98f, 0.9f, 1.0f},
	  {0.2f, 0.2f, 0.2f, 1.0f}
	);

	menuTxt.print(-0.95f, -0.52f,
		"Move with W-A-S-D / Q-E / R-F\n"
	    "Move arrows to look around\n"
	    "Change camera with I / O / P\n"
	    "Press ESC to return to the menu",
	    4, "SS",
	    false, true, true,
	    TAL_LEFT, TRH_LEFT, TRV_BOTTOM,
	    {1.0f,0.98f,0.9f,1.0f},
		{0.2f, 0.2f, 0.2f, 1.0f}
	);

    RP.end(commandBuffer);
}

// here is where we update the uniforms.
void DroneSimulator::updateUniformBuffer(uint32_t currentImage)
{
	bool escPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
	bool ePressed   = glfwGetKey(window, GLFW_KEY_E)      == GLFW_PRESS;
	bool mPressed   = glfwGetKey(window, GLFW_KEY_M)      == GLFW_PRESS;
	bool hPressed   = glfwGetKey(window, GLFW_KEY_H)      == GLFW_PRESS;
	bool sPressed   = glfwGetKey(window, GLFW_KEY_S)      == GLFW_PRESS;
	bool cPressed   = glfwGetKey(window, GLFW_KEY_C)      == GLFW_PRESS;
	bool bPressed   = glfwGetKey(window, GLFW_KEY_B)      == GLFW_PRESS;
	bool enterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;

	bool escJustPressed = escPressed && !prevEscPressed;
	const float DRONE_SCALE = 0.065f;

	// ─── GAME LOGIC (only PLAYING) ────
	if (state == AppState::Playing) {
		if (escJustPressed) {
	        // ESC pressed: reset game
			menuTxt.removeText(2);
			menuTxt.removeText(3);
			menuTxt.removeText(4);
	        reset();
	        state = AppState::Menu;
	        std::cout << "Return to Menu...!\n";
	        RebuildPipeline();
	        return;
	    }
	    // start timer
	    if (!gameStarted) { // if the game has not started yet
	        gameStartTime = std::chrono::high_resolution_clock::now();
	        gameStarted   = true;
	    }
	    // calculating remaining game time
	    float elapsed = std::chrono::duration<float>(
	        std::chrono::high_resolution_clock::now() - gameStartTime).count();
			gameTime = std::max(0.0f, initialGameDuration - elapsed);
		if (gameTime < 0.1f) {
		    thirdPerson = false;
		    topAngle = false;
		    firstPerson  = true;
		}

	    // input drone and collisions (checkRingPassage inside)
	    float deltaT; glm::vec3 m, r; bool fire=false;
	    getSixAxis(deltaT, m, r, fire); // assign deltaT, movement and rotation vectors
	    totalElapsedTime += deltaT;
	    getDroneInput(window, deltaT);
	    setCameraMode(window);

		// ring collision detection
		int passedCount = std::count(ringPassed.begin(), ringPassed.end(), true);
		int totalSec    = static_cast<int>(std::ceil(gameTime));

		// update HUD only if there are changes
		if (passedCount != lastPassedCount || totalSec != lastTotalSec) { // if the number of passed rings or the time has changed
		    lastPassedCount = passedCount;
		    lastTotalSec    = totalSec;

		    // writing in buffer: "Rings: XX/10 Time: MM:SS"
		    int mins = totalSec / 60;
		    int secs = totalSec % 60;
		    std::snprintf(hudBuffer, sizeof(hudBuffer),
		                  "Rings: %2d/10   Time: %02d:%02d",
		                  passedCount, mins, secs);

		    // remove old text and print a new one
		    menuTxt.removeText(HUD_ID); // HUD_ID = 2
		    menuTxt.print(
		        0.0f, 0.85f,
		        hudBuffer,
		        HUD_ID, "SS",
		        false, true, false,
		        TAL_CENTER, TRH_CENTER, TRV_TOP,
		        {1.0f,0.98f,0.9f,1.0f}, {0.2f, 0.2f, 0.2f, 1.0f}
		    );
		    menuTxt.updateCommandBuffer();
		}

	    // verify gameOver
	    if (gameTime <= 0.0f) { // lose condition
	        gameOver = -1.0f;
	        UBO_overlay[2].visible = true;
			menuTxt.removeText(2);
			menuTxt.removeText(3);
			menuTxt.removeText(4);
	        state = AppState::GameOver;
	    } else if (passedCount == static_cast<int>(ringPassed.size())) { // win condition
	        gameOver = 1.0f;
	        UBO_overlay[1].visible = true;
			menuTxt.removeText(2);
			menuTxt.removeText(3);
			menuTxt.removeText(4);
	        state = AppState::GameOver;
	    }

	    // ESC to menu
	    if (escPressed) {
	        reset();
	        state = AppState::Menu;
	        RebuildPipeline();
	    }
	}

	// ─── SWITCH on remaining states ────────────────────────────────────────────────────────────────────────────
	switch (state) {
		case AppState::Menu:
			// bind overlay menu
			for (int i = 0; i < 3; ++i)
				DS_overlay[i].map(currentImage, &UBO_overlay[i], 0); // map the overlay UBOs (1st is visible)

			if (escPressed) { // if ESC is pressed in the menu, we exit the application
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				break;
			}

			// ---- handle input difficulty ----
			if (sPressed) {
				// hide line1 + difficulty, show controls (ID 3)
				menuTxt.removeText(1);
				menuTxt.removeText(2);
				menuTxt.removeText(3);
				menuTxt.print(
				  0.0f, 0.94f,
				  "Find and collect all the rings to win the game!\n"
				  "Move: W-A-S-D / Q-E / R-F | Arrows : look around | Camera : I / O / P\n"
				  "To choose the difficulty, press: [E] Easy, [M] Medium, [H] Hard\n"
				  "Press [C] to close this text",
				  3, "SS",
				  true, true, false,
				  TAL_CENTER, TRH_CENTER, TRV_BOTTOM,
				  {0,0,0,1},{0,0,0,0}
				);
				menuTxt.updateCommandBuffer();
			}
			else if (cPressed || ePressed || mPressed || hPressed) {
				// C/E/M/H: hide controls (ID3), optionally change difficulty, then reprint line1+diff (ID1+2)

				menuTxt.removeText(3);

				// change difficulty if E, M, or H is pressed
				if      (ePressed) currentDifficulty = Difficulty::Easy;
				else if (mPressed) currentDifficulty = Difficulty::Medium;
				else if (hPressed) currentDifficulty = Difficulty::Hard;

				// re-print line1 (ID 1)
				menuTxt.print(
				  -0.85f, 0.7f,
				  "[ENTER] Start Simulation   [S] Show Help & Controls   [ESC] Exit",
				  1, "SS",
				  true, true, false,
				  TAL_LEFT, TRH_LEFT, TRV_BOTTOM,
				  {0,0,0,1},{0,0,0,0}
				);

				// re-print difficulty (ID 2)
				const char* diffText = "";
				switch (currentDifficulty) {
				  case Difficulty::Easy:   diffText = "Difficulty: Easy (6 minutes)";   break;
				  case Difficulty::Medium: diffText = "Difficulty: Medium (5 minutes)"; break;
				  case Difficulty::Hard:   diffText = "Difficulty: Hard (4 minutes)";   break;
				}
				menuTxt.print(
				  0.0f, 0.90f,
				  diffText,
				  2, "SS",
				  true, false, false,
				  TAL_CENTER, TRH_CENTER, TRV_BOTTOM,
				  {0,0,0,1},{0,0,0,0}
				);

				menuTxt.updateCommandBuffer();
			}
			else if (enterPressed) {
				// ENTER pressed: start the game
				menuTxt.removeText(1);
				menuTxt.removeText(2);
				menuTxt.removeText(3);

				// set initial game duration based on difficulty
				switch (currentDifficulty) {
				  case Difficulty::Easy:   initialGameDuration = 6*60.0f; break;
				  case Difficulty::Medium: initialGameDuration = 5*60.0f; break;
				  case Difficulty::Hard:   initialGameDuration = 4*60.0f; break;
				}
				gameTime    = initialGameDuration;
				gameStarted = false;
				state       = AppState::Playing;

				RebuildPipeline();
			}
		break;


		case AppState::GameOver:
  			thirdPerson = false;
			topAngle = false;
			firstPerson  = true;

			// bind overlay win/lose
			for (int i = 0; i < 3; ++i)
				DS_overlay[i].map(currentImage, &UBO_overlay[i], 0);

		    if (bPressed) { // if B is pressed, we return to the menu
			    reset();
				state = AppState::Menu;
				RebuildPipeline();
		    }
		break;

		default: break;
	}

    // ─── view and proj matrix ──────
     float fovDegrees = 45.0f;
    if (firstPerson) {
        fovDegrees = 30.0f;   // tighter, more zoomed in
    }
    glm::mat4 proj = glm::perspective(glm::radians(fovDegrees), Ar, 0.1f, 5000.0f);
    proj[1][1] *= -1;

	// view matrix: it defines the camera position and orientation
    glm::mat4 view;
    if (thirdPerson) {
        glm::vec3 offset = glm::vec3(0, 0, 6.4f * DRONE_SCALE);
    	// building the drone's rotation matrix (yaw → pitch → roll) [drone orientation]
        glm::mat4 R = glm::rotate(glm::mat4(1.0f), droneYaw,   glm::vec3(0,1,0))
                    * glm::rotate(glm::mat4(1.0f), dronePitch, glm::vec3(1,0,0))
                    * glm::rotate(glm::mat4(1.0f), droneRoll,  glm::vec3(0,0,1));

    	// computing the camera position in world space (drone position + rotated offset)
        glm::vec3 camP = global_pos_drone + glm::vec3(R * glm::vec4(offset,0.0f));
        glm::vec3 up   = glm::normalize(glm::vec3(R * glm::vec4(0,1,0,0)));
        view = glm::lookAt(camP, global_pos_drone, up);
    }
 else if (topAngle) {
    // follow from top-behind with an angle
    const float height       = 15.0f * DRONE_SCALE;    
    const float pitchDegrees = 20.0f;                   
    const float pitchRad     = glm::radians(pitchDegrees);
    const float horizDist    = height / tan(pitchRad);  

    glm::mat4 R_yaw = glm::rotate(glm::mat4(1.0f), droneYaw, glm::vec3(0,1,0));

    glm::vec4 localOff = glm::vec4(0.0f, height, horizDist, 1.0f);
    
    glm::vec3 worldOff = glm::vec3(R_yaw * localOff);

    glm::vec3 camP = global_pos_drone + worldOff;

    view = LookInDirMat(camP,glm::vec3(droneYaw,-pitchRad,0.0f));
}
    else {
        // firstPerson
        glm::vec3 camP = global_pos_drone + glm::vec3(
            glm::rotate(glm::mat4(1.0f), droneYaw, glm::vec3(0,1,0))
            * glm::vec4(0, 1.5f*DRONE_SCALE, 0, 1));
        view = LookInDirMat(camP, {droneYaw, dronePitch, droneRoll});
    }

    // ─── Global UBO ────────────────────────────────────────────────────────────────────────────────────────────
    GUBO.proj       = proj;
    GUBO.view       = view;
    GUBO.lightDir   = glm::normalize(glm::vec3(0,1,0)); // Direction of the light source
    GUBO.time       = totalElapsedTime;
    updateGlobalUBO(GUBO, totalElapsedTime);
    DS_global.map(currentImage, &GUBO, 0);

	// flatten the ringScale vector to 1.0f for all rings
	for (int j = 4; j < SC.TI[0].InstanceCount; ++j) {
		int ringIdx = j - 4;
		float s = ringScale[ringIdx];  // 1 = visible, 0 = hidden
		SC.TI[0].I[j].Wm =
		    originalRingWm[ringIdx]
		  * glm::scale(glm::mat4(1.0f), glm::vec3(s));
	}

	// ─── UBOs Mountain ──────────────────────────────────────────────────────────────────────────────────────────
	for (int i = 0; i < SC.TI[0].InstanceCount; ++i) {
	    ubos.mMat   = SC.TI[0].I[i].Wm;
	    ubos.mvpMat = proj * view * ubos.mMat;
	    ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));
	    // mapping first the GUBO (set 0), then all ubos (set 1):
	    SC.TI[0].I[i].DS[0][0]->map(currentImage, &GUBO, 0);
	    SC.TI[0].I[i].DS[0][1]->map(currentImage, &ubos, 0);
	}

	// ─── UBO Station ────────────────────────────────────────────────────────────────────────────────────────────
	for (int i = 0; i < SC.TI[1].InstanceCount; ++i) {
	    ubosStart.mMat   = SC.TI[1].I[i].Wm;
	    ubosStart.mvpMat = proj * view * ubosStart.mMat;
	    ubosStart.nMat   = glm::inverse(glm::transpose(ubosStart.mMat));
	    // mapping first the GUBO (set 0), then all ubosStart (set 1):
	    SC.TI[1].I[i].DS[0][0]->map(currentImage, &GUBO, 0);
	    SC.TI[1].I[i].DS[0][1]->map(currentImage, &ubosStart, 0);
	}

    // ─── UBO Drone ──────────────────────────────────────────────────────────────────────────────────────────────
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

    // ─── UBO SkyBox ─────────────────────────────────────────────────────────────────────────────────────────────
    {
        glm::mat4 skyModel = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1,0,0))
                           * glm::scale(glm::mat4(1.0f), glm::vec3(50.0f));
        UBO_skyBox.mvpMat = proj * glm::mat4(glm::mat3(view)) * skyModel;
        DS_skyBox.map(currentImage, &UBO_skyBox, 0);
    }
}

// this function creates a look-at matrix that rotates the camera to look in the direction specified by Angs
glm::mat4 DroneSimulator::LookInDirMat(glm::vec3 Pos, glm::vec3 Angs)
{
    glm::mat4 I(1.0f);
    glm::mat4 T   = glm::translate(I, -Pos); // move the scene opposite to the camera position
    glm::mat4 Ry  = glm::rotate(I, -Angs.x, glm::vec3(0,1,0)); // negative Yaw
    glm::mat4 Rx  = glm::rotate(I, -Angs.y, glm::vec3(1,0,0)); // negative Pitch
    glm::mat4 Rz  = glm::rotate(I, -Angs.z, glm::vec3(0,0,1)); // negative Roll
    return Rz * Rx * Ry * T; // translation -> rotation
}

void DroneSimulator::setCameraMode(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_I)) { thirdPerson=true; topAngle=false; firstPerson=false;  } // 3-rd
    if (glfwGetKey(w, GLFW_KEY_O)) { thirdPerson=false; topAngle=true; firstPerson=false;  } // 1-st
    if (glfwGetKey(w, GLFW_KEY_P)) { thirdPerson=false; topAngle=false; firstPerson=true;  } // 1-st
}

// get the drone input from the keyboard and update the drone position and orientation
void DroneSimulator::getDroneInput(GLFWwindow* w, float deltaT)
{
    const float ROT_SPEED  = glm::radians(45.0f);
    const float MOVE_SPEED = 50.0f;

    // rotations
    if(glfwGetKey(w, GLFW_KEY_LEFT))  droneYaw   += deltaT * ROT_SPEED;
    if(glfwGetKey(w, GLFW_KEY_RIGHT)) droneYaw   -= deltaT * ROT_SPEED;
    if(glfwGetKey(w, GLFW_KEY_UP))    dronePitch += deltaT * ROT_SPEED;
    if(glfwGetKey(w, GLFW_KEY_DOWN))  dronePitch -= deltaT * ROT_SPEED;
    if(glfwGetKey(w, GLFW_KEY_Q))     droneRoll  -= deltaT * ROT_SPEED;
    if(glfwGetKey(w, GLFW_KEY_E))     droneRoll  += deltaT * ROT_SPEED;

	// translations
	glm::mat4 R_yaw = glm::rotate(glm::mat4(1.0f), droneYaw, glm::vec3(0,1,0));
	glm::vec3 forward = glm::vec3(R_yaw * glm::vec4(0,0,-1,0));
	glm::vec3 right   = glm::vec3(R_yaw * glm::vec4(1,0, 0,0));

	if (glfwGetKey(w, GLFW_KEY_W)) {
		glm::vec3 attempt = global_pos_drone + MOVE_SPEED * forward * deltaT;
		// check if the drone is too close to any mountain point
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

	// clamp the drone position to a defined range
	global_pos_drone.x = glm::clamp(global_pos_drone.x, minX, maxX);
	global_pos_drone.y = glm::clamp(global_pos_drone.y, minY, maxY);
	global_pos_drone.z = glm::clamp(global_pos_drone.z, minZ, maxZ);

	// check gameOver condition
	gameOver = this->checkRingPassage(global_pos_drone, ringPositions, ringPassed);
}

// this function updates the color and intensity of the light source based on the elapsed time
const glm::vec3 dawnColor = glm::vec3(1.0f, 0.45f, 0.2f);
const glm::vec3 noonColor = glm::vec3(1.0f, 1.0f, 0.95f);
const glm::vec3 sunsetColor = glm::vec3(1.0f, 0.2f, 0.1f);

void DroneSimulator::updateGlobalUBO(GlobalUniformBufferObject& gubo, float elapsedTime)
{
	gubo.time = elapsedTime;

	// 4, 5, or 6 min  cycle
	float t = fmod(elapsedTime, initialGameDuration);

	// sun color and direction
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

void DroneSimulator::reset()
{
	gameOver = 0.0f;



	thirdPerson   = true;
	topAngle   = false;
	firstPerson    = false;
	global_pos_drone = glm::vec3(-1000.0f, 250.0f, 130.0f);
	droneYaw = 0.0f, dronePitch = 0.0f, droneRoll = 0.0f;

	ringPassed = std::vector<bool>(10, false);
	ringScale  = std::vector<float>(10, 1.0f);  // ← reset ring visibility


	// timer reset
	gameStarted = false;
	lastPassedCount = -1;
	lastTimeStr = "";
}

// load the mountain points from the OBJ file and apply transformations, so we can have all the points to set boundaries
void DroneSimulator::loadMountainPoints(std::vector<glm::vec3>& points)
{
	// open the OBJ file containing the base mountain mesh
	std::ifstream objFile("assets/models/snowyMountain.obj");
	if (!objFile.is_open()) {
		std::cerr << "Error loading snowyMountain.obj" << std::endl;
		return;
	}

	std::vector<glm::vec3> rawVertices; // temporary container for the original vertices

	std::string line;
	// parsing the OBJ file, reading only the vertex positions (lines starting with 'v ')
	while (std::getline(objFile, line)) {
		if (line.substr(0, 2) == "v ") {
			std::istringstream iss(line.substr(2));
			glm::vec3 v;
			iss >> v.x >> v.y >> v.z;
			rawVertices.push_back(v);
		}
	}
	// close the file after reading
	objFile.close();

	// define transforms for the 4 mountains: look at JSON
	std::vector<std::pair<glm::vec3, float>> transforms = {
		{{0, 150, 0}, 0.0f},
		{{-1960, 145, 0}, 180.0f},
		{{-1960, 150, -1960}, 180.0f},
		{{0, 145, -1960}, 0.0f}
	};

	float scale = 2000.0f;

	// for each transform we apply scale, rotation, and translation to the base vertices
	for (const auto& [pos, yawDeg] : transforms) {
		float yawRad = glm::radians(yawDeg); // converting yaw angle to radians
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), yawRad, glm::vec3(0, 1, 0));

		// applying transformations to each vertex
		for (const auto& v : rawVertices) {
			glm::vec3 scaled = v * scale; // scaling the vertex
			glm::vec3 rotated = glm::vec3(rotation * glm::vec4(scaled, 1.0f)); // rotating around Y axis
			glm::vec3 transformed = rotated + pos; // translating to the final position
			points.push_back(transformed); // adding to the output vector
		}
	}
	std::cout << "Loaded " << points.size() << " mountain vertices.\n";
}

// this function checks if the drone is too close to any mountain point
bool DroneSimulator::isTooCloseToMountain(const glm::vec3& pos, const std::vector<glm::vec3>& mountainPoints, float threshold)
{
	for (const auto& pt : mountainPoints) {
		if (glm::distance(pos, pt) < threshold)
			return true;
	}
	return false;
}

// this function checks if the drone has passed through any ring and updates the passed vector.
float DroneSimulator::checkRingPassage(glm::vec3 dronePos, std::vector<glm::vec3>& rings, std::vector<bool>& passed, float radius)
{
	float res = 0.0f;
	for (size_t i = 0; i < rings.size(); ++i) {
		if (!passed[i] && glm::distance(dronePos, rings[i]) < radius) {
			passed[i] = true;
			ringScale[i] = 0.0f;                   // <— hiding the ring
			std::cout << "Great, Ring " << (i + 1) << " passed!" << std::endl;
		}
	}
	if (std::all_of(passed.begin(), passed.end(), [](bool b){ return b; })) {
		std::cout << "All rings passed! Game complete!" << std::endl;
		res = 1.0f;
	}
	return res;
}
