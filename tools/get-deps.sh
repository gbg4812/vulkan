#!/bin/bash


loginfo() {
    echo -e "\033[33;1m\n"$1"\033[0m"
}

read -p "Do you want to install vulkan-dev deps with apt? (Y/n) " conf
if [[ $conf == Y ]]; then
    loginfo "Installing vulkan utility tools (like vulkan cube)!!!"
    sudo apt install vulkan-tools
    loginfo "Installing vulkan loader!!!"
    sudo apt install libvulkan-dev
    loginfo "Installing vulkan validation layers and spirv tools!!!"
    sudo apt install vulkan-validationlayers-dev spirv-tools
fi

read -p "Do you want to install glfw deps with apt? (Y/n) " conf
if [[ $conf == Y ]]; then
    loginfo "Installing glfw dependencies for X11"
    sudo apt install libxi-dev libxrandr-dev libxcursor-dev libxinerama-dev
fi
