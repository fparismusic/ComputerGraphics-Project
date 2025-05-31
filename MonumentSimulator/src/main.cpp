// This has been adapted from the Vulkan tutorial

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"

using namespace std;
using namespace glm;

// MonumentSimulator: demo subclass of BaseProject
class MonumentSimulator : public BaseProject {
protected:
    /*
    * SETUP & FUNCTIONS
    */




    // Here you set the main application parameters
	void setWindowParameters()
	{
		// Window size, title and initial background
		windowWidth = 1200;
		windowHeight = 800;
		windowTitle = "Monument Simulator";
		windowResizable = GLFW_TRUE;
		initialBackgroundColor = { 0.5f, 0.5f, 0.5f, 1.0f };
	}

	// What to do when the window changes size
	void onWindowResize(int w, int h)
	{
		std::cout << "this is an exemple()\n";
	}

    // Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
    void localInit()
    {
    	std::cout << "this is an exemple()\n";
    }

    // Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit()
	{
		std::cout << "this is an exemple()\n";
	}

    // Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup()
	{
		std::cout << "this is an exemple()\n";
	}

    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup()
	{
		std::cout << "this is an exemple()\n";
	}

    // Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw, with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage)
	{
		//std::cout << "this is an exemple()\n";
	}

    // Here is where you update the uniforms. Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage)
	{
		//std::cout << "this is an exemple()\n";
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