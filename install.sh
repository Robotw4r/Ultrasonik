#!/bin/bash

echo "Installation script for Embedded Operating Systems project: ultrasonic sensor driver - Welcome"

install_path="./"
read -p "Choose the buildroot installation path (default: ./) : " install_path

if [ "$install_path" == "" ]; then
    install_path="./"
fi

echo "Install path set to $install_path"

echo "Cloning buildroot 2025.02.x to $install_path/buildroot/"

echo -n "Press enter to continue"

read

echo -e "\033[1A\033[2K"


git clone --single-branch --branch 2025.02.x https://github.com/buildroot/buildroot.git $install_path/buildroot

echo "Copying the project's custom configuration to buildroot"

echo -n "Press enter to continue"

read

cp ./ultrasonik $install_path/buildroot/package
cp ./package_Config.in $install_path/buildroot/package/Config.in
cp ./see_defconfig $install_path/buildroot/configs
mkdir -p $install_path/buildroot/board/insa/see/
cp ./boardconfig.dts $install_path/buildroot/board/insa/see/stm32mp157c-dk2.dts

thread_numbers="4"
read -p "Choose the number of threads used for the building task installation path (default: 4) : " thread_numbers

if [ "$thread_numbers" == "" ]; then
    thread_numbers="4"
fi

echo "Number of threads set to $thread_numbers"

echo "Building the project (this can take a long time)"

echo -n "Press enter to continue"

read

echo -e "\033[1A\033[2K"

cd $install_path/buildroot/

make see_defconfig

make -j$thread_numbers

echo "Installation script for Embedded Operating Systems project: ultrasonic sensor driver - Finished"
echo "The linux image is accessible at $install_path/buildroot/output/images/sdcard.img"
