# Score-P Symbol Injector
Support library for Score-P that identifies and registers symbols from shared libraries.
Currently, works only on Linux.

## Requirements

 - [Score-P](https://www.vi-hps.org/projects/score-p/)
 - LLVM (for testing purposes)
 - CMake (>=3.15)

## Building

```
mkdir build && cd build
cmake -G Ninja -DLLVM_DIR="path/to/llvm/" -DSCOREP_PATH="path/to/scorep" ..
ninja
```

(works with make too)

## Usage

Simply link the the `libsymbol_injector.so` library into the instrumented executable along with the required Score-P dependencies.
Provide the name of your excutbale in the environment variable `SCOREP_EXECUTABLE`.

At startup, the library will query `/proc/self/maps` to identify the memory mapping of shared libraries.
All visible function symbols are then identified with `nm` and registered in Score-P.

There maybe a slight overhead in Score-P's address lookup mechanism, because there are potentially a lot of unneeded functions registered.
In the future, support for function filtering will be added.

See `test/shared_lib_test.c` for a basic example.

