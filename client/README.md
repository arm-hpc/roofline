# Roofline

This DynamoRIO client implements Roofline (TODO: Add some more references here)


# Build Instructions

For the time being this client can be built on Arm platforms only.
(The only obstacle for now to be able to run this on x86_64 is to be able to count correctly Floating Point Operations,
in particular those instructions that feature fused operations).

In order to do that, please follow the build instuctions that can be found in
[How to build DynamoRIO](https://github.com/DynamoRIO/dynamorio/wiki/How-To-Build)
This is a compulsory step.

After having downloaded DynamoRIO and having built it,
in order to build this client you'll need to:

* Specify an environment variable to the DynamoRIO build directory
* Run the following commands


mkdir <build_folder>
cd <build_folder>
cmake ..
make

As suggested in the [DynamoRIO Documentation](https://dynamorio.org/docs/using.html#sec_build), this client features the CMake
build system.

