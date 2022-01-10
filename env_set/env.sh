

DEVICE_NAME=ndmb
export DEVICE_NAME

LD_LIBRARY_PATH=./lib:/opt/gcc-linaro-7.4.1-2019.02-i686_arm-linux-gnueabihf/lib/
export LD_LIBRARY_PATH

CROSS_COMPILE=/opt/gcc-linaro-7.4.1-2019.02-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
export CROSS_COMPILE

PROJECT_DIRECTORY=$PWD/..
export PROJECT_DIRECTORY

ROOTFS=$PROJECT_DIRECTORY/rootFilesystem
export ROOTFS

USERFS=$PROJECT_DIRECTORY/userFilesystem
export USERFS

echo "====================================================================================================================="
echo "    BUILD ENV SET "
