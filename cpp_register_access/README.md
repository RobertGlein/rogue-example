# Rogue C++ Register Access

This directory contains an example of using Rogue to access registers in C++.

The current example hard codes the IP address and port number within the C++ file. Edit this file to match your data generator.

## Building the example

The example project uses cmake which will automatically generate a
makefile to properly link this example against rogue, boost and python3.

````
$ mkdir build
$ cd build
$ cmake ..
$ make
````

### Running the example

The executable can be found in bin/register_access

````
$ bin/register_access
````

