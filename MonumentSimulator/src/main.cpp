// This has been adapted from the Vulkan tutorial

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"

using namespace std;
using namespace glm;

vector<SingleText> outText = {
	{5, {
		"Monument Simulator",
		"",
		"Filippo Paris",
		"Francesco Moretti",
		"Moein "
	}, 0, 0, 0, 0, 0}
}; // HERE WE CAN SHOW OUR GAME INSTRUCTIONS

// The uniform buffer object used in this example
struct UniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
};

// The vertices data structures
struct Vertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

// MonumentSimulator: subclass of BaseProject
class MonumentSimulator : public BaseProject {
protected:

	float Ar; // Aspect Ratio
	TextMaker outTxt;

	// Camera controls
	glm::vec3 camPos;
	float camYaw;
	float camPitch;
	float camRoll;
	float camDist;

	// --- Pipelines ---
	Pipeline P_global;

	// --- Descriptor Set Layouts ---
	DescriptorSetLayout DSL_global;

	// --- Vertex Descriptors ---
	VertexDescriptor VD_basic;

	// --- Descriptor Sets ---
	DescriptorSet DS_global;

	// --- Models ---
	//Model<Vertex> M_drone;
	//vector <Texture> Textures;

	// --- Uniform Buffers ---
	UniformBufferObject UBO_drone;
	GlobalUniformBufferObject GUBO;


	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	glm::vec3 dronePosition = glm::vec3(0.0f, 0.5f, 0.0f);
	float droneYaw = 0.0f;

	const float DRONE_SPEED = 3.0f;
	/*
	* SETUP & FUNCTIONS
	*/

    // Here you set the main application parameters
	void setWindowParameters()
	{
		// Window size, title and initial background
		windowWidth = 1280;
		windowHeight = 720;
		windowTitle = "Monument Simulator";
		windowResizable = GLFW_TRUE;
		initialBackgroundColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		Ar = (float)windowWidth / (float)windowHeight;
	}

	// What to do when the window changes size
	void onWindowResize(int w, int h)
	{
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
	}

    // Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
    void localInit()
    {

		cout << "Initializing text\n";
		outTxt.init(this, &outText);
		cout << "Initialization completed!\n";
		startTime = std::chrono::high_resolution_clock::now();
    }

    // Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit()
	{

	}

    // Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup()
	{

	}

    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup()
	{

	}

    // Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw, with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage)
	{

	}

    // Here is where you update the uniforms. Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage)
	{

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