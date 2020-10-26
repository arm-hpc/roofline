It is possible to debug the Roofline Client in different way:

A first approach is to rebuild the Client with the VALIDATE flag enabled. (Edit the CMake file)



A Second approach is to build a debug version of DynamoRIO and try something like:

`/home/andbru01/RotationII/dynamorio/debug_build/bin64/drrun -debug -loglevel 4 -logdir ~/RotationII/roofline/ -c ./client/build/libroofline.so  -- benchmarks/main -i 1 -s 10 -c sum`
