#!/bin/bash

# Define variables
EXECUTABLE="gpupixel_app"
INPUT_FILE="$1"
CONFIG_FILE="$2"

# Change to the directory containing the executable
cd output/app/linux || exit

# Override OpenGL driver
export MESA_LOADER_DRIVER_OVERRIDE=nvidia
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json

# Run the executable with the optional input file and config file
if [ -z "$INPUT_FILE" ]; then
    ./"$EXECUTABLE"
elif [ -z "$CONFIG_FILE" ]; then
    ./"$EXECUTABLE" "$INPUT_FILE"
else
    ./"$EXECUTABLE" "$INPUT_FILE" "$CONFIG_FILE"
fi