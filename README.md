*IMPORTANT*: I am pretty new to C++, so this fork just a praticing C++ work

# For full README and Credits please check the original author links:
Repository: [https://github.com/pixpark/gpupixel]
Page: [https://gpupixel.pixpark.net/guide/build]

# The goal of this Fork:
- Modifying the sample app to process video file

# Setup Environment
## Install Nvidia and Vulkans
```
sudo apt-get update
sudo apt-get install nvidia-driver-535   # Replace 535  with the latest version available, 560 seem to have issue on Ubuntu 24.0
sudo reboot

sudo apt-get install vulkan-tools
sudo apt-get install vulkan-utils

sudo apt-get install libgl1-mesa-glx
sudo apt-get install libgl1-mesa-dri

vulkaninfo | less
```
Set .bashrc
```
export MESA_LOADER_DRIVER_OVERRIDE=nvidia
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json
```
Reboot .bashrc
```
source ~/.bashrc
```

## Install OpenCV for video processing
```
sudo apt-get update
sudo apt-get install libopencv-dev
```

## Install Glad - For VSCode IDE
```
git clone https://github.com/Dav1dde/glad.git
cd glad
python3 -m glad --out-path=build --api="gl:core=4.6" c
sudo mv build/include/glad /usr/local/include/glad
```
Update c_cpp_properties.json
```
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "/usr/include",
                "/usr/local/include",
                "/usr/include/opencv4/opencv2",
                "/usr/local/include/glad", // Updated path to where you have GLAD headers
                "/usr/include/GLFW"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/g++",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

# Build command in Linux (Ubuntu)
```
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
OR run build script
```
chmod +x build.sh #Make the Script Executable
./build.sh
```

# Usage CLI
```
cd output/app/linux
./gpupixel_app <input_file> <config_file>
```
OR run build script
```
chmod +x run-app.sh #Make the Script Executable
./run-app.sh
```

