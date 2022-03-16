// RUN: scorep-config --mpp=none --adapter-init > scorep_init.c
// RUN: clang -c scorep_init.c -o scorep_init.o
// RUN: clang++ -c -O2 -finstrument-functions -fPIE shared_lib_test.c -o shared_lib_test.o
// RUN: clang++ -finstrument-functions -fPIC -shared testlib.c -o libtestlib.so

// Test with linking symbol injector directly
// RUN:	clang++ -v -L `scorep-config --prefix`/lib -Wl,-rpath,`scorep-config --prefix`/lib -Wl,-start-group `scorep-config --libs --mpp=none --compiler`  -Wl,-end-group -L %lib_dir -Wl,-rpath,%lib_dir -lsymbol_injector -L./ -Wl,-rpath,./ -ltestlib scorep_init.o shared_lib_test.o -o shared_lib_test
// RUN: SCOREP_EXPERIMENT_DIRECTORY=scorep-test-profile SCOREP_ENABLE_PROFILING=true SCOREP_EXECUTABLE=shared_lib_test ./shared_lib_test
// RUN: scorep-score -r scorep-test-profile/profile.cubex | FileCheck %s
// RUN: rm -rf scorep-test-profile

// Test with LD_PRELOAD
// RUN:	clang++ -v -L `scorep-config --prefix`/lib -Wl,-rpath,`scorep-config --prefix`/lib -Wl,-start-group `scorep-config --libs --mpp=none --compiler`  -Wl,-end-group -L %lib_dir -Wl,-rpath,%lib_dir -L./ -Wl,-rpath,./ -ltestlib scorep_init.o shared_lib_test.o -o shared_lib_test
// RUN: SCOREP_EXPERIMENT_DIRECTORY=scorep-test-profile-preload SCOREP_ENABLE_PROFILING=true SCOREP_EXECUTABLE=shared_lib_test LD_PRELOAD="libsymbol_injector.so" ./shared_lib_test
// RUN: scorep-score -r scorep-test-profile-preload/profile.cubex | FileCheck %s
// RUN: rm -rf scorep-test-profile-preload

// Cleanup
// RUN:	rm scorep_init.c scorep_init.o
// RUN: rm shared_lib_test shared_lib_test.o libtestlib.so

#include <stdio.h>
#include <stdlib.h>

int lib_fn();

void foo(int n) {
    if (n >= 0) {
        foo(n-1);
    }
}

// CHECK: _Z6lib_fnv

int main(int argc, char** argv) {
    int x = lib_fn();
    foo(x);
    return 0;
}
