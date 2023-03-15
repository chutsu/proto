#!/bin/bash
set -e

export IGN_GAZEBO_SERVER_CONFIG_PATH=$PWD/server.config
export IGN_GAZEBO_RESOURCE_PATH=$PWD/worlds:$PWD/models
gz sim calibration.sdf --gui-config gui.config
# ign gazebo --gui-config gui.config sim calibration.sdf
