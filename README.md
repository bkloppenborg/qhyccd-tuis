# QHYCCD Text User Interfaces

This project implements some command line interfaces for the QHY series
of astronomical cameras.

# Prerequisites

## Debian-based Distributions

```
sudo apt-get install cmake git doxygen graphviz build-essential
sudo apt install qt6-base-dev libopencv-dev libcfitsio-dev

# optionally
sudo apt install cmake-curses-gui
```
# Building

```
cd build
cmake ..
make
```