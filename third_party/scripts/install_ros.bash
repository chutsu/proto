set -e
source "config.bash"

# Check if ROS is installed
if [ -f "/etc/apt/sources.list.d/ros-latest.list" ]; then
  exit 0;
fi

# Install ROS
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
sudo apt-key adv \
  --keyserver 'hkp://keyserver.ubuntu.com:80' \
  --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
apt_update
apt_install dpkg
sudo dpkg --configure -a
apt_install "ros-$ROS_VERSION-desktop"
apt_install "ros-$ROS_VERSION-gazebo-dev*"
apt_install python3-catkin-tools
apt_install python3-rosdep
apt_install python3-rosbag
sudo rosdep init
rosdep update
