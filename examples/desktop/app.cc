#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <memory>
#include <ini.h>
#include "gpupixel.h"
using namespace gpupixel;


std::shared_ptr<BeautyFaceFilter> beauty_face_filter_;
std::shared_ptr<FaceReshapeFilter> face_reshape_filter_;
std::shared_ptr<gpupixel::LipstickFilter> lipstick_filter_;
std::shared_ptr<gpupixel::BlusherFilter> blusher_filter_;
std::shared_ptr<SourceImage> gpuSourceImage;
std::shared_ptr<TargetRawDataOutput> output_;
std::shared_ptr<TargetView> target_view;

float beautyValue = 0;
float whithValue = 0;
float thinFaceValue = 0;
float bigeyeValue = 0;
float lipstickValue = 0;
float blusherValue = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void adjustBeautyBlur(float increment = 0.0);
void adjustBeautyWhite(float increment = 0.0);
void adjustFaceSlim(float increment = 0.0);
void adjustEyeZoom(float increment = 0.0);
void adjustLipstickBlend(float increment = 0.0);
void adjustBlusherBlend(float increment = 0.0);

 void error_callback( int error, const char *msg ) {
    std::string s;
    s = " [" + std::to_string(error) + "] " + msg + '\n';
    std::cerr << s << std::endl;
}

int iniHandler(void* user, const char* section, const char* name, const char* value) {
    if (std::string(section) == "Settings") {
        if (std::string(name) == "beautyValue") beautyValue = std::stof(value);
        else if (std::string(name) == "whithValue") whithValue = std::stof(value);
        else if (std::string(name) == "thinFaceValue") thinFaceValue = std::stof(value);
        else if (std::string(name) == "bigeyeValue") bigeyeValue = std::stof(value);
        else if (std::string(name) == "lipstickValue") lipstickValue = std::stof(value);
        else if (std::string(name) == "blusherValue") blusherValue = std::stof(value);
    }
    return 1;
}

void loadConfig(const std::string& configFilePath) {
    if (ini_parse(configFilePath.c_str(), iniHandler, NULL) < 0) {
        std::cerr << "Failed to load config file: " << configFilePath << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    std::string imageFilePath;
    std::string configFilePath = "appConfig.ini"; // Default config file path

    if (argc < 2) {
        std::cerr << "No image file provided. Using default: demo.png" << std::endl;
        imageFilePath = "demo.png";
    } else {
        imageFilePath = argv[1];
        if (argc >= 3) {
            configFilePath = argv[2]; // Optional config file path
        }
    }

    loadConfig(configFilePath); // Load configuration values

    glfwInit();
     GLFWwindow* window = GPUPixelContext::getInstance()->GetGLContext();
  
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    gladLoadGL();
    glfwMakeContextCurrent(window);

    glfwShowWindow(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // create filter
    // ----
    lipstick_filter_ = LipstickFilter::create();
    blusher_filter_ = BlusherFilter::create();
    face_reshape_filter_ = FaceReshapeFilter::create();
    
    //  filter pipline
    // ----
    gpuSourceImage = SourceImage::create(imageFilePath);
    target_view = std::make_shared<TargetView>();

    gpuSourceImage->RegLandmarkCallback([=](std::vector<float> landmarks) {
       lipstick_filter_->SetFaceLandmarks(landmarks);
       blusher_filter_->SetFaceLandmarks(landmarks);
       face_reshape_filter_->SetFaceLandmarks(landmarks);
     });

    beauty_face_filter_ = BeautyFaceFilter::create();
 
    
    // gpuSourceImage->addTarget(beauty_face_filter_)
    //               ->addTarget(target_view); 
   
    gpuSourceImage->addTarget(lipstick_filter_)
                    ->addTarget(blusher_filter_)
                    ->addTarget(face_reshape_filter_)
                    ->addTarget(beauty_face_filter_)
                    ->addTarget(target_view);
                    
    // 
    target_view->onSizeChanged(1280, 720);

    // Apply the configuration values
    adjustBeautyBlur();
    adjustBeautyWhite();
    adjustFaceSlim();
    adjustEyeZoom();
    adjustLipstickBlend();
    adjustBlusherBlend();
    
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);
        
        // 
        // -----
        gpuSourceImage->Render();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// Adjust value with increment and apply filter
float adjustValue(float value, float increment, float max = 10.0, float min = 0.0) {
    value += increment;
    if (value > max) value = max;
    if (value < min) value = min;
    return value;
}

// Individual filter adjustment functions
void adjustBeautyBlur(float increment) {
    beautyValue = adjustValue(beautyValue, increment);
    beauty_face_filter_->setBlurAlpha(beautyValue / 10);
}

void adjustBeautyWhite(float increment) {
    whithValue = adjustValue(whithValue, increment);
    beauty_face_filter_->setWhite(whithValue / 20);
}

void adjustFaceSlim(float increment) {
    thinFaceValue = adjustValue(thinFaceValue, increment);
    face_reshape_filter_->setFaceSlimLevel(thinFaceValue / 200);
}

void adjustEyeZoom(float increment) {
    bigeyeValue = adjustValue(bigeyeValue, increment);
    face_reshape_filter_->setEyeZoomLevel(bigeyeValue / 100);
}

void adjustLipstickBlend(float increment) {
    lipstickValue = adjustValue(lipstickValue, increment);
    lipstick_filter_->setBlendLevel(lipstickValue / 10);
}

void adjustBlusherBlend(float increment) {
    blusherValue = adjustValue(blusherValue, increment);
    blusher_filter_->setBlendLevel(blusherValue / 10);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        adjustBeautyBlur(1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        adjustBeautyBlur(-1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        adjustBeautyWhite(1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        adjustBeautyWhite(-1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        adjustFaceSlim(1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        adjustFaceSlim(-1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        adjustEyeZoom(1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
        adjustEyeZoom(-1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        adjustLipstickBlend(1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        adjustLipstickBlend(-1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        adjustBlusherBlend(1.0);
    }

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        adjustBlusherBlend(-1.0);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    
}
