Doodle2STL
=========

Doodle2STL is a small tool for converting doodles to a printable STL file. Filed area will be considered as solid, empty areas will be void.

Installation
------------

To compile and install this tool on Ubuntu, you need a few packages. Install them with the following command:

```bash
sudo apt install build-essential cmake libopencv-dev
```

Then compile the thing, but typing from this directory:

```bash
mkdir build && cd build
cmake ..
make && sudo make install
```

And you are ready to go!

Usage
-----

doodle2stl asks for the following parameters:

* --resolution (default: 256): resolution of the output STL file. The higher the resolution, the more precise the file is. It also gets substantially bigger
* --height (default: 1): height of the 3D object
* --prefix (default: /var/www/doodle2stl): path where the STL files are written
* filename: last parameter is the input image file path

Author and licensing
--------------------

This small software has been written by [Emmanuel Durand](https://emmanueldurand.net) and is in the public domain. Do whatever you want with it!
