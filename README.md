# Ultrasonik, a Linux ultrasonic sensor driver

### About
Made for an Embedded Operating Systems course project, use at your own risk.

### Aknowledgemnts
(assuming you're using an Ubuntu based Linux distribution, and have ```git``` and ```make``` installed)

### Installation

```git clone https://github.com/Robotw4r/Ultrasonik.git```

An install script does all the job for you, use it using ```bash ./install.sh```.
It will copy the configurations to a fresh download of [buildroot](https://github.com/buildroot/buildroot/tree/2025.02.x) and build the linux image.

You can enable and disable the driver in ```make menuconfig``` the menuconfig entry is in the ```Target Packages/Miscellaneous``` submenu.

### License
#TODO

### Authors

Driver code:

 * [Rob](https://github.com/Robotw4r)

 * [Ivannopolis](https://github.com/Ivannopolis)

### Project status
Ongoing
