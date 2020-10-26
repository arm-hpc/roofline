## Debugging

For debugging the tool, you can specify in the roofline DynamoRIO client 2 different level of logging:

In order to enable a first level of debugging information, please edit `client/CMakeLists.txt` and turn `ON` the `WITH_VALIDATION` option

In order to enable a more detailed and precise level of debugging information, please edit `client/CMakeLists.txt` and turn `ON` the `WITH_VALIDATION_VERBOSE` option

Please take into account that, in order to see the disassembly for the client instrumented code (which you can easily find in `roofline.disassemble`)
you will need to run the client command as standalone and not withing the roolfline python script, otherwise the file will be empty.
This probably happens because dynamoRIO gets confused about file descriptors when running inside a python environment


In order to get an even more detailed view on what is happening during the DynamoRIO instrumented computation, please check out:

*[How to build](https://github.com/DynamoRIO/dynamorio/wiki/How-To-Build)
*[Debugging DynamoRIO](https://github.com/DynamoRIO/dynamorio/wiki/Debugging)


