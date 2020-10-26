## Automatic Setup

In order to install Roofline, simply run the Makefile you can find in this project root directory.

`make`


In order to have the command handy, the suggestion is to have a command alias for it:
In ~/.bashrc:

`alias roofline='path/to/roofline/directory/roofline.py'`



## Manual Installation

If you want to tweak the tool or the automatic installation is not working on your system, here you can find a list of dependencies for this tool:


This tool has been build on top of [DynamoRIO](http://dynamorio.org/) and it requires it to be installed in the system.

If you have to manually install it, the steps to follow are the following:

* Checkout the DynamoRIO repository on [Github](https://github.com/DynamoRIO/dynamorio): `git clone https://github.com/DynamoRIO/dynamorio.git`
* Enter inside the local directory `cd dynamorio`
* Create an output directory and enter inside of it `mkdir build && cd build`
* Build the project: `cmake .. && make`


If you have trouble or need more information , this [DynamoRIO official guide] will be really helpful


This project also uses the support of another open source tool for gathering the current machine empirical obtainable performance: the [ERT TOOL](https://bitbucket.org/berkeleylab/cs-roofline-toolkit/src/master/).
This comes already packaged inside this project, but if you want to play with it you can simply clone it using git and replace it in the ert directory.

