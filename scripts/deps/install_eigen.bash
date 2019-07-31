#!/bin/bash
set -e  # Exit on first error
BASEDIR=$(dirname "$0")
source $BASEDIR/config.bash

# # MAIN
# install_hg_repo \
#   https://bitbucket.org/eigen/eigen/ \
#   eigen

apt-get install -qqq libeigen3-dev libeigen3-doc -y
