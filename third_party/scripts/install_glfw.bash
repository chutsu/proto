#!/bin/bash
set -e  # halt on first error
source "config.bash"

# Install deps
sudo apt-get install -y libxrandr-dev
sudo apt-get install -y libxinerama-dev
sudo apt-get install -y libxcursor-dev

# Build
install_git_repo https://github.com/glfw/glfw glfw
