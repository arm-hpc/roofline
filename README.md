## Roofline

Roofline is a tool allowing you to get performance insights of a target program in the context of the machine is executing on.

For background on this tool, please read:

* [Roofline Modeling and Analysis](https://crd.lbl.gov/assets/Uploads/CS267-2019-Roofline-SWWilliams.pdf)
* [Intel Advisor Roofline](https://software.intel.com/en-us/articles/intel-advisor-roofline)
* [Roofline: An Insightful Visual Performance Model for Multicore Architectures](http://www.eng.auburn.edu/~vagrawal/COURSE/READING/ARCH/roofline%20An%20insightful%20performance%20model%20for%20Multicore.pdf)
* [Cache-aware Roofline model: Upgrading the loft](http://www.inesc-id.pt/ficheiros/publicacoes/9068.pdf)

The tool outputs a roofline plot for your application.

## Building

Before you build, you must have certain dependencies installed.  On ubuntu you can obtain these with the following:
```
% sudo apt-get update && sudo apt-get install cmake g++ doxygen git zlib1g-dev libunwind-dev libsnappy-dev liblz4-dev gnuplot
```

By default, the Makefile will download and build Dynamo RIO for you.  If you would rather work from your own Dynamo RIO directory,
it is important you set the environment variable _DYNAMORIO_DIR_ to the path to the Dynamo RIO directory.  If the build directory
isn't in _DYNAMORIO_DIR_/build then you will also have to set _DYNAMORIO_BUILD_DIR_.


## Remark (Important!)

Please take into account that this tool is provided as a proof of conccept, without any warranty or support.


## Usage

The roofline tool is structured similarly to the well-known "perf" tool, with "record" and "report" phases:
if you are familiar with perf, you already know how to use this tool!







## Recording Capabilities

Roofline allows you to define a target Region of Interest (ROI) in your application, in different situations:

### If you have access to the source code - Manual instrumentation for the target application

Before using the tool, you'll need to define the region of interests for your target application.
In order to achieve this, you can simply include into your application the header file `roi_api.h` and call the API:


```
Roi_Start("label");

...
Code which is interesting for you
...

Roi_End("label");

```

"label" is the mnemonic for the specified region of interest: choose a meaningul name for your use case.
When you specify a label as a start for a region of interest, use the same label for delimitating the end. 


### If you have the executable only - Specify a ROI using symbols already present in the binary

Roofline gives you the capability to specify functions already present in the application as Region of Interest delimiters:

`roofline record --roi_start <Symbol Name> --roi_end <Symbol Name> ./my_app`

Please be careful of asking the tool to perform a meaningful operation: when the starting symbol executed, it does make sense to be sure and ending symbol will eventually be executed as well, in a 1:1 ratio.

Pay attention: if the specified symbols are executed multiple times, the tool will record multiple regions of interest. 

The tool also provides the capability of defining a single function as a region of interest:

`roofline record --trace_f <Symbol Name> ./my_app`


## Record
In order to use the tool for recording:

`roofline record -o <output_folder_to_store_the_results>  -- <target_appliaction> <target application flag>`

If you are interested into a more granular recording, the tool supports the '--[read/write]_bytes_only' flag which, if specified, will make the instrumentation client gather only bytes read or written respectively.


The tool will create two different files in the specified output directory reporting all the information gathered:

* roofline.xml - This file contains information about bytes accessed by all the bits of code falling into the specified regions of interest.
* roofline_time.xml - This file contains timinig information about all thei bits of code falling into the specified regions of interest.


## Report

Once you've recorded you application, you'll definitely want to actually see the plot being drawn:

`rooofline report -i <folder_created_on_roofline_record> --line <hostame>_<PRECISION>`

`-i` corresponds to the recorded application performance, while `--line` is instead the ERT recorded roofline-like line representing the machine capabilities.

Multiple `-i` or `-line` targets can be specified in such a way that you'll be able to compare different regions of interests under potentially different configurations, when potentially run on top of different machines.

This command will use the previously generated roofline.xml and roofline_time.xml files to draw a plot for you.
If you are using a remote machine/server and don't have any graphics packages there, the tool provides you a quick report on the command line you'll be able to see just after having executed the command.

This command also provides another output called roofline.gnu which you can use to get a better plot:


`gnuplot roofline.gnu`

This will produce a roofline.ps file in the current directory, which is a [PostScript](https://en.wikipedia.org/wiki/PostScript) file for the final plot.


A suggested way to be able to see the actual plot is to use [Okular](https://okular.kde.org/):

`okular roofline.ps`



## Beta Release Notes

* When using the tool (especially on x86_64), please check out your target application and make sure the floating point assembly instructions are counted correctly in the client file `client/count_fp.hpp`
* If you specify functions already present in the code base as start and stop, please make sure that they have not been inlined by the compiler, otherwise they won't be 'officially' executed and Dynamorio (and gdb as well) won't be able to detect the function execution


# Current Limitations

* Unfortunately the tool, when instrumenting the target application for gathering the FLOP and Bytes piece of information, significantly slows down the application running, resulting in an increased execution time.
This problem can actually be solved by improving the insturmentation client by finding a way of avoiding 'clean calls' and cleaning the code cache as soon as a region of interest starting demarker is met at runtime.
If you have time/resources for improving the tool and want to know more about this, please let us know by raising an issue on the project.

* The assumption, for this beta version, is the target application to be single-threaded. The Dynamorio client already features to trace multi-threaded applications, but this has to be finalized and tested properly.

* The tool has been designed to support Arm and x86_64. While it's able to precisely count the number of floating point operationsn which will actually be executed in the CPU, precise counting has been implemented only of normal floating point operations and NEON vector instructions.
For other Arm FP instructions extensions and x86_64 please check out `client/count_fp.hpp` and make sure the tool is counting correctly. Also in the same file some improvement needs to be done in order to be able to better spot SIMD instructions.


* The tool actually trusts ERT to gain the correct piece of information for the roofline chart. However, ERT benchmarking code, in order to search for the maximum flops value, issues multiple `fadd` scalar operations sequentially into the pipeline. This can be improved and, for future development of the tool, it may be worth taking into account different data sources or improving ERT itself.



# Troubleshooting 

If the current documentation is omitting something important or you are find issues running the tool or understanding some parts in the code, please feel free to raise an issue on the github project.
