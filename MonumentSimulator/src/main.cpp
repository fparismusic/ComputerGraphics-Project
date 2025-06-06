// This has been adapted from the Vulkan tutorial

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Utils.hpp"

using namespace std;
using namespace glm;

vector<SingleText> outText = {
	{5, {
		"Monument Simulator",
		"",
		"Filippo Paris",
		"Francesco Moretti",
		"Moein Zadeh"
	}, 0, 0, 0, 0, 0},
	{2, {
		"Press K (Keyboard)",
		"to see the command list"
	}, 0, 0, 0, 0, 1},
	{5, {
		"Move with W-A-S-D | Q-E | R-F",
		"Move mouse to look around",
		"Press SPACE to take pictures",
		"Press C to close this text",
		"Press ESC to exit the simulation"
	}, 0, 0, 0, 0, 1}
}; // HERE WE CAN SHOW OUR GAME INSTRUCTIONS

// MonumentSimulator: subclass of BaseProject
class MonumentSimulator : public BaseProject {
protected:

	float Ar; // Aspect Ratio
	TextMaker outTxt;
	bool showStartText = true;
	bool showCommandsKeyboard = false;

	// Camera controls
	glm::vec3 CamPos = glm::vec3(0.0f, 10.0f, 50.0f);
	float CamYaw = 0.0f, CamPitch = 0.0f, CamRoll = 0.0f, CamDist = 0.0f;

	// Speed controls
	const float ROT_SPEED  = glm::radians(120.0f);
	const float MOVE_SPEED = 4.0f;

	// Time
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	// --- Descriptor Set Layouts ---
	DescriptorSetLayout
		DSL_skybox,
		DSL_global;

	// --- Vertex Descriptors ---
	VertexDescriptor VD, VD_skyBox;

	// --- Pipelines ---
	Pipeline
		P_skyBox;

	// --- Model ---
	Model
		M_skyBox;

	// --- Textures ---

	// --- Descriptor Sets ---
	DescriptorSet
		DS_skyBox,
		DS_global;

	// --- Uniform Buffers ---
	UniformBufferObject
		UBO_drone;

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
		initialBackgroundColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		// Number of UBO and textures that we will use
		//DPSZs.uniformBlocksInPool = 2;  // UBOs
		//DPSZs.texturesInPool      = 3;  // Textures
		//DPSZs.setsInPool          = 2;  // DS

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
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you load and setup all your Vulkan Models and Textures.
	// Here you also create your DescriptorSetLayouts and load the Shaders for the pipelines
    void localInit()
    {
		// Descriptor Layouts [what will be passed to the shaders]


		/* Initialize vertex descriptor for Vertex { vec3 pos; vec2 UV; vec3 norm; }
		VD.init(this, {
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
			{0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm),  sizeof(vec3), NORMAL},
			{0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), sizeof(vec2), UV}
		});*/

		// Pipelines [Shader couples]
		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders
		// The last array, is a vector of pointer to the layouts of the sets that will be used in this pipeline. The first element will be set 0, and so on..
		//P_terrain.init(this, &VD, "shaders/example.vert.spv", "shaders/example.frag.spv", { &DSL_terrain });
		//P_terrain.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false);

		// Create models
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ or GLTF
		//M_terrain.init(this, &VD, "assets/models/example.gltf", GLTF);
		// Create the textures
		// The second parameter is the file name
		//tex_baseColor.init(this, "assets/textures/example.png", VK_FORMAT_R8G8B8A8_SRGB, true);
		//tex_metallicRoughness.init(this, "assets/textures/example.png", VK_FORMAT_R8G8B8A8_UNORM, true);
		//tex_normal.init(this, "assets/textures/example.jpeg", VK_FORMAT_R8G8B8A8_UNORM, true);

		// INIT TEXT
		cout << "Initializing text\n";
		outTxt.init(this, &outText);
		cout << "Initialization completed!\n";

		startTime = std::chrono::high_resolution_clock::now();
    }

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit()
	{
		// This creates a new pipeline (with the current surface), using its shaders


		// Here you define the data set


		// INIT TEXT
		outTxt.pipelinesAndDescriptorSetsInit();
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup()
	{
		// Cleanup pipelines

		// Cleanup datasets

		// INIT TEXT
		outTxt.pipelinesAndDescriptorSetsCleanup();
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

		// Cleanup models

		// Cleanup descriptor set layouts

		// Destroy the pipelines

		// INIT TEXT
		outTxt.localCleanup();
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw, with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage)
	{
		// For a pipeline object, this command binds the corresponing pipeline to the command buffer passed in its parameter binds the data set
		//P_terrain.bind(commandBuffer);
		// For a Dataset object, this command binds the corresponing dataset to the command buffer and pipeline passed in its first and second parameters.
		// The third parameter is the number of the set being bound
		// As described in the Vulkan tutorial, a different dataset is required for each image in the swap chain.
		// This is done automatically in file Starter.hpp, however the command here needs also the index of the current image in the swap chain, passed in its last parameter binds the model
		//DS_terrain.bind(commandBuffer, P_terrain, 0, currentImage);
		// For a Model object, this command binds the corresponing index and vertex buffer
		// to the command buffer passed in its parameter
		// record the drawing command in the command buffer
		//M_terrain.bind(commandBuffer);
		//vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M_terrain.indices.size()), 1, 0, 0, 0);

		int txtIndex;
		if (showStartText)
			txtIndex = 0;
		else if (showCommandsKeyboard)
			txtIndex = 2;
		else
			txtIndex = 1;

		outTxt.populateCommandBuffer(commandBuffer, currentImage, txtIndex);
	}

	//************************************************************************************************
	//************************************************************************************************
	//************************************************************************************************
    // Here is where you update the uniforms. Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage)
	{
		float time = chrono::duration<float>(chrono::high_resolution_clock::now() - startTime).count();
		static int index = 0;
		static bool debounce = false;
		static int curDebounce = 0;

		// Standard procedure to quit when the ESC key is pressed
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		// First, we manage the text display
		if (showStartText) {
			if (time > 5.0f) {
				showStartText = false;
				RebuildPipeline();
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
		getSixAxis(deltaT, m, r, fire);

		/*CamYaw   = CamYaw   - ROT_SPEED * deltaT * r.y;
		CamPitch = CamPitch - ROT_SPEED * deltaT * r.x;
		CamPitch = CamPitch < glm::radians(-90.0f) ? glm::radians(-90.0f) :
				  (CamPitch > glm::radians( 90.0f) ? glm::radians( 90.0f) : CamPitch);
		CamRoll = CamRoll - ROT_SPEED * deltaT * r.z;

		// Rotazione completa: roll (Z) → pitch (X) → yaw (Y)
		glm::mat4 R_yaw   = glm::rotate(glm::mat4(1.0f), CamYaw,   glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 R_pitch = glm::rotate(glm::mat4(1.0f), CamPitch, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 R_roll  = glm::rotate(glm::mat4(1.0f), CamRoll,  glm::vec3(0.0f, 0.0f, 1.0f));
		// Rotazione finale: R = Yaw * Pitch * Roll
		glm::mat4 R = R_yaw * R_pitch * R_roll;
		// Calcolo i versori dalla matrice
		glm::vec3 forward = glm::normalize(glm::vec3(R * glm::vec4(0, 0, -1, 0))); // -Z
		glm::vec3 right   = glm::normalize(glm::vec3(R * glm::vec4(1, 0,  0, 0))); // +X
		glm::vec3 up      = glm::normalize(glm::vec3(R * glm::vec4(0, 1,  0, 0))); // +Y

		// W/A/S/D/R/F
		CamPos += MOVE_SPEED * m.y * forward * deltaT;  // avanti/indietro
		CamPos += MOVE_SPEED * m.x * right   * deltaT;  // strafing
		CamPos += MOVE_SPEED * m.z * up      * deltaT;  // su/giù


		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));


		glm::mat4 view = glm::lookAt(CamPos, CamPos + forward, up);
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 100.0f);
		proj[1][1] *= -1;  // correzione Vulkan

		UBO_terrain.mvpMat = proj * view * model;
		UBO_terrain.mMat   = model;
		UBO_terrain.nMat   = glm::transpose(glm::inverse(model));

		// Scrive nel buffer uniforme
		DS_terrain.map(currentImage, &UBO_terrain, sizeof(UBO_terrain), 0);*/
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