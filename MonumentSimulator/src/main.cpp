// This has been adapted from the Vulkan tutorial

#include "modules/Starter.hpp"
//#include "modules/TextMaker.hpp"

using namespace std;
using namespace glm;

// MonumentSimulator: demo subclass of BaseProject
class MonumentSimulator : public BaseProject {
protected:

    void setWindowParameters() override;
    void onWindowResize(int w, int h) override;
    void localInit() override;
    void pipelinesAndDescriptorSetsInit() override;
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int i) override;
    void updateUniformBuffer(uint32_t currentImage) override;
    void pipelinesAndDescriptorSetsCleanup() override;
    void localCleanup() override;

};

void MonumentSimulator::setWindowParameters() {
    std::cout << "setWindowParameters()\n";
}

void MonumentSimulator::onWindowResize(int w, int h) {
    std::cout << "onWindowResize()\n";
}

void MonumentSimulator::localInit() {
    std::cout << "localInit()\n";
}

void MonumentSimulator::pipelinesAndDescriptorSetsInit() {
    std::cout << "pipelinesAndDescriptorSetsInit()\n";
}

void MonumentSimulator::populateCommandBuffer(VkCommandBuffer commandBuffer, int i) {
    std::cout << "populateCommandBuffer()\n";
}

void MonumentSimulator::updateUniformBuffer(uint32_t currentImage) {
    std::cout << "updateUniformBuffer()\n";
}

void MonumentSimulator::pipelinesAndDescriptorSetsCleanup() {
    std::cout << "pipelinesAndDescriptorSetsCleanup()\n";
}

void MonumentSimulator::localCleanup() {
    std::cout << "localCleanup()\n";
}

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
