#!/bin/sh

# The URL and SHA of the RPI image you're looking to download
# Get URL and SHA at https://www.raspberrypi.com/software/operating-systems/
RPI_IMAGE_URL=https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2022-09-26/2022-09-22-raspios-bullseye-armhf-lite.img.xz
RPI_IMAGE_DEST=/mnt/toolchain/sysroot
RPI_IMAGE_DEV=/dev/loop0
RPI_IMAGE_MNT=/mnt/sysroot

# Targeting the Raspberry Pi 1 and Zero are tricky!
# This is because stock GCC does not support their CPU, pretty annoying
# We will fetch a 3rd party prebuilt toolchain just for these platforms
# and use stock GCC for the others
XTOOLS_URL=https://github.com/tttapa/docker-arm-cross-toolchain/releases/download/0.0.9/x-tools-armv6-rpi-linux-gnueabihf.tar.xz
XTOOLS_DEST=/mnt/toolchain/x-tools

# Attempt an unmount 
echo "Cleaning up previous sysroot (ok if this fails)..."
umount $RPI_IMAGE_MNT
losetup -d $RPI_IMAGE_DEV

# This seems to fail because of docker permissions issues!
# Just setup manually at toolchain/x-tools for now...

# echo "Setting up Pi 1+Zero compiler..."
# if [ ! -d "$XTOOLS_DEST" ] ; then
#   echo "Downloading..."
#   curl -L $XTOOLS_URL > "$XTOOLS_DEST.tar.xz"
#   echo "Extracting..."
#   mkdir "$XTOOLS_DEST"
#   tar -C /mnt/toolchain -xf "$XTOOLS_DEST.tar.xz" 
# else
#   echo "Already setup!"
# fi

echo "Setting up RPi sysroot..."
if [ ! -f "$RPI_IMAGE_DEST.img" ] ; then
  echo "Downloading..."
  curl $RPI_IMAGE_URL > "$RPI_IMAGE_DEST.img.xz"
  echo "Extracting..."
  xz -d "$RPI_IMAGE_DEST.img.xz"
else
  echo "Already setup!"
fi

# Get the second partition offset and size in the disk
IMG_SECT_SIZE=512
IMG_SECT_OFFSET=`fdisk -l --bytes $RPI_IMAGE_DEST.img | grep img2 | tr -s " " | cut -d " " -f2`
IMG_BYTE_OFFSET=`expr $IMG_SECT_OFFSET \* $IMG_SECT_SIZE`
IMG_BYTE_SIZE=`fdisk -l --bytes $RPI_IMAGE_DEST.img | grep img2 | tr -s " " | cut -d " " -f5`
echo "Sysroot partion has offset $IMG_BYTE_OFFSET and size $IMG_BYTE_SIZE"

echo "Creating loop device..."
mknod $RPI_IMAGE_DEV b 7 0

echo "Mounting $RPI_IMAGE_DEST.img at $RPI_IMAGE_MNT"
if [ ! -d "$RPI_IMAGE_MNT" ] ; then
  mkdir "$RPI_IMAGE_MNT"
fi
losetup -o $IMG_BYTE_OFFSET --sizelimit $IMG_BYTE_SIZE --sector-size $IMG_SECT_SIZE $RPI_IMAGE_DEV $RPI_IMAGE_DEST.img
mount -r $RPI_IMAGE_DEV $RPI_IMAGE_MNT
