#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <cstdlib> // For exit()

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

std::string inputFilePath = "demo.png";
std::string inputFileType = "image";
std::string outputFilePath = "demo-output.png";
std::string configFilePath = "appConfig.ini";

GLFWwindow* window;

float beautyValue = 0;
float whithValue = 0;
float thinFaceValue = 0;
float bigeyeValue = 0;
float lipstickValue = 0;
float blusherValue = 0;

bool canStartConvertVideo = false;

// Headers declaration
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(int key, int action);
void adjustBeautyBlur(float increment = 0.0);
void adjustBeautyWhite(float increment = 0.0);
void adjustFaceSlim(float increment = 0.0);
void adjustEyeZoom(float increment = 0.0);
void adjustLipstickBlend(float increment = 0.0);
void adjustBlusherBlend(float increment = 0.0);

// Error callback
void error_callback( int error, const char *msg ) {
    std::string s;
    s = " [" + std::to_string(error) + "] " + msg + '\n';
    std::cerr << s << std::endl;
}

// Ini file handler
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

// Load configuration values from the ini file
void loadConfig(const std::string& configFilePath) {
    std::cout << "Loading config file: " << configFilePath << std::endl;
    if (ini_parse(configFilePath.c_str(), iniHandler, NULL) < 0) {
        std::cerr << "Failed to load config file: " << configFilePath << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Config file loaded successfully." << std::endl;
}

// Function to detect file type based on file extension
void detectFileType(const std::string& filePath, std::string& fileType) {
    std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "bmp" || extension == "tiff") {
        fileType = "image";
    } else if (extension == "avi" || extension == "mp4" || extension == "mov" || extension == "mkv" || extension == "flv") {
        fileType = "video";
    } else {
        fileType = "unknown";
    }
}

// Setup input parameters
void setupInputParams(int argc, char** argv) {
    // Parse command-line arguments
    if (argc >= 2) {
        inputFilePath = argv[1];
    }
    if (argc >= 3) {
        outputFilePath = argv[2];
    }

    // Detect inputFileType
    detectFileType(inputFilePath, inputFileType);

    std::cout << "Input File Path: " << inputFilePath << std::endl;
    std::cout << "Input File Type: " << inputFileType << std::endl;
    std::cout << "Output File Path: " << outputFilePath << std::endl;

    // Load configuration values
    loadConfig(configFilePath); 
}

// Setup filters
void setupFilters() {
    // create filter
    lipstick_filter_ = LipstickFilter::create();
    blusher_filter_ = BlusherFilter::create();
    face_reshape_filter_ = FaceReshapeFilter::create();
    beauty_face_filter_ = BeautyFaceFilter::create();

    // Apply the configuration values
    adjustBeautyBlur();
    adjustBeautyWhite();
    adjustFaceSlim();
    adjustEyeZoom();
    adjustLipstickBlend();
    adjustBlusherBlend();
}

// Render source image with filters
void setupSourceImageWithFilters() {
    
    target_view = std::make_shared<TargetView>();

    gpuSourceImage->RegLandmarkCallback([=](std::vector<float> landmarks) {
        lipstick_filter_->SetFaceLandmarks(landmarks);
        blusher_filter_->SetFaceLandmarks(landmarks);
        face_reshape_filter_->SetFaceLandmarks(landmarks);
    });

    gpuSourceImage->addTarget(lipstick_filter_)
                    ->addTarget(blusher_filter_)
                    ->addTarget(face_reshape_filter_)
                    ->addTarget(beauty_face_filter_)
                    ->addTarget(target_view);
    
    
    target_view->onSizeChanged(1280, 720);
}

// Initialize the window
void initUIWindow() {
    glfwInit();
    window = GPUPixelContext::getInstance()->GetGLContext();
  
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    // Set GLFW to use OpenGL if Vulkan is not available
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

    gladLoadGL();
    glfwMakeContextCurrent(window);

    glfwShowWindow(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Set the key callback
    glfwSetKeyCallback(window, keyCallback);
}

// Function to detect video codec based on file extension
int detectVideoCodec(const std::string& outputFileName) {
    std::string extension = outputFileName.substr(outputFileName.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "mp4") {
        return cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    } else if (extension == "avi") {
        return cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    } else {
        return cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    }
}

// Perform video filters
void performVideoFilters() {

    // Initialize OpenCV video capture
    cv::VideoCapture cap(inputFilePath);
    if (!cap.isOpened()) {
        std::cerr << "Error opening video file: " << inputFilePath << std::endl;
        return;
    }

    // Get video properties
    double fps = cap.get(cv::CAP_PROP_FPS);
    cv::Size frameSize(cap.get(cv::CAP_PROP_FRAME_WIDTH), cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

    // Initialize OpenCV video writer
    cv::VideoWriter writer;
    int codec = detectVideoCodec(outputFilePath);
    writer.open(outputFilePath, codec, fps, frameSize, true);

    if (!writer.isOpened()) {
        std::cerr << "Could not open the output video file for write" << std::endl;
        return;
    }

    // UI window
    initUIWindow();

    cv::Mat frame;
    cap.read(frame);    // Read the first frame
    bool isNewFrame = true;
    int width = 0;
    int height = 0;
    int channel_count = 0;
    int currentFrame = 0;
    while (!glfwWindowShouldClose(window)) {

        if (canStartConvertVideo == true) {
            if (currentFrame > 0) {
                if (cap.read(frame) == false) {
                    // Stop while loop at the end of the video
                    break;
                }
                isNewFrame = true;
            }
            currentFrame++;

            // Print progress
            std::cout << "\rProcessing frame " << currentFrame << " / " << totalFrames << " (" 
            << (currentFrame * 100 / totalFrames) << "%)" << std::flush;
        }

        if (isNewFrame == true) {
            // Convert cv::Mat to the format required by your filters
            width = frame.cols;
            height = frame.rows;
            channel_count = frame.channels();
            // const unsigned char* pixels = frame.data;

            // If the result is still bluish, you might need to convert the color format
            cv::Mat correctedFrame;
            cv::cvtColor(frame, correctedFrame, cv::COLOR_BGR2RGB);
            const unsigned char* pixels = correctedFrame.data;

            // Convert Frame to SourceImage
            gpuSourceImage = SourceImage::create_from_memory(width, height, channel_count, pixels);
            setupSourceImageWithFilters();

            isNewFrame = false;
        }

        // Render the frame
        gpuSourceImage->Render();

        if (canStartConvertVideo == true) {
            // Convert the processed frame back to cv::Mat'
            // Note: the processed pixels is 4-channels RGBA format
            cv::Mat processedFrame(height, width, CV_8UC(4), gpuSourceImage->getPixels());
            cv::Mat rgbFrame;
            cv::cvtColor(processedFrame, rgbFrame, cv::COLOR_RGBA2BGR);

            // Write the processed frame to the output video
            writer.write(rgbFrame);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "\nProcessing completed." << std::endl;

    // Release OpenCV resources
    cap.release();
    writer.release();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
}

// Perform image filters
void performImageFilters() {
        
    //  filter pipline
    // ----
    gpuSourceImage = SourceImage::create(inputFilePath);
    setupSourceImageWithFilters();
    
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // Render filter
        gpuSourceImage->Render();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
}

// ======================= Main function =========================
int main(int argc, char** argv) {    

    // Setup input parameters
    setupInputParams(argc, argv);
    
    // Apply the default configuration values
    setupFilters();

    // Init GLFW
    initUIWindow();

    // Check inputFileType
    if (inputFileType == "video") {
        performVideoFilters();
    } else if (inputFileType == "image") {
        performImageFilters();
    } else {
        std::cerr << "Invalid input file type: " << inputFileType << std::endl;
        return -1;
    }

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
    std::cout << "adjustBeautyBlur() " << "value: " << beautyValue << std::endl;
}

void adjustBeautyWhite(float increment) {
    whithValue = adjustValue(whithValue, increment);
    beauty_face_filter_->setWhite(whithValue / 20);
    std::cout << "adjustBeautyWhite() " << "value: " << whithValue << std::endl;
}

void adjustFaceSlim(float increment) {
    thinFaceValue = adjustValue(thinFaceValue, increment);
    face_reshape_filter_->setFaceSlimLevel(thinFaceValue / 200);
    std::cout << "adjustFaceSlim() " << "value: " << thinFaceValue << std::endl;
}

void adjustEyeZoom(float increment) {
    bigeyeValue = adjustValue(bigeyeValue, increment);
    face_reshape_filter_->setEyeZoomLevel(bigeyeValue / 100);
    std::cout << "adjustEyeZoom() " << "value: " << bigeyeValue << std::endl;
}

void adjustLipstickBlend(float increment) {
    lipstickValue = adjustValue(lipstickValue, increment);
    lipstick_filter_->setBlendLevel(lipstickValue / 10);
    std::cout << "adjustLipstickBlend() " << "value: " << lipstickValue << std::endl;
}

void adjustBlusherBlend(float increment) {
    blusherValue = adjustValue(blusherValue, increment);
    blusher_filter_->setBlendLevel(blusherValue / 10);
    std::cout << "adjustBlusherBlend() " << "value: " << blusherValue << std::endl;
}

// Key callback function
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Add other key actions here
    processInput(key, action);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(int key, int action)
{
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
        if (key == GLFW_KEY_A) adjustBeautyBlur(1.0);
        if (key == GLFW_KEY_Z) adjustBeautyBlur(-1.0);
        if (key == GLFW_KEY_S) adjustBeautyWhite(1.0);
        if (key == GLFW_KEY_X) adjustBeautyWhite(-1.0);
        if (key == GLFW_KEY_D) adjustFaceSlim(1.0);
        if (key == GLFW_KEY_C) adjustFaceSlim(-1.0);
        if (key == GLFW_KEY_F) adjustEyeZoom(1.0);
        if (key == GLFW_KEY_V) adjustEyeZoom(-1.0);
        if (key == GLFW_KEY_G) adjustLipstickBlend(1.0);
        if (key == GLFW_KEY_B) adjustLipstickBlend(-1.0);
        if (key == GLFW_KEY_H) adjustBlusherBlend(1.0);
        if (key == GLFW_KEY_N) adjustBlusherBlend(-1.0);
        if (key == GLFW_KEY_Q) canStartConvertVideo = true;
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
