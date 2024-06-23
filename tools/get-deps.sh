#!/bin/bash

loginfo() {
	echo -e "\033[33;1m\n"$1"\033[0m"
}

read -p "Wayland Fedora (1) X11 Debian (2): " dist

read -p "Do you want to install vulkan-dev deps with apt? (Y/n) " conf
if [[ $conf == Y ]]; then
	loginfo "Installing vulkan utility tools (like vulkan cube)!!!"
	sudo apt install vulkan-tools 2>/dev/null || sudo dnf install vulkan-tools
	loginfo "Installing vulkan loader!!!"
	sudo apt install libvulkan-dev 2>/dev/null || sudo dnf install vulkan
	loginfo "Installing vulkan validation layers and spirv tools!!!"
	sudo apt install vulkan-utility-libraries-dev spirv-tools 2>/dev/null || sudo dnf install vulkan-validation-layers spirv-tools vulkan-headers
fi

read -p "Do you want to install glfw deps with package manager? (Y/n) " conf
if [[ $conf == Y ]]; then
	if [[ $dist -eq 1 ]]; then
		loginfo "Installing glfw dependencies for Wayland"
		sudo dnf install wayland-devel libxkbcommon-devel wayland-protocols-devel extra-cmake-modules
	else
		loginfo "Installing glfw dependencies for X11"
		sudo apt install libxi-dev libxrandr-dev libxcursor-dev libxinerama-dev
	fi

fi
