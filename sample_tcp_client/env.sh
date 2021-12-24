DEVICE_NAME=imx6
export DEVICE_NAME

PATH=$PATH:/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/
export PATH="$PATH:/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/"

LD_LIBRARY_PATH=/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/lib:/home/adeng/work/ltib/L3.0.35_4.1.0_130816_source/ltib/rpm/BUILD/alsa-lib-1.0.24.1:/home/adeng/work/libiconv_install/lib
export PATH LD_LIBRARY_PATH

CROSS_COMPILE=/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-fsl-linux-gnueabi-
export CROSS_COMPILE

PROJECT_DIRECTORY=$PWD/..
export PROJECT_DIRECTORY

ROOTFS=$PROJECT_DIRECTORY/rootFilesystem
export ROOTFS

USERFS=$PROJECT_DIRECTORY/userFilesystem
export USERFS

